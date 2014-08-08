/**
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
