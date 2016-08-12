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
import org.alljoyn.bus.annotation.Position;

public class ECCPublicKey {

    /** 
     * the algorithm for this key
     */
    @Position(0) public byte algorithm;

    /**
     * the curve of the algorithm
     */
    @Position(1) public byte curve;

    /**
     * The x coordinate of the elliptic curve point
     */
    @Position(2) public byte x[];

    /**
     * The y coordinate of the elliptic curve point
     */
    @Position(3) public byte y[];

    public String toString() {
        return Byte.toString(algorithm) + ", " + Byte.toString(curve) + ", " + Arrays.toString(x) + ", " + Arrays.toString(y);
    }
}
