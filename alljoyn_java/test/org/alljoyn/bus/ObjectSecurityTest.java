/**
 * Copyright (c) 2013, 2014, AllSeen Alliance. All rights reserved.
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

import junit.framework.TestCase;

public class ObjectSecurityTest extends TestCase{
    public ObjectSecurityTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class InsecureService implements InsecureInterface, BusObject {
        public String insecurePing(String inStr) throws BusException { return inStr; }
    }

    public class SecureOffService implements SecureOffInterface, BusObject {
        public String ping(String str) throws BusException { return str; }
    }

    public class SrpAuthListener implements AuthListener {
        public boolean requestPasswordFlag;
        public boolean completedFlag;

        public SrpAuthListener() {
            requestPasswordFlag = false;
            completedFlag = false;
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                AuthRequest[] requests) {
            assertTrue("ALLJOYN_SRP_KEYX".equals(mechanism));
            for(AuthRequest request : requests){
                if(request instanceof PasswordRequest) {
                    ((PasswordRequest) request).setPassword("123456".toCharArray());
                    requestPasswordFlag = true;
                }
            }
            return true;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {
            assertTrue("ALLJOYN_SRP_KEYX".equals(mechanism));
            assertTrue(authenticated);
            completedFlag = authenticated;
        }
    }


    BusAttachment serviceBus;
    BusAttachment clientBus;
    SrpAuthListener serviceAuthListener;
    SrpAuthListener clientAuthListener;

    private TestKeyStoreListener serviceKeyStoreListener;
    private TestKeyStoreListener clientKeyStoreListener;

    public class TestKeyStoreListener implements KeyStoreListener {
        private byte[] keys;
        public boolean saveKeys;
        public byte[] getKeys() { return keys; }
        public char[] getPassword() { return "password".toCharArray(); }
        public void putKeys(byte[] keys) {
            if (saveKeys) {
                this.keys = keys;
            }
        }
    }

    //static private String INTERFACE_NAME = "org.alljoyn.test.objectSecurity.interface";
    //static private String WELLKNOWN_NAME = "org.alljoyn.test.objectSecurity";
    static private String OBJECT_PATH    = "/org/alljoyn/test/objectSecurity";


    public void setUp() throws Exception {
        serviceBus = new BusAttachment("ObjectSecurityTestClient");
        serviceKeyStoreListener = new TestKeyStoreListener();
        serviceBus.registerKeyStoreListener(serviceKeyStoreListener);

        serviceAuthListener = new SrpAuthListener();
        assertEquals(Status.OK, serviceBus.registerAuthListener("ALLJOYN_SRP_KEYX", serviceAuthListener));
        assertEquals(Status.OK, serviceBus.connect());
        serviceBus.clearKeyStore();

        clientBus = new BusAttachment("ObjectSecurityTestService");
        clientKeyStoreListener = new TestKeyStoreListener();
        clientBus.registerKeyStoreListener(clientKeyStoreListener);

        clientAuthListener = new SrpAuthListener();
        assertEquals(Status.OK, clientBus.registerAuthListener("ALLJOYN_SRP_KEYX", clientAuthListener));
        assertEquals(Status.OK, clientBus.connect());
        clientBus.clearKeyStore();
    }

    public void tearDown() throws Exception {
        if (serviceBus != null) {
            serviceBus.disconnect();
            serviceBus.release();
            serviceBus = null;
        }

        if (clientBus != null) {
            clientBus.disconnect();
            clientBus.release();
            clientBus = null;
        }
    }

    /*
     * This test takes an interface that has no security and make it secure
     * using object based security.
     */
    public void testInsecureInterfaceSecureObject1() {
        InsecureService insecureService = new InsecureService();
        assertEquals(Status.OK, serviceBus.registerBusObject(insecureService, OBJECT_PATH, true));

        ProxyBusObject proxy = new ProxyBusObject(clientBus, serviceBus.getUniqueName(), OBJECT_PATH, BusAttachment.SESSION_ID_ANY, new Class<?>[] {InsecureInterface.class}, true);

        InsecureInterface ifac = proxy.getInterface(InsecureInterface.class);

        try {
            assertEquals("alljoyn", ifac.insecurePing("alljoyn"));
        } catch (BusException e) {
            e.printStackTrace();
            // we don't expect to have a BusException if we have on fail
            assertTrue(false);
        }
        assertTrue(proxy.isSecure());
        assertTrue(serviceBus.isBusObjectSecure(insecureService));

        assertTrue(serviceAuthListener.requestPasswordFlag);
        assertTrue(clientAuthListener.requestPasswordFlag);
        assertTrue(serviceAuthListener.completedFlag);
        assertTrue(clientAuthListener.completedFlag);
    }

    /*
     * This test takes an interface that has no security and make it secure
     * using object based security.
     */
    public void testInsecureInterfaceSecureObject2() {
        InsecureService insecureService = new InsecureService();
        assertEquals(Status.OK, serviceBus.registerBusObject(insecureService, OBJECT_PATH, true));

        ProxyBusObject proxy = clientBus.getProxyBusObject(serviceBus.getUniqueName(), OBJECT_PATH, BusAttachment.SESSION_ID_ANY, new Class<?>[] {InsecureInterface.class}, true);

        InsecureInterface ifac = proxy.getInterface(InsecureInterface.class);

        try {
            assertEquals("alljoyn", ifac.insecurePing("alljoyn"));
        } catch (BusException e) {
            e.printStackTrace();
            // we don't expect to have a BusException if we have on fail
            assertTrue(false);
        }
        assertTrue(proxy.isSecure());
        assertTrue(serviceBus.isBusObjectSecure(insecureService));

        assertTrue(serviceAuthListener.requestPasswordFlag);
        assertTrue(clientAuthListener.requestPasswordFlag);
        assertTrue(serviceAuthListener.completedFlag);
        assertTrue(clientAuthListener.completedFlag);
    }

    /*
     * This test takes an interface that has security == off and tries to make
     * the object secure using object based security. Since the interface
     * explicitly states that the interface security is off it should not
     * attempt to use authentication when making a method call.
     */
    public void testSecureOffInterfaceSecureObject() {
        SecureOffService secureOffService = new SecureOffService();
        assertEquals(Status.OK, serviceBus.registerBusObject(secureOffService, OBJECT_PATH, true));

        ProxyBusObject proxy = clientBus.getProxyBusObject(serviceBus.getUniqueName(), OBJECT_PATH, BusAttachment.SESSION_ID_ANY, new Class<?>[] {SecureOffInterface.class}, true);

        SecureOffInterface ifac = proxy.getInterface(SecureOffInterface.class);

        try {
            assertEquals("alljoyn", ifac.ping("alljoyn"));
        } catch (BusException e) {
            e.printStackTrace();
            // we don't expect to have a BusException if we have on fail
            assertTrue(false);
        }
        /*
         * security was stated as true when proxyBusObject was created
         */
        assertTrue(proxy.isSecure());
        assertTrue(serviceBus.isBusObjectSecure(secureOffService));

        /*
         * authlistener should not have been called because interface security
         * annotation is @security(value='off')
         */
        assertFalse(serviceAuthListener.requestPasswordFlag);
        assertFalse(clientAuthListener.requestPasswordFlag);
        assertFalse(serviceAuthListener.completedFlag);
        assertFalse(clientAuthListener.completedFlag);
    }

    // TODO add tests that verify inherit security value.
}
