/**
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

package org.alljoyn.bus;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ifaces.DBusProxyObj;

import java.io.File;

import junit.framework.TestCase;

public class KeyStoreListenerTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment bus;
    private BusAttachment otherBus;
    private SecureService service;
    private SecureInterface proxy;
    private BusAuthListener authListener;
    private BusAuthListener otherAuthListener;

    public class InMemoryKeyStoreListener implements KeyStoreListener {
        private byte[] keys;

        public byte[] getKeys() { return keys; }
        public char[] getPassword() { return "password".toCharArray(); }
        public void putKeys(byte[] keys) { this.keys = keys; }
    }

    public class SecureService implements SecureInterface, BusObject {
        public String ping(String str) { return str; }
    }

    public class BusAuthListener implements AuthListener {
        private String authMechanismRequested;

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            authMechanismRequested = mechanism;
            assertEquals("", userName);
            for (AuthRequest request : requests) {
                if (request instanceof PasswordRequest) {
                    ((PasswordRequest) request).setPassword("123456".toCharArray());
                } else if (request instanceof ExpirationRequest) {
                } else {
                    return false;
                }
            }
            return true;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {}
    }

    public KeyStoreListenerTest(String name) {
        super(name);
    }

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        authListener = new BusAuthListener();
        if ( System.getProperty("os.name").startsWith("Windows")) {
            assertEquals(Status.OK,
                    bus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener));
        } else if ( System.getProperty("java.vm.name").startsWith("Dalvik")) {
            /*
             * on some Android devices File.createTempFile trys to create a file in
             * a location we do not have permission to write to.  Resulting in a
             * java.io.IOException: Permission denied error.
             * This code assumes that the junit tests will have file IO permission
             * for /data/data/org.alljoyn.bus
             */
            assertEquals(Status.OK,
                    bus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener,
                            "/data/data/org.alljoyn.bus/files/alljoyn.ks"));
        } else {
            assertEquals(Status.OK,
                    bus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener,
                                             File.createTempFile("alljoyn", "ks").getAbsolutePath()));
        }

        service = new SecureService();
        assertEquals(Status.OK, bus.registerBusObject(service, "/secure"));

        otherBus = new BusAttachment(getClass().getName() + ".other");
        otherAuthListener = new BusAuthListener();
        if ( System.getProperty("os.name").startsWith("Windows")) {
            assertEquals(Status.OK,
                    otherBus.registerAuthListener("ALLJOYN_SRP_KEYX", otherAuthListener));
        } else if ( System.getProperty("java.vm.name").startsWith("Dalvik")) {
            /*
             * on some Android devices File.createTempFile trys to create a file in
             * a location we do not have permission to write to.  Resulting in a
             * java.io.IOException: Permission denied error.
             * This code assumes that the junit tests will have file IO permission
             * for /data/data/org.alljoyn.bus
             */
            assertEquals(Status.OK,
                    otherBus.registerAuthListener("ALLJOYN_SRP_KEYX", otherAuthListener,
                            "/data/data/org.alljoyn.bus/files/alljoyn_other.ks"));
        } else {
            assertEquals(Status.OK,
                    otherBus.registerAuthListener("ALLJOYN_SRP_KEYX", otherAuthListener,
                                                  File.createTempFile("alljoyn_other", "ks").getAbsolutePath()));
        }
    }

    /* Must register key store listener before connecting, so setUp is split into two functions. */
    public void setUp2() throws Exception {
        assertEquals(Status.OK, bus.connect());
        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                     control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                                         DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        assertEquals(Status.OK, otherBus.connect());
        ProxyBusObject proxyObj = otherBus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                                                             "/secure",
                                                             BusAttachment.SESSION_ID_ANY,
                                                             new Class<?>[] { SecureInterface.class });
        proxy = proxyObj.getInterface(SecureInterface.class);
    }

    public void tearDown() throws Exception {
        proxy = null;
        otherBus.disconnect();
        otherBus = null;

        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.ReleaseNameResult.Released,
                     control.ReleaseName("org.alljoyn.bus.BusAttachmentTest"));
        bus.disconnect();
        bus.unregisterBusObject(service);
        bus = null;
    }

    public void testInMemoryKeyStoreListener() throws Exception {
        InMemoryKeyStoreListener keyStoreListener = new InMemoryKeyStoreListener();
        bus.registerKeyStoreListener(keyStoreListener);
        InMemoryKeyStoreListener otherKeyStoreListener = new InMemoryKeyStoreListener();
        otherBus.registerKeyStoreListener(otherKeyStoreListener);
        setUp2();

        proxy.ping("hello");
        assertEquals("ALLJOYN_SRP_KEYX", authListener.authMechanismRequested);
        assertEquals("ALLJOYN_SRP_KEYX", otherAuthListener.authMechanismRequested);

        tearDown();
        setUp();
        bus.registerKeyStoreListener(keyStoreListener);
        otherBus.registerKeyStoreListener(otherKeyStoreListener);
        setUp2();

        proxy.ping("hello");
        assertEquals(null, authListener.authMechanismRequested);
        assertEquals(null, otherAuthListener.authMechanismRequested);
    }

    public void testDefaultKeyStoreListener() throws Exception {
        setUp2();
        /*
         * The first method call may result in an auth request depending on
         * whether the default key store has been configured already.  But the
         * second request should not result in a request.
         */
        proxy.ping("hello");
        authListener.authMechanismRequested = null;
        proxy.ping("hello");
        assertEquals(null, authListener.authMechanismRequested);
    }

    public void testClearKeyStore() throws Exception {
        InMemoryKeyStoreListener keyStoreListener = new InMemoryKeyStoreListener();
        bus.registerKeyStoreListener(keyStoreListener);
        InMemoryKeyStoreListener otherKeyStoreListener = new InMemoryKeyStoreListener();
        otherBus.registerKeyStoreListener(otherKeyStoreListener);
        setUp2();

        proxy.ping("hello");
        assertEquals("ALLJOYN_SRP_KEYX", authListener.authMechanismRequested);
        assertEquals("ALLJOYN_SRP_KEYX", otherAuthListener.authMechanismRequested);

        tearDown();
        setUp();
        setUp2();

        bus.registerKeyStoreListener(keyStoreListener);
        bus.clearKeyStore();
        otherBus.registerKeyStoreListener(otherKeyStoreListener);
        otherBus.clearKeyStore();
        proxy.ping("hello");
        assertEquals("ALLJOYN_SRP_KEYX", authListener.authMechanismRequested);
        assertEquals("ALLJOYN_SRP_KEYX", otherAuthListener.authMechanismRequested);
    }

    public void testGetSetKeyExpiration() throws Exception {
        InMemoryKeyStoreListener keyStoreListener = new InMemoryKeyStoreListener();
        bus.registerKeyStoreListener(keyStoreListener);
        InMemoryKeyStoreListener otherKeyStoreListener = new InMemoryKeyStoreListener();
        otherBus.registerKeyStoreListener(otherKeyStoreListener);
        setUp2();

        proxy.ping("hello");
        assertEquals("ALLJOYN_SRP_KEYX", authListener.authMechanismRequested);
        assertEquals("ALLJOYN_SRP_KEYX", otherAuthListener.authMechanismRequested);

        Mutable.StringValue peerGUID = new Mutable.StringValue();
        assertEquals(Status.OK, bus.getPeerGUID(otherBus.getUniqueName(), peerGUID));
        assertEquals(Status.OK, bus.setKeyExpiration(peerGUID.value, 1000));
        Mutable.IntegerValue expiration = new Mutable.IntegerValue();
        assertEquals(Status.OK, bus.getKeyExpiration(peerGUID.value, expiration));
        assertTrue(expiration.value <= 1000);
    }
}
