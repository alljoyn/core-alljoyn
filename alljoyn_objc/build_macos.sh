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
SDKROOT_MACOS=`xcodebuild -version -sdk macosx Path`
CPU_NUM=`sysctl -n hw.ncpu`

echo "Building AllJoyn Core for macOS"
echo "SDKROOT_MACOS: $SDKROOT_MACOS"
echo "CPU_NUM: $CPU_NUM"

set -e

export PLATFORM_NAME=macosx
scons -u --jobs $CPU_NUM OS=darwin CPU=x86_64 CRYPTO=builtin BR=on BINDINGS="cpp" WS=off VARIANT=debug SDKROOT=$SDKROOT_MACOS
scons -u --jobs $CPU_NUM OS=darwin CPU=x86_64 CRYPTO=builtin BR=on BINDINGS="cpp" WS=off VARIANT=release SDKROOT=$SDKROOT_MACOS