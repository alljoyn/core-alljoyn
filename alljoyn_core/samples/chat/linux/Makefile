# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0


ALLJOYN_DIST := ../..
#ALLJOYN_DIST := ../../../build/linux/x86/debug/dist

ALLJOYN_LIB := $(ALLJOYN_DIST)/lib/liballjoyn.a

CXXFLAGS = -Wall -pipe -std=c++98 -fno-rtti -fno-exceptions -Wno-long-long -Wno-deprecated -g -DQCC_OS_LINUX -DQCC_OS_GROUP_POSIX

LIBS = -lalljoyn -lstdc++ -lcrypto -lpthread -lrt 

.PHONY: default clean

default: chat

clean:
	rm -f chat.o chat

chat: chat.o $(ALLJOYN_LIB)
	$(CXX) -o $@ chat.o -L$(ALLJOYN_DIST)/lib $(LIBS)

chat.o: chat.cc $(ALLJOYN_LIB)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $@ chat.cc
