# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0


ifneq ($(TARGET_CC),)
CC:=$(TARGET_CC)
endif

ifneq ($(TARGET_CXX),)
CXX:=$(TARGET_CXX)
else
endif

CPPFLAGS:=$(TARGET_CPPFLAGS) -DQCC_OS_GROUP_POSIX -DQCC_OS_LINUX
CXXFLAGS:=$(TARGET_CXXFLAGS) -Wall
LDLIBS:=$(TARGET_LDFLAGS) -lalljoyn -lcrypto -lssl -lpthread -lrt

all: chatresponder

clean:
	$(RM) chatresponder chatresponder.o

chatresponder: chatresponder.cc

