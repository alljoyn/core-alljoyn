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

all : common

common:
	cd src; make; 
	cd os/$(OS_GROUP); make;
	cd crypto/$(CRYPTO); make;

	@mkdir -p $(INSTALLDIR)/dist/inc/qcc
	cp inc/qcc/Log.h \
       inc/qcc/ManagedObj.h \
       inc/qcc/String.h \
       inc/qcc/atomic.h \
       inc/qcc/SocketWrapper.h \
       inc/qcc/platform.h $(INSTALLDIR)/dist/inc/qcc

	@mkdir -p $(INSTALLDIR)/dist/inc/qcc/$(OS_GROUP)
	cp inc/qcc/${OS_GROUP}/atomic.h \
       inc/qcc/${OS_GROUP}/platform_types.h \
       inc/qcc/${OS_GROUP}/unicode.h $(INSTALLDIR)/dist/inc/qcc/$(OS_GROUP)

clean:
	cd src; make clean;
	cd os/$(OS_GROUP); make clean;
	cd crypto/$(CRYPTO); make clean;
	@rm -f *.o
