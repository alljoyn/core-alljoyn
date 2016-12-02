#!/bin/bash
# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0

#


SELF_DIR=$(cd "$(dirname "$0")" > /dev/null; pwd)
ROOT=$(readlink -f "${SELF_DIR}/../")

EXEC=$(find "${ROOT}" -name "alljoyn-daemon" -print -quit)

if [ -z "${EXEC}" ]; then
  echo "AllJoyn Bus Daemon not found!  Build it first!"
  exit 1
fi

CONFIG=$(find "${ROOT}" -name "alljoyn_daemon_config.xml" -print -quit)

LIB_PATH=$(find "${ROOT}" -name "liballjoyn.so" -print -quit)
LIB_PATH="$(dirname "${LIB_PATH}")"

export LD_LIBRARY_PATH="${LIB_PATH}"
exec "${EXEC}" --config-file="${CONFIG}" --nofork --verbosity=7
