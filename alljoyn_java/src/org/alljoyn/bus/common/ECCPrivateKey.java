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

public class ECCPrivateKey {

    /**
     * The ECCPrivateKey data
     */
    private byte d[];

    public ECCPrivateKey() {}

    /**
     * ECCPrivateKey constructor
     */
    public ECCPrivateKey(byte d_key[]) {
        d = d_key;
    }

    public byte[] getD() {
        return d;
    }

    public void setD(byte[] m_d) {
        d = m_d;
    }

    @Override
    public String toString() {
        return Arrays.toString(d);
    }

    @Override
    public boolean equals(Object compObj) {
        if (this == compObj) {
            return true;
        }
        if (!(compObj instanceof ECCPrivateKey)) {
            return false;
        }
        if (compObj == null) {
            return false;
        }

        ECCPrivateKey compPublicKey = (ECCPrivateKey) compObj;

        if (d.length != compPublicKey.d.length) {
            return false;
        }

        for (int index = 0; index < d.length; index++) {
            if (d[index] != compPublicKey.d[index]) {
                return false;
            }
        }

        return true;
    }
}