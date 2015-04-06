/**
 * Copyright AllSeen Alliance. All rights reserved.
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

import java.lang.ref.WeakReference;
import java.util.ArrayList;

import junit.framework.TestCase;

public class AuthListenerTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment bus;
    private WeakReference<BusAttachment> busRef = new WeakReference<BusAttachment>(bus);
    private BusAttachment serviceBus;
    private WeakReference<BusAttachment> serviceBusRef = new WeakReference<BusAttachment>(serviceBus);
    private SecureService service;
    private SecureInterface proxy;
    private InsecureInterface insecureProxy;
    private BusAuthListener authListener;
    private BusAuthListener serviceAuthListener;
    private String logonEntry;
    private int abortCount;
    private int clientPasswordCount;
    private int servicePasswordCount;
    private TestKeyStoreListener serviceKeyStoreListener;
    private TestKeyStoreListener keyStoreListener;

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

    public class SecureService implements SecureInterface, InsecureInterface, BusObject {
        public String ping(String str) {
            assertTrue(bus.getMessageContext().authMechanism.length() > 0);
            return str;
        }
        public String insecurePing(String str) {
            return str;
        }
    }

    public class BusAuthListener implements AuthListener {
        protected String name;
        public String userName;
        public boolean setUserName;
        public char[] password;
        public String mechanism;
        public int count;
        public int completed;
        public ArrayList<AuthRequest> authRequests;

        public BusAuthListener(String name) {
            this.name = name;
            userName = "userName";
            setUserName = true;
            password = "123456".toCharArray();
            authRequests = new ArrayList<AuthRequest>();
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            /* Bail out if we get stuck in a loop. */
            assertTrue(count < 10);

            boolean succeeded = true;
            boolean expirationRequested = false;
            boolean verifyRequested = false;

            this.mechanism = mechanism;
            for (AuthRequest request : requests) {
                authRequests.add(request);
                if (request instanceof LogonEntryRequest) {
                    assertEquals(this.userName, userName);
                    this.count = count;
                    if (logonEntry != null) {
                        ((LogonEntryRequest) request).setLogonEntry(logonEntry.toCharArray());
                    }
                } else if (request instanceof PasswordRequest) {
                    this.count = count;
                    boolean setPassword = false;
                    if ("client".equals(name)) {
                        if (clientPasswordCount == 0) {
                            setPassword = true;
                        } else {
                            --clientPasswordCount;
                        }
                    } else if ("service".equals(name)) {
                        if ("ALLJOYN_SRP_LOGON".equals(mechanism)) {
                            assertEquals(this.userName, userName);
                        }
                        if (servicePasswordCount == 0) {
                            setPassword = true;
                        } else {
                            --servicePasswordCount;
                        }
                    } else if (logonEntry == null) {
                        setPassword = true;
                    }
                    if (setPassword) {
                        if (!setUserName) {
                            assertEquals(this.userName, userName);
                        }
                        ((PasswordRequest) request).setPassword(password);
                    }
                } else if (request instanceof UserNameRequest) {
                    assertEquals("", userName);
                    this.count = count;
                    if (setUserName) {
                        ((UserNameRequest) request).setUserName(this.userName);
                    }
                } else if (request instanceof VerifyRequest) {
                    assertEquals("", userName);
                    verifyRequested = true;
                } else if (request instanceof ExpirationRequest) {
                    expirationRequested = true;
                    ((ExpirationRequest) request).setExpiration(60 * 60 * 24 * 356); /* 1 year */
                } else {
                    succeeded = false;
                    break;
                }
            }

            /*
             * If verify is not requested then the expiration should have been requested,
             * and vice-versa.
             */
            assertTrue(verifyRequested != expirationRequested);

            return succeeded;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {
            ++completed;
        }
    }

    public class UserNameOnceAuthListener extends BusAuthListener {

        public UserNameOnceAuthListener(String name) {
            super(name);
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            boolean result = super.requested(mechanism, authPeer, count, userName, requests);
            setUserName = false;
            return result;
        }
    }

    public class RetryAuthListener extends BusAuthListener {

        public RetryAuthListener(String name) {
            super(name);
            password = "654321".toCharArray();
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            boolean result = super.requested(mechanism, authPeer, count, userName, requests);
            password = "123456".toCharArray();
            return result;
        }
    }

    public class UserNameAuthListener extends BusAuthListener {

        public UserNameAuthListener(String name) {
            super(name);
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            /* Bail out if we get stuck in a loop. */
            assertTrue(count < 10);

            this.mechanism = mechanism;
            for (AuthRequest request : requests) {
                if (request instanceof PasswordRequest) {
                    this.count = count;
                    if (userName.equals(this.userName)) {
                        ((PasswordRequest) request).setPassword(password);
                    }
                    return true;
                }
            }
            return false;
        }
    }

    public class RetryUserNameAuthListener extends BusAuthListener {

        public RetryUserNameAuthListener(String name) {
            super(name);
            userName = "notUser";
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            boolean result = super.requested(mechanism, authPeer, count, userName, requests);
            this.userName = "userName";
            return result;
        }
    }

    public class AbortAuthListener extends BusAuthListener {

        public AbortAuthListener(String name) {
            super(name);
            password = "654321".toCharArray();
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            /* Bail out if we get stuck in a loop. */
            assertTrue(count < 10);

            if (count <= abortCount) {
                return super.requested(mechanism, authPeer, count, userName, requests);
            } else {
                for (AuthRequest request : requests) {
                    if (request instanceof VerifyRequest) {
                    } else {
                        this.count = count;
                    }
                }
            }
            return false;
        }
    }

    public class AbortFirstMechanismAuthListener extends BusAuthListener {

        private String mechanism;

        public AbortFirstMechanismAuthListener(String name) {
            super(name);
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName,
                                 AuthRequest[] requests) {
            /* Bail out if we get stuck in a loop. */
            assertTrue(count < 10);

            if (this.mechanism == null) {
                this.mechanism = mechanism;
            }
            if (this.mechanism.equals(mechanism)) {
                password = "654321".toCharArray();
            } else {
                password = "123456".toCharArray();
            }

            if (count <= abortCount) {
                return super.requested(mechanism, authPeer, count, userName, requests);
            } else {
                for (AuthRequest request : requests) {
                    if (request instanceof VerifyRequest) {
                    } else {
                        this.count = count;
                    }
                }
            }
            return false;
        }
    }

    public AuthListenerTest(String name) {
        super(name);
    }

    public void setUp() throws Exception {
        serviceKeyStoreListener = new TestKeyStoreListener();
        keyStoreListener = new TestKeyStoreListener();
        setUp(serviceKeyStoreListener, keyStoreListener);
    }

    public void setUp(KeyStoreListener serviceKeyStoreListener,
                      KeyStoreListener keyStoreListener) throws Exception{
        serviceBus = new BusAttachment("receiver");
        serviceBus.registerKeyStoreListener(serviceKeyStoreListener);
        service = new SecureService();
        assertEquals(Status.OK, serviceBus.registerBusObject(service, "/secure"));
        assertEquals(Status.OK, serviceBus.connect());
        DBusProxyObj control = serviceBus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                     control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                                         DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        bus = new BusAttachment("sender");
        bus.registerKeyStoreListener(keyStoreListener);
        assertEquals(Status.OK, bus.connect());
        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                                                        "/secure",
                                                        BusAttachment.SESSION_ID_ANY,
                                                        new Class<?>[] { SecureInterface.class, InsecureInterface.class });
        proxy = proxyObj.getInterface(SecureInterface.class);
        insecureProxy = proxyObj.getInterface(InsecureInterface.class);
        abortCount = 3;
    }

    public void partTearDown() throws Exception {
        logonEntry = null;
        proxy = null;
        insecureProxy = null;

        bus.disconnect();
        bus = null;

        DBusProxyObj control = serviceBus.getDBusProxyObj();
        assertEquals(DBusProxyObj.ReleaseNameResult.Released,
                     control.ReleaseName("org.alljoyn.bus.BusAttachmentTest"));
        serviceBus.disconnect();
        serviceBus.unregisterBusObject(service);

        serviceBus = null;
        /*
         * Each BusAttachment is a very heavy object that creates many file
         * descripters for each BusAttachment.  This will force Java's Garbage
         * collector to remove the BusAttachments 'bus' and 'serviceBus' before
         * continuing on to the next test.
         */
        do{
            System.gc();
            Thread.sleep(5);
        } while (busRef.get() != null && serviceBusRef.get() != null);
    }
    public void tearDown() throws Exception {
        partTearDown();
        keyStoreListener = null;
        serviceKeyStoreListener = null;
    }

    public void doPing(String mechanism) throws Exception {
        assertEquals(Status.OK, serviceBus.registerAuthListener(mechanism, serviceAuthListener));
        assertEquals(Status.OK, bus.registerAuthListener(mechanism, authListener));
        BusException ex = null;
        try {
            proxy.ping("hello");
        } catch (BusException e) {
            ex = e;
        }
        /*
         * Make insecure second call to ensure that all the authentication transactions have run
         * their course on both sides.
         */
        insecureProxy.insecurePing("goodbye");
        if (ex != null) {
            throw ex;
        }
    }

    public void listener(String mechanism) throws Exception {
        serviceAuthListener = new BusAuthListener("service");
        authListener = new BusAuthListener("client");
        doPing(mechanism);
        assertEquals(mechanism, authListener.mechanism);
        assertEquals(1, authListener.count);
        assertEquals(1, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }

    public void retryClient(String mechanism) throws Exception {
        serviceAuthListener = new BusAuthListener("service");
        authListener = new RetryAuthListener("client");
        doPing(mechanism);
    }

    public void retryService(String mechanism) throws Exception {
        serviceAuthListener = new RetryAuthListener("service");
        authListener = new BusAuthListener("client");
        doPing(mechanism);
    }

    public void abortClient(String mechanism) throws Exception {
        serviceAuthListener = new BusAuthListener("service");
        authListener = new AbortAuthListener("client");
        boolean thrown = false;
        try {
            doPing(mechanism);
        } catch (BusException ex) {
            /*
             * we expect a BusException cause by org.alljoyn.Bus.ErStatus
             */
            thrown = true;
        }
        assertTrue(thrown);
    }

    public void abortService(String mechanism) throws Exception {
        serviceAuthListener = new AbortAuthListener("service");
        authListener = new BusAuthListener("client");
        boolean thrown = false;
        try {
            doPing(mechanism);
        } catch (BusException ex) {
            /*
             * we expect a BusException cause by org.alljoyn.Bus.ErStatus
             */
            thrown = true;
        }
        assertTrue(thrown);
    }

    public void testSrpListener() throws Exception {
        listener("ALLJOYN_SRP_KEYX");
    }
    public void testSrpRetryClient() throws Exception {
        retryClient("ALLJOYN_SRP_KEYX");
        assertEquals(2, authListener.count);
        assertEquals(2, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    public void testSrpRetryService() throws Exception {
        retryService("ALLJOYN_SRP_KEYX");
        assertEquals(2, authListener.count);
        assertEquals(2, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    public void testSrpAbortClient() throws Exception {
        abortClient("ALLJOYN_SRP_KEYX");
        assertEquals(4, authListener.count);
        assertEquals(4, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    public void testSrpAbortService() throws Exception {
        abortService("ALLJOYN_SRP_KEYX");
        assertEquals(3, authListener.count);
        assertEquals(4, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    public void testSrpAbortImmediatelyClient() throws Exception {
        abortCount = 0;
        abortClient("ALLJOYN_SRP_KEYX");
        assertEquals(1, authListener.count);
        assertEquals(1, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    public void testSrpAbortImmediatelyService() throws Exception {
        abortCount = 0;
        abortService("ALLJOYN_SRP_KEYX");
        assertEquals(0, authListener.count);
        assertEquals(1, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }

    public void testSrpLogonListenerPassword() throws Exception {
        listener("ALLJOYN_SRP_LOGON");
    }
    public void testSrpLogonListenerPasswordSeparately() throws Exception {
        clientPasswordCount = 1;
        listener("ALLJOYN_SRP_LOGON");
    }
    public void testSrpLogonListenerPasswordUserNameOnce() throws Exception {
        clientPasswordCount = 1;
        String mechanism = "ALLJOYN_SRP_LOGON";
        serviceAuthListener = new BusAuthListener("service");
        authListener = new UserNameOnceAuthListener("client");
        doPing(mechanism);
        assertEquals(mechanism, authListener.mechanism);
        assertEquals(1, authListener.count);
        assertEquals(1, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    public void testSrpLogonListenerLogonEntry() throws Exception {
        logonEntry = "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8EF4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA9AFD5138FE8376435B9FC61D2FC0EB06E3:02:9D446DE02A:825862685243B939BA032B712431FF2175F1F4B93F113DD114D542EC2FB8F4A908393F0D532A990CA8BE342FBBB30C16202B67B152CEE75856EFCA4F3F2ABC3179DA2F555741D85E8C17F90CE7B2CF9737AD7DE1BB871B90372B91B8906114DC4DFDF2D24265F124F2032E9A161AB281A05F84ED2FA5E2357A9B5DA4C4843260";
        listener("ALLJOYN_SRP_LOGON");
    }
    public void testSrpLogonRetryPasswordClient() throws Exception {
        retryClient("ALLJOYN_SRP_LOGON");
        assertEquals(2, authListener.count);
        assertEquals(1, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    /* ALLJOYN-75 */
    public void testSrpLogonRetryUserNameClient() throws Exception {
        serviceAuthListener = new UserNameAuthListener("service");
        authListener = new RetryUserNameAuthListener("client");
        doPing("ALLJOYN_SRP_LOGON");
        assertEquals(2, authListener.count);
        assertEquals(2, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    /* Service only gets asked for password once, so no service retry test */
    public void testSrpLogonAbortClient() throws Exception {
        abortClient("ALLJOYN_SRP_LOGON");
        assertEquals(4, authListener.count);
        assertEquals(1, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    public void testSrpLogonAbortService() throws Exception {
        abortCount = 0;
        abortService("ALLJOYN_SRP_LOGON");
        assertEquals(1, authListener.count);
        assertEquals(1, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }
    public void testSrpLogonAbortImmediatelyClient() throws Exception {
        abortCount = 0;
        abortClient("ALLJOYN_SRP_LOGON");
        assertEquals(1, authListener.count);
        assertEquals(0, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(0, serviceAuthListener.completed);
    }

    public void assertNewPasswordRequested(ArrayList<AuthListener.AuthRequest> authRequests) {
        boolean newPasswordRequested = false;
        for (AuthListener.AuthRequest request : authRequests) {
            if (request instanceof AuthListener.PasswordRequest
                && ((AuthListener.PasswordRequest) request).isNewPassword()) {
                newPasswordRequested = true;
                break;
            }
        }
        assertEquals(true, newPasswordRequested);
    }
    public void assertPasswordRequested(ArrayList<AuthListener.AuthRequest> authRequests) {
        boolean passwordRequested = false;
        for (AuthListener.AuthRequest request : authRequests) {
            if (request instanceof AuthListener.PasswordRequest
                && !((AuthListener.PasswordRequest) request).isNewPassword()
                && !((AuthListener.PasswordRequest) request).isOneTimePassword()) {
                passwordRequested = true;
                break;
            }
        }
        assertEquals(true, passwordRequested);
    }
    public int passwordRequests(ArrayList<AuthListener.AuthRequest> authRequests) {
        int requests = 0;
        for (AuthListener.AuthRequest request : authRequests) {
            if (request instanceof AuthListener.PasswordRequest) {
                ++requests;
            }
        }
        return requests;
    }

    /* TODO Only test abort of multiple mechanisms from client, service has no say in picking mechanism. */

    public void testAbortFirstMechanismClient() throws Exception {
        /* Authentication uses ALLJOYN_SRP_KEYX first regardless of order. */
        String authMechanisms = "ALLJOYN_SRP_KEYX ALLJOYN_SRP_LOGON";
        serviceAuthListener = new BusAuthListener("service");
        authListener = new AbortFirstMechanismAuthListener("client");
        assertEquals(Status.OK, serviceBus.registerAuthListener(authMechanisms, serviceAuthListener));
        assertEquals(Status.OK, bus.registerAuthListener(authMechanisms, authListener));
        boolean thrown = false;
        try {
            proxy.ping("hello");
        } catch (BusException ex) {
            /*
             * We don't expect to see a BusExpection for this test if the bus
             * exception  print a stacktrace to aid debugging.
             */
            ex.printStackTrace();
            thrown = true;
        }
        assertFalse(thrown);
        assertEquals(1, authListener.count);
        assertEquals(1, serviceAuthListener.count);
        assertEquals(1, authListener.completed);
        assertEquals(1, serviceAuthListener.completed);
    }

    /* ALLJOYN-25 */
    public void testAuthListenerUnknownName() throws Exception {
        serviceAuthListener = new BusAuthListener("service");
        authListener = new BusAuthListener("client");
        assertEquals(Status.OK, serviceBus.registerAuthListener("ALLJOYN_SRP_KEYX", serviceAuthListener));
        assertEquals(Status.OK, bus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener));
        ProxyBusObject proxyObj = bus.getProxyBusObject("unknown.name",
                                                        "/secure",
                                                        BusAttachment.SESSION_ID_ANY,
                                                        new Class<?>[] { SecureInterface.class });
        proxy = proxyObj.getInterface(SecureInterface.class);
        boolean thrown = false;
        try {
           proxy.ping("hello");
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
    }
}
