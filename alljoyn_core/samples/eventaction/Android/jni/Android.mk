LOCAL_PATH := $(call my-dir)

# An ALLJOYN_DIST environment variable may be provided in order to
# locate the cpp distribution files. The referenced directory must
# contain the /lib and /inc directories of the desired build variant;
# e.g. $(AJ_ROOT)/core/alljoyn/build/android/arm/$(APP_OPTIM)/dist/cpp
# If ALLJOYN_DIST is not defined, then relative paths are defaulted
# here that reference the /lib and /inc directories in order to work
# with the Jenkins build.
#
ifndef $(ALLJOYN_DIST)
    ALLJOYN_DIST_LIB := ../../../../lib
    ALLJOYN_DIST_INC := ../../../inc
else
    ALLJOYN_DIST_LIB := $(ALLJOYN_DIST)/lib
    ALLJOYN_DIST_INC := $(ALLJOYN_DIST)/inc
endif

include $(CLEAR_VARS)
LOCAL_MODULE := ajrouter
LOCAL_SRC_FILES := $(ALLJOYN_DIST_LIB)/libajrouter.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := alljoyn
LOCAL_SRC_FILES := $(ALLJOYN_DIST_LIB)/liballjoyn.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := alljoyn_about
LOCAL_SRC_FILES := $(ALLJOYN_DIST_LIB)/liballjoyn_about.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := MyAllJoynCode

TARGET_PLATFORM := android-16

LOCAL_C_INCLUDES := \
	$(ALLJOYN_DIST_INC) \
	$(ALLJOYN_DIST_INC)/alljoyn \
	$(ALLJOYN_DIST_INC)/alljoyn/about

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
