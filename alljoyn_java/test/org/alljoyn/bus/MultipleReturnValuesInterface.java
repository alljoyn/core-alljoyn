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

package org.alljoyn.bus;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.Position;

import java.util.Arrays;
import java.util.Map;

@BusInterface
public interface MultipleReturnValuesInterface {

    public class Values {
        public static class Inner {
            @Position(0) public int x;
            public boolean equals(Object obj) {
                return x == ((Inner)obj).x;
            }
        }
        @Position(0) public int a;
        @Position(1) public int b;
        @Position(2) public Map<String, String> c;
        @Position(3) public Map<String, String>[] d;
        @Position(4) public long[] e;
        @Position(5) public Inner f;
        public boolean equals(Object obj) {
            Values v = (Values)obj;
            return a == v.a &&
                b == v.b &&
                c.equals(v.c) &&
                Arrays.equals(d, v.d) &&
                Arrays.equals(e, v.e) &&
                f.equals(v.f);
        }
    }

    @BusMethod(name="Method", replySignature="iia{ss}aa{ss}ax(i)")
    public Values method() throws BusException;
}