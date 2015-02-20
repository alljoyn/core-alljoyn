/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

package org.alljoyn.securitymgr;

import java.util.Arrays;

public class GUID {
    final byte[] guid;

    public GUID(byte[] guid) {
        if(guid.length != 16) {
            throw new IllegalArgumentException("wrong size: " + guid.length);
        }
        this.guid = guid.clone();;
    }

    /**
     * @return the guid data
     */
    public byte[] getGuidData() {
        return guid.clone();
    }
    @Override
    public int hashCode() {
        return 31 + Arrays.hashCode(guid);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        GUID other = (GUID) obj;
        return Arrays.equals(guid, other.guid);
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder("GUID [");
        builder.append(Integer.toHexString(guid[0] & 0xff));
        for (int i = 1; i < guid.length; i++) {
            builder.append('-').append(Integer.toHexString(guid[i] & 0xff));
        }
        return builder.append("]").toString();
    }
}
