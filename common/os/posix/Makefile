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

all: oslib

ifeq "$(OS)" "linux"
  IFCONFIG=IfConfigLinux
  UARTSTREAM=UARTStreamLinux
else
  IFCONFIG=IfConfigDarwin
  UARTSTREAM=UARTStreamDarwin
endif

oslib: \
        atomic.o \
	Environ.o \
	Event.o \
	FileStream.o \
	$(IFCONFIG).o \
	Mutex.o \
	OSLogger.o \
	osUtil.o \
	Socket.o \
	SslSocket.o \
	Thread.o \
	time.o \
	Timer.o \
	$(UARTSTREAM).o

%.o:%.cc
	$(CC) $(INCLUDE) $(CXXFLAGS) $(CPPDEFINES)  -c -o $@ $<

clean:
	@rm -f *.o *~
