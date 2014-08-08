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
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ifaces.DBusProxyObj;

import junit.framework.TestCase;

public class MultipleAuthListenersTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment serviceBus;
    private SecureService service;
    private BusAuthListener serviceAuthListener;
    private BusAttachment clientBus;
    private BusAuthListener clientAuthListener;
    private SecureInterface proxy;

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
                } else if (request instanceof CertificateRequest) {
                } else if (request instanceof PrivateKeyRequest) {
                } else if (request instanceof VerifyRequest) {
                } else if (request instanceof ExpirationRequest) {
                } else {
                    return false;
                }
            }
            return true;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {}

        public String getAuthMechanismRequested() { return authMechanismRequested; }
    }

    public MultipleAuthListenersTest(String name) {
        super(name);
    }

    public void setUp() throws Exception {
        serviceBus = new BusAttachment(getClass().getName());
        serviceBus.registerKeyStoreListener(new NullKeyStoreListener());
        service = new SecureService();
        assertEquals(Status.OK, serviceBus.registerBusObject(service, "/secure"));
        assertEquals(Status.OK, serviceBus.connect());
        DBusProxyObj control = serviceBus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                     control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                                         DBusProxyObj.REQUEST_NAME_NO_FLAGS));
        serviceAuthListener = new BusAuthListener();

        clientBus = new BusAttachment(getClass().getName());
        clientBus.registerKeyStoreListener(new NullKeyStoreListener());
        assertEquals(Status.OK, clientBus.connect());
        clientAuthListener = new BusAuthListener();
        ProxyBusObject proxyObj = clientBus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                                                              "/secure",
                                                              BusAttachment.SESSION_ID_ANY,
                                                              new Class<?>[] { SecureInterface.class });
        proxy = proxyObj.getInterface(SecureInterface.class);
    }

    public void tearDown() throws Exception {
        proxy = null;
        clientBus.disconnect();
        clientBus = null;

        DBusProxyObj control = serviceBus.getDBusProxyObj();
        assertEquals(DBusProxyObj.ReleaseNameResult.Released,
                     control.ReleaseName("org.alljoyn.bus.BusAttachmentTest"));
        serviceBus.disconnect();
        serviceBus.unregisterBusObject(service);
        serviceBus = null;
    }

    public void testSrpAndRsaAuthListeners() throws Exception {
        assertEquals(Status.OK, serviceBus.registerAuthListener("ALLJOYN_SRP_KEYX ALLJOYN_RSA_KEYX",
                                                                serviceAuthListener));
        assertEquals(Status.OK, clientBus.registerAuthListener("ALLJOYN_SRP_KEYX ALLJOYN_RSA_KEYX",
                                                               clientAuthListener));
        proxy.ping("hello");
        assertEquals(serviceAuthListener.getAuthMechanismRequested(),
                     clientAuthListener.getAuthMechanismRequested());
    }

    public void testNoCommonAuthMechanism() throws Exception {
        assertEquals(Status.OK, serviceBus.registerAuthListener("ALLJOYN_RSA_KEYX", serviceAuthListener));
        assertEquals(Status.OK, clientBus.registerAuthListener("ALLJOYN_SRP_KEYX", clientAuthListener));
        boolean thrown = false;
        try {
            proxy.ping("hello");
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
    }
}
