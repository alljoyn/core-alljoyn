# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0

# 

.PHONY: all clean

all: progs

LIBS += -lalljoyn
PROG_BINS =  \
        bignum \
        socktest \
        autochat \
        remarshal \
        unpack \
        srp \
        aes_ccm \
        keystore \
        bbservice \
        bbsig \
        bbclient \
        bbjoin \
        bbjitter \
        marshal \
        names \
        compression \
        rawclient \
        rawservice \
        sessions

# Test Programs
progs : $(PROG_BINS)

%:%.cc
	$(CC) $(CXXFLAGS) $(CPPDEFINES) $(INCLUDE) $(LINKFLAGS) -o $@ $< $(LIBS)
	cp $@ $(INSTALLDIR)/dist/bin

clean:
	@rm -f $(PROG_BINS) *~

