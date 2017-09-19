LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := ringAndNdkTest
LOCAL_SRC_FILES := ringANT.cpp
LOCAL_CPPFLAGS := -std=gnu++0x -Wall -fPIE         # whatever g++ flags you like
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like

include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.

cls:
	rm -rf libs
	rm -rf obj

