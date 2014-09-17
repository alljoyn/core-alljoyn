/**
 * Copyright (c) 2009-2011, 2013-2014, AllSeen Alliance. All rights reserved.
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

import junit.framework.TestCase;

public class SecurityViolationListenerTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment bus;
    private BusAttachment serviceBus;
    private SecureService service;
    private SimpleInterface proxy;

    public class SecureService implements SecureInterface, BusObject {
        public String ping(String str) { return str; }
    }

    public class BusAuthListener implements AuthListener {
        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
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

    public SecurityViolationListenerTest(String name) {
        super(name);
    }

    public void setUp() throws Exception {
        serviceBus = new BusAttachment(getClass().getName());
        serviceBus.registerKeyStoreListener(new NullKeyStoreListener());
        assertEquals(Status.OK, serviceBus.registerAuthListener("ALLJOYN_SRP_KEYX", new BusAuthListener()));
        service = new SecureService();
        assertEquals(Status.OK, serviceBus.registerBusObject(service, "/secure"));
        assertEquals(Status.OK, serviceBus.connect());
        DBusProxyObj control = serviceBus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                     control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                                         DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());
        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                                                        "/secure",
                                                        BusAttachment.SESSION_ID_ANY,
                                                        new Class<?>[] { SimpleInterface.class });
        proxy = proxyObj.getInterface(SimpleInterface.class);
    }

    public void tearDown() throws Exception {
        proxy = null;
        bus.disconnect();
        bus = null;

        DBusProxyObj control = serviceBus.getDBusProxyObj();
        assertEquals(DBusProxyObj.ReleaseNameResult.Released,
                     control.ReleaseName("org.alljoyn.bus.BusAttachmentTest"));
        serviceBus.disconnect();
        serviceBus.unregisterBusObject(service);
        serviceBus = null;
    }

    private int violation;

    /* ALLJOYN-74 */
    public void testSecurityViolation() throws Exception {
        violation = 0;
        serviceBus.registerSecurityViolationListener(new SecurityViolationListener() {
                public void violated(Status status) {
                    ++violation;
                    MessageContext ctx = bus.getMessageContext();
                    assertEquals(false, ctx.isUnreliable);
                    assertEquals("/secure", ctx.objectPath);
                    assertEquals("org.alljoyn.bus.SimpleInterface", ctx.interfaceName);
                    assertEquals("Ping", ctx.memberName);
                    assertEquals("s", ctx.signature);
                    assertEquals("", ctx.authMechanism);
                }
            });
        boolean thrown = false;
        try {
            proxy.ping("hello");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
        }
        assertTrue(thrown);
        assertEquals(1, violation);
    }
}
