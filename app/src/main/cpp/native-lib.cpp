//
// Created by stanley on 10/5/21.
//

#include <jni.h>
#include <string>

extern "C" {
#include "libavcodec/avcodec.h"
}

extern "C" JNIEXPORT jstring  extern "C" JNICALL
Java_com_stanley_virtualmonitor_JNIUtils_stringFromJNI(
        JNIEnv *env,
        jobject) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF("hello!!!");
}