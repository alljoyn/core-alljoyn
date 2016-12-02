#! /bin/sh
# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0

#

LD_LIBRARY_PATH=$PWD/build/linux/x86_64/debug/lib/core/:$PWD/build/linux/x86_64/debug/dist/cpp/lib/:$PWD/build/linux/x86_64/debug/dist/security/lib:build/linux/x86_64/debug/dist/about/lib/ ./build/linux/x86_64/debug/dist/security/bin/samples/secmgr