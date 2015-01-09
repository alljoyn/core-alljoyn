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

public class AboutIconProxyTest extends TestCase{
    static {
        System.loadLibrary("alljoyn_java");
    }

    AboutListenerTestSessionPortListener sessionPortlistener;
    private BusAttachment serviceBus;
    static short PORT_NUMBER = 542;

    public void setUp() throws Exception {
        serviceBus = new BusAttachment("AboutIconTestService");

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
            aboutData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }));
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
            announceData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }));
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
    public void testIconInterfaceIsAnnounced() {
        AboutIcon icon = null;
        try {
            icon = new AboutIcon("image/png", "http://www.example.com");
        } catch (BusException e1) {
            e1.printStackTrace();
            fail("Unexpected BusException.");
        }
        //icon.setUrl("image/png", "http://www.example.com");
        AboutIconObj aio = new AboutIconObj(serviceBus, icon);


        BusAttachment clientBus = new BusAttachment("AboutIconTestClient");
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME}));

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
        boolean aboutIconPathFound = false;
        boolean aboutIconInterfaceFound = false;
        assertEquals(PORT_NUMBER, aListener.port);
        for (AboutObjectDescription aod : aListener.aod) {
            if (aod.path.equals(org.alljoyn.bus.ifaces.Icon.OBJ_PATH)) {
                aboutIconPathFound = true;
                for (String s : aod.interfaces) {
                    if (s.equals(org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME)) {
                        aboutIconInterfaceFound = true;
                    }
                }
            }
        }
        assertTrue(aboutIconPathFound);
        assertTrue(aboutIconInterfaceFound);

        assertEquals(Status.OK, aboutObj.unannounce());
        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME}));
        clientBus.disconnect();
        clientBus.release();
    }

    // We can Join a session using the information that comes in on the announce
    // signal
    public void testAboutIconMethodCalls_urlset() {
        AboutIcon icon = null;
        try {
            icon = new AboutIcon("image/png", "http://www.example.com");
        } catch (BusException e1) {
            e1.printStackTrace();
            fail("Unexpected BusException.");
        }
        //assertEquals(Status.OK, icon.setUrl("image/png", "http://www.example.com"));
        AboutIconObj aio = new AboutIconObj(serviceBus, icon);

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME}));

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

        AboutIconProxy aipo = new AboutIconProxy(clientBus, aListener.remoteBusName, sessionPortlistener.sessionId);
        try {
            assertEquals(org.alljoyn.bus.AboutIconObj.VERSION, aipo.getVersion());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }

        try {
            assertEquals(0, aipo.getSize());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            assertEquals("image/png", aipo.getMimeType());
        } catch (BusException e) {
            fail("Unexpected BusException.");
            e.printStackTrace();
        }
        try {
            assertEquals("http://www.example.com", aipo.getUrl());
        } catch (BusException e) {
            fail("Unexpected BusException.");
            e.printStackTrace();
        }
        try {
            AboutIcon ai;
            ai = aipo.getAboutIcon();
            assertEquals(icon.getMimeType(), ai.getMimeType());
            assertEquals(icon.getUrl(), ai.getUrl());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }

        assertEquals(Status.OK, aboutObj.unannounce());
        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME}));
        clientBus.disconnect();
        clientBus.release();
    }
    // We can Join a session using the information that comes in on the announce
    // signal
    public void testAboutIconMethodCalls_contentset() {
        byte[] iconContent = { (byte)0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
                0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x0A,
                0x00, 0x00, 0x00, 0x0A, 0x08, 0x02, 0x00, 0x00, 0x00, 0x02,
                0x50, 0x58, (byte)0xEA, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D,
                0x41, 0x00, 0x00, (byte)0xAF, (byte)0xC8, 0x37, 0x05, (byte)0x8A, (byte)0xE9, 0x00,
                0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66,
                0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64, 0x6F, 0x62,
                0x65, 0x20, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61,
                0x64, 0x79, 0x71, (byte)0xC9, 0x65, 0x3C, 0x00, 0x00, 0x00, 0x18,
                0x49, 0x44, 0x41, 0x54, 0x78, (byte)0xDA, 0x62, (byte)0xFC, 0x3F, (byte)0x95,
                (byte)0x9F, 0x01, 0x37, 0x60, 0x62, (byte)0xC0, 0x0B, 0x46, (byte)0xAA, 0x34,
                0x40, (byte)0x80, 0x01, 0x00, 0x06, 0x7C, 0x01, (byte)0xB7, (byte)0xED, 0x4B,
                0x53, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
                (byte)0xAE, 0x42, 0x60, (byte)0x82 };
        AboutIcon icon = null;
        try {
            icon = new AboutIcon("image/png", iconContent);
        } catch (BusException e1) {
            e1.printStackTrace();
            fail("AboutIcon threw a BusException when it was unexpected.");
        }

        //assertEquals(Status.OK, icon.setContent("image/png", iconContent));
        AboutIconObj aio = new AboutIconObj(serviceBus, icon);

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME}));

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

        AboutIconProxy aipo = new AboutIconProxy(clientBus, aListener.remoteBusName, sessionPortlistener.sessionId);
        try {
            assertEquals(org.alljoyn.bus.AboutIconObj.VERSION, aipo.getVersion());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            assertEquals(iconContent.length, aipo.getSize());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            assertEquals("image/png", aipo.getMimeType());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            assertEquals("", aipo.getUrl());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }

        try {
            byte[] content_out = aipo.getContent();
            assertEquals(iconContent.length, content_out.length);
            for (int i = 0; i < content_out.length; ++i) {
                assertEquals(iconContent[i], content_out[i]);
            }
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            AboutIcon ai;
            ai = aipo.getAboutIcon();
            assertEquals(icon.getMimeType(), ai.getMimeType());
            byte[] content_out = ai.getContent();
            for (int i = 0; i < content_out.length; ++i) {
                assertEquals(iconContent[i], content_out[i]);
            }
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }

        assertEquals(Status.OK, aboutObj.unannounce());
        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME}));
        clientBus.disconnect();
        clientBus.release();
    }

    public void testAboutIconMethodCalls_contentset_large_icon() {
        byte[] badContent = new byte[AboutIcon.MAX_CONTENT_LENGTH+1];

        AboutIcon icon = null;
        boolean busExceptionCaught = false;
        try {
            icon = new AboutIcon("image/png", badContent);
            fail("AboutIcon did not throw an exception when it was expected to.");
        } catch (BusException e1) {
            assertEquals(e1.getMessage(), "MAX_CONTENT_LENGTH exceeded");
            busExceptionCaught = true;
        }

        assertTrue(busExceptionCaught);

        byte[] iconContent = new byte[AboutIcon.MAX_CONTENT_LENGTH];
        // set the icon to byte incremented starting at zero
        for(int i = 0; i < AboutIcon.MAX_CONTENT_LENGTH; ++i) {
            iconContent[i] = (byte)i;
        }

        try {
            icon = new AboutIcon("image/png", iconContent);
        } catch (BusException e1) {
            e1.printStackTrace();
            fail("AboutIcon threw a BusException when it was unexpected.");
        }

        AboutIconObj aio = new AboutIconObj(serviceBus, icon);

        BusAttachment clientBus = new BusAttachment("AboutListenerTestClient", RemoteMessage.Receive);
        assertEquals(Status.OK, clientBus.connect());

        AboutListenerTestAboutListener aListener = new AboutListenerTestAboutListener();
        aListener.announcedFlag = false;
        clientBus.registerAboutListener(aListener);
        assertEquals(Status.OK, clientBus.whoImplements(new String[] {org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME}));

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

        AboutIconProxy aipo = new AboutIconProxy(clientBus, aListener.remoteBusName, sessionPortlistener.sessionId);
        try {
            assertEquals(org.alljoyn.bus.AboutIconObj.VERSION, aipo.getVersion());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            assertEquals(iconContent.length, aipo.getSize());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            assertEquals("image/png", aipo.getMimeType());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            assertEquals("", aipo.getUrl());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            byte[] content_out = aipo.getContent();
            assertEquals(iconContent.length, content_out.length);
            for (int i = 0; i < content_out.length; ++i) {
                assertEquals(iconContent[i], content_out[i]);
            }
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }
        try {
            AboutIcon ai;
            ai = aipo.getAboutIcon();
            assertEquals(icon.getMimeType(), ai.getMimeType());
            byte[] content_out = ai.getContent();
            for (int i = 0; i < content_out.length; ++i) {
                assertEquals(iconContent[i], content_out[i]);
            }
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected BusException.");
        }

        assertEquals(Status.OK, aboutObj.unannounce());
        assertEquals(Status.OK, clientBus.cancelWhoImplements(new String[] {org.alljoyn.bus.ifaces.Icon.INTERFACE_NAME}));
        clientBus.disconnect();
        clientBus.release();
    }

    public void testAboutIconMethodCalls_uninitialized() {
        boolean exceptionCaught = false;
        try {
            AboutIcon icon = null;
            AboutIconObj aio = new AboutIconObj(serviceBus, icon);
            fail("Expected NullPointerException");
        } catch(NullPointerException e) {
            assertEquals("icon must not be null", e.getMessage());
            exceptionCaught = true;
        }
        assertTrue(exceptionCaught);

        exceptionCaught = false;
        try {
            AboutIcon icon = null;
            AboutIconObj aio = new AboutIconObj(null, icon);
            fail("Expected NullPointerException");
        } catch(NullPointerException e) {
            assertEquals("bus must not be null", e.getMessage());
            exceptionCaught = true;
        }
        assertTrue(exceptionCaught);
    }

    public void testAboutIcon_constructContentAndUrl() {
        AboutIcon icon = null;
        byte[] iconContent = new byte[64];
        // set the icon to byte incremented starting at zero
        for(int i = 0; i < 64; ++i) {
            iconContent[i] = (byte)i;
        }
        try {
            icon = new AboutIcon("image/png", "http://www.example.com", iconContent);
        } catch (BusException e1) {
            e1.printStackTrace();
            fail("AboutIcon threw a BusException when it was unexpected.");
        }

        assertEquals("image/png", icon.getMimeType());
        assertEquals("http://www.example.com", icon.getUrl());
        assertEquals(64, icon.getContent().length);
        for(int i = 0; i < 64; ++i) {
            assertEquals((byte)i, icon.getContent()[i]);
        }
    }
}
