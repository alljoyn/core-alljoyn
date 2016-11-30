#!/bin/sh
# Copyright AllSeen Alliance. All rights reserved.
#
#    Permission to use, copy, modify, and/or distribute this software for any
#    purpose with or without fee is hereby granted, provided that the above
#    copyright notice and this permission notice appear in all copies.
#
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

set -e

./build_core_macos.sh

BASE_DIR=../build/darwin
BUILD_DIR="${BASE_DIR}/AllJoynFramework"

echo "Building AllJoynFramework for macOS..."

xcodebuild -project AllJoynFramework/AllJoynFramework.xcodeproj -target AllJoynFramework_macOS  ONLY_ACTIVE_ARCH=NO -configuration Debug -sdk macosx  BUILD_DIR="../${BUILD_DIR}" SYMROOT="../${BUILD_DIR}/obj"
xcodebuild -project AllJoynFramework/AllJoynFramework.xcodeproj -target AllJoynFramework_macOS  ONLY_ACTIVE_ARCH=NO -configuration Release -sdk macosx  BUILD_DIR="../${BUILD_DIR}" SYMROOT="../${BUILD_DIR}/obj"

echo "Copying Headers..."

cp -R "${BUILD_DIR}/Release/include" "${BUILD_DIR}"
cp -R "${BASE_DIR}/x86_64/release/dist/cpp/inc/alljoyn" "${BUILD_DIR}/include/alljoyn"
cp -R "${BASE_DIR}/x86_64/release/dist/cpp/inc/qcc" "${BUILD_DIR}/include/qcc"

rm -R "${BUILD_DIR}/obj"
rm -R "${BUILD_DIR}/Release/include"
rm -R "${BUILD_DIR}/Debug/include"

echo "Copying Core Libs..."

copy_core_lib() {
    cp "${BASE_DIR}/x86_64/release/dist/cpp/lib/$1" "${BUILD_DIR}/Release/$1"
    cp "${BASE_DIR}/x86_64/debug/dist/cpp/lib/$1" "${BUILD_DIR}/Debug/$1"
}

copy_core_lib libajrouter.a
copy_core_lib liballjoyn_about.a
copy_core_lib liballjoyn_config.a
copy_core_lib liballjoyn.a

rm -R "${BASE_DIR}/x86_64"
