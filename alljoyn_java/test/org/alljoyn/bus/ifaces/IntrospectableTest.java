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

package org.alljoyn.bus.ifaces;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ifaces.Introspectable;

import junit.framework.TestCase;

public class IntrospectableTest extends TestCase {
    public IntrospectableTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment bus;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);
    }

    public void tearDown() throws Exception {
        bus.disconnect();
        bus = null;
    }

    public void testIntrospect() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject("org.freedesktop.DBus",
                                                         "/org/freedesktop/DBus",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { Introspectable.class });
        Introspectable introspectable = remoteObj.getInterface(Introspectable.class);
        String data = introspectable.Introspect();
        // This test returns the xml definition of the org.freedesktop.DBus interface
        // rather than check the xml output we are only checking that the returned
        // string is not empty and contains the tag <interface name="org.freedesktop.DBus.Introspectable">
        assertFalse(data.equals(""));
        assertTrue(data.contains("<interface name=\"org.freedesktop.DBus.Introspectable\">"));
    }
}