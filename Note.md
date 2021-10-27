# xl-player参考
**流程-初始化**

*java*

MainActivity.java(57)->ChooseFileActivity.java(129)->SinglePlayerActivity.java(200)
->XLPlayer.java(34)(*BaseNativeInterface.initPlayer(...);*)->BaseNativeInterface.java
=>System.loadLibrary("avutil-55""swresample-2""avcodec-57""avformat-57""avfilter-6"   "ekf"(xl_head_tracker)"xl_render"(!xl_head_tracker)),
=>(25)initPlayer(),=>(41)play()

*cpp*

xl_player.c(20)(*Java_com_xl_media_library_base_BaseNativeInterface_initPlayer*)=>(24)xl_player_create()
->xl_playerCore.c(119)

**流程-播放**

*java*

BaseNativeInterface.java=>(41)play()

*cpp*


xl_player.c(20)(*Java_com_xl_media_library_base_BaseNativeInterface_play*)=>(55) xl_player_play()
->xl_playerCore.c(227)

