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
