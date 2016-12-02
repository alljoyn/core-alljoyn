#!/bin/sh
# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0

#

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
scons -u --jobs $CPU_NUM OS=iOS VARIANT=release CPU=universal CRYPTO=builtin BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS
export PLATFORM_NAME=iphonesimulator
scons -u --jobs $CPU_NUM OS=iOS VARIANT=release CPU=universal CRYPTO=builtin BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS_SIMULATOR

export CONFIGURATION=debug
export PLATFORM_NAME=iphoneos
scons -u --jobs $CPU_NUM OS=iOS VARIANT=debug CPU=universal CRYPTO=builtin BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS
export PLATFORM_NAME=iphonesimulator
scons -u --jobs $CPU_NUM OS=iOS VARIANT=debug CPU=universal CRYPTO=builtin BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS_SIMULATOR