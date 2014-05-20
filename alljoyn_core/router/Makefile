# Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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

ROUTER_LIB = libajrouter.a

all: $(ROUTER_LIB) daemon bundled_router_obj test_progs

ROUTER_SRCS = $(wildcard *.cc)
ROUTER_SRCS += $(wildcard *.c)
ROUTER_SRCS += $(wildcard ns/*.cc)
ROUTER_SRCS += \
	$(OS_GROUP)/Socket.cc 

INCLUDE += -I$(PWD)/ice -I$(PWD)/$(OS_GROUP) -I$(PWD)/JSON -I$(PWD)/ns

ifeq "$(OS)" "darwin"
    # Darwin has its own version of the daemon transport
    ROUTER_SRCS += $(OS)/DaemonTransport.cc
else
    ROUTER_SRCS += $(OS_GROUP)/DaemonTransport.cc
endif

ifeq "$(OS)" "android"
    ROUTER_SRCS += $(OS)/PermissionMgr.cc
else
    ROUTER_SRCS += $(OS_GROUP)/PermissionMgr.cc
endif

ROUTER_OBJS = $(patsubst %.cc,%.o,$(ROUTER_SRCS))
$(ROUTER_OBJS) : $(ROUTER_SRCS)
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) -c -o $@ $(patsubst %.o,%.cc,$@)


CONFIG_SRCS = ConfigDB.cc ServiceDB.cc PropertyDB.cc PolicyDB.cc
CONFIG_OBJS = $(patsubst %.cc,%.o,$(CONFIG_SRCS))

DAEMON_MAIN = $(OS_GROUP)/daemon-main.cc
DAEMON_MAIN_OBJ = $(patsubst %.cc,%.o,$(DAEMON_MAIN))

TESTDIR = test

$(TESTDIR)/advtunnel.o : $(TESTDIR)/advtunnel.cc
$(TESTDIR)/bbdaemon.o : $(TESTDIR)/bbdaemon.cc
$(TESTDIR)/mcmd.o : $(TESTDIR)/mcmd.cc

BUNDLED_ROUTER_SRCS = bundled/BundledRouter.cc
BUNDLED_ROUTER_OBJ = $(patsubst %.cc,%.o,$(BUNDLED_ROUTER_SRCS))

LIBS += -lalljoyn

%.o:%.cc
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) -c -o $@ $<

$(ROUTER_LIB): $(ROUTER_OBJS)
	$(AR) r $(ROUTER_LIB) $(ROUTER_OBJS)
	cp $(ROUTER_LIB) $(INSTALLDIR)/dist/lib

daemon : $(ROUTER_OBJS) $(DAEMON_MAIN_OBJ)
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o alljoyn-daemon $(ROUTER_OBJS) $(DAEMON_MAIN_OBJ) $(LIBS)
	cp alljoyn-daemon $(INSTALLDIR)/dist/bin
 
bundled_router_obj : $(BUNDLED_ROUTER_OBJ)
	cp $(BUNDLED_ROUTER_OBJ) $(INSTALLDIR)/dist/lib

test_progs: advtunnel bbdaemon

advtunnel : $(ROUTER_OBJS) $(TESTDIR)/advtunnel.o
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o advtunnel $(ROUTER_OBJS) $(TESTDIR)/advtunnel.o $(LIBS)
	cp advtunnel $(INSTALLDIR)/dist/bin

bbdaemon : $(ROUTER_OBJS) $(TESTDIR)/bbdaemon.o
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o bbdaemon $(ROUTER_OBJS) $(TESTDIR)/bbdaemon.o $(LIBS)
	cp bbdaemon $(INSTALLDIR)/dist/bin

DaemonTest : $(ROUTER_OBJS) $(TESTDIR)/DaemonTest.o
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o DaemonTest $(ROUTER_OBJS) $(TESTDIR)/DaemonTest.o $(LIBS)
	cp DaemonTest $(INSTALLDIR)/dist/bin

clean:
	@rm -f *.o *~ $(OS_GROUP)/*.o $(TESTDIR)/*.o bt_bluez/*.o ice/*.o bundled/*.o JSON/*.o ns/*.o alljoyn-daemon $(ROUTER_LIB) advtunnel bbdaemon DaemonTest mcmd


