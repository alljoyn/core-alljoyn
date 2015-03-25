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

all: door_consumer door_provider

door_provider: door_provider.o $(ALLJOYN_LIB)
	mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/door_provider.o $(TOP)$(ALLJOYN_LIB) $(LIBS)

door_provider.o: door_provider.cc $(ALLJOYN_LIB)
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ door_provider.cc

door_consumer: door_consumer.o $(ALLJOYN_LIB)
	mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/door_consumer.o $(TOP)$(ALLJOYN_LIB) $(LIBS)

door_consumer.o: door_consumer.cc $(ALLJOYN_LIB)
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ door_consumer.cc

clean: clean_door_consumer clean_door_provider
	rmdir $(OBJ_DIR)
	rmdir $(BIN_DIR)

clean_door_provider:
	rm -f $(OBJ_DIR)/door_provider.o $(BIN_DIR)/door_provider

clean_door_consumer:
	rm -f $(OBJ_DIR)/door_consumer.o $(BIN_DIR)/door_consumer
