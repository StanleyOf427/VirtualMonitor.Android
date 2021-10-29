/**Create by Stanley
 * 
 * hardware decode and render in native level
 * */
/*extern "C"{

}*/
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavutil/imgutils.h"
#include "include/libavutil/opt.h"
#include "include/libswresample/swresample.h"
#include "include/libswscale/swscale.h"

 #include <android/native_window.h>
 #include <android/native_window_jni.h>
#include <android/looper.h>
 #include <android/log.h>
 #include <jni.h>

#include <unistd.h>
#include <pthread.h>

#if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION
#include <media/NdkMediaCodec.h>
#endif
// #include <cstdio>
// #include <cstdlib>
#pragma region 默认设置
#define DEFAULT_BUFFERSIZE 1024*1024*5
#define DEFAULT_BUFFERTIME 0.1f
#define DEFAULT_READ_TIMEOUT 0.1f

#define MESSAGE_STOP 1
#define MESSAGE_BUFFER_EMPTY 2
#define MESSAGE_BUFFER_FULL 3
#define MESSAGE_ERROR 999

#define HAS_VIDEO_FLAG 0x2

#pragma endregion


#define LOGI(FORMAT, ...)                                                      \
  __android_log_print(ANDROID_LOG_INFO, "jason", FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT, ...)                                                      \
  __android_log_print(ANDROID_LOG_ERROR, "jason", FORMAT, ##__VA_ARGS__);

#pragma region 结构体
typedef struct player_java_tclass {
//    jmethodID player_onPlayStatusChanged;
//    jmethodID player_onPlayError;

    jclass HwDecodeBridge;
    jmethodID codec_init;
    jmethodID codec_stop;
    jmethodID codec_flush;
    jmethodID codec_dequeueInputBuffer;
    jmethodID codec_queueInputBuffer;
    jmethodID codec_getInputBuffer;
    jmethodID codec_dequeueOutputBufferIndex;
    jmethodID codec_formatChange;
    __attribute__((unused))
    jmethodID codec_getOutputBuffer;
    jmethodID codec_releaseOutPutBuffer;
    jmethodID codec_release;

    jclass SurfaceTextureBridge;
    jmethodID texture_getSurface;
    jmethodID texture_updateTexImage;
    jmethodID texture_getTransformMatrix;
    __attribute__((unused))
    jmethodID texture_release;

}player_java_class ;

typedef struct player_data {
    JavaVM *vm;
    JNIEnv *jniEnv;
    // int run_android_version;
    int sample_rate;
    jobject *player;
    player_java_class *jc;

    //读取及缓存设置
    int buffer_size_max;
    float buffer_time_length;
    float read_timeout;

    // 各个线程
    pthread_t read_stream_thread;
    pthread_t video_decode_thread;
    pthread_t gl_thread;

    // 封装
    AVFormatContext *format_context;
    uint8_t av_track_flags;
    uint64_t timeout_start;

    // packet容器
    player_packet_queue *video_packet_queue, *audio_packet_queue;
    player_pakcet_pool *packet_pool;

    // frame容器
    player_frame_pool *video_frame_pool;
    player_frame_queue *video_frame_queue;

    //frame参数
    video_render_context *video_render_ctx;
    AVFrame *video_frame;
    int width, height;
    int frame_rotation;

    // 硬解
    mediacodec_context *mediacodec_ctx;

    //end of stream
    bool eof;

    //以后再定义该成员
    int error_code;

    // 统计
    xl_statistics *statistics;

    // message
    ALooper *main_looper;
    int pipe_fd[2];

    void (*send_message)(struct struct_xl_play_data *pd, int message);

//    void (*change_status)(struct struct_xl_play_data *pd, PlayStatus status);

    void (*on_error)(struct struct_xl_play_data * pd, int error_code);
} player_data;

typedef struct struct_packet_queue {
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    AVPacket **packets;
    int readIndex;
    int writeIndex;
    int count;
    int total_bytes;
    unsigned int size;
    uint64_t duration;
    uint64_t max_duration;
    AVPacket flush_packet;

    void (*full_cb)(void *);

    void (*empty_cb)(void *);

    void *cb_data;
} packet_queue;


typedef struct struct_frame_pool {
    int index;
    int size;
    int count;
    AVFrame *frames;
} frame_pool;

typedef struct struct_frame_queue {
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    AVFrame **frames;
    int readIndex;
    int writeIndex;
    int count;
    unsigned int size;
    AVFrame flush_frame;
} frame_queue;

typedef struct struct_packet_pool {
    int index;
    int size;
    int count;
    AVPacket **packets;
} pakcet_pool;

typedef enum enum__draw_mode {
    wait_frame, fixed_frequency
} draw_mode;

typedef struct struct_video_render_context {
    JNIEnv *jniEnv;
    int width, height;
    EGLConfig config;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    struct ANativeWindow *window;
    struct ANativeWindow *texture_window;
    GLuint texture[4];
    // model *model;//VR用
    bool enable_tracker;
    pthread_mutex_t *lock;
    draw_mode draw_mode;

    uint8_t cmd;
    ModelType require_model_type;
    jfloat require_model_scale;
    float require_model_rotation[3];

    void (*set_window)(struct struct_video_render_context *ctx, struct ANativeWindow *window);

    void (*change_model)(struct struct_video_render_context *ctx, ModelType model_type);

} video_render_context;

typedef struct struct_mediacodec_context {
    JNIEnv *jniEnv;
#if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION
    AMediaCodec *codec;
    AMediaFormat *format;
#endif
    size_t nal_size;
    int width, height;
    enum AVPixelFormat pix_format;
    enum AVCodecID codec_id;
} mediacodec_context;

typedef struct struct_statistics {
    int64_t last_update_time;
    int64_t last_update_bytes;
    int64_t last_update_frames;
    int64_t bytes;
    int64_t frames;
    uint8_t *ret_buffer;
    jobject ret_buffer_java;
} statistics;
#pragma endregion

#pragma region 工具函数
AVFrame *  frame_queue_get(frame_queue *queue){
    pthread_mutex_lock(queue->mutex);
    if(queue->count == 0){
        pthread_mutex_unlock(queue->mutex);
        return NULL;
    }
    AVFrame * frame = queue->frames[queue->readIndex];
    queue->readIndex = (queue->readIndex + 1) % queue->size;
    queue->count--;
    pthread_cond_signal(queue->cond);
    pthread_mutex_unlock(queue->mutex);
    return frame;
}

AVPacket *packet_queue_get(packet_queue *queue) {
    pthread_mutex_lock(queue->mutex);
    if (queue->count == 0) {
        pthread_mutex_unlock(queue->mutex);
        if (queue->empty_cb != NULL) {
            queue->empty_cb(queue->cb_data);
        }
        return NULL;
    }
    AVPacket *packet = queue->packets[queue->readIndex];
    queue->readIndex = (queue->readIndex + 1) % queue->size;
    queue->count--;
    queue->duration -= packet->duration;
    queue->total_bytes -= packet->size;
    pthread_cond_signal(queue->cond);
    pthread_mutex_unlock(queue->mutex);
    return packet;
}

void statistics_reset(statistics *s) {
    s->last_update_time = 0;
    s->last_update_bytes = 0;
    s->last_update_frames = 0;
    s->bytes = 0;
    s->frames = 0;
}

#pragma region 资源释放
void frame_queue_free(frame_queue *queue){
    pthread_mutex_destroy(queue->mutex);
    pthread_cond_destroy(queue->cond);
    free(queue->frames);
    free(queue);
}

void packet_pool_unref_packet(pakcet_pool * pool, AVPacket * packet){
    av_packet_unref(packet);
    pool->count--;
}

void xl_mediacodec_release_context(player_data *pd) {
    JNIEnv *jniEnv = pd->jniEnv;
    player_java_class * jc = pd->jc;
    (*jniEnv)->CallStaticVoidMethod(jniEnv, jc->HwDecodeBridge, jc->codec_release);
    mediacodec_context *ctx = pd->mediacodec_ctx;
    free(ctx);
    pd->mediacodec_ctx = NULL;
}
static void reset_pd(player_data *pd) {
    if (pd == NULL) return;
    pd->eof = false;
    pd->video_index = -1;
    pd->width = 0;
    pd->height = 0;
    pd->video_frame = NULL;
    pd->seeking = 0;
    pd->timeout_start = 0;
    clock_reset(pd->video_clock);
    statistics_reset(pd->statistics);
    pd->error_code = 0;
    pd->frame_rotation = ROTATION_0;
    packet_pool_reset(pd->packet_pool);
    pd->change_status(pd, IDEL);
    video_render_ctx_reset(pd->video_render_ctx);
}

void frame_pool_unref_frame(frame_pool * pool, AVFrame * frame){
    av_frame_unref(frame);
    pool->count--;
}

void packet_pool_reset(pakcet_pool * pool){
    pool->count = 0;
    pool->index = 0;
}

static inline void clean_queues(player_data *pd) {
    AVPacket *packet;
    // clear pd->video_frame video_frame_queue video_frame_packet
    if ((pd->av_track_flags & HAS_VIDEO_FLAG) > 0) {
        if (pd->video_frame != NULL) {
            if (pd->video_frame != &pd->video_frame_queue->flush_frame) {
                frame_pool_unref_frame(pd->video_frame_pool, pd->video_frame);
            }
        }
        while (1) {
            pd->video_frame = frame_queue_get(pd->video_frame_queue);
            if (pd->video_frame == NULL) {
                break;
            }
            if (pd->video_frame != &pd->video_frame_queue->flush_frame) {
                frame_pool_unref_frame(pd->video_frame_pool, pd->video_frame);
            }
        }
        while (1) {
            packet = packet_queue_get(pd->video_packet_queue);
            if (packet == NULL) {
                break;
            }
            if (packet != &pd->video_packet_queue->flush_packet) {
                packet_pool_unref_packet(pd->packet_pool, packet);
            }
        }
    }
}


static void reset(player_data *pd) {
    if (pd == NULL) return;
    pd->eof = false;
    pd->av_track_flags = 0;
    pd->width = 0;
    pd->height = 0;
    pd->video_frame = NULL;
    pd->timeout_start = 0;
    statistics_reset(pd->statistics);
    pd->error_code = 0;
    packet_pool_reset(pd->packet_pool);
    pd->change_status(pd, IDEL);
    video_render_ctx_reset(pd->video_render_ctx);
}

#pragma endregion

void jni_reflect_java_class(player_java_class ** p_jc, JNIEnv *jniEnv) {
    player_java_class *  jc =malloc(sizeof(player_java_class));//??2  为何改为cpp后此处出现无法赋值问题
    jclass xlPlayerClass = (*jniEnv)->FindClass(jniEnv, "com/stanley/virtualmonitor/Player");
//    jc->player_onPlayStatusChanged = (*jniEnv)->GetMethodID(jniEnv, xlPlayerClass,
//                                                            "onPlayStatusChanged", "(I)V");
//    jc->player_onPlayError = (*jniEnv)->GetMethodID(jniEnv, xlPlayerClass,
//                                                    "onPlayError", "(I)V");
//    jc->XLPlayer_class = (*jniEnv)->NewGlobalRef(jniEnv, xlPlayerClass);
    (*jniEnv)->DeleteLocalRef(jniEnv, xlPlayerClass);

    jclass java_HwDecodeBridge = (*jniEnv)->FindClass(jniEnv, "com/stanley/virtualmonitor/HwDecodeBridge");

    jc->codec_init = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "init", "(Ljava/lang/String;IILjava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
    jc->codec_stop = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "stop", "()V");
    jc->codec_flush = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "flush",  "()V");
    jc->codec_dequeueInputBuffer = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "dequeueInputBuffer", "(J)I");
    jc->codec_queueInputBuffer = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "queueInputBuffer", "(IIJI)V");
    jc->codec_getInputBuffer = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "getInputBuffer", "(I)Ljava/nio/ByteBuffer;");
    jc->codec_getOutputBuffer = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "getOutputBuffer", "(I)Ljava/nio/ByteBuffer;");
    jc->codec_releaseOutPutBuffer = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "releaseOutPutBuffer",  "(I)V");
    jc->codec_release = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "release", "()V");
    jc->codec_formatChange = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "formatChange", "()Ljava/nio/ByteBuffer;");
    jc->codec_dequeueOutputBufferIndex = (*jniEnv)->GetStaticMethodID(jniEnv, java_HwDecodeBridge, "dequeueOutputBufferIndex", "(J)Ljava/nio/ByteBuffer;");
    jc->HwDecodeBridge = (*jniEnv)->NewGlobalRef(jniEnv, java_HwDecodeBridge);
    (*jniEnv)->DeleteLocalRef(jniEnv, java_HwDecodeBridge);

    jclass java_SurfaceTextureBridge = (*jniEnv)->FindClass(jniEnv, "com/stanley/virtualmonitor/SurfaceTextureBridge");
    jc->texture_getSurface = (*jniEnv)->GetStaticMethodID(jniEnv, java_SurfaceTextureBridge, "getSurface", "(I)Landroid/view/Surface;");
    jc->texture_updateTexImage = (*jniEnv)->GetStaticMethodID(jniEnv, java_SurfaceTextureBridge, "updateTexImage", "()V");
    jc->texture_getTransformMatrix = (*jniEnv)->GetStaticMethodID(jniEnv, java_SurfaceTextureBridge, "getTransformMatrix",  "()[F");
    jc->texture_release = (*jniEnv)->GetStaticMethodID(jniEnv, java_SurfaceTextureBridge, "release", "()V");
    jc->SurfaceTextureBridge = (*jniEnv)->NewGlobalRef(jniEnv, java_SurfaceTextureBridge);
    (*jniEnv)->DeleteLocalRef(jniEnv, java_SurfaceTextureBridge);

    *p_jc = jc;


}

void jni_free(player_java_class **p_jc, JNIEnv *jniEnv){
    player_java_class * jc = *p_jc;
    (*jniEnv)->DeleteGlobalRef(jniEnv, jc->HwDecodeBridge);
    (*jniEnv)->DeleteGlobalRef(jniEnv, jc->SurfaceTextureBridge);
    free(jc);
    *p_jc = NULL;
}

packet_queue *queue_create(unsigned int size) {
    packet_queue *queue = (packet_queue *) malloc(sizeof(packet_queue));
    queue->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    queue->cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(queue->mutex, NULL);
    pthread_cond_init(queue->cond, NULL);
    queue->readIndex = 0;
    queue->writeIndex = 0;
    queue->count = 0;
    queue->size = size;
    queue->duration = 0;
    queue->max_duration = 0;
    queue->total_bytes = 0;
    queue->flush_packet.duration = 0;
    queue->flush_packet.size = 0;
    queue->packets = (AVPacket **) malloc(sizeof(AVPacket *) * size);
    queue->full_cb = NULL;
    queue->empty_cb = NULL;
    return queue;
}

frame_pool * frame_pool_create(int size){
    frame_pool * pool = (frame_pool *)malloc(sizeof(frame_pool));
    pool->size = size;
    pool->count = 0;
    pool->index = 0;
    pool->frames = (AVFrame *)av_mallocz(sizeof(AVFrame) * size);
    for(int i = 0; i < size; i++){
        get_frame_defaults(&pool->frames[i]);
    }
    return pool;
}

frame_queue * frame_queue_create(unsigned int size){
    frame_queue * queue = (frame_queue *)malloc(sizeof(frame_queue));
    queue->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    queue->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(queue->mutex, NULL);
    pthread_cond_init(queue->cond, NULL);
    queue->readIndex = 0;
    queue->writeIndex = 0;
    queue->count = 0;
    queue->size = size;
    queue->frames = (AVFrame **)malloc(sizeof(AVFrame *) * size);
    return queue;
}

pakcet_pool * packet_pool_create(int size){
    pakcet_pool * pool = (pakcet_pool *)malloc(sizeof(pakcet_pool));
    pool->size = size;
    pool->packets = (AVPacket **)av_malloc(sizeof(AVPacket *) * size);
    for(int i = 0; i < pool->size; i++){
        pool->packets[i] = av_packet_alloc();
    }
    return pool;
}

statistics *statistics_create(JNIEnv *jniEnv) {
    statistics *statistics = malloc(sizeof(statistics));
    statistics->ret_buffer = malloc(12);
    jobject buf = (*jniEnv)->NewDirectByteBuffer(jniEnv, statistics->ret_buffer, 12);
    statistics->ret_buffer_java = (*jniEnv)->NewGlobalRef(jniEnv, buf);
    (*jniEnv)->DeleteLocalRef(jniEnv, buf);
    return statistics;
}

void video_render_set_window(video_render_context *ctx, struct ANativeWindow *window) {
    pthread_mutex_lock(ctx->lock);
    ctx->window = window;
    ctx->cmd |= CMD_SET_WINDOW;
    pthread_mutex_unlock(ctx->lock);
}

void video_render_change_model(video_render_context *ctx, ModelType model_type) {
    pthread_mutex_lock(ctx->lock);
    ctx->require_model_type = model_type;
    ctx->cmd |= CMD_CHANGE_MODEL;
    pthread_mutex_unlock(ctx->lock);
}

void video_render_ctx_reset(video_render_context * ctx){
    ctx->surface = EGL_NO_SURFACE;
    ctx->context = EGL_NO_CONTEXT;
    ctx->display = EGL_NO_DISPLAY;
    // ctx->model = NULL;
    ctx->enable_tracker = false;
    ctx->require_model_type = NONE;
    ctx->require_model_scale = 1;
    ctx->cmd = NO_CMD;
    ctx->enable_tracker = false;
    ctx->draw_mode = wait_frame;
    ctx->require_model_rotation[0] = 0;
    ctx->require_model_rotation[1] = 0;
    ctx->require_model_rotation[2] = 0;
    ctx->width = ctx->height = 1;
}

video_render_context *video_render_ctx_create() {
    video_render_context *ctx = malloc(sizeof(video_render_context));
    ctx->lock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(ctx->lock, NULL);
    ctx->set_window = video_render_set_window;
    ctx->change_model = video_render_change_model;
    ctx->window = NULL;
    ctx->texture_window = NULL;
    video_render_ctx_reset(ctx);
    return ctx;
}

mediacodec_context *create_mediacodec_context(player_data *pd) {
    mediacodec_context *ctx = (mediacodec_context *) malloc(sizeof(mediacodec_context));
    AVCodecParameters *codecpar = pd->format_context->streams[pd->video_index]->codecpar;
    ctx->width = codecpar->width;
    ctx->height = codecpar->height;
    ctx->codec_id = codecpar->codec_id;
    ctx->nal_size = 0;
    ctx->pix_format = AV_PIX_FMT_NONE;
    return ctx;
}

static void set_renderwindow(player_data * pd) {
    video_render_context *ctx = pd->video_render_ctx;
    if(ctx->display == EGL_NO_DISPLAY){
        init_egl(pd);
    }else {
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(ctx->display, ctx->surface);
        eglDestroySurface(ctx->display, ctx->surface);

        ctx->surface = eglCreateWindowSurface(ctx->display, ctx->config, ctx->window, NULL);
        EGLint err = eglGetError();
        if (err != EGL_SUCCESS) {
            LOGE("egl error");
        }
        if (eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context) == EGL_FALSE) {
            LOGE("------EGL-FALSE");
        }
        eglQuerySurface(ctx->display, ctx->surface, EGL_WIDTH, &ctx->width);
        eglQuerySurface(ctx->display, ctx->surface, EGL_HEIGHT, &ctx->height);
    }
}

static int stop(player_data *pd) {
    // remove buffer call back
    pd->video_packet_queue->empty_cb = NULL;
    pd->video_packet_queue->full_cb = NULL;

    clean_queues(pd);
    // 停止各个thread
    void *thread_res;
    pthread_join(pd->read_stream_thread, &thread_res);
    if ((pd->av_track_flags & HAS_VIDEO_FLAG) > 0) {
        pthread_join(pd->video_decode_thread, &thread_res);
        xl_mediacodec_release_context(pd);
        pthread_join(pd->gl_thread, &thread_res);
    }

    clean_queues(pd);
    avformat_close_input(&pd->format_context);
    reset(pd);
    LOGI("player stoped");
    return 0;
}

static int message_callback(int fd, int events, void *data) {
    player_data *pd = data;
    int message;
    for (int i = 0; i < events; i++) {
        read(fd, &message, sizeof(int));
        LOGI("recieve message ==> %d", message);
        switch (message) {
            case MESSAGE_STOP:
                stop(pd);
                break;
            case MESSAGE_BUFFER_EMPTY:
                change_status(pd, BUFFER_EMPTY);
                break;
            case MESSAGE_BUFFER_FULL:
                change_status(pd, BUFFER_FULL);
                break;
            case MESSAGE_ERROR:
                on_error(pd);
                break;
            default:
                break;
        }
    }
    return 1;
}


mediacodec_context *create_mediacodec_context(player_data *pd) {
    xl_mediacodec_context *ctx = (xl_mediacodec_context *) malloc(sizeof(xl_mediacodec_context));
    AVCodecParameters *codecpar = pd->format_context->streams[pd->video_index]->codecpar;
    ctx->width = codecpar->width;
    ctx->height = codecpar->height;
    ctx->codec_id = codecpar->codec_id;
    ctx->nal_size = 0;
    ctx->format = AMediaFormat_new();
    ctx->pix_format = AV_PIX_FMT_NONE;
//    "video/x-vnd.on2.vp8" - VP8 video (i.e. video in .webm)
//    "video/x-vnd.on2.vp9" - VP9 video (i.e. video in .webm)
//    "video/avc" - H.264/AVC video
//    "video/hevc" - H.265/HEVC video
//    "video/mp4v-es" - MPEG4 video
//    "video/3gpp" - H.263 video
    switch (ctx->codec_id) {
        case AV_CODEC_ID_H264:
            ctx->codec = AMediaCodec_createDecoderByType("video/avc");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/avc");
            if (codecpar->extradata[0] == 1) {
                size_t sps_size, pps_size;
                uint8_t *sps_buf;
                uint8_t *pps_buf;
                sps_buf = (uint8_t *) malloc((size_t) codecpar->extradata_size + 20);
                pps_buf = (uint8_t *) malloc((size_t) codecpar->extradata_size + 20);
                if (0 != convert_sps_pps2(codecpar->extradata, (size_t) codecpar->extradata_size,
                                          sps_buf, &sps_size, pps_buf, &pps_size, &ctx->nal_size)) {
                    LOGE("%s:convert_sps_pps: failed\n", __func__);
                }
                AMediaFormat_setBuffer(ctx->format, "csd-0", sps_buf, sps_size);
                AMediaFormat_setBuffer(ctx->format, "csd-1", pps_buf, pps_size);
                free(sps_buf);
                free(pps_buf);
            }
            break;
        case AV_CODEC_ID_HEVC:
            ctx->codec = AMediaCodec_createDecoderByType("video/hevc");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/hevc");
            if (codecpar->extradata_size > 3 &&
                (codecpar->extradata[0] == 1 || codecpar->extradata[1] == 1)) {
                size_t sps_pps_size = 0;
                size_t convert_size = (size_t) (codecpar->extradata_size + 20);
                uint8_t *convert_buf = (uint8_t *) malloc((size_t) convert_size);
                if (0 != convert_hevc_nal_units(codecpar->extradata, (size_t) codecpar->extradata_size,
                                                convert_buf, convert_size, &sps_pps_size,
                                                &ctx->nal_size)) {
                    LOGE("%s:convert_sps_pps: failed\n", __func__);
                }
                AMediaFormat_setBuffer(ctx->format, "csd-0", convert_buf, sps_pps_size);
                free(convert_buf);
            }
            break;
        case AV_CODEC_ID_MPEG4:
            ctx->codec = AMediaCodec_createDecoderByType("video/mp4v-es");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/mp4v-es");
            AMediaFormat_setBuffer(ctx->format, "csd-0", codecpar->extradata,
                                   (size_t) codecpar->extradata_size);
            break;
        case AV_CODEC_ID_VP8:
            ctx->codec = AMediaCodec_createDecoderByType("video/x-vnd.on2.vp8");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/x-vnd.on2.vp8");
            break;
        case AV_CODEC_ID_VP9:
            ctx->codec = AMediaCodec_createDecoderByType("video/x-vnd.on2.vp9");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/x-vnd.on2.vp9");
            break;
        case AV_CODEC_ID_H263:
            ctx->codec = AMediaCodec_createDecoderByType("video/3gpp");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/3gpp");
            AMediaFormat_setBuffer(ctx->format, "csd-0", codecpar->extradata,
                                   (size_t) codecpar->extradata_size);
            break;
        default:
            break;
    }
//    AMediaFormat_setInt32(ctx->format, "rotation-degrees", 90);
    AMediaFormat_setInt32(ctx->format, AMEDIAFORMAT_KEY_WIDTH, ctx->width);
    AMediaFormat_setInt32(ctx->format, AMEDIAFORMAT_KEY_HEIGHT, ctx->height);
//    media_status_t ret = AMediaCodec_configure(ctx->codec, ctx->format, NULL, NULL, 0);

    return ctx;
}


#pragma endregion


//extern "C" JNIEXPORT int JNICALL//cpp
JNIEXPORT int JNICALL
Java_com_stanley_virtualmonitor_JNIUtils_CorePlayer(JNIEnv *env, jobject player_instance, jstring input_jstr, jobject surface,int samplerate)
{
    #pragma region 初始化部分
    player_data *pd = (player_data *) malloc(sizeof(player_data));
    pd->jniEnv = env;
/*
    *env->GetJavaVM(env,&pd->vm);//??1 为何需要获得此项？
*/
    pd->player = (*pd->jniEnv)->NewGlobalRef(pd->jniEnv, player_instance);
    jni_reflect_java_class(&pd->jc, pd->jniEnv);
    pd->sample_rate=samplerate;
    pd->buffer_size_max=DEFAULT_BUFFERSIZE;
    pd->buffer_time_length=DEFAULT_BUFFERTIME;
    pd->read_timeout=DEFAULT_READ_TIMEOUT;
    pd->video_packet_queue = queue_create(100);
    pd->video_frame_pool = frame_pool_create(6);
    pd->video_frame_queue = frame_queue_create(4);
    pd->packet_pool = packet_pool_create(400);
pd->statistics = statistics_create(pd->jniEnv);
 av_register_all();
//    avfilter_register_all();
    avformat_network_init();
pd->video_render_ctx = video_render_ctx_create();
    pd->main_looper = ALooper_forThread();
pipe(pd->pipe_fd);
  if (ALooper_addFd(pd->main_looper, pd->pipe_fd[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
                      message_callback, pd)!=1) {
        LOGE("error. when add fd to main looper");
    }
    // pd->change_status = change_status;
    pd->send_message = send_message;
    pd->on_error = on_error_cb;
    reset_pd(pd);

    pd->mediacodec_ctx =create_mediacodec_context(pd);

#pragma endregion

#pragma region开始解码播放


#pragma endregion

}

