# An Android.mk file must begin with the definition of the LOCAL_PATH
# variable. It is used to locate source files in the development tree. Here
# the macro function 'my-dir', provided by the build system, is used to return
# the path of the current directory.
#
LOCAL_PATH := $(call my-dir)

# AllJoyn specifics
#ALLJOYN_DIST := ../../../../../build/android/arm/$(APP_OPTIM)/dist
ALLJOYN_DIST := ../../..

# The CLEAR_VARS variable is provided by the build system and points to a
# special GNU Makefile that will clear many LOCAL_XXX variables for you
# (e.g. LOCAL_MODULE, LOCAL_SRC_FILES, LOCAL_STATIC_LIBRARIES, etc...),
# with the exception of LOCAL_PATH. This is needed because all build
# control files are parsed in a single GNU Make execution context where
# all variables are global. 
#
include $(CLEAR_VARS)

# The LOCAL_MODULE variable must be defined to identify each module you
# describe in your Android.mk. The name must be *unique* and not contain
# any spaces. Note that the build system will automatically add proper
# prefix and suffix to the corresponding generated file. In other words,
# a shared library module named 'foo' will generate 'libfoo.so'.
# 
LOCAL_MODULE := SimpleService


# The TARGET_PLATFORM defines the targetted Android Platform API level
TARGET_PLATFORM := android-10

# An optional list of paths, relative to the NDK *root* directory,
# which will be appended to the include search path when compiling
# all sources (C, C++ and Assembly). These are placed before any 
# corresponding inclusion flag in LOCAL_CFLAGS / LOCAL_CPPFLAGS.
#
# The path for NDK r8 and below is:
#	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/include \
#	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include \
# The path for NDK r8b is:
#	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/include \
#	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi/include
#

LOCAL_C_INCLUDES := \
	$(MYDROID_PATH)/external/openssl/include \
	$(ALLJOYN_DIST)/inc \
	$(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/include \
	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/include \
	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include \
	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/include \
	$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi/include

# An optional set of compiler flags that will be passed when building
# C ***AND*** C++ source files.
#
# NOTE1: flag "-Wno-psabi" removes warning about GCC 4.4 va_list warning
# NOTE2: flag "-Wno-write-strings" removes warning about deprecated conversion
#        from string constant to "char*"
#
LOCAL_CFLAGS := -Wno-psabi -Wno-write-strings -DANDROID_NDK -DTARGET_ANDROID -DLINUX -DQCC_OS_GROUP_POSIX -DQCC_OS_ANDROID -DQCC_CPU_ARM -DANDROID

# The following line is just for reference here (thus commented out) 
# as it is unnecessary because the default extension for C++ source 
# files is '.cpp'. 
#
LOCAL_DEFAULT_CPP_EXTENSION := cc 

# The LOCAL_SRC_FILES variables must contain a list of C and/or C++ source
# files that will be built and assembled into a module. Note that you should
# not list header and included files here, because the build system will
# compute dependencies automatically for you; just list the source files
# that will be passed directly to a compiler, and you should be good. 
#
LOCAL_SRC_FILES := \
	Service_jni.cpp

# The list of additional linker flags to be used when building your
# module. This is useful to pass the name of specific system libraries
# with the "-l" prefix and library paths with the "-L" prefix.
#
# NOTE: linking with libgcc.a is necessary due to some strangely missing
# symbols such as "__aeabi_f2uiz" present in libgcc.a. It has to be linked
# as the very last library.
#
# The path for NDK r8 and below is:
#        -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi \
# The path for NDK r8b is:
#        -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi \
#

LOCAL_LDLIBS := \
	-L$(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/lib \
	-L$(ALLJOYN_DIST)/lib \
        -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi \
        -L$(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi \
	-L./libs \
    $(ALLJOYN_DIST)/lib/BundledRouter.o \
	-lajrouter -lalljoyn -llog -lz -ldl -lssl -lcrypto -lm -lc -lstdc++  -lgcc -lgnustl_static

# By default, ARM target binaries will be generated in 'thumb' mode, where
# each instruction are 16-bit wide. You can define this variable to 'arm'
# if you want to force the generation of the module's object files in
# 'arm' (32-bit instructions) mode
#
LOCAL_ARM_MODE := arm

# The BUILD_SHARED_LIBRARY is a variable provided by the build system that
# points to a GNU Makefile script that is in charge of collecting all the
# information you defined in LOCAL_XXX variables since the latest
# 'include $(CLEAR_VARS)' and determine what to build, and how to do it
# exactly. There is also BUILD_STATIC_LIBRARY to generate a static library. 
#
include $(BUILD_SHARED_LIBRARY)
