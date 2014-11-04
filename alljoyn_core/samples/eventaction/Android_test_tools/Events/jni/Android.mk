LOCAL_PATH := $(call my-dir)

# AllJoyn specifics
ALLJOYN_DIST := ../../../../

include $(CLEAR_VARS)

LOCAL_MODULE := MyAllJoynCode

TARGET_PLATFORM := android-10

LOCAL_C_INCLUDES := \
	$(ALLJOYN_DIST)/cpp/inc \
	$(ALLJOYN_DIST)/cpp/inc/alljoyn \
	$(ALLJOYN_DIST)/about/inc \
	$(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/include \
	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/include \
	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include \
	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/include \
	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi/include

LOCAL_CFLAGS := -Wno-psabi -Wno-write-strings -DANDROID_NDK -DTARGET_ANDROID -DLINUX -DQCC_OS_GROUP_POSIX -DQCC_OS_ANDROID -DQCC_CPU_ARM -DANDROID

LOCAL_CPP_EXTENSION := .cc 

LOCAL_SRC_FILES := \
	AndroidJNIBridge.cc \
	MyAllJoynCode.cc

LOCAL_LDLIBS := \
	-L$(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/lib \
	-L$(ALLJOYN_DIST)/cpp/lib \
	-L$(ALLJOYN_DIST)/about/lib \
    -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi \
    -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi \
    $(ALLJOYN_DIST)/cpp/lib/BundledRouter.o \
	-lajrouter -lalljoyn -llog -ldl -lssl -lcrypto -lm -lc -lstdc++ -lgcc -lgnustl_shared -lalljoyn_about

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)
