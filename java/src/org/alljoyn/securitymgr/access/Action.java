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

/**
 * Describe and Action on a policy rules
 *
 */
public enum Action {
    DENIED(0x1), PROVIDE(0x2), OBSERVE(0x4), MODIFY(0x8);

    /**
     * Returns array containing all required to get full access to interface or
     * member. Full access allows the holder to provide, observe and modify
     * items.
     *
     * @return a non-null array.
     */
    public static Action[] fullAccess() {
        return new Action[] {PROVIDE, MODIFY};
    }

    private final int bitmask;

    private Action(int mask) {
        bitmask = mask;
    }

    /**
     * Returns the bit mask linked to the action.
     * 
     * @return the bit mask of this action.
     */
    public int getBitmask() {
        return bitmask;
    }
}
