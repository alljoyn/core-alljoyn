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

STATIC_LIB = liballjoyn.a
DYNAMIC_LIB = liballjoyn.so

.PHONY: all clean

all: corelib

ifeq "$(OS)" "darwin"
   OS_SRC_DIR = $(OS)
else
   OS_SRC_DIR = $(OS_GROUP)
endif

ALLJOYN_CC_SRCS = $(wildcard *.cc)
ALLJOYN_CC_SRCS += $(wildcard $(OS_SRC_DIR)/*.cc) $(wildcard $(OS_SRC_DIR)/linux/*.cc)
ALLJOYN_CC_OBJS = $(patsubst %.cc,%.o,$(ALLJOYN_CC_SRCS))
COMMON_OBJS = $(wildcard $(COMMONDIR)/src/*.o) $(wildcard $(COMMONDIR)/os/$(OS_GROUP)/*.o) $(wildcard $(COMMONDIR)/crypto/$(CRYPTO)/*.o)
LINKFLAGS += -shared

$(ALLJOYN_CC_OBJS) : $(ALLJOYN_CC_SRCS)
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) -c -o $@ $(patsubst %.o,%.cc,$@)

ALLJOYN_C_SRCS = $(wildcard *.c)
ALLJOYN_C_OBJS = $(patsubst %.c,%.o,$(ALLJOYN_C_SRCS))

$(ALLJOYN_C_OBJS) : $(ALLJOYN_C_SRCS)
	$(CC) $(CFLAGS) $(CPPDEFINES) $(INCLUDE) -c -o $@ $(patsubst %.o,%.c,$@)

%.o:%.cc
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) -c -o $@ $<
    
corelib: $(ALLJOYN_CC_OBJS) $(ALLJOYN_C_OBJS)
	$(AR) r $(STATIC_LIB) $(ALLJOYN_CC_OBJS) $(ALLJOYN_C_OBJS) $(COMMON_OBJS)
	cp $(STATIC_LIB) $(INSTALLDIR)/dist/lib
	$(CC) -o $(DYNAMIC_LIB) $(CXXFLAGS) $(CPPDEFINES) $(LINKFLAGS) $(INCLUDE)  $(ALLJOYN_CC_OBJS) $(ALLJOYN_C_OBJS) $(COMMON_OBJS) $(LIBS)
	cp $(DYNAMIC_LIB) $(INSTALLDIR)/dist/lib

clean:
	@rm -f *.o *~ $(STATIC_LIB) $(DYNAMIC_LIB) $(OS_SRC_DIR)/*.o $(OS_SRC_DIR)/linux/*.o

