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