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
}
