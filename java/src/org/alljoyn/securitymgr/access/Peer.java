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

package org.alljoyn.securitymgr.access;

import java.util.Arrays;

import org.alljoyn.securitymgr.Guild;

/**
 * A Class describing a peer. Policy Rules can be defined for peers.
 */
public final class Peer {
    private final PeerType type;
    private final byte[] id;
    private final byte[] keyInfo; // public key of guild authority or psk key

    /* Constructs a Peer */
    public Peer(PeerType type, byte[] id, byte[] keyInfo) {
        this.type = type;
        this.id = id;
        this.keyInfo = keyInfo;
    }

    /* Constructs a Peer */
    public Peer(PeerType type, byte[] id) {
        this.type = type;
        this.id = id;
        this.keyInfo = null;
    }

    /* Constructs a Peer */
    public Peer(Guild guild) {
        this.type = PeerType.GUILD;
        this.id = guild.getGuid().getGuidData();
        this.keyInfo = guild.getOwnerKey();
    }
    /**
     * The type of peer.
     * @return a non-null PeerType.
     */
    public PeerType getType() {
        return type;
    }

    /**
     * The id or GUID of the peer.
     * @return a non-null array.
     */
    public byte[] getId() {
        return id.clone();
    }

    /**
     * A description of the key of the Peer.
     * @return a non-null array.
     */
    public byte[] getKeyInfo() {
        return keyInfo.clone();
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = prime + Arrays.hashCode(id);
        result = prime * result + Arrays.hashCode(keyInfo);
        result = prime * result + ((type == null) ? 0 : type.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof Peer) {
            Peer other = (Peer) obj;
            return type == other.type && Arrays.equals(id, other.id)
                    && Arrays.equals(keyInfo, other.keyInfo);
        }
        return false;
    }

    @Override
    public String toString() {
        return "Peer [type=" + type + ", id=" + Arrays.toString(id)
                + ", keyInfo=" + Arrays.toString(keyInfo) + "]";
    }

    /* Helper function for the JNI code */
    int getPeerType() {
        return type.ordinal();
    }
}
