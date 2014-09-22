/**
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

import org.alljoyn.bus.ifaces.DBusProxyObj;
import java.lang.ref.WeakReference;

import junit.framework.TestCase;

public class AuthListenerECDHETest extends TestCase {
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


    public AuthListenerECDHETest(String name) {
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
        boolean daemonDebugEnabled = false;
        if (daemonDebugEnabled) {
            bus.useOSLogging(true);
            bus.setDaemonDebug("ALLJOYN_AUTH", 3);
            bus.setDebugLevel("ALLJOYN_AUTH", 3);
            bus.setDaemonDebug("CRYPTO", 3);
            bus.setDebugLevel("CRYPTO", 3);
            bus.setDaemonDebug("AUTH_KEY_EXCHANGER", 3);
            bus.setDebugLevel("AUTH_KEY_EXCHANGER", 3);
        }
        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                                                        "/secure",
                                                        BusAttachment.SESSION_ID_ANY,
                                                        new Class<?>[] { SecureInterface.class, InsecureInterface.class });
        proxy = proxyObj.getInterface(SecureInterface.class);
        insecureProxy = proxyObj.getInterface(InsecureInterface.class);
    }

    public void partTearDown() throws Exception {
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

    public void doPing(ECDHEKeyXListener serviceAuthListener, ECDHEKeyXListener clientAuthListener) throws Exception {
        assertEquals(Status.OK, serviceBus.registerAuthListener(serviceAuthListener.getMechanisms(), serviceAuthListener));
        assertEquals(Status.OK, bus.registerAuthListener(clientAuthListener.getMechanisms(), clientAuthListener));
        BusException ex = null;
        try {
            proxy.ping("hello");
        } catch (BusException e) {
            ex = e;
            // e.printStackTrace();
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

    class ECDHEKeyXListener implements AuthListener {

        public static final int SERVER_MODE = 0;
        public static final int CLIENT_MODE = 1;

        public ECDHEKeyXListener(int mode, String authList) {
            this(mode, true, authList);
        }
        public ECDHEKeyXListener(int mode, boolean sendBackKeys, String authList) {
            this.mode = mode;
            /* the keys can be stored in a key store */
            this.sendBackKeys = sendBackKeys;
            setMechanisms(authList);
            setPsk("679812");
        }
        /* Set the mechanisms */
        public void setMechanisms(String list) {
            mechanisms = list;
        }
        /* Returns the list of supported mechanisms. */
        public String getMechanisms() {
            return  mechanisms;
        }

        private String getName() {
            if (mode == SERVER_MODE) {
                return "server";
            }
            return "client";
        }
        /*
         * Authentication requests are being made.  Contained in this call are the mechanism in use,
         * the number of attempts made so far, the desired user name for the requests, and the
         * specific credentials being requested in the form of AuthRequests.
         *
         * The ALLJOYN_ECDHE_KEYX can request for private key, public key, and to verify the peer
         */
        @Override
        public boolean requested(String authMechanism, String authPeer, int count, String userName, AuthRequest[] requests) {
            try {
                System.out.println(getName() + ":requested with authMechanisms " + authMechanism + " and " + requests.length + " requests");
                if (!authMechanism.equals("ALLJOYN_ECDHE_NULL") &&
                    !authMechanism.equals("ALLJOYN_ECDHE_PSK") &&
                    !authMechanism.equals("ALLJOYN_ECDHE_ECDSA")) {
                    return false;
                }
                for (AuthRequest rqst: requests) {
                    if (rqst instanceof PrivateKeyRequest) {
                        System.out.println(getName() + ": got a PrivateKeyRequest");
                        if (sendBackKeys) {
                            PrivateKeyRequest pkRqst = (PrivateKeyRequest) rqst;
                            String privateKeyPEM;
                            if (mode == CLIENT_MODE) {
                                privateKeyPEM = CLIENT_PK_PEM;
                            }
                            else {
                                privateKeyPEM = SERVER_PK_PEM;
                            }
                            pkRqst.setPrivateKey(privateKeyPEM);
                            System.out.println(getName() + ": send back private key " + privateKeyPEM);
                        }
                    }
                    else if (rqst instanceof CertificateRequest) {
                        System.out.println(getName() + ": got a CertificateRequest");
                        if (sendBackKeys) {
                            String certChainPEM;
                            if (mode == CLIENT_MODE) {
                                certChainPEM = CLIENT_CERT2_PEM;
                            }
                            else {
                                certChainPEM = SERVER_CERT1_PEM;
                            }
                            CertificateRequest certChainRqst = (CertificateRequest) rqst;
                            certChainRqst.setCertificateChain(certChainPEM);
                            System.out.println(getName() + ": send back cert chain " + certChainPEM);
                        }
                    }
                    else if (rqst instanceof VerifyRequest) {
                        System.out.println(getName() + ": got a VerifyRequest");
                        peerVerificationCalled = true;
                        if (failVerification) {
                            return false;  /* auto fail verification */
                        }
                        VerifyRequest verifyRqst = (VerifyRequest) rqst;
                        String certPEM = verifyRqst.getCertificateChain();
                        System.out.println(getName() + ": verifying cert " + certPEM);
                        /* verify cert chain if possible */
                        System.out.println(getName() + ": verifying cert " + certPEM);
                        if (mode == CLIENT_MODE) {
                            peerCertVerified = certPEM.equals(SERVER_CERT1_PEM);
                        }
                        else {
                            peerCertVerified = certPEM.equals(CLIENT_CERT2_PEM);
                        }
                        System.out.println(getName() + ": verifying cert succeeds");
                    }
                    else if (rqst instanceof PasswordRequest) {
                        System.out.println(getName() + ": got a PasswordRequest");
                        if (sendBackKeys) {
                            System.out.println(getName() + ": send back PSK " + getPsk());
                            ((PasswordRequest) rqst).setPassword(getPsk().toCharArray());
                        }
                    }
                    else if (rqst instanceof ExpirationRequest) {
                        System.out.println(getName() + ": got an ExpirationRequest");
                        ExpirationRequest er = (ExpirationRequest) rqst;
                        er.setExpiration(100);  /* expired 100 seconds from now */
                    }
                    else {
                        System.out.println(getName() + ": got other Request" + rqst);
                    }
                }

                return true;

            } catch (Exception ex) {
            }
            return false;
        }

        @Override
        public void completed(String authMechanism, String authPeer, boolean authenticated) {
            System.out.println(getName() + ": completed called with authenticated " + authenticated);
            this.authenticated = authenticated;
            if (authenticated) {
                setAuthenticatedMechanism(authMechanism);
            }
        }

        public boolean isAuthenticated() {
            return authenticated;
        }
        public boolean isPeerCertVerified() {
            return peerCertVerified;
        }
        public boolean isPeerVerificationCalled() {
            return peerVerificationCalled;
        }
        public void setFailVerification(boolean flag) {
            failVerification = flag;
        }
        public String getPsk() {
            return psk;
        }
        public void setPsk(String psk) {
            this.psk = psk;
        }
        public String getAuthenticatedMechanism() {
            return authenticatedMechanism;
        }
        public void setAuthenticatedMechanism(String mech) {
            authenticatedMechanism = mech;
        }

        private int mode;
        private String mechanisms;
        private String psk;
        private boolean authenticated = false;
        private String authenticatedMechanism;
        private boolean sendBackKeys = true;
        private boolean peerCertVerified = false;
        private boolean peerVerificationCalled = false;
        private boolean failVerification = false;

        private static final String CLIENT_PK_PEM =
                "-----BEGIN PRIVATE KEY-----" +
                "CkzgQdvZSOQMmqOnddsw0BRneCNZhioNMyUoJwec9rMAAAAA" +
                "-----END PRIVATE KEY-----";

        private static final String CLIENT_CERT2_PEM =
        "-----BEGIN CERTIFICATE-----" +
        "AAAAAp1LKGlnpVVtV4Sa1TULsxGJR9C53Uq5AH3fxqxJjNdYAAAAAAobbdvBKaw9\n" +
        "eHox7o9fNbN5usuZw8XkSPSmipikYCPJAAAAAAAAAABiToQ8L3KZLwSCetlNJwfd\n" +
        "bbxbo2x/uooeYwmvXbH2uwAAAABFQGcdlcsvhdRxgI4SVziI4hbg2d2xAMI47qVB\n" +
        "ZZsqJAAAAAAAAAAAAAAAAAABYGEAAAAAAAFhjQCJ9dkuY0Z6jjx+a8azIQh4UF0h\n" +
        "8plX3uAhOlF2vT2jfxe5U06zaWSXcs9kBEQvfOeMM4sUtoXPArUA+TNahfOS9Bbf\n" +
        "0Hh08SvDJSDgM2OetQAAAAAYUr2pw2kb90fWblBWVKnrddtrI5Zs8BYx/EodpMrS\n" +
        "twAAAAA=\n" +
        "-----END CERTIFICATE-----";
        private static final String SERVER_PK_PEM =
                "-----BEGIN PRIVATE KEY-----" +
                "tV/tGPp7kI0pUohc+opH1LBxzk51pZVM/RVKXHGFjAcAAAAA" +
                "-----END PRIVATE KEY-----";
        private static final String SERVER_CERT1_PEM =
        "-----BEGIN CERTIFICATE-----" +
        "AAAAAfUQdhMSDuFWahMG/rFmFbKM06BjIA2Scx9GH+ENLAgtAAAAAIbhHnjAyFys\n" +
        "6DoN2kKlXVCgtHpFiEYszOYXI88QDvC1AAAAAAAAAAC5dRALLg6Qh1J2pVOzhaTP\n" +
        "xI+v/SKMFurIEo2b4S8UZAAAAADICW7LLp1pKlv6Ur9+I2Vipt5dDFnXSBiifTmf\n" +
        "irEWxQAAAAAAAAAAAAAAAAABXLAAAAAAAAFd3AABMa7uTLSqjDggO0t6TAgsxKNt\n" +
        "+Zhu/jc3s242BE0drPcL4K+FOVJf+tlivskovQ3RfzTQ+zLoBH5ZCzG9ua/dAAAA\n" +
        "ACt5bWBzbcaT0mUqwGOVosbMcU7SmhtE7vWNn/ECvpYFAAAAAA==\n" +
        "-----END CERTIFICATE-----";
    }

    class ServerECDHEKeyXListener extends ECDHEKeyXListener {
        public ServerECDHEKeyXListener(boolean sendBackKeys, String authList) {
            super(SERVER_MODE, sendBackKeys, authList);
        }
    }
    class ClientECDHEKeyXListener extends ECDHEKeyXListener {
        public ClientECDHEKeyXListener(boolean sendBackKeys, String authList) {
            super(CLIENT_MODE, sendBackKeys, authList);
        }
    }

    public void testECDHESuccessNULL() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(false, "ALLJOYN_ECDHE_NULL");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(false, "ALLJOYN_ECDHE_NULL");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
        assertFalse(serviceAuthListener.isPeerVerificationCalled());
        assertFalse(clientAuthListener.isPeerVerificationCalled());
    }
    public void testECDHESuccessPSK_SendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFailPSK_DoNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(false, "ALLJOYN_ECDHE_PSK");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(false, "ALLJOYN_ECDHE_PSK");
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {}
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFailPSK_ServerNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(false, "ALLJOYN_ECDHE_PSK");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {}
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFailPSK_ClientNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(false, "ALLJOYN_ECDHE_PSK");
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {}
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFailPSK_DifferentSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        serviceAuthListener.setPsk("123456");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        clientAuthListener.setPsk("654321");
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {}
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHESuccessECDSA_SendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
        assertTrue(serviceAuthListener.isPeerVerificationCalled());
        assertTrue(clientAuthListener.isPeerVerificationCalled());
        assertTrue(serviceAuthListener.isPeerCertVerified());
        assertTrue(clientAuthListener.isPeerCertVerified());
    }
    public void testECDHESuccessECDSA_DoNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(false, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(false, "ALLJOYN_ECDHE_ECDSA");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
        assertFalse(serviceAuthListener.isPeerVerificationCalled());
        assertFalse(clientAuthListener.isPeerVerificationCalled());
        assertFalse(serviceAuthListener.isPeerCertVerified());
        assertFalse(clientAuthListener.isPeerCertVerified());
    }

    public void testECDHESuccessECDSA_ClientNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(false, "ALLJOYN_ECDHE_ECDSA");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
        assertFalse(serviceAuthListener.isPeerVerificationCalled());
        assertTrue(clientAuthListener.isPeerVerificationCalled());
        assertFalse(serviceAuthListener.isPeerCertVerified());
        assertTrue(clientAuthListener.isPeerCertVerified());
    }
    public void testECDHESuccessECDSA_ServerNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(false, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
        assertTrue(serviceAuthListener.isPeerVerificationCalled());
        assertFalse(clientAuthListener.isPeerVerificationCalled());
        assertTrue(serviceAuthListener.isPeerCertVerified());
        assertFalse(clientAuthListener.isPeerCertVerified());
    }
    public void testECDHESuccessPSK_ECDSA_SendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
        assertTrue(serviceAuthListener.isPeerVerificationCalled());
        assertTrue(clientAuthListener.isPeerVerificationCalled());
        assertTrue(serviceAuthListener.isPeerCertVerified());
        assertTrue(clientAuthListener.isPeerCertVerified());
        assertEquals(serviceAuthListener.getAuthenticatedMechanism(), "ALLJOYN_ECDHE_ECDSA");
        assertEquals(clientAuthListener.getAuthenticatedMechanism(), "ALLJOYN_ECDHE_ECDSA");
    }
    public void testECDHESuccess_NULL_PSK_ECDSA_SendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
        assertTrue(serviceAuthListener.isPeerVerificationCalled());
        assertTrue(clientAuthListener.isPeerVerificationCalled());
        assertTrue(serviceAuthListener.isPeerCertVerified());
        assertTrue(clientAuthListener.isPeerCertVerified());
        assertEquals(serviceAuthListener.getAuthenticatedMechanism(), "ALLJOYN_ECDHE_ECDSA");
        assertEquals(clientAuthListener.getAuthenticatedMechanism(), "ALLJOYN_ECDHE_ECDSA");
    }
    public void testECDHESuccess_NULL_PSK_SendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK");
        doPing(serviceAuthListener, clientAuthListener);
        assertTrue(serviceAuthListener.isAuthenticated());
        assertTrue(clientAuthListener.isAuthenticated());
        assertEquals(serviceAuthListener.getAuthenticatedMechanism(), "ALLJOYN_ECDHE_PSK");
        assertEquals(clientAuthListener.getAuthenticatedMechanism(), "ALLJOYN_ECDHE_PSK");
        assertFalse(serviceAuthListener.isPeerVerificationCalled());
        assertFalse(clientAuthListener.isPeerVerificationCalled());
    }
    public void testECDHEFail_NULL_PSK_SendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_NULL");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        try {
            doPing(serviceAuthListener, clientAuthListener);
            fail("Expected to fail but succeeded.");
        }
        catch (Exception e) {
        }
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFail_NULL_ECDSA() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_NULL");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        try {
            doPing(serviceAuthListener, clientAuthListener);
            fail("Expected to fail but succeeded.");
        }
        catch (Exception e) {
        }
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFail_ECDSA_PSK() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        try {
            doPing(serviceAuthListener, clientAuthListener);
            fail("Expected to fail but succeeded.");
        }
        catch (Exception e) {
        }
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFail_ECDSA_NULL() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_NULL");
        try {
            doPing(serviceAuthListener, clientAuthListener);
            fail("Expected to fail but succeeded.");
        }
        catch (Exception e) {
        }
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
}
