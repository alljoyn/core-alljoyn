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
