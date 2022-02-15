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
#include  "include/libavutil/frame.h"
#include "include/libswresample/swresample.h"
#include "include/libswscale/swscale.h"

 #include <android/native_window.h>
 #include <android/native_window_jni.h>
#include <android/looper.h>
#include <android/log.h>
#include  <android/sensor.h>

#include <unistd.h>
#include <jni.h>

#include <pthread.h>
#include <sys/prctl.h>
#include  <GLES2/gl2.h>
#include  <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#include <GLES3/gl3.h>
#include  <EGL/egl.h>

#include <stdbool.h>
#include <stdlib.h>

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

// error code
#define ERROR_VIDEO_MEDIACODEC_RECEIVE_FRAME 501

#define HAS_VIDEO_FLAG 0x2

#define NO_CMD 0
#define CMD_SET_WINDOW 1
#define CMD_CHANGE_MODEL 1 << 1
#define CMD_CHANGE_SCALE 1 << 2
#define CMD_CHANGE_ROTATION 1 << 3

// 100 ms
#define NULL_LOOP_SLEEP_US 100000
// 10 ms
#define BUFFER_EMPTY_SLEEP_US 10000
// 30 fps
#define WAIT_FRAME_SLEEP_US 33333

#define PIX_FMT_EGL_EXT 10000

#define STR(s) #s

#pragma endregion

#pragma region 函数定义声明
static void init_rect_nv12();
static void init_rect_yuv_420p();
static void init_rect_oes();
static void init_ball_nv12();
static void init_ball_yuv_420p();
static void init_ball_oes();
static void init_expand_nv12();
static void init_expand_yuv_420p();
static void init_expand_oes();

#pragma endregion

#define LOGI(FORMAT, ...)                                                      \
  __android_log_print(ANDROID_LOG_INFO, "jason", FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT, ...)                                                      \
  __android_log_print(ANDROID_LOG_ERROR, "jason", FORMAT, ##__VA_ARGS__);

#pragma region 枚举类型
typedef enum {
    UNINIT = -1, IDEL = 0, PLAYING=1, PAUSED=2, BUFFER_EMPTY=3, BUFFER_FULL=4
} PlayStatus;
typedef enum {
    NONE = -1,Rect = 0, Ball = 1, VR = 2, Planet = 3, Architecture = 4, Expand = 5
} ModelType;
typedef enum enum__draw_mode {
    wait_frame, fixed_frequency
} draw_mode;

#pragma endregion

#pragma region 结构体

static bool run = false;
typedef struct ASensorManager ASensorManager;
typedef struct ASensor ASensor;
typedef struct ASensorEventQueue ASensorEventQueue;
typedef struct ekf_context_struct {
    ASensorManager *sensor_manager;
    ASensor const *acc;
    ASensor const *gyro;
    int acc_min_delay, gyro_min_delay;
    ALooper *looper;
    ASensorEventQueue *event_queue;
    pthread_mutex_t * lock;
} ekf_context;
static ekf_context * c;


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



typedef struct struct_player_java_class {
    jmethodID player_onPlayStatusChanged;
    jmethodID player_onPlayError;

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


typedef struct struct_glsl_program {
    uint8_t has_init;

    void (*init)();

    GLuint program;
    GLint positon_location;
    GLint texcoord_location;
    GLint linesize_adjustment_location;
    GLint x_scale_location;
    GLint y_scale_location;
    GLint frame_rotation_location;
    GLint modelMatrixLoc;
    GLint viewMatrixLoc;
    GLint projectionMatrixLoc;
    GLint tex_y, tex_u, tex_v;
    GLint texture_matrix_location;
} glsl_program;

typedef struct struct_glsl_program_distortion{
    uint8_t has_init;
    GLuint program;
    GLint positon_location;
    GLint texcoord_r_location;
    GLint texcoord_g_location;
    GLint texcoord_b_location;
    GLint vignette_location;
    GLint texcoord_scale_location;
    GLint tex;
} glsl_program_distortion;

typedef struct struct_model {
    ModelType type;
    enum AVPixelFormat pixel_format;
    GLuint vbos[3];
    size_t elementsCount;
    GLuint texture[4];
    glsl_program * program;

    GLfloat modelMatrix[16];
    GLfloat view_matrix[16];
    GLfloat projectionMatrix[16];
    GLfloat head_matrix[16];
    GLfloat texture_matrix[16];
    float width_adjustment;
    int viewport_w, viewport_h;

    // only RECT Model use
    float x_scale, y_scale;
    int frame_rotation;

    //only VR Model use
//    GLfloat view_matrix_r[16];
//    xl_eye left_eye, right_eye;
//    xl_glsl_program_distortion * program_distortion;
    GLuint frame_texture_id;
    GLuint color_render_id;
    GLuint frame_buffer_id;

    void (*draw)(struct struct_model *model);

    void (*resize)(struct struct_model *model, int w, int h);

    void (*updateTexture)(struct struct_model *model, AVFrame *frame);

    void (*bind_texture)(struct struct_model *model);

    void (*update_frame)(struct struct_model *model, AVFrame *frame);

    // rx ry rz ∈ [-2π, 2π]
    void (*updateModelRotation)(struct struct_model *model, GLfloat rx, GLfloat ry, GLfloat rz, bool enable_tracker);

    // distance ∈ [-1.0, 1.0]
    void (*update_distance)(struct struct_model *model, GLfloat distance);

    void (*updateHead)(struct struct_model *model, GLfloat *headMatrix);
} player_model;

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
    player_model *model;//VR用
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
} player_statistics;

typedef struct struct_player_data {
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

    // 播放器状态
    PlayStatus status;

    // 各个线程
    pthread_t read_stream_thread;
    pthread_t video_decode_thread;
    pthread_t gl_thread;

    // 封装
    AVFormatContext *format_context;
    int video_index;
    uint8_t av_track_flags;
    uint64_t timeout_start;

    // packet容器
    packet_queue *video_packet_queue;
    pakcet_pool *packet_pool;

    // frame容器
    frame_pool *video_frame_pool;
    frame_queue *video_frame_queue;

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
    player_statistics *statistics;

    // message
    ALooper *main_looper;
    int pipe_fd[2];

    void (*send_message)(struct struct_player_data *pd, int message);

    void (*change_status)(struct struct_player_data *pd, PlayStatus status);

    void (*on_error)(struct struct_player_data * pd, int error_code);
} player_data;

typedef struct H264ConvertState {
    uint32_t nal_len;
    uint32_t nal_pos;
} H264ConvertState;

static void buffer_empty_cb(void *data) {
    player_data *pd = data;
    if (pd->status != BUFFER_EMPTY && !pd->eof) {
        pd->send_message(pd,MESSAGE_BUFFER_EMPTY);
    }
}

typedef struct struct_clock {
    int64_t update_time;
    int64_t pts;
} player_clock;

typedef struct {
    float* pp;
    float* tt;
    unsigned int * index;
    int ppLen, ttLen, indexLen;
} player_mesh;

#pragma region glsl_program部分 glsl
static const char * vs_rect = STR(
        attribute vec3 position;
        attribute vec2 texcoord;
        uniform float width_adjustment;
        uniform float x_scale;
        uniform float y_scale;
        uniform int frame_rotation;
        varying vec2 tx;

        const mat2 rotation90 = mat2(
                0.0, -1.0,
                1.0, 0.0
        );
        const mat2 rotation180 = mat2(
                -1.0, 0.0,
                0.0, -1.0
        );
        const mat2 rotation270 = mat2(
                0.0, 1.0,
                -1.0, 0.0
        );
        void main(){
            tx = vec2(texcoord.x * width_adjustment, texcoord.y);
            vec2 xy = vec2(position.x * x_scale, position.y * y_scale);
            if(frame_rotation == 1){
                xy = rotation90 * xy;
            }else if(frame_rotation == 2){
                xy = rotation180 * xy;
            }else if(frame_rotation == 3){
                xy = rotation270 * xy;
            }
            gl_Position = vec4(xy, position.z, 1.0);
        }
);

static const char * vs_ball = STR(
        attribute vec3 position;
        attribute vec2 texcoord;
        uniform float width_adjustment;
        uniform mat4 modelMatrix;
        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;
        varying vec2 tx;

        void main(){
            tx = vec2(texcoord.x * width_adjustment, texcoord.y);
            gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
        }
);

static const char * vs_expand = STR(
        attribute vec4 position;
        attribute vec2 texcoord;
        uniform float width_adjustment;
        uniform mat4 modelMatrix;
        varying vec2 tx;
        void main(void){
            tx = vec2(texcoord.x * width_adjustment, texcoord.y);

            vec4 ballPos = modelMatrix * position;
            float j = asin(ballPos.y);
            float i = degrees(acos(ballPos.x / cos(j)));
            i -= 180.0;
            j = degrees(j);
            if(ballPos.z < 0.0){ i = -i; }
            float xx = i / 180.0 * 1.1;
            float yy = j / 90.0 * 1.1;
            gl_Position = vec4(xx, yy, 0.1, 1.0);
        }
);

static const char * fs_egl_ext = "#extension GL_OES_EGL_image_external : require\n"
                                 "precision mediump float;\n"
                                 "uniform mat4 tx_matrix;\n"
                                 "uniform samplerExternalOES tex_y;\n"
                                 "varying vec2 tx;\n"
                                 "void main(){\n"
                                 "    vec2 tx_transformed = (tx_matrix * vec4(tx, 0, 1.0)).xy;\n"
                                 "    gl_FragColor = texture2D(tex_y, tx_transformed);\n"
                                 "}\n";

static const char * fs_nv12 = STR(
        precision mediump float;
        varying vec2 tx;
        uniform sampler2D tex_y;
        uniform sampler2D tex_u;
        void main(void)
        {
            vec2 tx_flip_y = vec2(tx.x, 1.0 - tx.y);
            float y = texture2D(tex_y, tx_flip_y).r;
            vec4 uv = texture2D(tex_u, tx_flip_y);
            float u = uv.r - 0.5;
            float v = uv.a - 0.5;
            float r = y +             1.402 * v;
            float g = y - 0.344 * u - 0.714 * v;
            float b = y + 1.772 * u;
            gl_FragColor = vec4(r, g, b, 1.0);
        }
);

static const char * fs_yuv_420p = STR(
        precision mediump float;
        varying vec2 tx;
        uniform sampler2D tex_y;
        uniform sampler2D tex_u;
        uniform sampler2D tex_v;
        void main(void)
        {
            vec2 tx_flip_y = vec2(tx.x, 1.0 - tx.y);
            float y = texture2D(tex_y, tx_flip_y).r;
            float u = texture2D(tex_u, tx_flip_y).r - 0.5;
            float v = texture2D(tex_v, tx_flip_y).r - 0.5;
            float r = y +             1.402 * v;
            float g = y - 0.344 * u - 0.714 * v;
            float b = y + 1.772 * u;
            gl_FragColor = vec4(r, g, b, 1.0);
        }
);

static const char * vs_distortion = STR(
        attribute vec2 aPosition;
        attribute float aVignette;
        attribute vec2 aRedTextureCoord;
        attribute vec2 aGreenTextureCoord;
        attribute vec2 aBlueTextureCoord;
        varying vec2 vRedTextureCoord;
        varying vec2 vBlueTextureCoord;
        varying vec2 vGreenTextureCoord;
        varying float vVignette;
        uniform float uTextureCoordScale;
        void main() {
            gl_Position = vec4(aPosition, 0.0, 1.0);
            vRedTextureCoord = aRedTextureCoord.xy * uTextureCoordScale;
            vGreenTextureCoord = aGreenTextureCoord.xy * uTextureCoordScale;
            vBlueTextureCoord = aBlueTextureCoord.xy * uTextureCoordScale;
            vVignette = aVignette;
        }
);

static const char * fs_distortion = STR(
        precision mediump float;
        varying vec2 vRedTextureCoord;
        varying vec2 vBlueTextureCoord;
        varying vec2 vGreenTextureCoord;
        varying float vVignette;
        uniform sampler2D uTextureSampler;
        void main() {
            gl_FragColor = vVignette * vec4(texture2D(uTextureSampler, vRedTextureCoord).r,
                                            texture2D(uTextureSampler, vGreenTextureCoord).g,
                                            texture2D(uTextureSampler, vBlueTextureCoord).b, 1.0);
        }
);

static glsl_program rect_nv12 = {
        .has_init = 0,
        .init = init_rect_nv12
};
static glsl_program rect_yuv_420p = {
        .has_init = 0,
        .init = init_rect_yuv_420p
};
static glsl_program rect_oes = {
        .has_init = 0,
        .init = init_rect_oes
};
static glsl_program ball_nv12 = {
        .has_init = 0,
        .init = init_ball_nv12
};
static glsl_program ball_yuv_420p = {
        .has_init = 0,
        .init = init_ball_yuv_420p
};
static glsl_program ball_oes = {
        .has_init = 0,
        .init = init_ball_oes
};
static glsl_program expand_nv12 = {
        .has_init = 0,
        .init = init_expand_nv12
};
static glsl_program expand_yuv_420p = {
        .has_init = 0,
        .init = init_expand_yuv_420p
};
static glsl_program expand_oes = {
        .has_init = 0,
        .init = init_expand_oes
};
static glsl_program_distortion distortion = {
        .has_init = 0
};

static GLuint loadShader(GLenum shaderType, const char * shaderSrc){
    GLuint shader;
    GLint compiled;
    shader = glCreateShader(shaderType);
    if(shader == 0) return 0;
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled){
        GLint infoLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 0){
            char * infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            LOGE("compile shader error ==>\n%s\n\n%s\n", shaderSrc, infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


static GLuint loadProgram(const char * vsSrc, const char * fsSrc){
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fsSrc);
    if(vertexShader == 0 || fragmentShader == 0) return 0;
    GLint linked;
    GLuint pro = glCreateProgram();
    if(pro == 0){
        LOGE("create program error!");
    }
    glAttachShader(pro, vertexShader);
    glAttachShader(pro, fragmentShader);
    glLinkProgram(pro);
    glGetProgramiv(pro, GL_LINK_STATUS, &linked);
    if(!linked){
        GLint infoLen;
        glGetProgramiv(pro, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 0){
            char * infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(pro, infoLen, NULL, infoLog);
            LOGE("link program error ==>\n%s\n", infoLog);
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(pro);
        return 0;
    }
    return pro;
}

static void init_rect_nv12(){
    GLuint pro = loadProgram(vs_rect, fs_nv12);
    rect_nv12.program = pro;
    rect_nv12.positon_location = glGetAttribLocation(pro, "position");
    rect_nv12.texcoord_location = glGetAttribLocation(pro, "texcoord");
    rect_nv12.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    rect_nv12.x_scale_location = glGetUniformLocation(pro, "x_scale");
    rect_nv12.y_scale_location = glGetUniformLocation(pro, "y_scale");
    rect_nv12.frame_rotation_location = glGetUniformLocation(pro, "frame_rotation");
    rect_nv12.tex_y = glGetUniformLocation(pro, "tex_y");
    rect_nv12.tex_u = glGetUniformLocation(pro, "tex_u");
}

static void init_rect_yuv_420p(){
    GLuint pro = loadProgram(vs_rect, fs_yuv_420p);
    rect_yuv_420p.program = pro;
    rect_yuv_420p.positon_location = glGetAttribLocation(pro, "position");
    rect_yuv_420p.texcoord_location = glGetAttribLocation(pro, "texcoord");
    rect_yuv_420p.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    rect_yuv_420p.x_scale_location = glGetUniformLocation(pro, "x_scale");
    rect_yuv_420p.y_scale_location = glGetUniformLocation(pro, "y_scale");
    rect_yuv_420p.frame_rotation_location = glGetUniformLocation(pro, "frame_rotation");
    rect_yuv_420p.tex_y = glGetUniformLocation(pro, "tex_y");
    rect_yuv_420p.tex_u = glGetUniformLocation(pro, "tex_u");
    rect_yuv_420p.tex_v = glGetUniformLocation(pro, "tex_v");
}

static void init_rect_oes(){
    GLuint pro = loadProgram(vs_rect, fs_egl_ext);
    rect_oes.program = pro;
    rect_oes.positon_location = glGetAttribLocation(pro, "position");
    rect_oes.texcoord_location = glGetAttribLocation(pro, "texcoord");
    rect_oes.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    rect_oes.x_scale_location = glGetUniformLocation(pro, "x_scale");
    rect_oes.y_scale_location = glGetUniformLocation(pro, "y_scale");
    rect_oes.frame_rotation_location = glGetUniformLocation(pro, "frame_rotation");
    rect_oes.tex_y = glGetUniformLocation(pro, "tex_y");
    rect_oes.texture_matrix_location = glGetUniformLocation(pro, "tx_matrix");
}

static void init_ball_nv12(){
    GLuint pro = loadProgram(vs_ball, fs_nv12);
    ball_nv12.program = pro;
    ball_nv12.positon_location = glGetAttribLocation(pro, "position");
    ball_nv12.texcoord_location = glGetAttribLocation(pro, "texcoord");
    ball_nv12.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    ball_nv12.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    ball_nv12.viewMatrixLoc = glGetUniformLocation(pro, "viewMatrix");
    ball_nv12.projectionMatrixLoc = glGetUniformLocation(pro, "projectionMatrix");
    ball_nv12.tex_y = glGetUniformLocation(pro, "tex_y");
    ball_nv12.tex_u = glGetUniformLocation(pro, "tex_u");
}

static void init_ball_yuv_420p() {
    GLuint pro = loadProgram(vs_ball, fs_yuv_420p);
    ball_yuv_420p.program = pro;
    ball_yuv_420p.positon_location = glGetAttribLocation(pro, "position");
    ball_yuv_420p.texcoord_location = glGetAttribLocation(pro, "texcoord");
    ball_yuv_420p.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    ball_yuv_420p.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    ball_yuv_420p.viewMatrixLoc = glGetUniformLocation(pro, "viewMatrix");
    ball_yuv_420p.projectionMatrixLoc = glGetUniformLocation(pro, "projectionMatrix");
    ball_yuv_420p.tex_y = glGetUniformLocation(pro, "tex_y");
    ball_yuv_420p.tex_u = glGetUniformLocation(pro, "tex_u");
    ball_yuv_420p.tex_v = glGetUniformLocation(pro, "tex_v");
}

static void init_ball_oes(){
    GLuint pro = loadProgram(vs_ball, fs_egl_ext);
    ball_oes.program = pro;
    ball_oes.positon_location = glGetAttribLocation(pro, "position");
    ball_oes.texcoord_location = glGetAttribLocation(pro, "texcoord");
    ball_oes.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    ball_oes.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    ball_oes.viewMatrixLoc = glGetUniformLocation(pro, "viewMatrix");
    ball_oes.projectionMatrixLoc = glGetUniformLocation(pro, "projectionMatrix");
    ball_oes.tex_y = glGetUniformLocation(pro, "tex_y");
    ball_oes.texture_matrix_location = glGetUniformLocation(pro, "tx_matrix");
}

static void init_expand_nv12(){
    GLuint pro = loadProgram(vs_expand, fs_nv12);
    expand_nv12.program = pro;
    expand_nv12.positon_location = glGetAttribLocation(pro, "position");
    expand_nv12.texcoord_location = glGetAttribLocation(pro, "texcoord");
    expand_nv12.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    expand_nv12.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    expand_nv12.tex_y = glGetUniformLocation(pro, "tex_y");
    expand_nv12.tex_u = glGetUniformLocation(pro, "tex_u");
}

static void init_expand_yuv_420p(){
    GLuint pro = loadProgram(vs_expand, fs_yuv_420p);
    expand_yuv_420p.program = pro;
    expand_yuv_420p.positon_location = glGetAttribLocation(pro, "position");
    expand_yuv_420p.texcoord_location = glGetAttribLocation(pro, "texcoord");
    expand_yuv_420p.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    expand_yuv_420p.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    expand_yuv_420p.tex_y = glGetUniformLocation(pro, "tex_y");
    expand_yuv_420p.tex_u = glGetUniformLocation(pro, "tex_u");
    expand_yuv_420p.tex_v = glGetUniformLocation(pro, "tex_v");
}

static void init_expand_oes(){
    GLuint pro = loadProgram(vs_expand, fs_egl_ext);
    expand_oes.program = pro;
    expand_oes.positon_location = glGetAttribLocation(pro, "position");
    expand_oes.texcoord_location = glGetAttribLocation(pro, "texcoord");
    expand_oes.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    expand_oes.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    expand_oes.tex_y = glGetUniformLocation(pro, "tex_y");
    expand_oes.texture_matrix_location = glGetUniformLocation(pro, "tx_matrix");
}

glsl_program * xl_glsl_program_get(ModelType type, int pixel_format){
    glsl_program * pro = NULL;
    switch(type){
        case Rect:
            switch(pixel_format){
                case AV_PIX_FMT_NV12:
                    pro = &rect_nv12;
                    break;
                case AV_PIX_FMT_YUV420P:
                    pro = &rect_yuv_420p;
                    break;
                case PIX_FMT_EGL_EXT:
                    pro = &rect_oes;
                    break;
                default:
                    break;
            }
            break;
        case Ball:
        case VR:
        case Planet:
        case Architecture:
            switch(pixel_format){
                case AV_PIX_FMT_NV12:
                    pro = &ball_nv12;
                    break;
                case AV_PIX_FMT_YUV420P:
                    pro = &ball_yuv_420p;
                    break;
                case PIX_FMT_EGL_EXT:
                    pro = &ball_oes;
                    break;
                default:
                    break;
            }
            break;
        case Expand:
            switch(pixel_format){
                case AV_PIX_FMT_NV12:
                    pro = &expand_nv12;
                    break;
                case AV_PIX_FMT_YUV420P:
                    pro = &expand_yuv_420p;
                    break;
                case PIX_FMT_EGL_EXT:
                    pro = &expand_oes;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    if(pro != NULL && pro->has_init == 0){
        pro->init();
        pro->has_init = 1;
    }
    return pro;
}

glsl_program_distortion * xl_glsl_program_distortion_get(){
    if(distortion.has_init == 0){
        GLuint pro = loadProgram(vs_distortion, fs_distortion);
        distortion.program = pro;
        distortion.positon_location = glGetAttribLocation(pro, "aPosition");
        distortion.vignette_location = glGetAttribLocation(pro, "aVignette");
        distortion.texcoord_r_location = glGetAttribLocation(pro, "aRedTextureCoord");
        distortion.texcoord_g_location = glGetAttribLocation(pro, "aGreenTextureCoord");
        distortion.texcoord_b_location = glGetAttribLocation(pro, "aBlueTextureCoord");
        distortion.texcoord_scale_location = glGetUniformLocation(pro, "uTextureCoordScale");
        distortion.tex = glGetUniformLocation(pro, "uTextureSampler");
    }
    return &distortion;
}

void xl_glsl_program_clear_all(){
    glDeleteProgram(rect_nv12.program);
    rect_nv12.has_init = 0;

    glDeleteProgram(rect_yuv_420p.program);
    rect_yuv_420p.has_init = 0;

    glDeleteProgram(rect_oes.program);
    rect_oes.has_init = 0;

    glDeleteProgram(ball_nv12.program);
    ball_nv12.has_init = 0;

    glDeleteProgram(ball_yuv_420p.program);
    ball_yuv_420p.has_init = 0;

    glDeleteProgram(ball_oes.program);
    ball_oes.has_init = 0;

    glDeleteProgram(expand_nv12.program);
    expand_nv12.has_init = 0;

    glDeleteProgram(expand_yuv_420p.program);
    expand_yuv_420p.has_init = 0;

    glDeleteProgram(expand_oes.program);
    expand_oes.has_init = 0;

    glDeleteProgram(distortion.program);
    distortion.has_init = 0;
}


#pragma endregion


#pragma endregion

#pragma  region 全局变量
static pthread_t ekf_tid;
static const int LOOPER_ID_USER = 3;
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

void statistics_reset(player_statistics *s) {
    s->last_update_time = 0;
    s->last_update_bytes = 0;
    s->last_update_frames = 0;
    s->bytes = 0;
    s->frames = 0;
}

void mediacodec_start(player_data *pd){
    mediacodec_context *ctx = pd->mediacodec_ctx;
    while(pd->video_render_ctx->texture_window == NULL){
        usleep(10000);
    }
    media_status_t ret = AMediaCodec_configure(ctx->codec, ctx->format, pd->video_render_ctx->texture_window, NULL, 0);
    if (ret != AMEDIA_OK) {
        LOGE("open mediacodec failed \n");
    }
    ret = AMediaCodec_start(ctx->codec);
    if (ret != AMEDIA_OK) {
        LOGE("open mediacodec failed \n");
    }
}


int mediacodec_receive_frame(player_data *pd, AVFrame *frame) {
    mediacodec_context *ctx = pd->mediacodec_ctx;
    AMediaCodecBufferInfo info;
    int output_ret = 1;
    ssize_t outbufidx = AMediaCodec_dequeueOutputBuffer(ctx->codec, &info, 0);
    if (outbufidx >= 0) {
            frame->pts = info.presentationTimeUs;
            frame->format = PIX_FMT_EGL_EXT;
            frame->width = pd->width;
            frame->linesize[0] = pd->width;
            frame->height = pd->height;
            frame->pkt_pos = outbufidx;
            output_ret = 0;
    } else {
        switch (outbufidx) {
            case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED: {
                int pix_format = -1;
                AMediaFormat *format = AMediaCodec_getOutputFormat(ctx->codec);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &ctx->width);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &ctx->height);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &pix_format);
                //todo 仅支持了两种格式
                switch (pix_format) {
                    case 19:
                        ctx->pix_format = AV_PIX_FMT_YUV420P;
                        break;
                    case 21:
                        ctx->pix_format = AV_PIX_FMT_NV12;
                        break;
                    default:
                        break;
                }
                output_ret = -2;
                break;
            }
            case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
                break;
            case AMEDIACODEC_INFO_TRY_AGAIN_LATER:
                break;
            default:
                break;
        }

    }
    return output_ret;
}

int frame_queue_put(frame_queue *queue, AVFrame *frame){
    pthread_mutex_lock(queue->mutex);
    while(queue->count == queue->size){
        pthread_cond_wait(queue->cond, queue->mutex);
    }
    queue->frames[queue->writeIndex] = frame;
    queue->writeIndex = (queue->writeIndex + 1) % queue->size;
    queue->count++;
    pthread_mutex_unlock(queue->mutex);
    return 0;
}

void frame_pool_unref_frame(frame_pool * pool, AVFrame * frame){
    av_frame_unref(frame);
    pool->count--;
}

void packet_pool_unref_packet(pakcet_pool * pool, AVPacket * packet){
    av_packet_unref(packet);
    pool->count--;
}

void frame_queue_flush(frame_queue *queue, frame_pool *pool){
    pthread_mutex_lock(queue->mutex);
    while(queue->count > 0){
        AVFrame * frame = queue->frames[queue->readIndex];
        if(frame != &queue->flush_frame){
            frame_pool_unref_frame(pool, frame);
        }
        queue->readIndex = (queue->readIndex + 1) % queue->size;
        queue->count--;
    }
    queue->readIndex = 0;
    queue->frames[0] = &queue->flush_frame;
    queue->writeIndex = 1;
    queue->count = 1;
    pthread_cond_signal(queue->cond);
    pthread_mutex_unlock(queue->mutex);
}

AVFrame * frame_pool_get_frame(frame_pool *pool){
    AVFrame * p = &pool->frames[pool->index];
    pool->index = (pool->index + 1) % pool->size;
    pool->count ++;
    return p;
}


static void convert_h264_to_annexb( uint8_t *p_buf, size_t i_len,
                                    size_t i_nal_size,
                                    H264ConvertState *state )
{
    if( i_nal_size < 3 || i_nal_size > 4 )
        return;

    /* NAL 大小 3-4 */
    while( i_len > 0 )
    {
        if( state->nal_pos < i_nal_size ) {
            unsigned int i;
            for( i = 0; state->nal_pos < i_nal_size && i < i_len; i++, state->nal_pos++ ) {
                state->nal_len = (state->nal_len << 8) | p_buf[i];
                p_buf[i] = 0;
            }
            if( state->nal_pos < i_nal_size )
                return;
            p_buf[i - 1] = 1;
            p_buf += i;
            i_len -= i;
        }
        if( state->nal_len > INT_MAX )
            return;
        if( state->nal_len > i_len )
        {
            state->nal_len -= i_len;
            return;
        }
        else
        {
            p_buf += state->nal_len;
            i_len -= state->nal_len;
            state->nal_len = 0;
            state->nal_pos = 0;
        }
    }
}


int mediacodec_send_packet(player_data *pd, AVPacket *packet) {
    mediacodec_context *ctx = pd->mediacodec_ctx;
    if (packet == NULL) { return -2; }
    uint32_t keyframe_flag = 0;
//    av_packet_split_side_data(packet);
    int64_t time_stamp = packet->pts;
    if (!time_stamp && packet->dts)
        time_stamp = packet->dts;
    if (time_stamp > 0) {
        time_stamp = av_rescale_q(time_stamp, pd->format_context->streams[pd->video_index]->time_base,
                                  AV_TIME_BASE_Q);
    } else {
        time_stamp = 0;
    }
    if (ctx->codec_id == AV_CODEC_ID_H264 ||
        ctx->codec_id == AV_CODEC_ID_HEVC) {
        H264ConvertState convert_state = {0, 0};
        convert_h264_to_annexb(packet->data, (size_t) packet->size, ctx->nal_size, &convert_state);
    }
    if ((packet->flags | AV_PKT_FLAG_KEY) > 0) {
        keyframe_flag |= 0x1;
    }
    ssize_t id = AMediaCodec_dequeueInputBuffer(ctx->codec, 1000000);
    media_status_t media_status;
    size_t size;
    if (id >= 0) {
        uint8_t *buf = AMediaCodec_getInputBuffer(ctx->codec, (size_t) id, &size);
        if (buf != NULL && size >= packet->size) {
            memcpy(buf, packet->data, (size_t) packet->size);
            media_status = AMediaCodec_queueInputBuffer(ctx->codec, (size_t) id, 0, (size_t) packet->size,
                                                        (uint64_t) time_stamp,
                                                        keyframe_flag);
            if (media_status != AMEDIA_OK) {
                LOGE("AMediaCodec_queueInputBuffer error. status ==> %d", media_status);
                return (int) media_status;
            }
        }
    }else if(id == AMEDIACODEC_INFO_TRY_AGAIN_LATER){
        return -1;
    }else{
        LOGE("input buffer id < 0  value == %zd", id);
    }
    return 0;
}

void * video_decode_thread(void * data){
    prctl(PR_SET_NAME, __func__);
    player_data * pd = (player_data *)data;
    (*pd->vm)->AttachCurrentThread(pd->vm, &pd->mediacodec_ctx->jniEnv, NULL);
    mediacodec_start(pd);
    int ret;
    AVPacket * packet = NULL;
    AVFrame * frame = frame_pool_get_frame(pd->video_frame_pool);
    while (pd->error_code == 0) {
        ret = mediacodec_receive_frame(pd, frame);
            if (ret == 0) {
                frame_queue_put(pd->video_frame_queue, frame);
                frame = frame_pool_get_frame(pd->video_frame_pool);
            }else if(ret == 1) {
                if(packet == NULL){
                    packet = packet_queue_get(pd->video_packet_queue);
                }
                // buffer empty ==> wait  10ms
                // eof          ==> break
                if(packet == NULL){
                    if(pd->eof){
                        break;
                    }else{
                        usleep(BUFFER_EMPTY_SLEEP_US);
                        continue;
                    }
                }
                // seek
                if(packet == &pd->video_packet_queue->flush_packet){
                    frame_queue_flush(pd->video_frame_queue, pd->video_frame_pool);

                     #if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION
                     mediacodec_context *ctx = pd->mediacodec_ctx;
                    AMediaCodec_flush(ctx->codec);
    #else
    JNIEnv *jniEnv = pd->mediacodec_ctx->jniEnv;
    xl_java_class * jc = pd->jc;
    (*jniEnv)->CallStaticVoidMethod(jniEnv, jc->HwDecodeBridge, jc->codec_flush);
#endif
                    packet = NULL;
                    continue;
                }
                if(0 == mediacodec_send_packet(pd, packet)){
                    packet_pool_unref_packet(pd->packet_pool, packet);
                        packet = NULL;
                }else{
                    // some device AMediacodec input buffer ids count < frame_queue->size
                    // when pause   frame_queue not full
                    // thread will not block in  "xl_frame_queue_put" function
                    if(pd->status == PAUSED){
                        usleep(NULL_LOOP_SLEEP_US);
                    }
                }

            }else if(ret == -2) {
                //frame = xl_frame_pool_get_frame(pd->video_frame_pool);
            }else {
                pd->on_error(pd, ERROR_VIDEO_MEDIACODEC_RECEIVE_FRAME);
                break;
            }
    }
#if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION
    AMediaCodec_stop(pd->mediacodec_ctx->codec);
    #else
JNIEnv *jniEnv = pd->mediacodec_ctx->jniEnv;
    xl_java_class * jc = pd->jc;
    (*jniEnv)->CallStaticVoidMethod(jniEnv, jc->HwDecodeBridge, jc->codec_stop);
#endif

    (*pd->vm)->DetachCurrentThread(pd->vm);
    LOGI("thread ==> %s exit", __func__);
    return NULL;
}



#pragma region 资源释放，清除存储
void frame_queue_free(frame_queue *queue){
    pthread_mutex_destroy(queue->mutex);
    pthread_cond_destroy(queue->cond);
    free(queue->frames);
    free(queue);
}

void clock_reset(player_clock * clock){
    clock->pts = 0;
    clock->update_time = 0;
}

void mediacodec_release_context(player_data *pd) {
    JNIEnv *jniEnv = pd->jniEnv;
    player_java_class * jc = pd->jc;
    (*jniEnv)->CallStaticVoidMethod(jniEnv, jc->HwDecodeBridge, jc->codec_release);
    mediacodec_context *ctx = pd->mediacodec_ctx;
    free(ctx);
    pd->mediacodec_ctx = NULL;
}

void packet_pool_reset(pakcet_pool * pool){
    pool->count = 0;
    pool->index = 0;
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

static void
reset_pd(player_data *pd) {
    if (pd == NULL) return;
    pd->eof = false;
    pd->video_index = -1;
    pd->width = 0;
    pd->height = 0;
    pd->video_frame = NULL;
//    pd->seeking = 0;
    pd->timeout_start = 0;
//    clock_reset(pd->video_clock);
    statistics_reset(pd->statistics);
    pd->error_code = 0;
    packet_pool_reset(pd->packet_pool);
//    pd->change_status(pd, IDEL);
        pd->status = IDEL;
//    (*pd->jniEnv)->CallVoidMethod(pd->jniEnv, pd->player, pd->jc->player_onPlayStatusChanged,
//                                  IDEL);

    video_render_ctx_reset(pd->video_render_ctx);
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

void freeModel(player_model *model) {
    if(model != NULL){
        glDeleteBuffers(3, model->vbos);

        free(model);
    }
}

void free_mesh(player_mesh *p) {
    if (p != NULL) {
        if (p->index != NULL) {
            free(p->index);
        }
        if (p->pp != NULL) {
            free(p->pp);
        }
        if (p->tt != NULL) {
            free(p->tt);
        }
        free(p);
    }
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


static void get_frame_defaults(AVFrame *frame)
{
    // from ffmpeg libavutil frame.c
    if (frame->extended_data != frame->data)
        av_freep(&frame->extended_data);
    memset(frame, 0, sizeof(*frame));
    frame->pts                   =
    frame->pkt_dts               = AV_NOPTS_VALUE;
    frame->best_effort_timestamp = AV_NOPTS_VALUE;
    frame->pkt_duration        = 0;
    frame->pkt_pos             = -1;
    frame->pkt_size            = -1;
    frame->key_frame           = 1;
    frame->sample_aspect_ratio = (AVRational){ 0, 1 };
    frame->format              = -1; /* unknown */
    frame->extended_data       = frame->data;
    frame->color_primaries     = AVCOL_PRI_UNSPECIFIED;
    frame->color_trc           = AVCOL_TRC_UNSPECIFIED;
    frame->colorspace          = AVCOL_SPC_UNSPECIFIED;
    frame->color_range         = AVCOL_RANGE_UNSPECIFIED;
    frame->chroma_location     = AVCHROMA_LOC_UNSPECIFIED;
    frame->flags               = 0;

    frame->width = 0;
    frame->height = 0;
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

player_statistics *statistics_create(JNIEnv *jniEnv) {
    player_statistics *statistics = malloc(sizeof(statistics));
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

void initTexture(player_data * pd) {
    video_render_context * ctx = pd->video_render_ctx;
       //硬解的时候只需要创建一个surfacetexture即可
        glGenTextures(1, &ctx->texture[3]);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, ctx->texture[3]);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void init_egl(player_data * pd){
    video_render_context *ctx = pd->video_render_ctx;
    const EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE,
                              8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
                              EGL_NONE};
    EGLint numConfigs;
    ctx->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint majorVersion, minorVersion;
    eglInitialize(ctx->display, &majorVersion, &minorVersion);
    eglChooseConfig(ctx->display, attribs, &ctx->config, 1, &numConfigs);
    ctx->surface = eglCreateWindowSurface(ctx->display, ctx->config, ctx->window, NULL);
    EGLint attrs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    ctx->context = eglCreateContext(ctx->display, ctx->config, NULL, attrs);
    EGLint err = eglGetError();
    if (err != EGL_SUCCESS) {
        LOGE("egl error");
    }
    if (eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context) == EGL_FALSE) {
        LOGE("------EGL-FALSE");
    }
    eglQuerySurface(ctx->display, ctx->surface, EGL_WIDTH, &ctx->width);
    eglQuerySurface(ctx->display, ctx->surface, EGL_HEIGHT, &ctx->height);
    initTexture(pd);
    player_java_class * jc = pd->jc;
    JNIEnv * jniEnv = pd->video_render_ctx->jniEnv;
    jobject surface_texture = (*jniEnv)->CallStaticObjectMethod(jniEnv, jc->SurfaceTextureBridge, jc->texture_getSurface, ctx->texture[3]);
    ctx->texture_window = ANativeWindow_fromSurface(jniEnv, surface_texture);
}


void texture_delete(player_data * pd){
    video_render_context * ctx = pd->video_render_ctx;
    glDeleteTextures(1, &ctx->texture[3]);
}


void glsl_program_clear_all(){
    glDeleteProgram(rect_nv12.program);
    rect_nv12.has_init = 0;

    glDeleteProgram(rect_yuv_420p.program);
    rect_yuv_420p.has_init = 0;

    glDeleteProgram(rect_oes.program);
    rect_oes.has_init = 0;

    glDeleteProgram(ball_nv12.program);
    ball_nv12.has_init = 0;

    glDeleteProgram(ball_yuv_420p.program);
    ball_yuv_420p.has_init = 0;

    glDeleteProgram(ball_oes.program);
    ball_oes.has_init = 0;

    glDeleteProgram(expand_nv12.program);
    expand_nv12.has_init = 0;

    glDeleteProgram(expand_yuv_420p.program);
    expand_yuv_420p.has_init = 0;

    glDeleteProgram(expand_oes.program);
    expand_oes.has_init = 0;

    glDeleteProgram(distortion.program);
    distortion.has_init = 0;
}

static void release_egl(player_data * pd) {
    video_render_context *ctx = pd->video_render_ctx;
    if (ctx->display == EGL_NO_DISPLAY) return;
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(ctx->display, ctx->surface);
    texture_delete(pd);
    glsl_program_clear_all();
    if(ctx->texture_window != NULL){
        ANativeWindow_release(ctx->texture_window);
        ctx->texture_window = NULL;
    }
    if (ctx->model != NULL) {
        freeModel(ctx->model);
        ctx->model = NULL;
    }
    eglMakeCurrent(ctx->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (ctx->context != EGL_NO_CONTEXT) {
        eglDestroyContext(ctx->display, ctx->context);
    }
    if (ctx->surface != EGL_NO_SURFACE) {
        eglDestroySurface(ctx->display, ctx->surface);
    }
    eglTerminate(ctx->display);
    ctx->display = EGL_NO_DISPLAY;
    ctx->context = EGL_NO_CONTEXT;
    ctx->surface = EGL_NO_SURFACE;
}

void tracker_stop() {
    if(!run) return;
    if(c != NULL){
        ASensorEventQueue_disableSensor(c->event_queue, c->acc);
        ASensorEventQueue_disableSensor(c->event_queue, c->gyro);
    }
    run = false;
    if(c != NULL){
        ALooper_wake(c->looper);
    }
    pthread_join(ekf_tid, NULL);
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
        mediacodec_release_context(pd);
        pthread_join(pd->gl_thread, &thread_res);
    }

    clean_queues(pd);
        avformat_close_input(pd->format_context);
    reset(pd);
    LOGI("player stoped");
    return 0;
}

int player_resume(player_data *pd) {
    pd->change_status(pd, PLAYING);
    return 0;
}

void change_status(player_data *pd, PlayStatus status) {
    if (status == BUFFER_FULL) {
        player_resume(pd);
    } else {
        pd->status = status;
    }
    //更新UI界面部分，暂存
//    (*pd->jniEnv)->CallVoidMethod(pd->jniEnv, pd->player, pd->jc->player_onPlayStatusChanged,
//                                  status);
}

static void on_error(player_data *pd) {
    (*pd->jniEnv)->CallVoidMethod(pd->jniEnv, pd->player, pd->jc->player_onPlayError,
                                  pd->error_code);
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

static int convert_sps_pps2(const uint8_t *p_buf, size_t i_buf_size,
                            uint8_t * out_sps_buf, size_t * out_sps_buf_size,
                            uint8_t * out_pps_buf, size_t * out_pps_buf_size,
                            size_t *p_nal_size) {
    uint32_t i_data_size = (uint32_t) i_buf_size, i_nal_size ;
    unsigned int i_loop_end;

    if (i_data_size < 7) {
        LOGE("Input Metadata too small");
        return -1;
    }

    /* Read infos in first 6 bytes */
    // i_profile    = (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    if (p_nal_size)
        *p_nal_size = (size_t) ((p_buf[4] & 0x03) + 1);
    p_buf += 5;
    i_data_size -= 5;

    for (unsigned int j = 0; j < 2; j++) {
        /* First time is SPS, Second is PPS */
        if (i_data_size < 1) {
            LOGE("PPS too small after processing SPS/PPS %u",
                 i_data_size);
            return -1;
        }
        i_loop_end = (unsigned int) (p_buf[0] & (j == 0 ? 0x1f : 0xff));
        p_buf++;
        i_data_size--;

        for (unsigned int i = 0; i < i_loop_end; i++) {
            if (i_data_size < 2) {
                LOGE("SPS is too small %u", i_data_size);
                return -1;
            }

            i_nal_size = (p_buf[0] << 8) | p_buf[1];
            p_buf += 2;
            i_data_size -= 2;

            if (i_data_size < i_nal_size) {
                LOGE("SPS size does not match NAL specified size %u",
                     i_data_size);
                return -1;
            }
            if(j == 0){
                out_sps_buf[0] = 0;
                out_sps_buf[1] = 0;
                out_sps_buf[2] = 0;
                out_sps_buf[3] = 1;
                memcpy(out_sps_buf + 4, p_buf, i_nal_size);
                * out_sps_buf_size = i_nal_size + 4;
            }else{
                out_pps_buf[0] = 0;
                out_pps_buf[1] = 0;
                out_pps_buf[2] = 0;
                out_pps_buf[3] = 1;
                memcpy(out_pps_buf + 4, p_buf, i_nal_size);
                * out_pps_buf_size = i_nal_size + 4;
            }

            p_buf += i_nal_size;
            i_data_size -= i_nal_size;
        }
    }
    return 0;
}

mediacodec_context *create_mediacodec_context(player_data *pd) {
    mediacodec_context *ctx = (mediacodec_context *) malloc(sizeof(mediacodec_context));
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
//        case AV_CODEC_ID_HEVC:
//            ctx->codec = AMediaCodec_createDecoderByType("video/hevc");
//            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/hevc");
//            if (codecpar->extradata_size > 3 &&
//                (codecpar->extradata[0] == 1 || codecpar->extradata[1] == 1)) {
//                size_t sps_pps_size = 0;
//                size_t convert_size = (size_t) (codecpar->extradata_size + 20);
//                uint8_t *convert_buf = (uint8_t *) malloc((size_t) convert_size);
//                if (0 != convert_hevc_nal_units(codecpar->extradata, (size_t) codecpar->extradata_size,
//                                                convert_buf, convert_size, &sps_pps_size,
//                                                &ctx->nal_size)) {
//                    LOGE("%s:convert_sps_pps: failed\n", __func__);
//                }
//                AMediaFormat_setBuffer(ctx->format, "csd-0", convert_buf, sps_pps_size);
//                free(convert_buf);
//            }
//            break;
        case AV_CODEC_ID_MPEG4:
            ctx->codec = AMediaCodec_createDecoderByType("video/mp4v-es");
            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/mp4v-es");
            AMediaFormat_setBuffer(ctx->format, "csd-0", codecpar->extradata,
                                   (size_t) codecpar->extradata_size);
            break;
//        case AV_CODEC_ID_VP8:
//            ctx->codec = AMediaCodec_createDecoderByType("video/x-vnd.on2.vp8");
//            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/x-vnd.on2.vp8");
//            break;
//        case AV_CODEC_ID_VP9:
//            ctx->codec = AMediaCodec_createDecoderByType("video/x-vnd.on2.vp9");
//            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/x-vnd.on2.vp9");
//            break;
//        case AV_CODEC_ID_H263:
//            ctx->codec = AMediaCodec_createDecoderByType("video/3gpp");
//            AMediaFormat_setString(ctx->format, AMEDIAFORMAT_KEY_MIME, "video/3gpp");
//            AMediaFormat_setBuffer(ctx->format, "csd-0", codecpar->extradata,
//                                   (size_t) codecpar->extradata_size);
//            break;
        default:
            break;
    }
//    AMediaFormat_setInt32(ctx->format, "rotation-degrees", 90);
    AMediaFormat_setInt32(ctx->format, AMEDIAFORMAT_KEY_WIDTH, ctx->width);
    AMediaFormat_setInt32(ctx->format, AMEDIAFORMAT_KEY_HEIGHT, ctx->height);
//    media_status_t ret = AMediaCodec_configure(ctx->codec, ctx->format, NULL, NULL, 0);

    return ctx;
}

static void send_message(player_data *pd, int message) {
    int sig = message;
    write(pd->pipe_fd[1], &sig, sizeof(int));
}

static void on_error_cb(player_data *pd, int error_code) {
    pd->error_code = error_code;
    pd->send_message(pd, MESSAGE_ERROR);
}

static void buffer_full_cb(void *data) {
    player_data *pd = data;
    if (pd->status == BUFFER_EMPTY) {
        pd->send_message(pd, MESSAGE_BUFFER_FULL);
    }
}

static inline void set_buffer_time(player_data *pd) {
    float buffer_time_length = pd->buffer_time_length;
    AVRational time_base;
    if (pd->av_track_flags & HAS_VIDEO_FLAG) {
        time_base = pd->format_context->streams[pd->video_index]->time_base;
        pd->video_packet_queue->max_duration = (uint64_t) (buffer_time_length / av_q2d(time_base));
        pd->video_packet_queue->empty_cb = buffer_empty_cb;
        pd->video_packet_queue->full_cb = buffer_full_cb;
        pd->video_packet_queue->cb_data = pd;
    }
}

static void double_size(pakcet_pool * pool){
    // step1  为指针开辟空间
    AVPacket ** temp_packets = (AVPacket **)av_malloc(sizeof(AVPacket *) * pool->size * 2);
    // step2  复制指针
    memcpy(temp_packets, pool->packets, sizeof(AVPacket *) * pool->size);
    // step3 用av_packet_alloc填充剩余空间
    for(int i = pool->size; i < pool->size * 2; i++){
        temp_packets[i] = av_packet_alloc();
    }
    // step4 释放旧空间
    free(pool->packets);
    pool->packets = temp_packets;
    // step5 当前指针位置移动到后半部分
    pool->index = pool->size;
    pool->size *= 2;
    LOGI("packet pool double size. new size ==> %d", pool->size);
}


AVPacket * packet_pool_get_packet(pakcet_pool * pool){
    if(pool->count > pool->size / 2){
        double_size(pool);
    }
    AVPacket * p = pool->packets[pool->index];
    pool->index = (pool->index + 1) % pool->size;
    pool->count ++;
    return p;
}

int packet_queue_put(packet_queue *queue, AVPacket *packet) {
    pthread_mutex_lock(queue->mutex);
    if (queue->max_duration > 0 && queue->duration + packet->duration > queue->max_duration) {
        if (queue->full_cb != NULL) {
            queue->full_cb(queue->cb_data);
        }
        pthread_cond_wait(queue->cond, queue->mutex);
    }
    if (queue->count == queue->size) {
        double_size(queue);
    }
    queue->duration += packet->duration;
    queue->packets[queue->writeIndex] = packet;
    queue->writeIndex = (queue->writeIndex + 1) % queue->size;
    queue->count++;
    queue->total_bytes += packet->size;
    pthread_mutex_unlock(queue->mutex);
    return 0;
}

static int video_width = 0, video_height = 0;

static inline void resize_video(player_model *model){
    int screen_width = model->viewport_w, screen_height = model->viewport_h;
    if(screen_width == 0
       || screen_height == 0
       || video_width == 0
       || video_height == 0
            ){
        model->x_scale = 1.0f;
        model->y_scale = 1.0f;
    }
    float screen_rate = (float)screen_width / (float)screen_height;
    float video_rate = (float)video_width / (float)video_height;
    if(screen_rate > video_rate){
        model->x_scale = video_rate / screen_rate;
        model->y_scale = 1.0f;
    }else{
        model->x_scale = 1.0f;
        model->y_scale = screen_rate / video_rate;
    }
}

void model_rect_resize(player_model *model, int w, int h) {
    model->viewport_w = w;
    model->viewport_h = h;
    resize_video(model);
}


void bind_texture_yuv420p(player_model *model){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model->texture[0]);
    glUniform1i(model->program->tex_y, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, model->texture[1]);
    glUniform1i(model->program->tex_u, 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, model->texture[2]);
    glUniform1i(model->program->tex_v, 2);
}

void update_texture_nv12(player_model *model, AVFrame *frame){
    glBindTexture(GL_TEXTURE_2D, model->texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[0], frame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);
    glBindTexture(GL_TEXTURE_2D, model->texture[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, frame->linesize[1] / 2, frame->height / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, frame->data[1]);
}

void update_texture_yuv420p(player_model *model, AVFrame *frame){
    glBindTexture(GL_TEXTURE_2D, model->texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[0], frame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);
    glBindTexture(GL_TEXTURE_2D, model->texture[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[1], frame->height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[1]);
    glBindTexture(GL_TEXTURE_2D, model->texture[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[2], frame->height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[2]);
}

void bind_texture_nv12(player_model *model){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model->texture[0]);
    glUniform1i(model->program->tex_y, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, model->texture[1]);
    glUniform1i(model->program->tex_u, 1);
}

void bind_texture_oes(player_model *model){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, model->texture[3]);
    glUniform1i(model->program->tex_y, 0);
    glUniformMatrix4fv(model->program->texture_matrix_location, 1, GL_FALSE, model->texture_matrix);
}

void update_texture_oes(__attribute__((unused)) player_model *model, __attribute__((unused)) AVFrame *frame){

}

void update_frame_rect(player_model *model, AVFrame * frame){
    if(model->pixel_format != frame->format){
        model->pixel_format = (enum AVPixelFormat)frame->format;
        model->program = xl_glsl_program_get(model->type, model->pixel_format);
        glUseProgram(model->program->program);
        switch(frame->format){
            case AV_PIX_FMT_YUV420P:
                model->bind_texture = bind_texture_yuv420p;
                model->updateTexture = update_texture_yuv420p;
                break;
            case AV_PIX_FMT_NV12:
                model->bind_texture = bind_texture_nv12;
                model->updateTexture = update_texture_nv12;
                break;
            case PIX_FMT_EGL_EXT:
                model->bind_texture = bind_texture_oes;
                model->updateTexture = update_texture_oes;
                break;
            default:
                LOGE("not support this pix_format ==> %d", frame->format);
                return;
        }
        glBindBuffer(GL_ARRAY_BUFFER, model->vbos[0]);
        glVertexAttribPointer((GLuint) model->program->positon_location, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray((GLuint) model->program->positon_location);
        glBindBuffer(GL_ARRAY_BUFFER, model->vbos[1]);
        glVertexAttribPointer((GLuint) model->program->texcoord_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray((GLuint) model->program->texcoord_location);
    }
    model->updateTexture(model, frame);
    // some video linesize > width
    model->width_adjustment = (float)frame->width / (float)frame->linesize[0];
    if(frame->width != video_width
       || frame->height != video_height){
        video_width = frame->width;
        video_height = frame->height;
        resize_video(model);
    }
}


static void draw_rect(player_model *model){
    glsl_program * program = model->program;
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, model->viewport_w, model->viewport_h);
    model->bind_texture(model);
    glUniform1f(program->linesize_adjustment_location, model->width_adjustment);
    glUniform1f(program->x_scale_location, model->x_scale);
    glUniform1f(program->y_scale_location, model->y_scale);
    glUniform1i(program->frame_rotation_location, model->frame_rotation);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->vbos[2]);
    glDrawElements(GL_TRIANGLES, (GLsizei) model->elementsCount, GL_UNSIGNED_INT, 0);
}


player_mesh *get_rect_mesh() {
    player_mesh *rect_mesh;
    rect_mesh = (player_mesh *) malloc(sizeof(player_mesh));
    memset(rect_mesh, 0, sizeof(player_mesh));

    rect_mesh->ppLen = 12;
    rect_mesh->ttLen = 8;
    rect_mesh->indexLen = 6;
    rect_mesh->pp = (float *) malloc((size_t) (sizeof(float) * rect_mesh->ppLen));
    rect_mesh->tt = (float *) malloc((size_t) (sizeof(float) * rect_mesh->ttLen));
    rect_mesh->index = (unsigned int *) malloc(sizeof(unsigned int) * rect_mesh->indexLen);
    float pa[12] = {-1, 1, -0.1f, 1, 1, -0.1f, 1, -1, -0.1f, -1, -1, -0.1f};
    float ta[8] = {0, 1, 1, 1, 1, 0, 0, 0};
    unsigned int ia[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 12; ++i) {
        *rect_mesh->pp++ = pa[i];
    }
    for (int i = 0; i < 8; ++i) {
        *rect_mesh->tt++ = ta[i];
    }
    for (int i = 0; i < 6; ++i) {
        *rect_mesh->index++ = ia[i];
    }
    rect_mesh->pp -= rect_mesh->ppLen;
    rect_mesh->tt -= rect_mesh->ttLen;
    rect_mesh->index -= rect_mesh->indexLen;
    return rect_mesh;
}

ekf_context *xl_ekf_context_create() {
    ekf_context *ctx = (ekf_context *) malloc(sizeof(ekf_context));
    memset(ctx, 0, sizeof(ekf_context));
//    ctx->sensor_manager = ASensorManager_getInstanceForPackage("com.cls.xl.xl");
    ctx->sensor_manager = ASensorManager_getInstance();
    ctx->acc = ASensorManager_getDefaultSensor(ctx->sensor_manager, ASENSOR_TYPE_ACCELEROMETER);
    ctx->gyro = ASensorManager_getDefaultSensor(ctx->sensor_manager, ASENSOR_TYPE_GYROSCOPE);
    ctx->acc_min_delay = ASensor_getMinDelay(ctx->acc);
    ctx->gyro_min_delay = ASensor_getMinDelay(ctx->gyro);
    if (ctx->acc == NULL || ctx->gyro == NULL) {
        free(ctx);
        return NULL;
    }
    ctx->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ctx->event_queue = ASensorManager_createEventQueue(
            ctx->sensor_manager,
            ctx->looper,
            LOOPER_ID_USER,
            NULL,
            NULL
    );
    ASensorEventQueue_enableSensor(ctx->event_queue, ctx->acc);
    ASensorEventQueue_enableSensor(ctx->event_queue, ctx->gyro);
    int acc_delay = 20000;
    int gyro_delay = 20000;
    acc_delay = acc_delay > ctx->acc_min_delay ? acc_delay : ctx->acc_min_delay;
    gyro_delay = gyro_delay > ctx->gyro_min_delay ? gyro_delay : ctx->gyro_min_delay;
    ASensorEventQueue_setEventRate(ctx->event_queue, ctx->acc, acc_delay);
    ASensorEventQueue_setEventRate(ctx->event_queue, ctx->gyro, gyro_delay);
    ctx->lock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(ctx->lock, NULL);
    return ctx;
}
//
//void *ekf_thread(__attribute__((unused)) void *data) {
//    prctl(PR_SET_NAME, __func__);
//    LOGI("head tracker thread start");
//    ekf_context * ctx = xl_ekf_context_create();
//    if(ctx == NULL){
//        return NULL;
//    }
//    c = ctx;
//    int ident;
//    int events;
//    struct android_poll_source* source;
//
//    c = NULL;
//    LOGI("head tracker thread exit");
//    return NULL;
//}


player_model * model_rect_create(){
    player_model *model = (player_model *) malloc(sizeof(player_model));
    model->type = Rect;
    model->program = NULL;
    model->draw = NULL;
    model->updateTexture = NULL;
    model->resize = model_rect_resize;
    model->update_frame = update_frame_rect;
    model->draw = draw_rect;
    model->pixel_format = AV_PIX_FMT_NONE;
    model->width_adjustment = 1;
    model->x_scale = 1.0f;
    model->y_scale = 1.0f;

    model->updateModelRotation = NULL;
    model->update_distance = NULL;
    model->updateHead = NULL;

    player_mesh *rect = get_rect_mesh();
    glGenBuffers(3, model->vbos);
    glBindBuffer(GL_ARRAY_BUFFER, model->vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * rect->ppLen, rect->pp, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, model->vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * rect->ttLen, rect->tt, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->vbos[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * rect->indexLen, rect->index,
                 GL_STATIC_DRAW);
    model->elementsCount = (size_t) rect->indexLen;
    free_mesh(rect);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    return model;
}

player_model *createModel(ModelType mType) {
    switch (mType) {
        case Rect:
            return model_rect_create();
        default:
            LOGE("invalid model type");
            return NULL;
    }
}

void player_tracker_start() {
    if(run) return;
    run = true;
    pthread_create(&ekf_tid, NULL,NULL, NULL);
}

void change_model(video_render_context *ctx) {
    if (ctx->model != NULL) {
        freeModel(ctx->model);
    }
    ctx->model = createModel(ctx->require_model_type);
    memcpy(ctx->model->texture, ctx->texture, sizeof(GLuint) * 4);
    ctx->model->resize(ctx->model, ctx->width, ctx->height);
    if (ctx->model->type == Ball || ctx->model->type == VR || ctx->model->type == Architecture) {
        ctx->draw_mode = fixed_frequency;
        player_tracker_start();
    } else {
        ctx->draw_mode = wait_frame;
    }
}

static void set_window(player_data * pd) {
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

static inline void player_release_video_frame(player_data *pd, AVFrame *frame) {
    frame_pool_unref_frame(pd->video_frame_pool, frame);
    pd->video_frame = NULL;
}

static inline void draw_now(video_render_context *ctx) {
//    player_model *model = ctx->model;
    pthread_mutex_lock(ctx->lock);
//    if (ctx->draw_mode == fixed_frequency && ctx->enable_tracker) {
//        xl_tracker_get_last_view(model->head_matrix);
//        model->updateHead(model, model->head_matrix);
//    }
    // The initialization is done
//    if (model->pixel_format != AV_PIX_FMT_NONE) {
//        model->draw(model);
//    }
    eglSwapBuffers(ctx->display, ctx->surface);
    pthread_mutex_unlock(ctx->lock);
}


/**
 *
 * @param pd
 * @param frame
 * @return  0   draw
 *         -1   sleep 33ms  continue
 *         -2   break
 */
static inline int draw_video_frame(player_data *pd) {
    // 上一次可能没有画， 这种情况就不需要取新的了
    if (pd->video_frame == NULL) {
        pd->video_frame = frame_queue_get(pd->video_frame_queue);
    }
    // buffer empty  ==> sleep 10ms , return 0
    // eos           ==> return -2
    if (pd->video_frame == NULL) {
        if (pd->eof) {
            return -2;
        } else {
            usleep(BUFFER_EMPTY_SLEEP_US);
            return 0;
        }
    }

    int64_t time_stamp;
    time_stamp = pd->video_frame->pts;
    player_model *model = pd->video_render_ctx->model;

    pthread_mutex_lock(pd->video_render_ctx->lock);
    model->update_frame(model, pd->video_frame);
    pthread_mutex_unlock(pd->video_render_ctx->lock);
    player_release_video_frame(pd, pd->video_frame);
    JNIEnv * jniEnv = pd->video_render_ctx->jniEnv;
    (*jniEnv)->CallStaticVoidMethod(jniEnv, pd->jc->SurfaceTextureBridge, pd->jc->texture_updateTexImage);
    jfloatArray texture_matrix_array = (*jniEnv)->CallStaticObjectMethod(jniEnv, pd->jc->SurfaceTextureBridge, pd->jc->texture_getTransformMatrix);
    (*jniEnv)->GetFloatArrayRegion(jniEnv, texture_matrix_array, 0, 16, model->texture_matrix);
    (*jniEnv)->DeleteLocalRef(jniEnv, texture_matrix_array);

    draw_now(pd->video_render_ctx);
    pd->statistics->frames ++;
    return 0;
}

uint64_t clock_get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

static int av_format_interrupt_cb(void *data) {
    player_data *pd = data;
    if (pd->timeout_start == 0) {
        pd->timeout_start = clock_get_current_time();
        return 0;
    } else {
        uint64_t time_use = clock_get_current_time() - pd->timeout_start;
        if (time_use > pd->read_timeout * 1000000) {
            pd->on_error(pd, -2);
            return 1;
        } else {
            return 0;
        }
    }
}

// 读文件线程
void *read_thread(void *data) {
    prctl(PR_SET_NAME, __func__);
    player_data *pd = (player_data *) data;
    AVPacket *packet = NULL;
    int ret = 0;
    while (pd->error_code == 0) {
        // get a new packet from pool
        if (packet == NULL) {
            packet = packet_pool_get_packet(pd->packet_pool);
        }
        // read data to packet
        ret = av_read_frame(pd->format_context, packet);
        if (ret == 0) {
            pd->timeout_start = 0;
            if (packet->stream_index == pd->video_index) {
                packet_queue_put(pd->video_packet_queue, packet);
                pd->statistics->bytes += packet->size;
                packet = NULL;
            } else {
                av_packet_unref(packet);
            }
        } else if (ret == AVERROR_INVALIDDATA) {
            pd->timeout_start = 0;
            packet_pool_unref_packet(pd->packet_pool, packet);
        } else if (ret == AVERROR_EOF) {
            pd->eof = true;
            if (pd->status == BUFFER_EMPTY) {
                pd->send_message(pd, MESSAGE_BUFFER_FULL);
            }
            break;
        } else {
            // error
            pd->on_error(pd, ret);
            LOGE("read file error. error code ==> %d", ret);
            break;
        }
    }
    LOGI("thread ==> %s exit", __func__);
    return NULL;
}

void *player_gl_thread(void *data) {
    prctl(PR_SET_NAME, __func__);
    player_data *pd = (player_data *) data;
    video_render_context *ctx = pd->video_render_ctx;
    (*pd->vm)->AttachCurrentThread(pd->vm, &ctx->jniEnv, NULL);
    int ret;
    while (pd->error_code == 0) {
        // 处理egl
        pthread_mutex_lock(ctx->lock);
        if (ctx->cmd != NO_CMD) {
            if ((ctx->cmd & CMD_SET_WINDOW) != 0) {
                set_window(pd);
                change_model(ctx);
            }
            if ((ctx->cmd & CMD_CHANGE_MODEL) != 0) {
                if (ctx->display == EGL_NO_DISPLAY) {
                    if (ctx->window != NULL) {
                        set_window(pd);
                    } else {
                        pthread_mutex_unlock(ctx->lock);
                        usleep(NULL_LOOP_SLEEP_US);
                        continue;
                    }
                }
                change_model(ctx);
            }
            if ((ctx->cmd & CMD_CHANGE_SCALE) != 0 && ctx->model != NULL && ctx->model->update_distance != NULL) {
                ctx->model->update_distance(ctx->model, ctx->require_model_scale);
            }
            if ((ctx->cmd & CMD_CHANGE_ROTATION) != 0 && ctx->model != NULL && ctx->model->updateModelRotation != NULL) {
                ctx->model->updateModelRotation(ctx->model, ctx->require_model_rotation[0],
                                                ctx->require_model_rotation[1],
                                                ctx->require_model_rotation[2],ctx->enable_tracker);
            }
            ctx->cmd = NO_CMD;
        }
        pthread_mutex_unlock(ctx->lock);
        // 处理pd->status
        if (pd->status == PAUSED || pd->status == BUFFER_EMPTY) {
            usleep(NULL_LOOP_SLEEP_US);
        } else if (pd->status == PLAYING) {
            ret = draw_video_frame(pd);
            if (ret == 0) {
                continue;
            } else if (ret == -1) {
                usleep(WAIT_FRAME_SLEEP_US);
                continue;
            } else if (ret == -2) {
                // 如果有视频   就在这发结束信号
                pd->send_message(pd, MESSAGE_STOP);
                break;
            }
        } else if (pd->status == IDEL) {
            usleep(WAIT_FRAME_SLEEP_US);
        } else {
            LOGE("error state  ==> %d", pd->status);
            break;
        }
    }
    release_egl(pd);
    tracker_stop();
    (*pd->vm)->DetachCurrentThread(pd->vm);
    LOGI("thread ==> %s exit", __func__);
    return NULL;
}
#pragma endregion


//extern "C" JNIEXPORT int JNICALL//cpp
JNIEXPORT int JNICALL
Java_com_stanley_virtualmonitor_JNIUtils_CorePlayer(JNIEnv *env, jobject player_instance, jobject surface, jstring input_url,int samplerate)
{
    int ret,i;
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

    if (pd->video_render_ctx->window != NULL) {
        ANativeWindow_release(pd->video_render_ctx->window);
    }
    ANativeWindow *sur = ANativeWindow_fromSurface(env, surface);
    pd->video_render_ctx->set_window(pd->video_render_ctx, sur);

    pd->main_looper = ALooper_forThread();
pipe(pd->pipe_fd);
  if (ALooper_addFd(pd->main_looper, pd->pipe_fd[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
                      message_callback, pd)!=1) {
        LOGE("error. when add fd to main looper");
    }
    // pd->change_status = change_status;
    pd->send_message = send_message;
    pd->on_error = on_error_cb;

   reset_pd(pd);//存疑，是否需要重置？

//    pd->mediacodec_ctx =create_mediacodec_context(pd);

#pragma endregion

#pragma region开始解码播放
    pd->format_context = avformat_alloc_context();
    pd->format_context->interrupt_callback.callback = av_format_interrupt_cb;
    pd->format_context->interrupt_callback.opaque = pd;
        if (avformat_open_input(&pd->format_context, input_url, NULL, NULL) != 0) {
        LOGE("can not open url\n");
        ret = 100;
        goto fail;
    }
   if (avformat_find_stream_info(pd->format_context, NULL) < 0) {
        LOGE("can not find stream\n");
        ret = 101;
        goto fail;
    }
    i = av_find_best_stream(pd->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (i != AVERROR_STREAM_NOT_FOUND) {
        pd->video_index = i;
        pd->av_track_flags |= HAS_VIDEO_FLAG;
    }

    AVCodecParameters *codecpar;

    float buffer_time_length = pd->buffer_time_length;

 if (pd->av_track_flags & HAS_VIDEO_FLAG) {
        codecpar = pd->format_context->streams[pd->video_index]->codecpar;
        pd->width = codecpar->width;
        pd->height = codecpar->height;

        // if (pd->force_sw_decode) {
        //     pd->is_sw_decode = true;
        // } else {
        //     switch (codecpar->codec_id) {
        //         case AV_CODEC_ID_H264:
        //         // case AV_CODEC_ID_HEVC:
        //         // case AV_CODEC_ID_MPEG4:
        //         // case AV_CODEC_ID_VP8:
        //         // case AV_CODEC_ID_VP9:
        //         // case AV_CODEC_ID_H263:
        //             break;
        //         default:
        //             break;
        //     }
        // }

        // ret = codec_init(pd);
        //更改为此处初始化
     pd->mediacodec_ctx =create_mediacodec_context(pd);

        if (ret != 0) {
            goto fail;
        }
        AVStream *v_stream = pd->format_context->streams[pd->video_index];
        AVDictionaryEntry *m = NULL;

    }
    set_buffer_time(pd);

    pthread_create(&pd->read_stream_thread, NULL, read_thread, pd);
    if (pd->av_track_flags & HAS_VIDEO_FLAG) {
        pthread_create(&pd->video_decode_thread, NULL, video_decode_thread, pd);
        pthread_create(&pd->gl_thread, NULL, player_gl_thread, pd);
    }
    pd->change_status(pd, PLAYING);

#pragma endregion

    return ret;

    fail:
    if (pd->format_context) {
        avformat_close_input(&pd->format_context);
    }
    return ret;
}