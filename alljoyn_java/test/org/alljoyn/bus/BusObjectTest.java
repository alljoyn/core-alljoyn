/**
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

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Status;

import junit.framework.TestCase;

public class BusObjectTest extends TestCase {
    public BusObjectTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment bus;

    public void setUp() throws Exception {
        bus = null;
    }

    public void tearDown() throws Exception {
        if (bus != null) {
            bus.disconnect();
            bus = null;
        }
    }

    public class Service implements BusObject,
                                    BusObjectListener {

        public boolean registered;

        public Service() {
            registered = false;
        }

        public void registered() { registered = true; }

        public void unregistered() { registered = false; }
    }

    public void testObjectRegistered() throws Exception {
        bus = new BusAttachment(getClass().getName());

        Service service = new Service();
        Status status = bus.registerBusObject(service, "/service");
        assertEquals(Status.OK, status);
        Thread.sleep(100);
        assertFalse(service.registered);

        status = bus.connect();
        assertEquals(Status.OK, status);
        Thread.sleep(100);
        assertTrue(service.registered);

        bus.unregisterBusObject(service);
        Thread.sleep(100);
        assertFalse(service.registered);
    }
}