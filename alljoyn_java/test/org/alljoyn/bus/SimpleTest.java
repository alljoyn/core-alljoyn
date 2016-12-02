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
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.Status;

import junit.framework.TestCase;

public class SimpleTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    public SimpleTest(String name) {
        super(name);
    }

    public class Service implements SimpleInterface, BusObject  {

        public String ping(String inStr) {
            return inStr;
        }
    }

    public void testPing() throws Exception {

        /* Create a bus connection and connect to the bus */
        BusAttachment bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        if (Status.OK != status) {
            throw new BusException(status.toString());
        }

        /* Register the service */
        Service service = new Service();
        status = bus.registerBusObject(service, "/testobject");
        if (Status.OK != status) {
            throw new BusException(status.toString());
        }

        /* Request a well-known name */
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.samples.simple",
                                                                DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        if (res != DBusProxyObj.RequestNameResult.PrimaryOwner) {
            throw new BusException("Failed to obtain well-known name");
        }

        /* Get a remote object */
        Class<?>[] ifaces = { SimpleInterface.class };
        ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.samples.simple",
                                                         "/testobject",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         ifaces);
        SimpleInterface proxy = remoteObj.getInterface(SimpleInterface.class);

        /* Call the ping method on the remote object */
        assertEquals("Hello World", proxy.ping("Hello World"));
    }
}