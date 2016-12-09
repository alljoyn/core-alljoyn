/**
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 */

package org.alljoyn.bus;

import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import junit.framework.TestCase;

import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.ifaces.DBusProxyObj;

import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.MethodDef;
import org.alljoyn.bus.defs.PropertyDef;
import org.alljoyn.bus.defs.PropertyDef.ChangedSignalPolicy;
import org.alljoyn.bus.defs.ArgDef;

import java.util.Arrays;

public class ProxyBusObjectTest extends TestCase {
    public ProxyBusObjectTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    private int uniquifier = 0;
    private String genUniqueName(BusAttachment bus) {
        return "test.x" + bus.getGlobalGUIDString() + ".x" + uniquifier++;
    }

    private String name;
    private BusAttachment otherBus;
    private Service service;
    private BusAttachment bus;
    private ProxyBusObject proxyObj;

    public void setUp() throws Exception {
        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        name = genUniqueName(otherBus);
        service = new Service();
        assertEquals(Status.OK, otherBus.registerBusObject(service, "/simple"));
        assertEquals(Status.OK, otherBus.connect());

        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, bus.connect());
        while (bus.getDBusProxyObj().NameHasOwner(name)) {
            Thread.sleep(100);
        }

        assertEquals(Status.OK, otherBus.requestName(name, DBusProxyObj.REQUEST_NAME_NO_FLAGS));
    }

    public void tearDown() throws Exception {
        bus.disconnect();

        otherBus.releaseName(name);
        otherBus.unregisterBusObject(service);
        otherBus.disconnect();

        bus.release();
        otherBus.release();
    }

    public class DelayReply implements SimpleInterface,
                                       BusObject {
        public String ping(String str) {
            boolean thrown = false;
            try {
                Thread.sleep(100);
            } catch (InterruptedException ex) {
                // we don't expect the sleep to be interrupted. If it is print
                // the stacktrace to aid with debugging.
                ex.printStackTrace();
                thrown = true;
            }
            assertFalse(thrown);
            return str;
        }
    }

    public void testReplyTimeout() throws Exception {
        DelayReply service = new DelayReply();
        assertEquals(Status.OK, otherBus.registerBusObject(service, "/delayreply"));

        proxyObj = bus.getProxyBusObject(name, "/delayreply", BusAttachment.SESSION_ID_ANY, new Class<?>[] { SimpleInterface.class });
        proxyObj.setReplyTimeout(10);

        boolean thrown = false;
        try {
            SimpleInterface simple = proxyObj.getInterface(SimpleInterface.class);
            simple.ping("testReplyTimeout");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
    }

    public class Service implements SimpleInterface, BusObject {
        public String ping(String inStr) { return inStr; }
    }

    public void testCreateRelease() throws Exception {
        proxyObj = bus.getProxyBusObject(name, "/simple", BusAttachment.SESSION_ID_ANY, new Class<?>[] { SimpleInterface.class });
        proxyObj.release();
    }


    public void testMethodCall() throws Exception {
        assertEquals(Status.OK, otherBus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        proxyObj = bus.getProxyBusObject(name, "/simple", BusAttachment.SESSION_ID_ANY, new Class<?>[] { SimpleInterface.class });
        SimpleInterface proxy = proxyObj.getInterface(SimpleInterface.class);
        for (int i = 0; i < 10; ++i) {
            assertEquals("ping", proxy.ping("ping"));
        }

        proxyObj.release();

        otherBus.cancelAdvertiseName(name, SessionOpts.TRANSPORT_ANY);
    }

    public void testMultipleProxyBusObjects() throws Exception {
        // Connect two proxy objects
        proxyObj = bus.getProxyBusObject(name, "/simple", BusAttachment.SESSION_ID_ANY, new Class<?>[] { SimpleInterface.class });
        ProxyBusObject proxyObj2 = bus.getProxyBusObject(name, "/simple", BusAttachment.SESSION_PORT_ANY, new Class<?>[] { SimpleInterface.class });

        // Verify they're both operating
        assertEquals("ping", proxyObj.getInterface(SimpleInterface.class).ping("ping"));
        assertEquals("ping2", proxyObj2.getInterface(SimpleInterface.class).ping("ping2"));

        // release one of them
        proxyObj2.release();

        // Verify the other is still working
        assertEquals("ping", proxyObj.getInterface(SimpleInterface.class).ping("ping"));

        // Disconnect other one
        proxyObj.release();
    }

    public class Emitter implements EmitterInterface,
                                    BusObject {
        public void emit(String str) { /* Do nothing, this is a signal. */ }
    }

    /* Call a @BusSignal on a ProxyBusObject interface. */
    public void testSignalFromInterface() throws Exception {
        Emitter service = new Emitter();
        assertEquals(Status.OK, otherBus.registerBusObject(service, "/emitter"));

        boolean thrown = false;
        try {
            proxyObj = bus.getProxyBusObject(name, "/emitter", BusAttachment.SESSION_ID_ANY, new Class<?>[] { EmitterInterface.class });
            proxyObj.getInterface(EmitterInterface.class).emit("emit");
        } catch (BusException ex) {
            thrown = true;
        }
        assertTrue(thrown);

        proxyObj.release();
    }

    private boolean methods;
    private boolean methodi;

    public class MultiMethod implements MultiMethodInterfaceA, MultiMethodInterfaceB,
                                        BusObject {
        public void method(String str) { methods = true; };
        public void method(int i) { methodi = true; };
    };

    public void testMultiMethod() throws Exception {
        MultiMethod service = new MultiMethod();
        assertEquals(Status.OK, otherBus.registerBusObject(service, "/multimethod"));

        proxyObj = bus.getProxyBusObject(name, "/multimethod", BusAttachment.SESSION_ID_ANY,
                                         new Class<?>[] { MultiMethodInterfaceA.class, MultiMethodInterfaceB.class });

        try {
            methods = methodi = false;
            MultiMethodInterfaceA ifacA = proxyObj.getInterface(MultiMethodInterfaceA.class);
            ifacA.method("str");
            assertEquals(true, methods);
            assertEquals(false, methodi);

            methods = methodi = false;
            proxyObj.getInterface(MultiMethodInterfaceB.class).method(10);
            assertEquals(false, methods);
            assertEquals(true, methodi);
        } catch(BusException ex) {
            /*
             * This catch statement should not be run if it is run print out a
             * stack trace and fail the unit test.
             */
            ex.printStackTrace();
            System.out.println(ex.getMessage());
            assertTrue(false);
        }
        proxyObj.release();
    }

    public void testGetBusName() throws Exception {
        proxyObj = bus.getProxyBusObject(name, "/simple", BusAttachment.SESSION_ID_ANY, new Class<?>[] { SimpleInterface.class });
        assertEquals(name, proxyObj.getBusName());
        proxyObj.release();
    }

    public void testGetObjPath() throws Exception {
        proxyObj = bus.getProxyBusObject(name, "/simple", BusAttachment.SESSION_ID_ANY, new Class<?>[] { SimpleInterface.class });
        assertEquals("/simple", proxyObj.getObjPath());
        proxyObj.release();
    }

    public void testProxiedMethodsFromClassObject() throws Exception {
        proxyObj = bus.getProxyBusObject(name, "/simple",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterface.class });
        Object intfObject = proxyObj.getInterface(SimpleInterface.class);
        assertTrue(intfObject.equals(intfObject));
        assertFalse(intfObject.equals(null));
        assertFalse(intfObject.equals(this));

        assertNotNull(intfObject.toString());

        assertEquals(System.identityHashCode(intfObject), intfObject.hashCode());

        proxyObj.release();
    }

    public void testDynamicMethodCall() throws Exception {
        assertEquals(Status.OK, otherBus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        // Setup dynamic interface description and proxy

        String ifaceName = "org.alljoyn.bus.SimpleInterface";
        InterfaceDef interfaceDef = new InterfaceDef(ifaceName);
        MethodDef methodDef = new MethodDef("Ping", "s", "s", ifaceName);
        methodDef.addArg( new ArgDef("inStr", "s") );
        methodDef.addArg( new ArgDef("result", "s", ArgDef.DIRECTION_OUT) );
        interfaceDef.addMethod(methodDef);

        proxyObj = bus.getProxyBusObject(name, "/simple", BusAttachment.SESSION_ID_ANY,
                Arrays.asList(new InterfaceDef[]{interfaceDef}), false);
        GenericInterface proxy = proxyObj.getInterface(GenericInterface.class);

        // Test proxy calling a bus method via GenericInterface.methodCall()

        assertEquals("hello", proxy.methodCall(ifaceName, "Ping", "hello"));

        // Test other proxy methods available (toString, hashCode, equals)

        assertTrue(proxy.toString().contains(ifaceName));
        assertNotNull(proxy.hashCode());
        assertTrue(proxy.equals(proxy));

        // Test proxy calling a non-existant bus method

        try {
            proxy.methodCall(ifaceName, "Bogus", "hello");
            fail("Expected BusException('No such method')");
        } catch (BusException e) {
            assertTrue(e.getMessage().startsWith("No such method"));
        }

        // Test proxy not suppose to call GenericInterface.signal()

        try {
            proxy.signal(ifaceName, "Ping", "hello");
            fail("Expected IllegalArgumentException('Invalid proxy method')");
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().startsWith("Invalid proxy method"));
        }

        proxyObj.release();
        otherBus.cancelAdvertiseName(name, SessionOpts.TRANSPORT_ANY);
    }

    public void testDynamicPropertyCall() throws Exception {
        String ifaceName = "org.alljoyn.bus.BasicProperty";
        final String path = "/myProperties";

        // Register service object on the other bus

        PropertyObject serviceObj = new PropertyObject();
        assertEquals(Status.OK, otherBus.registerBusObject(serviceObj, path));

        // Setup dynamic interface description and get the proxy object

        InterfaceDef interfaceDef = new InterfaceDef(ifaceName);
        PropertyDef propertyDef = new PropertyDef("Name", "s", PropertyDef.ACCESS_READWRITE, ifaceName);
        propertyDef.addAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL, ChangedSignalPolicy.TRUE.text);
        interfaceDef.addProperty(propertyDef);

        proxyObj = bus.getProxyBusObject(name, path, BusAttachment.SESSION_ID_ANY,
                Arrays.asList(new InterfaceDef[]{interfaceDef}), false);
        GenericInterface proxy = proxyObj.getInterface(GenericInterface.class);

        // Test dynamic proxy calls via GenericInterface's getProperty() and setProperty()

        assertEquals("MyName", serviceObj.name); // serviceObj initially has name=MyName
        assertEquals("MyName", proxy.getProperty(ifaceName, "Name"));

        proxy.setProperty(ifaceName, "Name", "Call me Ishmael");
        assertEquals("Call me Ishmael", serviceObj.name);
        assertEquals("Call me Ishmael", proxy.getProperty(ifaceName, "Name"));

        // Test attempt to get/set non-existant bus property

        try {
            proxy.getProperty(ifaceName, "Bogus");
            fail("Expected BusException('No such property')");
        } catch (BusException e) {
            assertTrue(e.getMessage().startsWith("No such property"));
        }
        try {
            proxy.setProperty(ifaceName, "Bogus", "your name here");
            fail("Expected BusException('No such property')");
        } catch (BusException e) {
            assertTrue(e.getMessage().startsWith("No such property"));
        }

        proxyObj.release();
        otherBus.cancelAdvertiseName(name, SessionOpts.TRANSPORT_ANY);
    }

    class PropertyObject implements BasicProperty, BusObject {
        private String name = "MyName";
        final AtomicInteger callCount = new AtomicInteger(0);
        @Override
        public String getName() throws BusException {
            callCount.incrementAndGet();
            return name;
        }
        @Override
        public void setName(String value) throws BusException {
            name = value;
        }
    }

    public void testPropertyCache() throws Exception {
        final String ifaceName = "org.alljoyn.bus.BasicProperty";
        final String propertyServicePath = "/myProperties";
        PropertyObject ps = new PropertyObject();

        assertEquals(Status.OK, otherBus.registerBusObject(ps, propertyServicePath));
        proxyObj = bus.getProxyBusObject(name, propertyServicePath,
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { BasicProperty.class });
        assertNotNull(proxyObj);
        final BasicProperty pp = proxyObj.getInterface(BasicProperty.class);
        assertNotNull(pp);
        assertEquals(ps.name, pp.getName());
        assertEquals(1, ps.callCount.get());
        proxyObj.enablePropertyCaching();
        /* wait until the property cache is enabled. We have no reliable way to check this,
         * so we'll just wait for a sufficiently long time period. */
        Thread.sleep(500);
        assertEquals(ps.name, pp.getName());
        assertEquals(2, ps.callCount.get());
        assertEquals(ps.name, pp.getName());
        assertEquals(2, ps.callCount.get());
        proxyObj.enablePropertyCaching();
        proxyObj.enablePropertyCaching();
        proxyObj.enablePropertyCaching();
        assertEquals(ps.name, pp.getName());
        assertEquals(2, ps.callCount.get());

        String oldName = ps.name;
        ps.name = "newName";
        assertEquals(oldName, pp.getName());

        /* updates to the property should be reflected in the cache.
         * The update must be available during the property change callback. */
        synchronized (this) {
            final ArrayList<String> values = new ArrayList<String>();
            proxyObj.registerPropertiesChangedListener(ifaceName, new String[]{"Name"}, new PropertiesChangedListener() {
                @Override
                public void propertiesChanged(ProxyBusObject pObj, String ifaceName,
                        Map<String, Variant> changed, String[] invalidated) {
                    try {
                        values.add(pp.getName());
                    } catch (BusException e) {
                    }
                    synchronized (ProxyBusObjectTest.this) {
                        ProxyBusObjectTest.this.notifyAll();
                    }
                }
            });
            PropertyChangedEmitter pce = new PropertyChangedEmitter(ps);
            pce.PropertyChanged(ifaceName, "Name", new Variant(ps.name));
            wait(1500);
            assertEquals(1, values.size());
            assertEquals(ps.name, values.get(0));
            assertEquals(ps.name, pp.getName());
            assertEquals(2, ps.callCount.get());
        }
        otherBus.unregisterBusObject(ps);
        /* The property cache should still provide the last known value
         * even after the remote objects is unregistered. */
        assertEquals(ps.name, pp.getName());
        assertEquals(ps.name, pp.getName());
        assertEquals(ps.name, pp.getName());
        proxyObj.release();
    }

}