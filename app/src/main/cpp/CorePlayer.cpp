/**Create by Stanley
 * 
 * for hardware decode and render in native level
 * */
extern "C"{
// #include "include/libavcodec/avcodec.h"

}

// #include <android/native_window.h>
// #include <android/native_window_jni.h>

// #include <android/log.h>
// #include <jni.h>
// #include <cstdio>
// #include <cstdlib>

#define LOGI(FORMAT, ...)                                                      \
  __android_log_print(ANDROID_LOG_INFO, "jason", FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT, ...)                                                      \
  __android_log_print(ANDROID_LOG_ERROR, "jason", FORMAT, ##__VA_ARGS__);

#pragma region 结构体
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

    void (*change_status)(struct struct_xl_play_data *pd, PlayStatus status);

    void (*on_error)(struct struct_xl_play_data * pd, int error_code);
} player_data;


#pragma endregion


extern "C" JNIEXPORT int JNICALL
Java_com_stanley_virtualmonitor_JNIUtils_CorePlayer(JNIEnv *env, jclass jcls, jstring input_jstr, jobject surface)
{
    #pragma region 初始化部分
    player_data *pd = (player_data *) malloc(sizeof(player_data));
    pd->jniEnv = env;
    *env->GetJavaVM(env,&pd->vm);//?1
    pd->Player = (*pd->jniEnv)->NewGlobalRef(pd->jniEnv, instance);


    #pragma endregion
}