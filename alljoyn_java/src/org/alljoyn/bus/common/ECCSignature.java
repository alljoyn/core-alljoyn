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

public class ECCSignature {
    private byte[] r;
    private byte[] s;

    private static final long ECC_COORDINATE_SZ = 32;

    public ECCSignature() {
        r = new byte[(int) ECC_COORDINATE_SZ];
        s = new byte[(int) ECC_COORDINATE_SZ];
    }

    public ECCSignature(byte[] tr, byte[] ts) {
        r = tr;
        s = ts;
    }

    public byte[] getR() {
        return r;
    }

    public byte[] getS() {
        return s;
    }
}