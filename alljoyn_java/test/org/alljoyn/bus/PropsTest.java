/**
 * Copyright (c) 2009-2011, 2013-2014, AllSeen Alliance. All rights reserved.
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

import java.util.Map;

import junit.framework.TestCase;

import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Properties;

public class PropsTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    public PropsTest(String name) {
        super(name);
    }

    public class Service implements PropsInterface, BusObject {

        private String stringProperty = "Hello";

        private int intProperty = 6;

        public String getStringProp() { return stringProperty; }

        public void setStringProp(String stringProperty) { this.stringProperty = stringProperty; }

        public int getIntProp() { return intProperty; }

        public void setIntProp(int intProperty) { this.intProperty = intProperty; }

        public String ping(String str) throws BusException {
            return str;
        }
    }

    BusAttachment bus;
    BusAttachment clientBus;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());

        /* Register the service */
        Service service = new Service();
        Status status = bus.registerBusObject(service, "/testProperties");
        if (Status.OK != status) {
            throw new BusException("BusAttachment.registerBusObject() failed: " + status.toString());
        }
    }

    public void tearDown() throws Exception {
        if (bus != null) {
            bus.disconnect();
            bus.release();
            bus = null;
        }
    }

    public void testProps() throws Exception {
        /* Request a well-known name */
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.samples.props",
                                                                DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        if (res != DBusProxyObj.RequestNameResult.PrimaryOwner) {
            throw new BusException("Failed to obtain well-known name");
        }

        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject("org.alljoyn.bus.samples.props",
                                                         "/testProperties",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class,
                                                                          Properties.class });
        PropsInterface proxy = remoteObj.getInterface(PropsInterface.class);

        /* Get a property */
        assertEquals("Hello", proxy.getStringProp());

        /* Set a property */
        proxy.setStringProp("MyNewValue");

        /* Get all of the properties of the interface */
        assertEquals("MyNewValue", proxy.getStringProp());
        assertEquals(6, proxy.getIntProp());

        /* Use the org.freedesktop.DBus.Properties interface to get all the properties */
        Properties properties = remoteObj.getInterface(Properties.class);
        Map<String, Variant> map = properties.GetAll("org.alljoyn.bus.PropsInterface");
        assertEquals("MyNewValue", map.get("StringProp").getObject(String.class));
        assertEquals(6, (int)map.get("IntProp").getObject(Integer.class));
   }

    public void testGetProperty() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testProperties",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class });
        Variant stringProp = remoteObj.getProperty(PropsInterface.class, "StringProp");
        assertEquals("Hello", stringProp.getObject(String.class));
    }

    public void testSetProperty() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testProperties",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class });
        remoteObj.setProperty(PropsInterface.class, "StringProp", new Variant("set"));
        Variant stringProp = remoteObj.getProperty(PropsInterface.class, "StringProp");
        assertEquals("set", stringProp.getObject(String.class));
    }

    public void testGetAllProperties() throws Exception {
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testProperties",  BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class });
        Map<String, Variant> map = remoteObj.getAllProperties(PropsInterface.class);
        assertEquals("Hello", map.get("StringProp").getObject(String.class));
        assertEquals(6, (int)map.get("IntProp").getObject(Integer.class));
    }

    /* ALLJOYN-2043 */
    public void testGetAllThenMethodCall() throws Exception {
        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject(bus.getUniqueName(),
                                                         "/testProperties",
                                                         BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] { PropsInterface.class,
                                                                          Properties.class });

        /* Use the org.freedesktop.DBus.Properties interface to get all the properties */
        Properties properties = remoteObj.getInterface(Properties.class);
        Map<String, Variant> map = properties.GetAll("org.alljoyn.bus.PropsInterface");
        assertEquals("Hello", map.get("StringProp").getObject(String.class));
        assertEquals(6, (int)map.get("IntProp").getObject(Integer.class));

        PropsInterface proxy = remoteObj.getInterface(PropsInterface.class);
        assertEquals("World", proxy.ping("World"));
   }
}