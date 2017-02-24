/*
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
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

    public ECCPublicKey() {}

    public ECCPublicKey(byte x_coor[], byte y_coor[]) {
        x = x_coor;
        y = y_coor;
    }

    public byte[] getX() {
        return x;
    }

    public byte[] getY() {
        return y;
    }

    public void setX(byte[] x_coor) {
        x = x_coor;
    }

    public void setY(byte[] y_coor) {
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

        return Arrays.equals(x, compPublicKey.x) && Arrays.equals(y, compPublicKey.y);
    }

    @Override
    public int hashCode() {
        return Arrays.hashCode(x) * 31 + Arrays.hashCode(y);
    }
}