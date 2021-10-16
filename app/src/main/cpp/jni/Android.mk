LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := nativeWind
LOCAL_SRC_FILES :=  decodeTest.cpp
LOCAL_LDLIBS +=  -llog -landroid

include $(BUILD_SHARED_LIBRARY)
#————————————————
#版权声明：本文为CSDN博主「白皮书CAN」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
#原文链接：https://blog.csdn.net/u012459903/article/details/118224506