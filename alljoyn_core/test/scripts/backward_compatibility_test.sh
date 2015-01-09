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
#

# This script can be run with extra input arguments to be able to add other branches or tags
# like for example the current working branch

# Add git tags or branches in order of release date
VERSIONS=(v14.02 v14.06a)

DIR=$PWD
OUTPUT="output"
PREFIX=../../../

function validator()
{
    retval=$?
    if [ $retval -ne 0 ]; then
        echo "$1"
        exit 1
    fi
}

# Build all different versions of alljoyn based on the VERSIONS variable
function build()
{
    rm -rf ${OUTPUT}
    mkdir -p ${OUTPUT}/lib ${OUTPUT}/bin

    echo "--- Start building the different versions ---"
    for i in "${VERSIONS[@]}"
    do
        echo "git checkout ${i}"
        git checkout ${i} > /dev/null 2>&1
        validator "git checkout failed"

        rm -rf ${PREFIX}build

        echo "Building ${i}"

        cd ${PREFIX}
        scons -j4 WS=fix BINDINGS=cpp > /dev/null 2>&1
        validator "Scons build failed"/
        cd ${DIR}

        mkdir -p ${OUTPUT}/lib/${i}/
        mkdir -p ${OUTPUT}/bin/${i}/

        echo "copy the needed lib and bin files"
        cp -r ${PREFIX}build/linux/x86_64/debug/dist/cpp/lib/* ${OUTPUT}/lib/${i}/
        cp -r ${PREFIX}build/linux/x86_64/debug/dist/cpp/bin/* ${OUTPUT}/bin/${i}/
    done
}

function run_introspection_backward_compatibility_test()
{
    local client=${2}
    local service=${3}

    local clientLib="${OUTPUT}/lib/${client}/"
    local serviceLib="${OUTPUT}/lib/${service}/"

    local clientBin="${OUTPUT}/bin/${client}/"
    local serviceBin="${OUTPUT}/bin/${service}/"

    echo "starting ${service} bbservice"
    LD_LIBRARY_PATH=${serviceLib} ./${serviceBin}bbservice > /dev/null 2>&1 &
    SERVICE_PID=$!

    # Give the bbservice some time to start
    sleep 2

    echo "starting ${client} bbclient"
    LD_LIBRARY_PATH=${clientLib} timeout 5 ./${clientBin}bbclient -i > /dev/null 2>&1

    rc=$?
    disown ${SERVICE_PID}
    kill ${SERVICE_PID} > /dev/null 2>&1
    if [ 0 -ne ${rc} ]; then
        RESULTS=${RESULTS}"daemon: $1 | client: $2 | service: $3 --> FAILED"$'\r\n'
    else
        RESULTS=${RESULTS}"daemon: $1 | client: $2 | service: $3 --> SUCCEEDED"$'\r\n'
    fi
}

function build_and_run_introspection_backward_compatibility_test()
{
    build
    for i in "${VERSIONS[@]}"
    do
        # start alljoyn-daemon
        echo "--- Starting ${i} alljoyn-daemon ---"
        LD_LIBRARY_PATH="${OUTPUT}/lib/${i}" ./${OUTPUT}/bin/${i}/alljoyn-daemon > /dev/null 2>&1 &

        DAEMON_PID=$!
        for j in "${VERSIONS[@]}"
        do
            for k in "${VERSIONS[@]}"
            do
                run_introspection_backward_compatibility_test ${i} ${j} ${k}
                # This check is to make sure a newer version of the bbservice
                # will not run with an old alljoyn-daemon
                if [ "${i}" == "${k}" ]; then
                    break
                fi
            done
            # This check is to make sure a newer version of the bbclient
            # will not run with an old alljoyn-daemon
            if [ "${i}" == "${j}" ]; then
                break
            fi
        done
        disown ${DAEMON_PID}
        kill -9 ${DAEMON_PID}
    done

    echo "${RESULTS}"
    echo "${RESULTS}" | grep FAILED
    if [ $? -ne 1 ]; then
        echo "Backward compatibility test failed"
        exit 1
    fi
    echo "Backward compatibility test succeeded"
    exit 0
}

VERSIONS=(${VERSIONS[@]} "$@")
build_and_run_introspection_backward_compatibility_test
