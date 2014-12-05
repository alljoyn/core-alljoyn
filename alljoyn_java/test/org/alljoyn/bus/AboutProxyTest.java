/*
 * Copyright (c) 2014 AllSeen Alliance. All rights reserved.
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

import java.util.HashMap;
import java.util.Map;

import junit.framework.TestCase;

import org.alljoyn.bus.BusAttachment.RemoteMessage;
import org.alljoyn.bus.ifaces.About;

public class AboutProxyTest extends TestCase{
    static {
        System.loadLibrary("alljoyn_java");
    }

    AboutListenerTestSessionPortListener sessionPortlistener;
    private BusAttachment serviceBus;
    static short PORT_NUMBER = 542;

    public void setUp() throws Exception {
        serviceBus = new BusAttachment("AboutListenerTestService");

        assertEquals(Status.OK, serviceBus.connect());
        sessionPortlistener = new AboutListenerTestSessionPortListener();
        short contactPort = PORT_NUMBER;
        Mutable.ShortValue sessionPort = new Mutable.ShortValue(contactPort);
        assertEquals(Status.OK, serviceBus.bindSessionPort(sessionPort, new SessionOpts(), sessionPortlistener));
    }

    public void tearDown() throws Exception {
        serviceBus.disconnect();
        serviceBus.release();
    }

    public class AboutListenerTestSessionPortListener extends SessionPortListener {
        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
            if (sessionPort == PORT_NUMBER) {
                return true;
            } else {
                return false;
            }
        }
        public void sessionJoined(short sessionPort, int id, String joiner) {
            sessionId = id;
            sessionEstablished = true;
        }

        public int sessionId;
        public boolean sessionEstablished;
    }


    /*
     * An quick simple implementation of the AboutDataListener interface used
     * when making an announcement.
     */
    public class AboutListenerTestAboutData implements AboutDataListener {

        @Override
        public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException {
            Map<String, Variant> aboutData = new HashMap<String, Variant>();
            //nonlocalized values
            aboutData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            aboutData.put("DefaultLanguage",  new Variant(new String("en")));
            aboutData.put("DeviceId",  new Variant(new String("93c06771-c725-48c2-b1ff-6a2a59d445b8")));
            aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
            aboutData.put("SupportedLanguages", new Variant(new String[] {"en", "es"}));
            aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
            aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
            aboutData.put("AJSoftwareVersion", new Variant(Version.get()));
            aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
            aboutData.put("SupportUrl", new Variant(new String("http://www.example.com/support")));
            //localized values
            // If the language String is null or an empty string we return the
            // default language
            if ((language == null) || (language.length() == 0) || language.equalsIgnoreCase("en")) {
                aboutData.put("DeviceName", new Variant(new String("A device name")));
                aboutData.put("AppName", new Variant(new String("An application name")));
                aboutData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
                aboutData.put("Description", new Variant(new String("Sample showing the about feature in a service application")));
            } else if (language.equalsIgnoreCase("es")) { //Spanish
                aboutData.put("DeviceName", new Variant(new String("Un nombre de dispositivo")));
                aboutData.put("AppName", new Variant(new String("Un nombre de aplicación")));
                aboutData.put("Manufacturer", new Variant(new String("Una empresa de fabricación de poderosos")));
                aboutData.put("Description", new Variant(new String("Muestra que muestra la característica de sobre en una aplicación de servicio")));
            } else {
                throw new ErrorReplyBusException(Status.LANGUAGE_NOT_SUPPORTED);
            }
            return aboutData;
        }

        @Override
        public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException {
            Map<String, Variant> announceData = new HashMap<String, Variant>();
            announceData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            announceData.put("DefaultLanguage",  new Variant(new String("en")));
            announceData.put("DeviceName", new Variant(new String("A device name")));
            announceData.put("DeviceId",  new Variant(new String("93c06771-c725-48c2-b1ff-6a2a59d445b8")));
            announceData.put("AppName", new Variant(new String("An application name")));
            announceData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
            announceData.put("ModelNumber", new Variant(new String("A1B2C3")));
            return announceData;
        }

    }

    class Intfa implements AboutListenerTestInterfaceA, BusObject {

        @Override
        public String echo(String str) throws BusException {
            return str;
        }
    }

    class AboutListenerTestAboutListener implements AboutListener {
        AboutListenerTestAboutListener() {
            remoteBusName = null;
            version = 0;
            port = 0;
            announcedFlag = false;
            aod = null;
        }
        public void announced(String busName, int version, short port, AboutObjectDescription[] objectDescriptions, Map<String, Variant> aboutData) {
            remoteBusName = busName;
            this.version = version;
            this.port = port;
            aod = objectDescriptions;
            aod = new AboutObjectDescription[objectDescriptions.length];
            for (int i = 0; i < objectDescriptions.length; ++i) {
                aod[i] = objectDescriptions[i];
            }
            announcedAboutData = aboutData;
            announcedFlag = true;
        }
        public String remoteBusName;
        public int version;
        public short port;
        public boolean announcedFlag;
        public AboutObjectDescription[] aod;
        public Map<String,Variant> announcedAboutData;
    }

    //This test will verify that the Values that come in the announce signal
    // The tests following this test will assume the values are correct.
    public void testAnnounceSignal() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));

        AboutObj aboutObj = new AboutObj(serviceBus);
        AboutListenerTestAboutData aboutData = new AboutListenerTestAboutData();
        assertEquals(Status.OK, aboutObj.announce(PORT_NUMBER, aboutData));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (aListener.announcedFlag) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }

        assertTrue(aListener.announcedFlag);
        assertEquals(serviceBus.getUniqueName(), aListener.remoteBusName);
        boolean aboutPathFound = false;
        boolean aboutInterfaceFound = false;
        assertEquals(PORT_NUMBER, aListener.port);
        for (AboutObjectDescription aod : aListener.aod) {
            if (aod.path.equals("/about/test")) {
                aboutPathFound = true;
                for (String s : aod.interfaces) {
                    if (s.equals("com.example.test.AboutListenerTest.a")) {
                        aboutInterfaceFound = true;
                    }
                }
            }
        }
        assertTrue(aboutPathFound);
        assertTrue(aboutInterfaceFound);

        try {
        // Verify the values sent in the AboutDataListener
        // looping through to test the contents of the AppId works this way since
        // the AppId was set {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
        byte[] appId = aListener.announcedAboutData.get("AppId").getObject(byte[].class);
        for (int i = 0; i < appId.length; ++i) {
            assertEquals((byte)(i+1), appId[i]);
        }

            assertEquals("en", aListener.announcedAboutData.get("DefaultLanguage").getObject(String.class));
            assertEquals("A device name", aListener.announcedAboutData.get("DeviceName").getObject(String.class));
            assertEquals("93c06771-c725-48c2-b1ff-6a2a59d445b8", aListener.announcedAboutData.get("DeviceId").getObject(String.class));
            assertEquals("An application name", aListener.announcedAboutData.get("AppName").getObject(String.class));
            assertEquals("A mighty manufacturing company", aListener.announcedAboutData.get("Manufacturer").getObject(String.class));
            assertEquals("A1B2C3", aListener.announcedAboutData.get("ModelNumber").getObject(String.class));
        } catch (BusException e) {
            fail("Unable to read variant value from the announced AboutData.");
        }


        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));
        clientBus.disconnect();
        clientBus.release();
    }

    // We can Join a session using the information that comes in on the announce
    // signal
    public void testJoinSession() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));

        AboutObj aboutObj = new AboutObj(serviceBus);
        AboutListenerTestAboutData aboutData = new AboutListenerTestAboutData();
        assertEquals(Status.OK, aboutObj.announce(PORT_NUMBER, aboutData));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (aListener.announcedFlag) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }
        
        assertTrue(aListener.announcedFlag);

        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();;

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        assertEquals(Status.OK, clientBus.joinSession(aListener.remoteBusName, aListener.port, sessionId, sessionOpts, new SessionListener()));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (sessionPortlistener.sessionEstablished) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }

        assertTrue(sessionPortlistener.sessionEstablished);
        assertEquals(sessionPortlistener.sessionId, sessionId.value);

        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));
        clientBus.disconnect();
        clientBus.release();
    }

    public void testGetVersion() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));

        AboutObj aboutObj = new AboutObj(serviceBus);
        AboutListenerTestAboutData aboutData = new AboutListenerTestAboutData();
        assertEquals(Status.OK, aboutObj.announce(PORT_NUMBER, aboutData));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (aListener.announcedFlag) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }
        
        assertTrue(aListener.announcedFlag);

        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();;

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        assertEquals(Status.OK, clientBus.joinSession(aListener.remoteBusName, aListener.port, sessionId, sessionOpts, new SessionListener()));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (sessionPortlistener.sessionEstablished) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }

        assertTrue(sessionPortlistener.sessionEstablished);
        assertEquals(sessionPortlistener.sessionId, sessionId.value);

        AboutProxy proxy = new AboutProxy(clientBus, aListener.remoteBusName, sessionId.value);
        try {
            assertEquals(About.VERSION, proxy.getVersion());
        } catch (BusException e) {
            fail("Call to AboutProxyBus method GetVersion failed.");
        }
        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));
        clientBus.disconnect();
        clientBus.release();
    }

    public void testGetObjectDescription() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));

        AboutObj aboutObj = new AboutObj(serviceBus);
        AboutListenerTestAboutData aboutData = new AboutListenerTestAboutData();
        assertEquals(Status.OK, aboutObj.announce(PORT_NUMBER, aboutData));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (aListener.announcedFlag) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }
        
        assertTrue(aListener.announcedFlag);

        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();;

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        assertEquals(Status.OK, clientBus.joinSession(aListener.remoteBusName, aListener.port, sessionId, sessionOpts, new SessionListener()));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (sessionPortlistener.sessionEstablished) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }

        assertTrue(sessionPortlistener.sessionEstablished);
        assertEquals(sessionPortlistener.sessionId, sessionId.value);

        AboutProxy proxy = new AboutProxy(clientBus, aListener.remoteBusName, sessionId.value);
        try {
            AboutObjectDescription[] aod = proxy.getObjectDescription();
            assertEquals(1, aod.length);
            assertEquals(aod[0].path, "/about/test");
            assertEquals(1, aod[0].interfaces.length);
            assertEquals("com.example.test.AboutListenerTest.a", aod[0].interfaces[0]);
        } catch (BusException e) {
            fail("Call to AboutProxyBus method GetVersion failed.");
        }
        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));
        clientBus.disconnect();
        clientBus.release();
    }

    public void testGetAboutDataDefaultLanguage() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));

        AboutObj aboutObj = new AboutObj(serviceBus);
        AboutListenerTestAboutData aboutData = new AboutListenerTestAboutData();
        assertEquals(Status.OK, aboutObj.announce(PORT_NUMBER, aboutData));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (aListener.announcedFlag) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }
        
        assertTrue(aListener.announcedFlag);

        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();;

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        assertEquals(Status.OK, clientBus.joinSession(aListener.remoteBusName, aListener.port, sessionId, sessionOpts, new SessionListener()));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (sessionPortlistener.sessionEstablished) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }

        assertTrue(sessionPortlistener.sessionEstablished);
        assertEquals(sessionPortlistener.sessionId, sessionId.value);

        AboutProxy proxy = new AboutProxy(clientBus, aListener.remoteBusName, sessionId.value);
        Map<String, Variant> aData = new HashMap<String, Variant>();
        try {
            aData = proxy.getAboutData(aListener.announcedAboutData.get("DefaultLanguage").getObject(String.class));
        } catch (BusException e) {
            fail("Call to AboutProxyBus method GetVersion failed.");
        }

        try {
            // Verify the values sent in the AboutDataListener
            // looping through to test the contents of the AppId works this way since
            // the AppId was set to {1, 2, 3, 4, 5, 6, 7, 8, 9}
            byte[] appId = aData.get("AppId").getObject(byte[].class);
            for (int i = 0; i < appId.length; ++i) {
                assertEquals((byte)(i+1), appId[i]);
            }

                assertEquals("en", aData.get("DefaultLanguage").getObject(String.class));
                assertEquals("A device name", aData.get("DeviceName").getObject(String.class));
                assertEquals("93c06771-c725-48c2-b1ff-6a2a59d445b8", aData.get("DeviceId").getObject(String.class));
                assertEquals("An application name", aData.get("AppName").getObject(String.class));
                assertEquals("A mighty manufacturing company", aData.get("Manufacturer").getObject(String.class));
                assertEquals("A1B2C3", aData.get("ModelNumber").getObject(String.class));
                String[] supportedLanguages = aData.get("SupportedLanguages").getObject(String[].class);
                assertEquals(2, supportedLanguages.length);
                assertEquals("en", supportedLanguages[0]);
                assertEquals("es", supportedLanguages[1]);
                assertEquals("Sample showing the about feature in a service application", aData.get("Description").getObject(String.class));
                assertEquals("2014-09-23", aData.get("DateOfManufacture").getObject(String.class));
                assertEquals("1.0", aData.get("SoftwareVersion").getObject(String.class));
                assertEquals(Version.get(), aData.get("AJSoftwareVersion").getObject(String.class));
                assertEquals("0.1alpha", aData.get("HardwareVersion").getObject(String.class));
                assertEquals("http://www.example.com/support", aData.get("SupportUrl").getObject(String.class));

            } catch (BusException e) {
                fail("Unable to read variant value from the AboutData.");
            }

        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));
        clientBus.disconnect();
        clientBus.release();
    }

    public void testGetAboutDataSpanishLanguage() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));

        AboutObj aboutObj = new AboutObj(serviceBus);
        AboutListenerTestAboutData aboutData = new AboutListenerTestAboutData();
        assertEquals(Status.OK, aboutObj.announce(PORT_NUMBER, aboutData));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (aListener.announcedFlag) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }
        
        assertTrue(aListener.announcedFlag);

        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();;

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        assertEquals(Status.OK, clientBus.joinSession(aListener.remoteBusName, aListener.port, sessionId, sessionOpts, new SessionListener()));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (sessionPortlistener.sessionEstablished) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }

        assertTrue(sessionPortlistener.sessionEstablished);
        assertEquals(sessionPortlistener.sessionId, sessionId.value);

        AboutProxy proxy = new AboutProxy(clientBus, aListener.remoteBusName, sessionId.value);
        Map<String, Variant> aData = new HashMap<String, Variant>();
        try {
            aData = proxy.getAboutData("es");
        } catch (BusException e) {
            fail("Call to AboutProxyBus method GetVersion failed.");
        }

        try {
            // Verify the values sent in the AboutDataListener
            // looping through to test the contents of the AppId works this way since
            // the AppId was set to {1, 2, 3, 4, 5, 6, 7, 8, 9}
            byte[] appId = aData.get("AppId").getObject(byte[].class);
            for (int i = 0; i < appId.length; ++i) {
                assertEquals((byte)(i+1), appId[i]);
            }

                assertEquals("en", aData.get("DefaultLanguage").getObject(String.class));
                assertEquals("Un nombre de dispositivo", aData.get("DeviceName").getObject(String.class));
                assertEquals("93c06771-c725-48c2-b1ff-6a2a59d445b8", aData.get("DeviceId").getObject(String.class));
                assertEquals("Un nombre de aplicación", aData.get("AppName").getObject(String.class));
                assertEquals("Una empresa de fabricación de poderosos", aData.get("Manufacturer").getObject(String.class));
                assertEquals("A1B2C3", aData.get("ModelNumber").getObject(String.class));
                String[] supportedLanguages = aData.get("SupportedLanguages").getObject(String[].class);
                assertEquals(2, supportedLanguages.length);
                assertEquals("en", supportedLanguages[0]);
                assertEquals("es", supportedLanguages[1]);
                assertEquals("Muestra que muestra la característica de sobre en una aplicación de servicio", aData.get("Description").getObject(String.class));
                assertEquals("2014-09-23", aData.get("DateOfManufacture").getObject(String.class));
                assertEquals("1.0", aData.get("SoftwareVersion").getObject(String.class));
                assertEquals(Version.get(), aData.get("AJSoftwareVersion").getObject(String.class));
                assertEquals("0.1alpha", aData.get("HardwareVersion").getObject(String.class));
                assertEquals("http://www.example.com/support", aData.get("SupportUrl").getObject(String.class));

            } catch (BusException e) {
                fail("Unable to read variant value from the AboutData.");
            }

        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));
        clientBus.disconnect();
        clientBus.release();
    }

    public void testGetAboutDataUnsupportedLanguage() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));

        AboutObj aboutObj = new AboutObj(serviceBus);
        AboutListenerTestAboutData aboutData = new AboutListenerTestAboutData();
        assertEquals(Status.OK, aboutObj.announce(PORT_NUMBER, aboutData));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (aListener.announcedFlag) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }
        
        assertTrue(aListener.announcedFlag);

        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();;

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        assertEquals(Status.OK, clientBus.joinSession(aListener.remoteBusName, aListener.port, sessionId, sessionOpts, new SessionListener()));

        for (int msec = 0; msec < 10000; msec += 5) {
            if (sessionPortlistener.sessionEstablished) {
                break;
            }
            try {
                Thread.sleep(5);
            } catch (InterruptedException e) {
                e.printStackTrace();
                fail("unexpected InterruptedException");
            }
        }
        
        assertTrue(sessionPortlistener.sessionEstablished);
        assertEquals(sessionPortlistener.sessionId, sessionId.value);

        AboutProxy proxy = new AboutProxy(clientBus, aListener.remoteBusName, sessionId.value);
        Map<String, Variant> aData = new HashMap<String, Variant>();
        try {
            aData = proxy.getAboutData("fr");
            fail("Call to AboutProxyBus getAboutData should have thrown bus exception.");
        } catch (BusException e) {
            assertEquals("org.alljoyn.Error.LanguageNotSupported", e.getMessage());
        }
        assertTrue(aData.isEmpty());
        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {"com.example.test.AboutListenerTest.a"}));
        clientBus.disconnect();
        clientBus.release();
    }
}
