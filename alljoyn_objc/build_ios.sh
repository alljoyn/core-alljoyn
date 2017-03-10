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

set -e

./build_core_ios.sh $*

BASE_DIR=../build/iOS
BUILD_DIR="${BASE_DIR}/AllJoynFramework"

build() {
    CONFIGURATION=$1

    xcodebuild -project AllJoynFramework/AllJoynFramework.xcodeproj -target AllJoynFramework_iOS  ONLY_ACTIVE_ARCH=NO -configuration $CONFIGURATION -sdk iphoneos  BUILD_DIR="../${BUILD_DIR}" SYMROOT="../${BUILD_DIR}/obj"
    xcodebuild -project AllJoynFramework/AllJoynFramework.xcodeproj -target AllJoynFramework_iOS  ONLY_ACTIVE_ARCH=NO -configuration $CONFIGURATION -sdk iphonesimulator  BUILD_DIR="../${BUILD_DIR}" SYMROOT="../${BUILD_DIR}/obj"

    mkdir -p "${BUILD_DIR}/${CONFIGURATION}"
    lipo -create -output "${BUILD_DIR}/${CONFIGURATION}/libAllJoynFramework_iOS.a" "${BUILD_DIR}/${CONFIGURATION}-iphoneos/libAllJoynFramework_iOS.a" "${BUILD_DIR}/${CONFIGURATION}-iphonesimulator/libAllJoynFramework_iOS.a"
}

echo "Building AllJoynFramework for iOS..."

build Debug
build Release

echo "Copying Headers..."

cp -R "${BUILD_DIR}/Release-iphoneos/include" "${BUILD_DIR}"
cp -R "${BASE_DIR}/universal/iphoneos/release/dist/cpp/inc/alljoyn" "${BUILD_DIR}/include/alljoyn"
cp -R "${BASE_DIR}/universal/iphoneos/release/dist/cpp/inc/qcc" "${BUILD_DIR}/include/qcc"

rm -R "${BUILD_DIR}/Debug-iphoneos"
rm -R "${BUILD_DIR}/Debug-iphonesimulator"
rm -R "${BUILD_DIR}/Release-iphoneos"
rm -R "${BUILD_DIR}/Release-iphonesimulator"
rm -R "${BUILD_DIR}/obj"

echo "Copying Core Libs..."

lipo_core_lib() {
    lipo -create -output "${BUILD_DIR}/Release/$1" "${BASE_DIR}/universal/iphoneos/release/dist/cpp/lib/$1" "${BASE_DIR}/universal/iphonesimulator/release/dist/cpp/lib/$1"
    lipo -create -output "${BUILD_DIR}/Debug/$1" "${BASE_DIR}/universal/iphoneos/debug/dist/cpp/lib/$1" "${BASE_DIR}/universal/iphonesimulator/debug/dist/cpp/lib/$1"
}

lipo_core_lib libajrouter.a
lipo_core_lib liballjoyn_about.a
lipo_core_lib liballjoyn_config.a
lipo_core_lib liballjoyn.a

rm -R "${BASE_DIR}/universal"
