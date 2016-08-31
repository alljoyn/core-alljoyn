# The following can be provided as an env var or from the
# ndk-build command line. For example: ndk-build ABI=arm64-v8a
# If not provided then a default value is used.
ABI ?= armeabi
VARIANT ?= debug

APP_OPTIM := $(VARIANT)
APP_ABI := $(ABI) 
APP_STL := gnustl_static
