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

public class ECCPublicKey {

    /**
     * The x coordinate of the elliptic curve point
     */
    private byte x[];

    /**
     * The y coordinate of the elliptic curve point
     */
    private byte y[];

    private ECCPublicKey(byte x_coor[], byte y_coor[]) {
        x = x_coor;
        y = y_coor;
    }

    public long getCoordinateSize() {
        assert x.length == y.length;
        return x.length;
    }

    public long getSize() {
        return getCoordinateSize() * 2;
    }

    @Override
    public String toString() {
        return Arrays.toString(x) + ", " + Arrays.toString(y);
    }

    @Override
    public boolean equals(Object compObj) {
        if (this == compObj) {
            return true;
        }
        if (!(compObj instanceof ECCPublicKey)) {
            return false;
        }
        if (compObj == null) {
            return false;
        }

        ECCPublicKey compPublicKey = (ECCPublicKey) compObj;

        if (getCoordinateSize() != compPublicKey.getCoordinateSize()) {
            return false;
        }

        for (int index = 0; index < getCoordinateSize(); index++) {
            if (x[index] != compPublicKey.x[index]) {
                return false;
            }
            if (y[index] != compPublicKey.y[index]) {
                return false;
            }
        }

        return true;
    }
}
