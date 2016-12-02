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

public class KeyInfo {

    public enum FormatType {
        /**
         * AllJoyn format
         */
        FORMAT_ALLJOYN,
        /**
         * JSON Web Key format
         */
        FORMAT_JWK,
        /**
         * X.509 format
         */
        FORMAT_X509
    }

    private FormatType m_format;

    public enum KeyUsageType {
        /**
         * Key is used for signing
         */
        USAGE_SIGNING,
        /**
         * Key is used for encryption
         */
        USAGE_ENCRYPTION
    }

    private byte[] m_keyId;

    public KeyInfo (FormatType format) {
        m_format = format;
    }

    public KeyInfo (FormatType format, byte[] keyId) {
        m_format = format;
        m_keyId = keyId;
    }

    public FormatType getFormat() {
        return m_format;
    }

    public byte[] getKeyId() {
        return m_keyId;
    }

    @Override
    public boolean equals (Object obj) {
        if (obj == null) {
            return false;
        }

        if (!(obj instanceof KeyInfo)) {
            return false;
        }

        if (((KeyInfo) obj).m_format != this.m_format) {
            return false;
        }

        return Arrays.equals(((KeyInfo) obj).m_keyId, this.m_keyId); 
    }
}