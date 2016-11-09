#!/bin/sh

cd ..
SDKROOT_IOS_SIMULATOR=`xcodebuild -version -sdk iphonesimulator Path`
SDKROOT_IOS=`xcodebuild -version -sdk iphoneos Path`
CPU_NUM=`sysctl -n hw.ncpu`

echo "Building AllJoyn Core for iOS"
echo "SDKROOT_IOS_SIMULATOR: $SDKROOT_IOS_SIMULATOR"
echo "SDKROOT_IOS: $SDKROOT_IOS"
echo "CPU_NUM: $CPU_NUM"

set -e

export CONFIGURATION=release
export PLATFORM_NAME=iphoneos
scons -u --jobs $CPU_NUM OS=darwin VARIANT=release CPU=iOS_universal CRYPTO=builtin BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS
export PLATFORM_NAME=iphonesimulator
scons -u --jobs $CPU_NUM OS=darwin VARIANT=release CPU=iOS_universal CRYPTO=builtin BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS_SIMULATOR

export CONFIGURATION=debug
export PLATFORM_NAME=iphoneos
scons -u --jobs $CPU_NUM OS=darwin VARIANT=debug CPU=iOS_universal CRYPTO=builtin BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS
export PLATFORM_NAME=iphonesimulator
scons -u --jobs $CPU_NUM OS=darwin VARIANT=debug CPU=iOS_universal CRYPTO=builtin BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS_SIMULATOR
