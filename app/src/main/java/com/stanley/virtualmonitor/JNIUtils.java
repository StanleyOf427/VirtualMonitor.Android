package com.stanley.virtualmonitor;
import android.view.Surface;

public class JNIUtils {
    static{
//        System.loadLibrary("native-lib");
//        System.loadLibrary("decodeTest");
        System.loadLibrary("CorePlayer");
    }
//    public static native String stringFromJNI();
//    public static native int testDecode(String filePath, Surface surface);
    public static native int CorePlayer(Surface surface,String filePath,int rate);
}