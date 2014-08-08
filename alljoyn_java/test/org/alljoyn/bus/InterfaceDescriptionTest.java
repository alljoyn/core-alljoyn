/**
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ifaces.DBusProxyObj;

import junit.framework.TestCase;

public class InterfaceDescriptionTest extends TestCase {
    public InterfaceDescriptionTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class Service implements SimpleInterface,
                                    BusObject {
        public String ping(String inStr) throws BusException { return inStr; }
    }

    public class ServiceC implements SimpleInterfaceC,
                                     BusObject {
        public String ping(String inStr) throws BusException { return inStr; }
        public String pong(String inStr) throws BusException { return inStr; }
    }

    private BusAttachment bus;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());

        Status status = bus.connect();
        assertEquals(Status.OK, status);

        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.InterfaceDescriptionTest",
                                                                 DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res);
    }

    public void tearDown() throws Exception {
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.ReleaseNameResult res = control.ReleaseName("org.alljoyn.bus.InterfaceDescriptionTest");
        assertEquals(DBusProxyObj.ReleaseNameResult.Released, res);

        bus.disconnect();
        bus = null;
    }

    public void testDifferentSignature() throws Exception {
        Service service = new Service();
        Status status = bus.registerBusObject(service, "/service");
        assertEquals(Status.OK, status);

        boolean thrown = false;
        try {
            SimpleInterfaceB proxy = bus.getProxyBusObject("org.alljoyn.bus.InterfaceDescriptionTest",
                "/service",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterfaceB.class }).getInterface(SimpleInterfaceB.class);
            proxy.ping(1);
        } catch (BusException ex) {
            thrown = true;
        }
        assertTrue(thrown);

        bus.unregisterBusObject(service);
    }

    public void testProxyInterfaceSubset() throws Exception {
        ServiceC service = new ServiceC();
        Status status = bus.registerBusObject(service, "/service");
        assertEquals(Status.OK, status);

        boolean thrown = false;
        try {
            SimpleInterface proxy = bus.getProxyBusObject("org.alljoyn.bus.InterfaceDescriptionTest", "/service",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterface.class }).getInterface(SimpleInterface.class);
            proxy.ping("str");
        } catch (BusException ex) {
            thrown = true;
        }
        assertTrue(thrown);

        bus.unregisterBusObject(service);
    }

    public void testProxyInterfaceSuperset() throws Exception {
        Service service = new Service();
        Status status = bus.registerBusObject(service, "/service");
        assertEquals(Status.OK, status);

        boolean thrown = false;
        try {
            SimpleInterfaceC proxy = bus.getProxyBusObject("org.alljoyn.bus.InterfaceDescriptionTest",
                "/service",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterfaceC.class }).getInterface(SimpleInterfaceC.class);
            proxy.ping("str");
        } catch (BusException ex) {
            thrown = true;
        }
        assertTrue(thrown);

        bus.unregisterBusObject(service);
    }
}
