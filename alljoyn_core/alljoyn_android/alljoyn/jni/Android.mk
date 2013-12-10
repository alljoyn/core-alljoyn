LOCAL_PATH := $(call my-dir)

#
# This library depends on the AllJoyn daemon library.  It must be separately
# built before building the JNI code in this directory.  We have the linker
# look for this library in the top level build directory (not in the subdir
# alljoyn_core/build.  To build the JNI library here, do "ndk-build -B V=1".
#
#BUS_LIB_DIR := ../../build/android/arm/$(APP_OPTIM)/dist/lib/
BUS_LIB_DIR := ../../lib/


include $(CLEAR_VARS)

LOCAL_MODULE    := daemon-jni
LOCAL_SRC_FILES := daemon-jni.cpp

# The path for NDK r8 and below is:
#        -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi \
# The path for NDK r8b is:
#        -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi \
#

LOCAL_LDLIBS := -L$(BUS_LIB_DIR) \
        -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi \
        -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi \
	-lalljoyn-daemon -lalljoyn -lcrypto -llog -lgcc -lssl -lgnustl_static

include $(BUILD_SHARED_LIBRARY)
