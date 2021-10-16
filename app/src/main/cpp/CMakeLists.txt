#cmake_minimum_required(VERSION 3.6.4111459)
cmake_minimum_required(VERSION 3.6.0)

#设置FFmpeg头文件的路径，共享库,include里面h文件调用其他h文件用到的
include_directories(
        include#因为和CMakeLists.txt在同一级，所以直接写include
)


#add_library(native-lib
#        SHARED
#        native-lib.cpp)

add_library(decodeTest
        SHARED
        decodeTest.cpp)

find_library(
        log-lib
        log)

#1.定义一个变量avcodec
add_library(
        avcodec
        SHARED
        IMPORTED
)
#给avcodec这个变量赋值
#set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI}/libavcodec-58.so)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/libavcodec.so)
#2.
add_library(
        avdevice
        SHARED
        IMPORTED
)
#set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI}/libavdevice-58.so)
set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/libavdevice.so)

#3.
add_library(
        avfilter
        SHARED
        IMPORTED
)
#set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI}/libavfilter-7.so)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/libavfilter.so)


#4.
add_library(
        avformat
        SHARED
        IMPORTED
)
#set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI}/libavformat-58.so)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/libavformat.so)

#5.
add_library(
        avutil
        SHARED
        IMPORTED
)
#set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI}/libavutil-56.so)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/libavutil.so)
#6.
#add_library(
#        postproc
#        SHARED
#        IMPORTED
#)
#set_target_properties(postproc PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI}/libpostproc-55.so)

#7.
add_library(
        swresample
        SHARED
        IMPORTED
)
#set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI}/libswresample-3.so)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/libswresample.so)

#8.
add_library(
        swscale
        SHARED
        IMPORTED
)
#set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI}/libswscale-5.so)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../main/jniLibs/libswscale.so)

#9.

#混合编译到所需库中
target_link_libraries(
        decodeTest
        ${log-lib}
        avcodec#1
        avdevice#2
        avfilter#3
        avformat#4
        avutil#5
#        postproc#6
        swresample#7
        swscale#8

        -landroid
)

#————————————————
#版权声明：本文为CSDN博主「自己的MVP」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
#原文链接：https://blog.csdn.net/qq_35014796/article/details/115789252