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
import java.util.Arrays;

import junit.framework.TestCase;

public class DescriptionTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    public DescriptionTest(String name) {
        super(name);
    }

    private SimpleDescriptionInterfaceTranslator sdit;

    /* The AllJoyn object that is our service. */
    private SimpleDescriptionService mSimpleDescriptionService;
    private SimpleDescriptionNoTranslateService mSimpleDescriptionNoTranslateService;
    private SimpleNoDescriptionService mSimpleNoDescriptionService;
    //service side
    private BusAttachment mServiceBus;

    //client side
    private BusAttachment mClientBus;
    private ProxyBusObject mProxyObj;
    private SimpleDescriptionInterface mSimpleDescriptionInterface;
    private SimpleDescriptionNoTranslateInterface mSimpleDescriptionNoTranslateInterface;
    private SimpleNoDescriptionInterface mSimpleNoDescriptionInterface;
    private org.alljoyn.bus.ifaces.AllSeenIntrospectable mIntrospectable;

    public void setUp() throws Exception {
        sdit = new SimpleDescriptionInterfaceTranslator();
        mSimpleDescriptionService = null;
        mSimpleDescriptionNoTranslateService = null;
        mSimpleDescriptionInterface = null;
        mSimpleDescriptionNoTranslateInterface = null;
        mProxyObj = null;
        mServiceBus = null;
        mClientBus = null;
        //Service side
        mServiceBus = new BusAttachment("ServiceBus", BusAttachment.RemoteMessage.Receive);
        mServiceBus.registerBusListener(new BusListener());

        //Client side
        mClientBus = new BusAttachment("ClientBus", BusAttachment.RemoteMessage.Receive);
    }

    public void tearDown() throws Exception {
    }

    public void testGetDescriptionLanguages()throws Exception {
        mSimpleDescriptionService = new SimpleDescriptionService();
        assertNotNull(mSimpleDescriptionService);
        assertEquals(Status.OK, mServiceBus.registerBusObject(mSimpleDescriptionService, "/SimpleService", false, "en", "My service object", sdit));
        assertEquals(Status.OK, mServiceBus.connect());

        assertEquals(Status.OK,mClientBus.connect());
        mProxyObj =  mClientBus.getProxyBusObject(mServiceBus.getUniqueName(),
                "/SimpleService",
                0,
                new Class<?>[] {SimpleDescriptionInterface.class , org.alljoyn.bus.ifaces.AllSeenIntrospectable.class });
            assertNotNull("mProxyObj is null",mProxyObj);
            mSimpleDescriptionInterface =  mProxyObj.getInterface(SimpleDescriptionInterface.class);
            assertNotNull("mSimpleDescriptionInterface is null",mSimpleDescriptionInterface);

            mIntrospectable =  mProxyObj.getInterface(org.alljoyn.bus.ifaces.AllSeenIntrospectable.class);
            assertNotNull("mIntrospectable is null",mIntrospectable);

        try {
            String[] descriptionlanguages = mIntrospectable.GetDescriptionLanguages();
            assertTrue(Arrays.toString(descriptionlanguages).equals("[de, en]"));
        } catch (BusException e) {
            e.printStackTrace();
            fail();
        }

        mServiceBus.unregisterBusObject(mSimpleDescriptionService);
        mServiceBus.disconnect();
        mClientBus.disconnect();
    }

    public static final String IntrospectWithDescriptionEn1 =
            "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"+
            "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"+
            "<node>\n"+
            "  <description>My service object</description>\n"+
            "  <interface name=\"org.alljoyn.bus.SimpleDescriptionInterface\">\n"+
            "    <description>This is a simple interface</description>\n"+
            "    <method name=\"Ping\">\n"+
            "      <description>The ping method sends a small piece of data</description>\n"+
            "      <arg type=\"s\" direction=\"in\"/>\n"+
            "      <arg type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"+
            "    <method name=\"Introspect\">\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.allseen.Introspectable\">\n"+
            "    <method name=\"GetDescriptionLanguages\">\n"+
            "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <method name=\"IntrospectWithDescription\">\n"+
            "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"+
            "  </interface>\n"+
            "</node>\n";

    public static final String IntrospectWithDescriptionDe1 =
        "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"+
        "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"+
        "<node>\n"+
        "  <description>DE: My service object</description>\n"+
        "  <interface name=\"org.alljoyn.bus.SimpleDescriptionInterface\">\n"+
        "    <description>DE: This is a simple interface</description>\n"+
        "    <method name=\"Ping\">\n"+
        "      <description>DE: The ping method sends a small piece of data</description>\n"+
        "      <arg type=\"s\" direction=\"in\"/>\n"+
        "      <arg type=\"s\" direction=\"out\"/>\n"+
        "    </method>\n"+
        "  </interface>\n"+
        "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"+
        "    <method name=\"Introspect\">\n"+
        "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
        "    </method>\n"+
        "  </interface>\n"+
        "  <interface name=\"org.allseen.Introspectable\">\n"+
        "    <method name=\"GetDescriptionLanguages\">\n"+
        "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"+
        "    </method>\n"+
        "    <method name=\"IntrospectWithDescription\">\n"+
        "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"+
        "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
        "    </method>\n"+
        "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"+
        "  </interface>\n"+
        "</node>\n";

    public void testIntrospectWithDescription()throws Exception {
        mSimpleDescriptionService = new SimpleDescriptionService();
        assertNotNull(mSimpleDescriptionService);
        assertEquals(Status.OK, mServiceBus.registerBusObject(mSimpleDescriptionService, "/SimpleService", false, "en", "My service object", sdit));
        assertEquals(Status.OK, mServiceBus.connect());

        assertEquals(Status.OK,mClientBus.connect());
        mProxyObj =  mClientBus.getProxyBusObject(mServiceBus.getUniqueName(),
                "/SimpleService",
                0,
                new Class<?>[] {SimpleDescriptionInterface.class , org.alljoyn.bus.ifaces.AllSeenIntrospectable.class });
            assertNotNull("mProxyObj is null",mProxyObj);
            mSimpleDescriptionInterface =  mProxyObj.getInterface(SimpleDescriptionInterface.class);
            assertNotNull("mSimpleDescriptionInterface is null",mSimpleDescriptionInterface);

            mIntrospectable =  mProxyObj.getInterface(org.alljoyn.bus.ifaces.AllSeenIntrospectable.class);
            assertNotNull("mIntrospectable is null",mIntrospectable);

        try {
            String description;
            description = mIntrospectable.IntrospectWithDescription("en");
            assertEquals(IntrospectWithDescriptionEn1, description);
            description = mIntrospectable.IntrospectWithDescription("de");
            assertEquals(IntrospectWithDescriptionDe1, description);
            description = mIntrospectable.IntrospectWithDescription("fr");
            assertEquals(IntrospectWithDescriptionEn1, description);
        } catch (BusException e) {
            e.printStackTrace();
            fail();
        }
        mServiceBus.unregisterBusObject(mSimpleDescriptionService);
        mServiceBus.disconnect();
        mClientBus.disconnect();
    }

    public static final String IntrospectWithDescriptionEn2 =
            "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"+
            "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"+
            "<node>\n"+
            "  <description>My service object</description>\n"+
            "  <interface name=\"org.alljoyn.bus.SimpleDescriptionNoTranslateInterface\">\n"+
            "    <description>This is a simple interface</description>\n"+
            "    <method name=\"Ping\">\n"+
            "      <description>The ping method sends a small piece of data</description>\n"+
            "      <arg type=\"s\" direction=\"in\"/>\n"+
            "      <arg type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"+
            "    <method name=\"Introspect\">\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.allseen.Introspectable\">\n"+
            "    <method name=\"GetDescriptionLanguages\">\n"+
            "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <method name=\"IntrospectWithDescription\">\n"+
            "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"+
            "  </interface>\n"+
            "</node>\n";

    public static final String IntrospectWithDescriptionDe2 =
            "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"+
            "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"+
            "<node>\n"+
            "  <description>DE: My service object</description>\n"+
            "  <interface name=\"org.alljoyn.bus.SimpleDescriptionNoTranslateInterface\">\n"+
            "    <description>This is a simple interface</description>\n"+
            "    <method name=\"Ping\">\n"+
            "      <description>The ping method sends a small piece of data</description>\n" +
            "      <arg type=\"s\" direction=\"in\"/>\n"+
            "      <arg type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"+
            "    <method name=\"Introspect\">\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.allseen.Introspectable\">\n"+
            "    <method name=\"GetDescriptionLanguages\">\n"+
            "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <method name=\"IntrospectWithDescription\">\n"+
            "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"+
            "  </interface>\n"+
            "</node>\n";

    public void testIntrospectWithDescriptionNoTranslate()throws Exception {
        mSimpleDescriptionNoTranslateService = new SimpleDescriptionNoTranslateService();
        assertNotNull(mSimpleDescriptionNoTranslateService);
        assertEquals(Status.OK, mServiceBus.registerBusObject(mSimpleDescriptionNoTranslateService, "/SimpleService", false, "en", "My service object", sdit));
        assertEquals(Status.OK, mServiceBus.connect());

        assertEquals(Status.OK,mClientBus.connect());
        mProxyObj =  mClientBus.getProxyBusObject(mServiceBus.getUniqueName(),
                "/SimpleService",
                0,
                new Class<?>[] {SimpleDescriptionNoTranslateInterface.class , org.alljoyn.bus.ifaces.AllSeenIntrospectable.class });
            assertNotNull("mProxyObj is null",mProxyObj);
            mSimpleDescriptionNoTranslateInterface =  mProxyObj.getInterface(SimpleDescriptionNoTranslateInterface.class);
            assertNotNull("mSimpleDescriptionNoTranslateInterface is null",mSimpleDescriptionNoTranslateInterface);

            mIntrospectable =  mProxyObj.getInterface(org.alljoyn.bus.ifaces.AllSeenIntrospectable.class);
            assertNotNull("mIntrospectable is null",mIntrospectable);

        try {
            String description;
            description = mIntrospectable.IntrospectWithDescription("en");
            assertEquals(IntrospectWithDescriptionEn2, description);
            description = mIntrospectable.IntrospectWithDescription("de");
            assertEquals(IntrospectWithDescriptionDe2, description);
            description = mIntrospectable.IntrospectWithDescription("fr");
            assertTrue(description.contains("<description>"));
        } catch (BusException e) {
            e.printStackTrace();
            fail();
        }

        mServiceBus.unregisterBusObject(mSimpleDescriptionNoTranslateService);
        mServiceBus.disconnect();
        mClientBus.disconnect();
    }

    public static final String IntrospectWithDescriptionEn3 =
            "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"+
            "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"+
            "<node>\n"+
            "  <description>My service object</description>\n"+
            "  <interface name=\"org.alljoyn.bus.SimpleNoDescriptionInterface\">\n"+
            "    <method name=\"Ping\">\n"+
            "      <arg type=\"s\" direction=\"in\"/>\n"+
            "      <arg type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"+
            "    <method name=\"Introspect\">\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.allseen.Introspectable\">\n"+
            "    <method name=\"GetDescriptionLanguages\">\n"+
            "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <method name=\"IntrospectWithDescription\">\n"+
            "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"+
            "  </interface>\n"+
            "</node>\n";

    public static final String IntrospectWithDescriptionDe3 =
            "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"+
            "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"+
            "<node>\n"+
            "  <description>DE: My service object</description>\n"+
            "  <interface name=\"org.alljoyn.bus.SimpleNoDescriptionInterface\">\n"+
            "    <method name=\"Ping\">\n"+
            "      <arg type=\"s\" direction=\"in\"/>\n"+
            "      <arg type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"+
            "    <method name=\"Introspect\">\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.allseen.Introspectable\">\n"+
            "    <method name=\"GetDescriptionLanguages\">\n"+
            "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <method name=\"IntrospectWithDescription\">\n"+
            "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"+
            "  </interface>\n"+
            "</node>\n";

    public void testIntrospectWithNoDescription()throws Exception {
        mSimpleNoDescriptionService = new SimpleNoDescriptionService();
        assertNotNull(mSimpleNoDescriptionService);
        assertEquals(Status.OK, mServiceBus.registerBusObject(mSimpleNoDescriptionService, "/SimpleService", false, "en", "My service object", sdit));
        assertEquals(Status.OK, mServiceBus.connect());

        assertEquals(Status.OK,mClientBus.connect());
        mProxyObj =  mClientBus.getProxyBusObject(mServiceBus.getUniqueName(),
                "/SimpleService",
                0,
                new Class<?>[] {SimpleNoDescriptionInterface.class , org.alljoyn.bus.ifaces.AllSeenIntrospectable.class });
            assertNotNull("mProxyObj is null",mProxyObj);
            mSimpleNoDescriptionInterface =  mProxyObj.getInterface(SimpleNoDescriptionInterface.class);
            assertNotNull("mSimpleNoDescriptionInterface is null",mSimpleNoDescriptionInterface);

            mIntrospectable =  mProxyObj.getInterface(org.alljoyn.bus.ifaces.AllSeenIntrospectable.class);
            assertNotNull("mIntrospectable is null",mIntrospectable);

        try {
            String description;
            description = mIntrospectable.IntrospectWithDescription("en");
            assertEquals(IntrospectWithDescriptionEn3, description);
            description = mIntrospectable.IntrospectWithDescription("de");
            assertEquals(IntrospectWithDescriptionDe3, description);
            description = mIntrospectable.IntrospectWithDescription("fr");
            assertEquals(IntrospectWithDescriptionEn3, description);
        } catch (BusException e) {
            e.printStackTrace();
            fail();
        }

        mServiceBus.unregisterBusObject(mSimpleNoDescriptionService);
        mServiceBus.disconnect();
        mClientBus.disconnect();
    }

    public static final String IntrospectWithDescriptionDe4 =
            "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"+
            "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"+
            "<node>\n"+
            "  <description>My service object</description>\n" + 
            "  <interface name=\"org.alljoyn.bus.SimpleDescriptionInterface\">\n"+
            "    <description>DE: This is a simple interface</description>\n"+
            "    <method name=\"Ping\">\n"+
            "      <description>DE: The ping method sends a small piece of data</description>\n"+
            "      <arg type=\"s\" direction=\"in\"/>\n"+
            "      <arg type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"+
            "    <method name=\"Introspect\">\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "  </interface>\n"+
            "  <interface name=\"org.allseen.Introspectable\">\n"+
            "    <method name=\"GetDescriptionLanguages\">\n"+
            "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <method name=\"IntrospectWithDescription\">\n"+
            "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n"+
            "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"+
            "    </method>\n"+
            "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"+
            "  </interface>\n"+
            "</node>\n";

    public void testIntrospectWithDescriptionNoBusTranslate()throws Exception {
        mSimpleDescriptionService = new SimpleDescriptionService();
        assertNotNull(mSimpleDescriptionService);
        assertEquals(Status.OK, mServiceBus.registerBusObject(mSimpleDescriptionService, "/SimpleService", false, "en", "My service object", null));
        assertEquals(Status.OK, mServiceBus.connect());

        assertEquals(Status.OK,mClientBus.connect());
        mProxyObj =  mClientBus.getProxyBusObject(mServiceBus.getUniqueName(),
                "/SimpleService",
                0,
                new Class<?>[] {SimpleDescriptionInterface.class , org.alljoyn.bus.ifaces.AllSeenIntrospectable.class });
            assertNotNull("mProxyObj is null",mProxyObj);
            mSimpleDescriptionInterface =  mProxyObj.getInterface(SimpleDescriptionInterface.class);
            assertNotNull("mSimpleDescriptionInterface is null",mSimpleDescriptionInterface);

            mIntrospectable =  mProxyObj.getInterface(org.alljoyn.bus.ifaces.AllSeenIntrospectable.class);
            assertNotNull("mIntrospectable is null",mIntrospectable);

        try {
            String description;
            description = mIntrospectable.IntrospectWithDescription("en");
            assertEquals(IntrospectWithDescriptionEn1, description);
            description = mIntrospectable.IntrospectWithDescription("de");
            assertEquals(IntrospectWithDescriptionDe4, description);
            description = mIntrospectable.IntrospectWithDescription("fr");
            assertTrue(description.contains("<description>"));
        } catch (BusException e) {
            e.printStackTrace();
            fail();
        }

        mServiceBus.unregisterBusObject(mSimpleDescriptionService);
        mServiceBus.disconnect();
        mClientBus.disconnect();
    }

    class SimpleDescriptionService implements SimpleDescriptionInterface, BusObject {
        public String ping(String inStr) {
            return inStr;
        }
    }

    class SimpleDescriptionNoTranslateService implements SimpleDescriptionNoTranslateInterface, BusObject {
        public String ping(String inStr) {
            return inStr;
        }
    }

    class SimpleNoDescriptionService implements SimpleNoDescriptionInterface, BusObject {
        public String ping(String inStr) {
            return inStr;
        }
    }
}
