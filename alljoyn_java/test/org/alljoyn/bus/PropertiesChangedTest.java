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

import java.util.Map;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import junit.framework.TestCase;

import org.alljoyn.bus.SignalEmitter.GlobalBroadcast;

public class PropertiesChangedTest
    extends TestCase
{

    private static final String NAME_IFACE = "org.alljoyn.bus.ifaces.PropertiesTest";
    private static final String NAME_PATH = "/testpropsobject";
    private static final short PORT = 4567;
    private Semaphore mSem;
    private String changedPropName;
    private String changedPropValue;
    private String invalidatedPropName;

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

    private BusAttachment bus;

    private PropertiesTestService service;

    @Override
    public void setUp()
        throws Exception
    {
        mSem = new Semaphore(0);
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        service = new PropertiesTestService();
        status = bus.registerBusObject(service, NAME_PATH);
        assertEquals(Status.OK, status);
    }

    @Override
    public void tearDown()
        throws Exception
    {
        bus.unregisterBusObject(service);
        service = null;

        bus.disconnect();
        bus.release();
        bus = null;
    }

    public void testEmit()
        throws Exception
    {
        Status s;
        Mutable.ShortValue contactPort = new Mutable.ShortValue(PORT);

        s = bus.requestName(NAME_IFACE, (BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING |
                                         BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE));
        assertEquals(Status.OK, s);


        /* Get a remote object */
        ProxyBusObject remoteObj = bus.getProxyBusObject(NAME_IFACE, NAME_PATH, BusAttachment.SESSION_ID_ANY,
                                                         new Class<?>[] {InterfaceWithAnnotations.class});

        System.out.println("Using iface: " + InterfaceWithAnnotations.class.getName());
        Set<String> props = new HashSet<String>();
        props.add("Prop1");
        props.add("Prop2");
        String[] propsArray = props.toArray(new String[props.size()]);

        MyPropertyChangedListener listener = new MyPropertyChangedListener();
        s = remoteObj.registerPropertiesChangedListener(InterfaceWithAnnotations.class.getName(), propsArray, listener);
        assertEquals(Status.OK, s);

        PropertyChangedEmitter pce = new PropertyChangedEmitter(service, GlobalBroadcast.On);

        service.setProp1("Hello");
        service.setProp2("World");

        System.out.println("Sending property changed signal...");
        pce.PropertiesChanged(InterfaceWithAnnotations.class, props);

        // wait for listener to receive properties
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
                    System.out.println("Prop changed: " + changedPropName + ": " + changedPropValue);
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
