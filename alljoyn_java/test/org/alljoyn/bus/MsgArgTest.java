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

import org.alljoyn.bus.MsgArg;
import org.alljoyn.bus.Variant;

import java.util.AbstractMap;
import java.util.HashMap;
import java.lang.reflect.ParameterizedType;

import junit.framework.TestCase;

public class MsgArgTest extends TestCase {
    public MsgArgTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public void testToTypeFromSignature() throws BusException {
        assertEquals(Object[].class, MsgArg.toType("sbi"));   // struct
        assertEquals(Object[].class, MsgArg.toType("(sbi)")); // struct

        ParameterizedType type = (ParameterizedType) MsgArg.toType("a{sv}"); // dict-entry array (map)
        assertEquals(HashMap.class, type.getRawType());
        assertEquals(String.class, type.getActualTypeArguments()[0]);
        assertEquals(Variant.class, type.getActualTypeArguments()[1]);

        type = (ParameterizedType) MsgArg.toType("{sv}"); // dict entry
        assertEquals(AbstractMap.SimpleEntry.class, type.getRawType());
        assertEquals(String.class, type.getActualTypeArguments()[0]);
        assertEquals(Variant.class, type.getActualTypeArguments()[1]);

        assertEquals(Boolean.class, MsgArg.toType("b"));
        assertEquals(Byte.class, MsgArg.toType("y"));
        assertEquals(Short.class, MsgArg.toType("n"));      // int16
        assertEquals(Short.class, MsgArg.toType("q"));      // uint16
        assertEquals(Integer.class, MsgArg.toType("i"));    // int32
        assertEquals(Integer.class, MsgArg.toType("u"));    // uint32
        assertEquals(Long.class, MsgArg.toType("x"));       // int64
        assertEquals(Long.class, MsgArg.toType("t"));       // uint64
        assertEquals(Double.class, MsgArg.toType("d"));
        assertEquals(String.class, MsgArg.toType("s"));
        assertEquals(String.class, MsgArg.toType("g"));     // signature
        assertEquals(String.class, MsgArg.toType("o"));     // object path
        assertEquals(Variant.class, MsgArg.toType("v"));
        assertEquals(Long.class, MsgArg.toType("h"));       // handle

        assertEquals(Boolean[].class, MsgArg.toType("ab"));
        assertEquals(Byte[].class, MsgArg.toType("ay"));
        assertEquals(Short[].class, MsgArg.toType("an"));   // array int16
        assertEquals(Short[].class, MsgArg.toType("aq"));   // array uint16
        assertEquals(Integer[].class, MsgArg.toType("ai")); // array int32
        assertEquals(Integer[].class, MsgArg.toType("au")); // array uint32
        assertEquals(Long[].class, MsgArg.toType("ax"));    // array int64
        assertEquals(Long[].class, MsgArg.toType("at"));    // array uint64
        assertEquals(Double[].class, MsgArg.toType("ad"));

        // Be careful of the MsgArg.ALLJOYN_*** constants; they are int values (actually char stored in int constants)
        assertEquals(String.class, MsgArg.toType(""+(char)MsgArg.ALLJOYN_STRING));
        assertEquals(Boolean.class, MsgArg.toType(""+(char)MsgArg.ALLJOYN_BOOLEAN));
        assertEquals(Integer.class, MsgArg.toType(""+(char)MsgArg.ALLJOYN_INT32));

        try {
            MsgArg.toType("");
            fail("Expected BusException due to empty signature");
        } catch (BusException e) {
        }

        try {
            MsgArg.toType(null);
            fail("Expected BusException due to null signature");
        } catch (BusException e) {
        }

        try {
            MsgArg.toType("Z");
            fail("Expected BusException due to unimplemented signature");
        } catch (BusException e) {
        }

        try {
            MsgArg.toType("115");
            fail("Expected BusException due to unimplemented signature");
        } catch (BusException e) {
        }
    }

}