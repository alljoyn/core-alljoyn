/******************************************************************************
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
 ******************************************************************************/

package org.alljoyn.bus;

import org.alljoyn.bus.annotation.Position;

/**
 * the SHA-256 digest of the manifest template.
 */
public class ManifestRuleMember {

    /** 
     * the version of the manifest
     */
    @Position(0) public String memberName;

    public enum MemberType {
        NOT_SPECIFIED,
        METHOD_CALL,
        SIGNAL,
        PROPERTY
    }

    /**
     * digest the buffer to hold the SHA-256 digest of the manifest template.
     */
    @Position(1) public MemberType memberType;


    /**
     * The permission action masks
     */
    public static class ActionMasks {
        /** 
         * allow to provide
         */
        public static final byte ACTION_PROVIDE = 0x01;
        /** 
         * allow to observe
         */
        public static final byte ACTION_OBSERVE = 0x02;
        /** 
         * allow to modify
         */
        public static final byte ACTION_MODIFY = 0x04;
    }

    @Position(2) public byte actionMask;

    public String toString() {
        return memberName + ", " + memberType + ", " + Byte.toString(actionMask);
    }
}
