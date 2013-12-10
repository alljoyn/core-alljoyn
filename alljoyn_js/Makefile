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

#
# Example command: make OS=linux CPU=x86 VARIANT=debug

.PHONY: all dist clean distclean

all: distclean dist

CC=gcc
CXX=g++
AR=ar

# build configuration
ifeq "$(CPU)" "x86"
    CPU	= x86
else
ifeq "$(CPU)" "x86_64"
    CPU	= x86_64
else
    $(error CPU=$(CPU) is not supported.)
endif
endif

ifeq "$(OS)" "linux"
    OS	= linux
else
    $(error OS=$(OS) is not supported.)
endif

ifeq "$(VARIANT)" "debug"
    VARIANT	= debug
else
ifeq "$(VARIANT)" "release"
    VARIANT	= release
else
    $(error VARIANT=$(VARIANT) is not supported.)
endif
endif

ifeq "$(OS)" "linux"
    # Linux specific flags
    CPPDEFINES+=-DQCC_OS_LINUX
    CPPDEFINES+=-DQCC_OS_GROUP_POSIX
    OS_GROUP=posix
endif

CFLAGS=-Wall \
       -Werror=non-virtual-dtor \
       -pipe \
       -std=c99 \
       -fno-strict-aliasing \
       -Wno-long-long

CXXFLAGS=-Wall \
        -Werror=non-virtual-dtor \
        -pipe \
        -std=c++0x \
        -fno-rtti \
        -fno-exceptions \
        -fno-strict-aliasing \
        -Wno-long-long \
        -Wno-deprecated

ifeq "$(CPU)" "x86"
# Force 32-bit builds
    CXXFLAGS+=-m32 -pthread
    CFLAGS+=-m32 -pthread
    LINKFLAGS+=-m32 -pthread
else
    ifeq "$(CPU)" "x86_64"
        CXXFLAGS+=-m64 -fPIC -pthread
        CFLAGS+=-m64 -fPIC -pthread
	    LINKFLAGS+=-m64 -pthread
    endif
endif

# Debug/Release Variants
ifeq "$(VARIANT)" "debug"
    CFLAGS+=-g
    CXXFLAGS+=-g
    JAVACFLAGS+=-g -Xlint -Xlint:-serial
else
    CFLAGS+=-O3
    CXXFLAGS+=-O3
    LINKFLAGS=-s
    CPPDEFINES+=-DNDEBUG
    JAVACFLAGS=-Xlint -Xlint:-serial
endif

# Header/lib path include
INCLUDE = -I$(PWD)/../common/inc -I$(PWD)/../alljoyn_core/src -I$(PWD)/../alljoyn_core/daemon

# Platform specifics system libs
ifeq "$(OS)" "linux"
    LIBS += -lrt -lstdc++ -lpthread -lcrypto -lssl
endif

INSTALLDIR = $(PWD)/../build/$(OS)/$(CPU)/$(VARIANT)
ALLJOYNLIB = $(INSTALLDIR)/dist/lib/liballjoyn.a
AJDAEMONLIB = $(INSTALLDIR)/dist/lib/libajdaemon.a
BUNDLED_OBJ = $(INSTALLDIR)/dist/lib/BundledDaemon.o
COMMONDIR = $(PWD)/../common

INCLUDE += -I$(INSTALLDIR)/dist/inc -I$(INSTALLDIR)/dist/inc/alljoyn
INCLUDE += -L$(INSTALLDIR)/dist/lib

JUNK=*.o *~
export

dist:
	@mkdir -p $(INSTALLDIR)/dist/js/lib
	cd jni; make;

clean:
	@rm -f $(JUNK)
	cd jni; make clean;

distclean: clean
	@rm -rf $(INSTALLDIR)/dist/js/lib

jstest:
	/bin/bash jsunit.sh $(INSTALLDIR)
