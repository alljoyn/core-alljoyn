# Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

# returns current working directory
TOP := $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

ALLJOYN_DIST := ../..
# for use by AllJoyn developers. Code built from alljoyn_core
#ALLJOYN_DIST := ../../build/linux/x86/debug/dist
#ALLJOYN_DIST := ../../build/linux/x86/release/dist
#ALLJOYN_DIST := ../../build/linux/x86_64/debug/dist
#ALLJOYN_DIST := ../../build/linux/x86_64/release/dist
# for use by AllJoyn developers. Code built from master
#ALLJOYN_DIST := ../../../build/linux/x86/debug/dist
#ALLJOYN_DIST := ../../../build/linux/x86/release/dist
#ALLJOYN_DIST := ../../../build/linux/x86_64/debug/dist
#ALLJOYN_DIST := ../../../build/linux/x86_64/release/dist

OBJ_DIR := obj

BIN_DIR := bin

ALLJOYN_LIB := $(ALLJOYN_DIST)/lib/liballjoyn.a

CXXFLAGS = -Wall -pipe -std=c++98 -fno-rtti -fno-exceptions -Wno-long-long -Wno-deprecated -g -DQCC_OS_LINUX -DQCC_OS_GROUP_POSIX -DQCC_CPU_X86

LIBS = -lstdc++ -lcrypto -lpthread -lrt

.PHONY: default clean

default: all

all: basic_client basic_service nameChange_client signal_service signalConsumer_client

basic_client: basic_client.o $(ALLJOYN_LIB)
	mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/basic_client.o $(TOP)$(ALLJOYN_LIB) $(LIBS)

basic_client.o: basic_client.cc $(ALLJOYN_LIB)
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ basic_client.cc

basic_service: basic_service.o $(ALLJOYN_LIB)
	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/basic_service.o $(TOP)$(ALLJOYN_LIB) $(LIBS)

basic_service.o: basic_service.cc $(ALLJOYN_LIB)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ basic_service.cc

nameChange_client: nameChange_client.o $(ALLJOYN_LIB)
	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/nameChange_client.o $(TOP)$(ALLJOYN_LIB) $(LIBS)

nameChange_client.o: nameChange_client.cc $(ALLJOYN_LIB)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ nameChange_client.cc

signal_service: signal_service.o $(ALLJOYN_LIB)
	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/signal_service.o $(TOP)$(ALLJOYN_LIB) $(LIBS)

signal_service.o: signal_service.cc $(ALLJOYN_LIB)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ signal_service.cc

signalConsumer_client: signalConsumer_client.o $(ALLJOYN_LIB)
	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/signalConsumer_client.o $(TOP)$(ALLJOYN_LIB) $(LIBS)

signalConsumer_client.o: signalConsumer_client.cc $(ALLJOYN_LIB)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ signalConsumer_client.cc

clean: clean_basic_client clean_basic_service clean_nameChange_client clean_signal_service clean_signalConsumer_client
	rmdir $(OBJ_DIR)
	rmdir $(BIN_DIR)

clean_basic_client:
	rm -f $(OBJ_DIR)/basic_client.o $(BIN_DIR)/basic_client

clean_basic_service:
	rm -f $(OBJ_DIR)/basic_service.o $(BIN_DIR)/basic_service

clean_nameChange_client:
	rm -f $(OBJ_DIR)/nameChange_client.o $(BIN_DIR)/nameChange_client

clean_signal_service:
	rm -f $(OBJ_DIR)/signal_service.o $(BIN_DIR)/signal_service

clean_signalConsumer_client:
	rm -f $(OBJ_DIR)/signalConsumer_client.o $(BIN_DIR)/signalConsumer_client
