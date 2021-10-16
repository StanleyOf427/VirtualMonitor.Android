// Stanley
// Created by stanley on 2021/10/7.
//解码流程
// av_register_all();//注册组件
// avformat_open_input();//打开文件
// avformat_find_stream_info();//获取文件信息
// avcodec_find_decoder();//查找解码器
// avcodec_open2();//打开解码器
// av_read_frame();//读取帧
// avcodec_decode_video2();//解码压缩数据
// avcodec_close();
// avformat_close_input();

//#include "com_dongnaoedu_dnffmpegplayer_VideoUtils.h"
extern "C"{
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavutil/imgutils.h"
#include "include/libavutil/opt.h"
#include "include/libswresample/swresample.h"
#include "include/libswscale/swscale.h"
}

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <android/log.h>
#include <jni.h>
#include <cstdio>
#include <cstdlib>

#define LOGI(FORMAT, ...)                                                      \
  __android_log_print(ANDROID_LOG_INFO, "jason", FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT, ...)                                                      \
  __android_log_print(ANDROID_LOG_ERROR, "jason", FORMAT, ##__VA_ARGS__);

extern "C" JNIEXPORT int JNICALL
Java_com_stanley_virtualmonitor_JNIUtils_testDecode(JNIEnv *env, jclass jcls, jstring input_jstr, jobject surface)
{
  const char *input_cstr= env->GetStringUTFChars(input_jstr, nullptr);
    LOGE("打开路径：%s", input_cstr);
#pragma region 解码部分

#pragma region 1.注册组件
  av_register_all();

  AVFormatContext *pFormatCtx = avformat_alloc_context(); //封装格式上下文
#pragma endregion

#pragma region 2.打开输入视频文件
  if ( avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL)!= 0) {
    LOGE("%s", "打开输入视频文件失败");
    return -1;
  }
#pragma  endregion

#pragma region 3.获取视频信息
  if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    LOGE("%s", "获取视频信息失败");
    return -1;
  }
#pragma endregion

#pragma region 4.视频解码，需要找到视频对应的索引位置
  int video_stream_idx = -1;
  for (int i = 0; i < pFormatCtx->nb_streams; i++) {
    //根据类型判断，是否是视频流
//    if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_idx = i;
      break;
    }
  }
  if(video_stream_idx==-1){
      LOGE("%s","未找到视频流索引");
      return  -1;
  }
#pragma  endregion

#pragma  region 5.获取视频解码器并创建上下文
//    AVCodecContext *pCodeCtx = pFormatCtx->streams[video_stream_idx]->codec;
//    AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);

    AVCodecParameters *pCodePtr  = pFormatCtx->streams[video_stream_idx]->codecpar;
    AVCodec *pCodec = avcodec_find_decoder(pCodePtr->codec_id);

    if (pCodec== nullptr) {//pCodec == NULL
    LOGE("%s", "无法解码");
    return -1;
  }
    //
    AVCodecContext *p_AVCodecContext = avcodec_alloc_context3(pCodec);
    if(avcodec_parameters_to_context(p_AVCodecContext,pCodePtr) != 0) {
        LOGE("%s","创建解码器上下文失败");
        return -1;
    }
#pragma  endregion

#pragma  region 6.打开解码器
  if (avcodec_open2(p_AVCodecContext , pCodec, NULL) < 0) {//  if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
      LOGE("%s", "解码器无法打开");
    return -1;
  }
#pragma  endregion

  // 6.获取surface
  ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
  ANativeWindow_Buffer windowBuffer;

  if (nativeWindow == 0) {
    LOGE("无法从surface获取native window");
    return -1;
  }

#pragma region 7.设置帧缓冲区
    // AVFrame *frame = av_frame_alloc();
    AVFrame *pFrame = av_frame_alloc(); //指向解码后的原始帧
//  AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    AVPacket *packet = av_packet_alloc();
//    AVFrame *yuvFrame = av_frame_alloc(); //解码数据

  // uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size());

//  int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodeCtx->width,
//                                          pCodeCtx->height, 1);
//  uint8_t *v_out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
//
//  av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, v_out_buffer,
//                       AV_PIX_FMT_RGBA, pCodeCtx->width, pCodeCtx->height, 1);
//
//  struct SwsContext *sws_ctx = sws_getContext(
//      pCodeCtx->width, pCodeCtx->height, pCodeCtx->pix_fmt, pCodeCtx->width,
//      pCodeCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
//
//  if (ANativeWindow_setBuffersGeometry(nativeWindow, pCodeCtx->width,
//                                       pCodeCtx->height,
//                                       WINDOW_FORMAT_RGBA_8888) < 0) {
//    LOGE("无法设置缓冲区");
//    ANativeWindow_release(nativeWindow);
//    return -1;
//  }
#pragma endregion

#pragma region 8.循环读帧并渲染
  while (av_read_frame(pFormatCtx, packet) >= 0) {
      if (packet->stream_index == video_stream_idx) {
//          int ret = avcodec_send_packet(pCodeCtx, packet);
//          if (ret < 0 && ret != AVERROR_EOF)
//          {
//              return -1;
//          }
if(avcodec_send_packet(p_AVCodecContext, packet)!=0){
    return -1;
}
//          ret = avcodec_receive_frame(pCodeCtx, pFrame);
//          if (ret < 0 && ret != AVERROR_EOF)
//          {
//              return -1;
//          }

while(avcodec_receive_frame(p_AVCodecContext, pFrame)==0)
{
    //解码数据，格式转换后渲染
    int width=p_AVCodecContext->width;
    int height=p_AVCodecContext->height;
    AVFrame *pRGBAFrame = av_frame_alloc();
    int bufferSize=av_image_get_buffer_size(AV_PIX_FMT_RGBA,width,height,1);
   uint8_t  *pFrameBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
   av_image_fill_arrays(pRGBAFrame->data,pRGBAFrame->linesize,pFrameBuffer,AV_PIX_FMT_RGBA,
                        width,height,1);
    SwsContext *pSwsCtx= sws_getContext(width, height, p_AVCodecContext->pix_fmt,
                     1080,1920 , AV_PIX_FMT_RGBA,
                     SWS_FAST_BILINEAR, NULL, NULL, NULL);
  int swsrst=  sws_scale(pSwsCtx,pFrame->data,pFrame->linesize,0,height,pRGBAFrame->data,pRGBAFrame->linesize);
  if(swsrst!=0)
  {
      LOGE("%s","像素转换出错！");
  }
    if(pRGBAFrame!= nullptr)
    {
        av_frame_free(&pRGBAFrame);
        pRGBAFrame= nullptr;
    }
    if(pFrameBuffer!= nullptr)
    {
        free(pFrameBuffer);
        pFrameBuffer= nullptr;
    }
    if(pSwsCtx!= nullptr)
    {
        sws_freeContext(pSwsCtx);
        pSwsCtx= nullptr;
    }


    //渲染
    ANativeWindow_setBuffersGeometry(nativeWindow,width,height,WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_lock(nativeWindow, &windowBuffer, nullptr);
    uint8_t *dstBuffer=static_cast<uint8_t *>(windowBuffer.bits);
    int srcLineSize=pRGBAFrame->linesize[0];//输入图像步长
    int dstLineSize=windowBuffer.stride*4;//RGBA缓冲区步长
    for(int i=0;i<height;++i)
    {
        memcpy(dstBuffer+i*dstLineSize,pFrameBuffer+i*srcLineSize,srcLineSize);
    }
    ANativeWindow_unlockAndPost(nativeWindow);
    if(nativeWindow!= nullptr)
    {
        ANativeWindow_release(nativeWindow);
    }
}

//          sws_scale(sws_ctx, (const uint8_t *const *) yuvFrame->data,
//                    yuvFrame->linesize, 0, yuvFrame->height, yuvFrame->data,
//                    yuvFrame->linesize); //转为指定的YUV420P像素帧

//          if (ANativeWindow_lock(nativeWindow, &windowBuffer, NULL) < 0)
//          {
//              LOGE("无法锁定窗口!");
//          }
//          else
//              {
//              auto *dst = (uint8_t *) windowBuffer.bits;
//              for (int h = 0; h < pCodeCtx->height; h++)
//              {
//                  memcpy(dst + h * windowBuffer.stride * 4,
//                         v_out_buffer + h * yuvFrame->linesize[0],
//                         yuvFrame->linesize[0]);
//              }
//              ANativeWindow_unlockAndPost(nativeWindow);
//          }
      }
      av_packet_unref(packet);
  }
#pragma endregion

#pragma endregion

    LOGE("完成播放!");

    if(pFrame!= nullptr){
        av_frame_free(&pFrame);
        pFrame= nullptr;
    }
    //    sws_freeContext(&sws_ctx);
//    av_free(packet);
//    LOGE("释放packet\n");
if(packet!= nullptr)
{
    av_packet_free(&packet);
    packet= nullptr;
}
//    av_frame_free(&yuvFrame);
//    LOGE("释放yuvFrame\n");
if(p_AVCodecContext!= nullptr)
{
    avcodec_close(p_AVCodecContext);
    avcodec_free_context(&p_AVCodecContext);
    p_AVCodecContext= nullptr;
    pCodec= nullptr;
}
//    avcodec_close(pCodeCtx);
//    LOGE("释放pCodeCtx\n");

if(pFormatCtx!= nullptr)
{
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
    pFormatCtx= nullptr;
}
    avformat_close_input(&pFormatCtx);
//    avformat_free_context(pFormatCtx);
    return 0;
}
