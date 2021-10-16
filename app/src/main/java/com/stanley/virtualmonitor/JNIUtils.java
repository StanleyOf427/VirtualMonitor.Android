package com.stanley.virtualmonitor;

import android.view.Surface;

public class JNIUtils {
    static{
//        System.loadLibrary("native-lib");
        System.loadLibrary("decodeTest");

    }
//    public static native String stringFromJNI();
    public static native int testDecode(String filePath, Surface surface);


}
