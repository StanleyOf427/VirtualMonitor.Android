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

 #include <android/log.h>
 #include <jni.h>
// #include <cstdio>
// #include <cstdlib>

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
    int run_android_version;
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
    xl_video_render_context *video_render_ctx;
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


#pragma endregion

#pragma region 工具函数

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

    jclass java_SurfaceTextureBridge = (*jniEnv)->FindClass(jniEnv, "com/xl/media/hwdecode/SurfaceTextureBridge");
    jc->texture_getSurface = (*jniEnv)->GetStaticMethodID(jniEnv, java_SurfaceTextureBridge, "getSurface", "(I)Landroid/view/Surface;");
    jc->texture_updateTexImage = (*jniEnv)->GetStaticMethodID(jniEnv, java_SurfaceTextureBridge, "updateTexImage", "()V");
    jc->texture_getTransformMatrix = (*jniEnv)->GetStaticMethodID(jniEnv, java_SurfaceTextureBridge, "getTransformMatrix",  "()[F");
    jc->texture_release = (*jniEnv)->GetStaticMethodID(jniEnv, java_SurfaceTextureBridge, "release", "()V");
    jc->SurfaceTextureBridge = (*jniEnv)->NewGlobalRef(jniEnv, java_SurfaceTextureBridge);
    (*jniEnv)->DeleteLocalRef(jniEnv, java_SurfaceTextureBridge);

    *p_jc = jc;
}

#pragma endregion

//extern "C" JNIEXPORT int JNICALL//cpp
JNIEXPORT int JNICALL
Java_com_stanley_virtualmonitor_JNIUtils_CorePlayer(JNIEnv *env, jclass jcls, jstring input_jstr, jobject surface)
{
    #pragma region 初始化部分
    player_data *pd = (player_data *) malloc(sizeof(player_data));
    pd->jniEnv = env;
/*
    *env->GetJavaVM(env,&pd->vm);//??1 为何需要获得此项？
*/
    pd->Player = (*pd->jniEnv)->NewGlobalRef(pd->jniEnv, instance);
xl_jni_reflect_java_class(&pd->jc, pd->jniEnv);


#pragma endregion
}

