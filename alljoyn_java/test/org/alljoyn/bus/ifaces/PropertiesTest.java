/*
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

package org.alljoyn.bus.ifaces;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Properties;

import java.util.Map;
import junit.framework.TestCase;

public class PropertiesTest extends TestCase {
    public PropertiesTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class PropertiesTestService implements PropertiesTestServiceInterface,
                                                  BusObject {

        private String string;

        public PropertiesTestService() { this.string = "get"; }

        public String getStringProp() { return this.string; }

        public void setStringProp(String string) { this.string = string; }
    }

    private BusAttachment bus;

    private PropertiesTestService service;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        service = new PropertiesTestService();
        status = bus.registerBusObject(service, "/testobject");
        assertEquals(Status.OK, status);

        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.ifaces.PropertiesTest",
                                                                DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res);
    }

    public void tearDown() throws Exception {
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.ReleaseNameResult res = control.ReleaseName("org.alljoyn.bus.ifaces.PropertiesTest");
        assertEquals(DBusProxyObj.ReleaseNameResult.Released, res);

        bus.unregisterBusObject(service);
        service = null;

        bus.disconnect();
        bus = null;
    }

    public void testGet() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.ifaces.PropertiesTest",
                                                         "/testobject",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { Properties.class });
        Properties properties = remoteObj.getInterface(Properties.class);
        Variant stringProp = properties.Get("org.alljoyn.bus.ifaces.PropertiesTestServiceInterface",
                                            "StringProp");
        assertEquals("get", stringProp.getObject(String.class));
    }

    public void testSet() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.ifaces.PropertiesTest",
                                                         "/testobject",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { Properties.class });
        Properties properties = remoteObj.getInterface(Properties.class);
        properties.Set("org.alljoyn.bus.ifaces.PropertiesTestServiceInterface",
                       "StringProp", new Variant("set"));
        Variant stringProp = properties.Get("org.alljoyn.bus.ifaces.PropertiesTestServiceInterface",
                                            "StringProp");
        assertEquals("set", stringProp.getObject(String.class));
    }

    public void testGetAll() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.ifaces.PropertiesTest",
                                                         "/testobject",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { Properties.class });
        Properties properties = remoteObj.getInterface(Properties.class);
        Map<String, Variant> map = properties.GetAll("org.alljoyn.bus.ifaces.PropertiesTestServiceInterface");
        assertEquals("get", map.get("StringProp").getObject(String.class));
    }

    public void testGetAllOnUnknownInterface() throws Exception {
        boolean thrown = false;
        try {
            ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.ifaces.PropertiesTest",
                                                             "/testobject", BusAttachment.SESSION_ID_ANY,
                                                             new Class<?>[] { Properties.class });
            Properties properties = remoteObj.getInterface(Properties.class);
            Map<String, Variant> map = properties.GetAll("unknownInterface");
            assertNotNull(map);
        } catch (BusException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }
}
