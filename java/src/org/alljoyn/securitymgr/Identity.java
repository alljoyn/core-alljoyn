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

/**
 * A class representing user identity.
 *
 * User identity are assigned to applications. Application act in name of that
 * user.
 */
public final class Identity {
    private String name;
    private final GUID guid;
    final byte[] key;

    /*
     * Called from native.
     */
    Identity(String idName, byte[] guidBytes, byte[] keyData) {
        guid = new GUID(guidBytes);
        name = idName;
        key = keyData;
    }

    /**
     * Constructs an Identity.
     *
     * @param name the non-null name of the user
     * @param guid the non-null GUID identifying the Identity
     */
    public Identity(String name, GUID guid) {
        this(name, guid, null);
    }

    /**
     * Constructs an Identity
     *
     * @param name the non-null name of the user
     * @param guid the non-null GUID identifying the Identity
     * @param
     */
    public Identity(String name, GUID guid, byte[] ownerKey) {
        this.name = name;
        this.guid = guid;
        key = ownerKey;
    }

    /**
     * @return the name
     */
    public String getName() {
        return name;
    }

    /**
     * @param name the name to set
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * @return the guid
     */
    public GUID getGuid() {
        return guid;
    }

    /**
     * @return the key
     */
    public final byte[] getKey() {
        return key == null ? null : key.clone();
    }

    /**
     * {@inheritDoc}
     *
     * @see java.lang.Object#hashCode()
     */
    @Override
    public int hashCode() {
        return 41 + guid.hashCode() + Arrays.hashCode(key);
    }

    /**
     * {@inheritDoc}
     *
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object obj) {
        if (obj instanceof Identity) {
            Identity other = (Identity) obj;
            return guid.equals(other.guid) && Arrays.equals(key, other.key);
        }
        return false;
    }

}
