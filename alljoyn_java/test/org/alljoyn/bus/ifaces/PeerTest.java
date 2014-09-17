/*
 * Copyright (c) 2009-2011,2014 AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.ifaces;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Peer;

import junit.framework.TestCase;

public class PeerTest extends TestCase {
    public PeerTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class Service implements BusObject {
    }

    private BusAttachment bus;

    private Service service;

    private Peer peer;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        service = new Service();
        status = bus.registerBusObject(service, "/testobject");
        assertEquals(Status.OK, status);

        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.ifaces.PeerTest",
                                                                DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res);

        ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.ifaces.PeerTest",
                                                         "/testobject", BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { Peer.class });
        peer = remoteObj.getInterface(Peer.class);
    }

    public void tearDown() throws Exception {
        peer = null;

        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.ReleaseNameResult res = control.ReleaseName("org.alljoyn.bus.ifaces.PeerTest");
        assertEquals(DBusProxyObj.ReleaseNameResult.Released, res);

        bus.unregisterBusObject(service);
        service = null;

        bus.disconnect();
        bus = null;
    }

    public void testPing() throws Exception {
        peer.Ping();
    }

    public void testGetMachineId() throws Exception {
        String id = peer.GetMachineId();
        // the id is a guid of type c2634fd9823215edf03157f2acf07611 and is not
        //known till runtime. for that.
        // reason we are only checking it is not an empty string.
        assertFalse(id.equals(""));
        assertTrue(id.length() > 4);
    }
}
