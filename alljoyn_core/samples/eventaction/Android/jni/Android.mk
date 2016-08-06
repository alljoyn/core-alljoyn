LOCAL_PATH := $(call my-dir)

# The ALLJOYN_DIST variable is used to locate needed distribution files.
# The referenced directory must contain the /lib and /inc directories.
# A default path is provided here which is relative to the directory
# containing the Android.mk file. This default can be overidden via
# an ALLJOYN_DIST environment variable which points to an alternate
# cpp distribution directory for the appropriate build variant; e.g.
# $(AJ_ROOT)/core/alljoyn/build/android/arm/$(APP_OPTIM)/dist/cpp
#
ifndef $(ALLJOYN_DIST)
    ALLJOYN_DIST := ../../../..
endif

include $(CLEAR_VARS)
LOCAL_MODULE := ajrouter
LOCAL_SRC_FILES := $(ALLJOYN_DIST)/lib/libajrouter.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := alljoyn
LOCAL_SRC_FILES := $(ALLJOYN_DIST)/lib/liballjoyn.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := alljoyn_about
LOCAL_SRC_FILES := $(ALLJOYN_DIST)/lib/liballjoyn_about.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := MyAllJoynCode

TARGET_PLATFORM := android-16

LOCAL_C_INCLUDES := \
	$(ALLJOYN_DIST)/inc \
	$(ALLJOYN_DIST)/inc/alljoyn \
	$(ALLJOYN_DIST)/inc/alljoyn/about

LOCAL_CFLAGS := -std=c++11 -Wno-psabi -Wno-write-strings -DANDROID_NDK -DTARGET_ANDROID -DLINUX -DQCC_OS_GROUP_POSIX -DQCC_OS_ANDROID -DANDROID

LOCAL_CPP_EXTENSION := .cc 

LOCAL_SRC_FILES := \
	event/AndroidJNIBridge.cc \
	event/PresenceDetection.cc \
	event/MyEventCode.cc

LOCAL_STATIC_LIBRARIES := \
	ajrouter \
	alljoyn \
	alljoyn_about

LOCAL_LDLIBS := \
	-llog

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)
