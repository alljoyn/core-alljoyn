# Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

.PHONY: all clean

DAEMON_LIB = libajdaemon.a

all: $(DAEMON_LIB) daemon bundled_obj test_progs

DAEMON_SRCS = $(wildcard *.cc)
DAEMON_SRCS += $(wildcard *.c)
DAEMON_SRCS += $(wildcard ns/*.cc)
DAEMON_SRCS += \
	$(OS_GROUP)/ProximityScanner.cc \
	$(OS_GROUP)/Socket.cc 

INCLUDE += -I$(PWD)/ice -I$(PWD)/$(OS_GROUP) -I$(PWD)/JSON -I$(PWD)/ns

ifeq "$(OS)" "darwin"
    # Darwin has its own version of the daemon transport
    DAEMON_SRCS += $(OS)/DaemonTransport.cc
else
    DAEMON_SRCS += $(OS_GROUP)/DaemonTransport.cc
endif

ifeq "$(OS)" "android"
    DAEMON_SRCS += $(OS)/PermissionMgr.cc
else
    DAEMON_SRCS += $(OS_GROUP)/PermissionMgr.cc
endif

ifeq "$(BT)" "on"
    # Use BlueZ
    INCLUDE += -I$(PWD)/bluetooth -I$(PWD)/bluetooth/bt_bluez
    DAEMON_SRCS += $(wildcard bluetooth/*.cc) $(wildcard bluetooth/bt_bluez/*.cc)
endif

ifeq "$(BT)" "on"
    # Use ICE
    INCLUDE += -I$(PWD)/packetengine
    DAEMON_SRCS += $(wildcard ice/*.cc)
    DAEMON_SRCS += $(wildcard JSON/*.cc)
    DAEMON_SRCS += $(wildcard packetengine/*.cc)

endif

DAEMON_OBJS = $(patsubst %.cc,%.o,$(DAEMON_SRCS))
$(DAEMON_OBJS) : $(DAEMON_SRC)
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) -c -o $@ $(patsubst %.o,%.cc,$@)


CONFIG_SRCS = ConfigDB.cc ServiceDB.cc PropertyDB.cc PolicyDB.cc
CONFIG_OBJS = $(patsubst %.cc,%.o,$(CONFIG_SRCS))

DAEMON_MAIN = $(OS_GROUP)/daemon-main.cc
DAEMON_MAIN_OBJ = $(patsubst %.cc,%.o,$(DAEMON_MAIN))

TESTDIR = test

$(TESTDIR)/advtunnel.o : $(TESTDIR)/advtunnel.cc
$(TESTDIR)/bbdaemon.o : $(TESTDIR)/bbdaemon.cc
$(TESTDIR)/mcmd.o : $(TESTDIR)/mcmd.cc

BUNDLED_SRCS = bundled/BundledDaemon.cc
BUNDLED_OBJ = $(patsubst %.cc,%.o,$(BUNDLED_SRCS))

LIBS += -lalljoyn

%.o:%.cc
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) -c -o $@ $<

$(DAEMON_LIB): $(DAEMON_OBJS)
	$(AR) r $(DAEMON_LIB) $(DAEMON_OBJS)
	cp $(DAEMON_LIB) $(INSTALLDIR)/dist/lib

daemon : $(DAEMON_OBJS) $(DAEMON_MAIN_OBJ)
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o alljoyn-daemon $(DAEMON_OBJS) $(DAEMON_MAIN_OBJ) $(LIBS)
	cp alljoyn-daemon $(INSTALLDIR)/dist/bin
 
bundled_obj : $(BUNDLED_OBJ)
	cp $(BUNDLED_OBJ) $(INSTALLDIR)/dist/lib

test_progs: advtunnel bbdaemon

advtunnel : $(DAEMON_OBJS) $(TESTDIR)/advtunnel.o
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o advtunnel $(DAEMON_OBJS) $(TESTDIR)/advtunnel.o $(LIBS)
	cp advtunnel $(INSTALLDIR)/dist/bin

bbdaemon : $(DAEMON_OBJS) $(TESTDIR)/bbdaemon.o
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o bbdaemon $(DAEMON_OBJS) $(TESTDIR)/bbdaemon.o $(LIBS)
	cp bbdaemon $(INSTALLDIR)/dist/bin

DaemonTest : $(DAEMON_OBJS) $(TESTDIR)/DaemonTest.o
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o DaemonTest $(DAEMON_OBJS) $(TESTDIR)/DaemonTest.o $(LIBS)
	cp DaemonTest $(INSTALLDIR)/dist/bin

clean:
	@rm -f *.o *~ $(OS_GROUP)/*.o $(TESTDIR)/*.o bt_bluez/*.o ice/*.o bundled/*.o JSON/*.o ns/*.o alljoyn-daemon $(DAEMON_LIB) advtunnel bbdaemon DaemonTest mcmd


