/*
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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

    public void setKeyId(byte[] keyId) {
        m_keyId = keyId;
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
