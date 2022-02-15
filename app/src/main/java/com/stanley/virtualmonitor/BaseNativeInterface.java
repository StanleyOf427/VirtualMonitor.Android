package com.stanley.virtualmonitor;

import android.content.res.AssetManager;
import android.view.Surface;

import java.nio.ByteBuffer;


class BaseNativeInterface {
    static {
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("avcodec");
        System.loadLibrary("avformat");
        System.loadLibrary("avfilter");

        System.loadLibrary("CorePlayer");

//        System.loadLibrary("xl_render");
    }
//    native static void initPlayer(Surface surface,String input_url,int samplerate);

//    native static void initPlayer(CorePlayer xlPlayer, int runAndroidVersion,int bestSampleRate);
//
//    native static void setBufferTime(float time);
//
//    native static void setBufferSize(int numByte);
//
//    native static void setPlayBackground(boolean playBackground);
//
//    native static void setForceSwDecode(boolean forceSwDecode);
//
//    native static void setSurface(Surface surface);
//
//    static native void resize(int width, int height);
//
//    static native void changeRate(float rate);
//
//    native static int play(String url, float time, int model);
//
//    native static void seek(float time, boolean isSeekTo);
//
//    native static void pause();
//
//    native static void resume();
//
//    native static void stop();
//
//    native static void rotate(boolean clockwise);
//
//    native static float getTotalTime();
//
//    native static float getCurrentTime();
//
//    native static void release();
//
//    native static int getPlayStatus();
//
//    native static void changeModel(int model);
//
//    native static void setScale(float scale);
//
//    native static void setRotation(float rx, float ry, float rz);
//
//    native static void setHeadTrackerEnable(boolean enable);
//
//    native static boolean getHeadTrackerEnable();
//
//    native static ByteBuffer getStatistics();

}

