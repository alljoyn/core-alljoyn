#!/bin/sh
#    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
#    Project (AJOSP) Contributors and others.
#    
#    SPDX-License-Identifier: Apache-2.0
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0
#    
#    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
#    Alliance. All rights reserved.
#    
#    Permission to use, copy, modify, and/or distribute this software for
#    any purpose with or without fee is hereby granted, provided that the
#    above copyright notice and this permission notice appear in all
#    copies.
#    
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
#    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
#    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
#    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
#    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
#    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#    PERFORMANCE OF THIS SOFTWARE.
#

echo_help()
{
  echo "Usage: $0 [options...]"
  echo " CRYPTO=crypto_module_name         Build AllJoyn Core with specific crypto module"
  echo " -h, --help                        Print help (this message)"
}

CRYPTO=builtin

# Process command line arguments
for i in "$@"
do
case $i in
  -h|--help)
    echo_help
    exit 1
    ;;
  CRYPTO=*)
    CRYPTO="${i#*=}"
    shift
    ;;
  *)
    echo "Unknown argument: ${i}"
    echo_help
    exit 1
    ;;
esac
done

cd ..
SDKROOT_IOS_SIMULATOR=`xcodebuild -version -sdk iphonesimulator Path`
SDKROOT_IOS=`xcodebuild -version -sdk iphoneos Path`
CPU_NUM=`sysctl -n hw.ncpu`

echo "Building AllJoyn Core for iOS..."
echo "SDKROOT_IOS_SIMULATOR: $SDKROOT_IOS_SIMULATOR"
echo "SDKROOT_IOS: $SDKROOT_IOS"
echo "CPU_NUM: $CPU_NUM"

set -e

export CONFIGURATION=release
export PLATFORM_NAME=iphoneos
scons -u --jobs $CPU_NUM OS=iOS VARIANT=release CPU=universal CRYPTO=$CRYPTO BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS
export PLATFORM_NAME=iphonesimulator
scons -u --jobs $CPU_NUM OS=iOS VARIANT=release CPU=universal CRYPTO=$CRYPTO BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS_SIMULATOR

export CONFIGURATION=debug
export PLATFORM_NAME=iphoneos
scons -u --jobs $CPU_NUM OS=iOS VARIANT=debug CPU=universal CRYPTO=$CRYPTO BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS
export PLATFORM_NAME=iphonesimulator
scons -u --jobs $CPU_NUM OS=iOS VARIANT=debug CPU=universal CRYPTO=$CRYPTO BR=on BINDINGS="cpp" WS=off SDKROOT=$SDKROOT_IOS_SIMULATOR