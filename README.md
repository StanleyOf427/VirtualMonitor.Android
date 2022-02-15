# VirtualMonitor.Android
Deisplay virtual monitors and desktops on your headset.

CurrentState:

    Receive socket packages:√
    Decode by MediaCodec api:working
    Decode by ffmpeg:working:×
    Control by side-button:waiting

Problem:

    1.MediaCodec working fine on Snapdragon 400, failed on Allwinner VR9 with [E/ACodec: [OMX.allwinner.video.decoder.avc] storeMetaDataInBuffers failed w/ err -1010]
    2.lldb-server not working on device by Android Studio with error[Invalid URL: unix-abstract-connect://[23104/00006005]/com.stanley.virtualmonitor-0/platform-1634170881706.sock]
    3.ffmpeg decode video with 25296kbps bitrate slowly, only 10+fps.

Build Environment

    IDE:Android Studio 2020.3.1 Patch 2(Windows or Linux)
    NDK:R21
    SDK:R21(Android7.1)
    Gradle:7.2(Windows),7.0.2(Linux)
    Gradle plug-in:7.0.2
