#!/bin/bash

# Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

set -x
set -e
SELF_DIR=$(cd $(dirname $0) > /dev/null; pwd)

if [ -z ${AJ_ROOT} ]; then
    AJ_ROOT="${SELF_DIR}/../../.."
fi

AJN_SM_PATH="${SELF_DIR}/.."


AJN_DAEMON_PNAME="alljoyn-daemon"
VARIANT="debug"

if [ -d "${AJN_SM_PATH}/build/linux/x86/${VARIANT}" ]; then
    PLATFORM="x86"
elif [ -d "${AJN_SM_PATH}/build/linux/x86_64/${VARIANT}" ]; then
    PLATFORM="x86_64"
else
    printf "Tests are not built yet!\nMake sure to export GTEST_DIR to the right Gtest home path and then build again, e.g.: $> scons CPU=x86 BINDINGS=cpp,c WS=off\n"
    exit 1
fi

PLATFORM_ROOT="${AJN_SM_PATH}/build/linux/${PLATFORM}/${VARIANT}"
TEST_ROOT="${PLATFORM_ROOT}/test/"

if ! nm "${TEST_ROOT}/core/unit_test/secmgrctest" | grep BundledRouter &> /dev/null; then
    if [ "$(pidof ${AJN_DAEMON_PNAME})" ]; then
         echo "alljoyn-daemon is active...running tests..."
    else
         echo "Please start an alljoyn-daemon to be able to run the tests !"
         exit 1
    fi
fi

LIB_PATH="${PLATFORM_ROOT}/dist/security/lib:${PLATFORM_ROOT}/dist/cpp/lib:${PLATFORM_ROOT}/dist/services_common/lib"

export LD_LIBRARY_PATH="${LIB_PATH}"

# make sure coverage starts clean
if [ ! -z "$(which lcov)" ]; then
    lcov --directory "${PLATFORM_ROOT}" --zerocounters
fi

# running unit tests
# we are doing some magic here to run each test in its own process as we still have some issues to run them in one go (AS-207)
echo "[[ Cleaning old Gtest results if any ]]"
rm -rf "${TEST_ROOT}"/gtestresults/
echo "[[ Running unit tests ]]"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
for target in "core"
do
    STORAGE_PATH=/tmp/secmgr.db "${TEST_ROOT}"/$target/unit_test/secmgrctest --gtest_output=xml:"${TEST_ROOT}"/gtestresults/ 
done

# running system tests
# echo "[[ Running system tests ]]"
# STORAGE_PATH=/tmp/secmgr.db "${TEST_ROOT}"/seccore/test/multipeer_claim/run.sh

#Kill any remaining multipeer_claim processes hanging around.
kill $(pidof multipeer_claim) || true
# generate coverage report (lcov 1.10 or better required for --no-external)
if [ ! -z "$(which lcov)" ]; then
    if [ $(lcov --version | cut -d'.' -f2) -ge 10 ]; then
        EXTRA_ARGS="--no-external"
    fi
        COVDIR="${AJN_SM_PATH}"/build/coverage
        for target in "core"
	    do
            mkdir -p "${COVDIR}"/"$target"src > /dev/null 2>&1
            mkdir -p "${COVDIR}"/"$target"inc > /dev/null 2>&1

	    lcov --quiet --capture -b "${AJN_SM_PATH}"/$target/src --directory "${PLATFORM_ROOT}"/lib/$target $EXTRA_ARGS --output-file "${COVDIR}"/secmgr_"$target"_src.info
            lcov --quiet --capture -b "${AJN_SM_PATH}"/$target/inc --directory "${PLATFORM_ROOT}"/lib/$target $EXTRA_ARGS --output-file "${COVDIR}"/secmgr_"$target"_inc.info
            genhtml --quiet --output-directory "${COVDIR}"/"$target"src "${COVDIR}"/secmgr_"$target"_src.info 
            genhtml --quiet --output-directory "${COVDIR}"/"$target"inc "${COVDIR}"/secmgr_"$target"_inc.info || true
	    done

        for target in "storage"
            do
            lcov --quiet --capture -b "${AJN_SM_PATH}"/$target/inc --directory "${PLATFORM_ROOT}"/lib/$target $EXTRA_ARGS --output-file "${COVDIR}"/secmgr_"$target"_inc.info
            genhtml --quiet --output-directory "${COVDIR}"/"$target"inc "${COVDIR}"/secmgr_"$target"_inc.info || true
            done
fi

exit 0
