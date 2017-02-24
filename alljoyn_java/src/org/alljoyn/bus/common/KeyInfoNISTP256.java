/*
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
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

    @Override
    public boolean equals(Object compObj) {
        if (this == compObj) {
            return true;
        }
        if (!(compObj instanceof KeyInfoNISTP256)) {
            return false;
        }
        if (compObj == null) {
            return false;
        }

        if (!super.equals(compObj)) {
            return false;
        }

        KeyInfoNISTP256 compKeyInfo = (KeyInfoNISTP256) compObj;

        return key.equals(compKeyInfo.key);
    }

    @Override
    public int hashCode() {
        return 31 * super.hashCode() + (key != null ? key.hashCode() : 0);
    }
}