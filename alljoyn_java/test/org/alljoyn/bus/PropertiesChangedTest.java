/*
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

import org.alljoyn.bus.SignalEmitter.GlobalBroadcast;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Properties;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import junit.framework.TestCase;

public class PropertiesChangedTest extends TestCase {

    private static final String NAME_IFACE = "org.alljoyn.bus.ifaces.PropertiesTest";
    private static final String NAME_PATH = "/testpropsobject";

    public PropertiesChangedTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class PropertiesTestService implements InterfaceWithAnnotations, Properties, BusObject {

        private String prop1 = "N/A";
        private String prop2 = "N/A";

        @Override
        public Variant Get(String iface, String propName) throws BusException {
            // TODO: Only needed because Properties interface needs to be implemented.
            // But this function is not really needed.
            return null;
        }

        @Override
        public void Set(String iface, String propName, Variant value) throws BusException {
            // TODO: Only needed because Properties interface needs to be implemented.
            // But this function is not really needed.
        }

        @Override
        public Map<String, Variant> GetAll(String iface) throws BusException {
            // TODO: Only needed because Properties interface needs to be implemented.
            // But this function is not really needed.
            return null;
        }

        @Override
        public void PropertiesChanged(String iface, Map<String, Variant> changedProps, String[] invalidatedProps)
            throws BusException {
        }

        @Override
        public String ping(String inStr) throws BusException {
            return null; // not used in this test
        }

        @Override
        public String pong(String inStr) throws BusException {
            return null; // not used in this test
        }

        @Override
        public void pong2(String inStr) throws BusException {
            // not used in this test
        }

        @Override
        public void signal() throws BusException {
            // not used in this test
        }

        @Override
        public String getProp1() throws BusException {
            return prop1;
        }

        @Override
        public void setProp1(String s) throws BusException {
            prop1 = s;

        }

        @Override
        public String getProp2() throws BusException {
            return prop2;
        }

        @Override
        public void setProp2(String s) throws BusException {
            prop2 = s;
        }

    }

    private BusAttachment bus;

    private PropertiesTestService service;

    @Override
    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        service = new PropertiesTestService();
        status = bus.registerBusObject(service, NAME_PATH);
        assertEquals(Status.OK, status);

        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName(NAME_IFACE, DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res);
    }

    @Override
    public void tearDown() throws Exception {
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.ReleaseNameResult res = control.ReleaseName(NAME_IFACE);
        assertEquals(DBusProxyObj.ReleaseNameResult.Released, res);

        bus.unregisterBusObject(service);
        service = null;

        bus.disconnect();
        bus = null;
    }

    public void testEmit() throws BusException {

        /* Get a remote object */
        ProxyBusObject remoteObj =
            bus.getProxyBusObject(NAME_IFACE, NAME_PATH, BusAttachment.SESSION_ID_ANY,
                new Class<?>[] {InterfaceWithAnnotations.class});

        System.out.println("Using iface: " + InterfaceWithAnnotations.class.getName());
        Set<String> props = new HashSet<String>();
        props.add("Prop1");
        props.add("Prop2");

        // TODO: this crashes.
        // MyPropertyChangedListener listener = new MyPropertyChangedListener(bus, NAME_IFACE, NAME_PATH);
        // Status s =
        // remoteObj.registerPropertiesChangedHandler(InterfaceWithAnnotations.class.getName(),
        // props.toArray(new String[props.size()]), listener);
        // System.out.println("STATUS: " + s);

        PropertyChangedEmitter pce = new PropertyChangedEmitter(service, GlobalBroadcast.On);

        System.out.println("Sending property changed signal...");
        pce.PropertiesChanged(InterfaceWithAnnotations.class, props);
    }

    private class MyPropertyChangedListener extends PropertyChangedListener {
        public MyPropertyChangedListener(BusAttachment busAttachment, String busName, String objPath) {
            super(busAttachment, busName, objPath, 0);
        }

        @Override
        public void propertyChanged(ProxyBusObject pObj, String ifaceName, String propName, Variant propValue) {
            System.out.println("Prop changed: " + propName + " -- " + propValue.toString());
        }

    }
}
