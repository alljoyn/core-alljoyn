/*
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*/

package org.alljoyn.bus;

import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import junit.framework.TestCase;

import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.PropertyDef;
import org.alljoyn.bus.defs.PropertyDef.ChangedSignalPolicy;

import org.alljoyn.bus.SignalEmitter.GlobalBroadcast;

public class PropertiesChangedTest
    extends TestCase
{
    private static final String NAME_BUS = "org.alljoyn.bus.ifaces.PropertiesTest"; // well-known name for the bus attachment
    private static final String NAME_IFACE = InterfaceWithAnnotations.class.getName();
    private static final String NAME_PATH = "/testpropsobject";
    private Semaphore mSem;
    private volatile String changedPropName;
    private volatile String changedPropValue;
    private volatile String invalidatedPropName;

    public PropertiesChangedTest(String name)
    {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class PropertiesTestService
        implements InterfaceWithAnnotations, BusObject
    {
        private String prop1 = "N/A";
        private String prop2 = "N/A";

        @Override
        public String ping(String inStr)
            throws BusException
        {
            return null; // not used in this test
        }

        @Override
        public String pong(String inStr)
            throws BusException
        {
            return null; // not used in this test
        }

        @Override
        public void pong2(String inStr)
            throws BusException
        {
            // not used in this test
        }

        @Override
        public void signal()
            throws BusException
        {
            // not used in this test
        }

        @Override
        public String getProp1()
            throws BusException
        {
            return prop1;
        }

        @Override
        public void setProp1(String s)
            throws BusException
        {
            prop1 = s;

        }

        @Override
        public String getProp2()
            throws BusException
        {
            return prop2;
        }

        @Override
        public void setProp2(String s)
            throws BusException
        {
            prop2 = s;
        }
    }

    public class PropertiesTestDynamicService extends AbstractDynamicBusObject {
        public Map<String,Object> propertyMap = new HashMap<String,Object>();

        public PropertiesTestDynamicService(BusAttachment bus, String path, List<InterfaceDef> interfaceDefs) {
            super(bus, path, interfaceDefs);
        }

        public void setProperty(String ifaceName, String propName, Object value) {
            String key = ifaceName + ":" + propName;
            propertyMap.put(key, value);
        }

        /** Generic Method handler used when bus object receives a remote "Set" property call */
        @Override
        public void setPropertyReceived(Object arg) throws BusException {
            MessageContext ctx = getBus().getMessageContext();
            setProperty(ctx.interfaceName, ctx.memberName, arg);
        }

        /** Generic Method handler used when bus object receives a remote "Get" property call */
        @Override
        public Object getPropertyReceived() throws BusException {
            MessageContext ctx = getBus().getMessageContext();
            String key = ctx.interfaceName + ":" + ctx.memberName;
            return propertyMap.get(key);
        }
    }


    private BusAttachment bus;
    private BusObject service;

    @Override
    public void setUp()
        throws Exception
    {
        mSem = new Semaphore(0);
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        changedPropName = "";
        changedPropValue = "";
        invalidatedPropName = "";
    }

    @Override
    public void tearDown()
        throws Exception
    {
        if (service != null) {
            bus.unregisterBusObject(service);
            service = null;
        }

        bus.disconnect();
        bus.release();
        bus = null;
    }

    public void testEmit()
        throws Exception
    {
        Status s;

        service = new PropertiesTestService();
        s = bus.registerBusObject(service, NAME_PATH);
        assertEquals(Status.OK, s);

        s = bus.requestName(NAME_BUS, (BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING |
                                         BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE));
        assertEquals(Status.OK, s);

        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject(NAME_BUS, NAME_PATH, BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] {InterfaceWithAnnotations.class});

        System.out.println("Using iface: " + NAME_IFACE);
        Set<String> props = new HashSet<String>();
        props.add("Prop1");
        props.add("Prop2");
        String[] propsArray = props.toArray(new String[props.size()]);

        MyPropertyChangedListener listener = new MyPropertyChangedListener();
        s = remoteObj.registerPropertiesChangedListener(NAME_IFACE, propsArray, listener);
        assertEquals(Status.OK, s);

        PropertyChangedEmitter pce = new PropertyChangedEmitter(service, GlobalBroadcast.On);

        ((PropertiesTestService)service).setProp1("Hello");
        ((PropertiesTestService)service).setProp2("World");

        System.out.println("Sending properties changed signal...");
        pce.PropertiesChanged(InterfaceWithAnnotations.class, props);

        // wait for listener to receive properties
        waitFor(mSem);

        assertEquals("Prop1", changedPropName);
        assertEquals("Hello", changedPropValue);
        assertEquals("Prop2", invalidatedPropName);
    }

    public void testEmitDynamicPropertiesChanged() throws Exception {
        List<InterfaceDef> interfaceDefs = buildInterfaceDef(NAME_IFACE);

        // Setup a dynamic bus object (service-side)
        service = new PropertiesTestDynamicService(bus, NAME_PATH, interfaceDefs);
        Status s = bus.registerBusObject(service, NAME_PATH);
        assertEquals(Status.OK, s);

        s = bus.requestName(NAME_BUS, (BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING |
                                         BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE));
        assertEquals(Status.OK, s);

        // Setup a remote object (client-side)
        ProxyBusObject remoteObj = bus.getProxyBusObject(NAME_BUS, NAME_PATH, BusAttachment.SESSION_ID_ANY, interfaceDefs, false);

        System.out.println("Using iface: " + NAME_IFACE);
        String[] propsArray = new String[] {"Prop1", "Prop2"};

        s = remoteObj.registerPropertiesChangedListener(NAME_IFACE, propsArray, new MyPropertyChangedListener());
        assertEquals(Status.OK, s);

        // Update bus object's property values and then notify registerd clients of change (service-side)
        ((PropertiesTestDynamicService)service).setProperty(NAME_IFACE, "Prop1", "Hello");
        ((PropertiesTestDynamicService)service).setProperty(NAME_IFACE, "Prop2", "World");

        System.out.println("Sending properties changed signal...");
        PropertyChangedEmitter pce = new PropertyChangedEmitter(service, GlobalBroadcast.On);
        pce.PropertiesChanged(interfaceDefs.get(0), new HashSet<String>(Arrays.asList(propsArray)));

        // Wait for listener to receive properties (client-side)
        waitFor(mSem);

        assertEquals("Prop1", changedPropName);
        assertEquals("Hello", changedPropValue);
        assertEquals("Prop2", invalidatedPropName);
    }

    public void testEmitDynamicPropertyChanged() throws Exception {
        List<InterfaceDef> interfaceDefs = buildInterfaceDef(NAME_IFACE);

        // Setup a dynamic bus object (service-side)
        service = new PropertiesTestDynamicService(bus, NAME_PATH, interfaceDefs);
        Status s = bus.registerBusObject(service, NAME_PATH);
        assertEquals(Status.OK, s);

        s = bus.requestName(NAME_BUS, (BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING |
                BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE));
        assertEquals(Status.OK, s);

        // Setup a remote object (client-side)
        ProxyBusObject remoteObj = bus.getProxyBusObject(NAME_BUS, NAME_PATH, BusAttachment.SESSION_ID_ANY, interfaceDefs, false);

        System.out.println("Using iface: " + NAME_IFACE);
        String[] propsArray = new String[] {"Prop1", "Prop2"};

        s = remoteObj.registerPropertiesChangedListener(NAME_IFACE, propsArray, new MyPropertyChangedListener());
        assertEquals(Status.OK, s);

        // Update bus object's property values and then notify registerd clients of change (service-side)
        ((PropertiesTestDynamicService)service).setProperty(NAME_IFACE, "Prop1", "Hello");
        ((PropertiesTestDynamicService)service).setProperty(NAME_IFACE, "Prop2", "World");

        System.out.println("Sending two property changed signals...");
        PropertyChangedEmitter pce = new PropertyChangedEmitter(service, GlobalBroadcast.On);
        pce.PropertyChanged(NAME_IFACE, "Prop1", new Variant("Hello")); // notify changed property
        pce.PropertyChanged(NAME_IFACE, "Prop2", null);                 // notify invalidated property

        // Wait for listener to receive both property changed signals (client-side)
        waitFor(mSem);
        waitFor(mSem);

        assertEquals("Prop1", changedPropName);
        assertEquals("Hello", changedPropValue);
        assertEquals("Prop2", invalidatedPropName);
    }

    private void waitFor(Semaphore sem)
        throws InterruptedException
    {
        if (!sem.tryAcquire(10, TimeUnit.SECONDS)) {
            fail("Timeout waiting for properties changed signal.");
        }
    }

    /* Return a dynamic interface definition containing property definitions: prop1, prop2 */
    private List<InterfaceDef> buildInterfaceDef(String interfaceName) {
        InterfaceDef interfaceDef = new InterfaceDef(interfaceName);

        PropertyDef propertyDef = new PropertyDef("Prop1", "s", PropertyDef.ACCESS_READWRITE, interfaceName);
        propertyDef.addAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL, ChangedSignalPolicy.TRUE.text);
        interfaceDef.addProperty(propertyDef);

        propertyDef = new PropertyDef("Prop2", "s", PropertyDef.ACCESS_READWRITE, interfaceName);
        propertyDef.addAnnotation(PropertyDef.ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL, ChangedSignalPolicy.INVALIDATES.text);
        interfaceDef.addProperty(propertyDef);

        return Arrays.asList(interfaceDef);
    }

    private class MyPropertyChangedListener
        extends PropertiesChangedListener
    {
        @Override
        public void propertiesChanged(ProxyBusObject pObj, String ifaceName, Map<String, Variant> changed, String[] invalidated)
        {
            try {
                for (Map.Entry<String, Variant> entry : changed.entrySet()) {
                    changedPropName = entry.getKey();
                    changedPropValue = entry.getValue().getObject(String.class);
                    System.out.println("Prop changed: " + changedPropName + " = " + changedPropValue);
                }
                for (String propName : invalidated) {
                    invalidatedPropName = propName;
                    System.out.println("Prop invalidated: " + invalidatedPropName);
                }
            } catch (BusException ex) {
                fail("Exception: " + ex);
            } finally {
                mSem.release();
            }
        }
    }

}
