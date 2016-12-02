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

import org.alljoyn.bus.Signature;

import junit.framework.TestCase;
import static org.alljoyn.bus.Assert.*;

public class SignatureTest extends TestCase {
    public SignatureTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public void testStructArgs_withObjectArray() throws Exception {
        Object[] struct = new Object[] {"1", "2", "3", "4", "5"};
        assertArrayEquals(struct, Signature.structArgs(struct));
    }

}