/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
package org.alljoyn.bus.common;

import java.util.Arrays;

public class KeyInfoNISTP256 extends KeyInfoECC {
    private byte form;
    private ECCPublicKey key;

    public KeyInfoNISTP256() {
        super(CryptoECC.ECC_NIST_P256);

        /* using uncompressed */
        form = 0x4;
    }

    public ECCPublicKey getPublicKey() {
        return key;
    }

    public void setPublicKey(ECCPublicKey pkey) {
        key = pkey;
    }
}