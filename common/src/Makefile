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

TARGET=src

.PHONY: all clean

all: commonsrc

commonsrc: \
	ASN1.o \
	BigNum.o \
	BufferedSink.o \
	BufferedSource.o \
	Config.o \
	Crypto.o \
	CryptoSRP.o \
	Debug.o \
	GUID.o \
	IPAddress.o \
	IODispatch.o \
	KeyBlob.o \
	Logger.o \
	Makefile \
	Pipe.o \
	SLAPPacket.o \
	SLAPStream.o \
	SocketStream.o \
	Stream.o \
	StreamPump.o \
	String.o \
	StringSource.o \
	StringUtil.o \
	ThreadPool.o \
	Util.o \
	XmlElement.o 

%.o:%.cc
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) -c -o $@ $<

clean:
	@rm -f *.o *~
