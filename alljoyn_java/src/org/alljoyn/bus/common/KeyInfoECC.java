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

public abstract class KeyInfoECC extends KeyInfo {

    /**
     * The ECC key type
     */
    public static final long KEY_TYPE = 0;

    /**
     * Default constructor.
     */
    public KeyInfoECC()
    {
        super(FormatType.FORMAT_ALLJOYN);
        m_curve = CryptoECC.ECC_NIST_P256;
    }

    /**
     * constructor.
     */
    KeyInfoECC(byte curve)
    {
        super(FormatType.FORMAT_ALLJOYN);
        m_curve = curve;
    }

    /**
     * Retrieve the ECC algorithm
     * @return the ECC algorithm
     */
    public byte getAlgorithm()
    {
        return 0;
    }

    /**
     * Retrieve the ECC curve type.
     * @return the ECC curve type
     */
    public byte getCurve()
    {
        return m_curve;
    }

    /**
     * Virtual member function that must be implemented so it returns
     * a pointer to the ECCPublicKey that was set.
     */
    public abstract ECCPublicKey getPublicKey();

    /**
     * Virtual member function that must be implemented so the public key is
     * assigned a value.
     *
     * @param key public ECC key
     */
    public abstract void setPublicKey(ECCPublicKey key);

    /**
     * The ECC algorithm
     */
    private byte m_curve;

    @Override
    public int hashCode() {
        return super.hashCode() * 31 + m_curve;
    }
}
