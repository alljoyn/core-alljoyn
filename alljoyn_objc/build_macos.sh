#!/bin/sh

cd ..
SDKROOT_MACOS=`xcodebuild -version -sdk macosx Path`
CPU_NUM=`sysctl -n hw.ncpu`

echo "Building AllJoyn Core for macOS"
echo "SDKROOT_MACOS: $SDKROOT_MACOS"
echo "CPU_NUM: $CPU_NUM"

set -e

export PLATFORM_NAME=macosx
scons -u --jobs $CPU_NUM OS=darwin CPU=x86_64 CRYPTO=builtin BR=on BINDINGS="cpp" WS=off VARIANT=debug SDKROOT=$SDKROOT_MACOS
scons -u --jobs $CPU_NUM OS=darwin CPU=x86_64 CRYPTO=builtin BR=on BINDINGS="cpp" WS=off VARIANT=release SDKROOT=$SDKROOT_MACOS
