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
            /*
             * In this example, the pre shared secret is a hard coded string. 
             * Pre-shared keys should be 128 bits long, and generated with a
             * cryptographically secure random number generator.
             */
            setPsk("faaa0af3dd3f1e0379da046a3ab6ca44");
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
                                certChainPEM = CLIENT_ECC_X509_PEM;
                            }
                            else {
                                certChainPEM = SERVER_ECC_X509_PEM;
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
                            peerCertVerified = certPEM.equals(SERVER_ECC_X509_PEM);
                            if (!peerCertVerified) {
                                System.out.println(getName() + ": verifying cert " + certPEM + " against server PEM " + SERVER_ECC_X509_PEM + " failed");
                                
                            }
                        }
                        else {
                            peerCertVerified = certPEM.equals(CLIENT_ECC_X509_PEM);
                            if (!peerCertVerified) {
                                System.out.println(getName() + ": verifying cert " + certPEM + " against client PEM " + CLIENT_ECC_X509_PEM + " failed");
                                
                            }
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
            System.out.println(getName() + ": completed for mechanism " + authMechanism + " with authenticated " + authenticated);
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

        /* the key and certificate is generated using openssl */
        private static final String CLIENT_PK_PEM =
        "-----BEGIN EC PRIVATE KEY-----\n" +
        "MHcCAQEEIBiLw29bf669g7MxMbXK2u8Lp5//w7o4OiVGidJdKAezoAoGCCqGSM49\n" +
        "AwEHoUQDQgAE+A0C9YTghZ1vG7198SrUHxFlhtbSsmhbwZ3N5aQRwzFXWcCCm38k\n" +
        "OzJEmS+venmF1o/FV0W80Mcok9CWlV2T6A==\n" +
        "-----END EC PRIVATE KEY-----";
        private static final String CLIENT_ECC_X509_PEM =
        "-----BEGIN CERTIFICATE-----\n" +
        "MIIBYTCCAQigAwIBAgIJAOVrhhJOre/7MAoGCCqGSM49BAMCMCQxIjAgBgNVBAoM\n" +
        "GUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwODI0MjAxODQ1WhcNMjkw\n" +
        "NTAyMjAxODQ1WjAgMR4wHAYDVQQKDBVBbGxKb3luVGVzdENsaWVudE5hbWUwWTAT\n" +
        "BgcqhkjOPQIBBggqhkjOPQMBBwNCAAT4DQL1hOCFnW8bvX3xKtQfEWWG1tKyaFvB\n" +
        "nc3lpBHDMVdZwIKbfyQ7MkSZL696eYXWj8VXRbzQxyiT0JaVXZPooycwJTAVBgNV\n" +
        "HSUEDjAMBgorBgEEAYLefAEBMAwGA1UdEwEB/wQCMAAwCgYIKoZIzj0EAwIDRwAw\n" +
        "RAIgevLUXoJBgUr6nVepBHQiv85CGuxu00V4uoARbH6qu1wCIA54iDRh6wit1zbP\n" +
        "kqkBC015LjxucTf3Y7lNGhXuZRsL\n" +
        "-----END CERTIFICATE-----\n" + 
        "-----BEGIN CERTIFICATE-----\n" +
        "MIIBdTCCARugAwIBAgIJAJTFhmdwDWsvMAoGCCqGSM49BAMCMCQxIjAgBgNVBAoM\n" +
        "GUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwODI0MjAxODQ1WhcNMjkw\n" +
        "NTAyMjAxODQ1WjAkMSIwIAYDVQQKDBlBbGxKb3luVGVzdFNlbGZTaWduZWROYW1l\n" +
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEF0nZmkzuK/2CVf7udexLZnlEB5D+\n" +
        "DBsx3POtsRyZWm2QiI1untDTp0uYp51tkP6wI6Gi5gWxB+86lEIPg4ZpTaM2MDQw\n" +
        "IQYDVR0lBBowGAYKKwYBBAGC3nwBAQYKKwYBBAGC3nwBBTAPBgNVHRMBAf8EBTAD\n" +
        "AQH/MAoGCCqGSM49BAMCA0gAMEUCIQDPQ1VRvdBhhneU5e7OvIFHK3d9XPZA7Fw6\n" +
        "VyeW/P5wIAIgD969ks/z9vQ1yCaVaxmVz63toC1ggp4AnBXqbDy8O+4=\n" +
        "-----END CERTIFICATE-----";
        /* these keys were generated by the unit test common/unit_test/CertificateECCTest.GenSelfSignECCX509CertForBBservice */
        private static final String SERVER_PK_PEM =
        "-----BEGIN EC PRIVATE KEY-----\n" +
        "MDECAQEEII9KTdqfje8CzLnQ0asUaVdW1hmjQrP1AINRd03fdTAloAoGCCqGSM49\n" +
        "AwEH\n" +
        "-----END EC PRIVATE KEY-----";

        private static final String SERVER_ECC_X509_PEM =
        "-----BEGIN CERTIFICATE-----\n" +
        "MIIBtTCCAVugAwIBAgIHMTAxMDEwMTAKBggqhkjOPQQDAjBCMRUwEwYDVQQLDAxv\n" +
        "cmdhbml6YXRpb24xKTAnBgNVBAMMIDAyMzRjYjFhMmRlYmY5OTBmZGQ0NGJmZjlh\n" +
        "MjhjZDUxMB4XDTE1MDgyMTIyMjEwNloXDTI1MDgxODIyMjEwNlowQjEVMBMGA1UE\n" +
        "CwwMb3JnYW5pemF0aW9uMSkwJwYDVQQDDCBjN2UzYmRmNzhlOWRmNmRkOTk3N2Q4\n" +
        "NGUwMTE0MDYwMzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABEjL84d4YPMMH5g9\n" +
        "kq5ErdTO4ouaGWLz7ikerB+HLPAu+aTekMxoA7uIejr5iekcWXxryJIxFwsdGh34\n" +
        "hl9YQKGjPDA6MAwGA1UdEwQFMAMBAf8wFQYDVR0lBA4wDAYKKwYBBAGC3nwBATAT\n" +
        "BgNVHSMEDDAKoAhCmX6GR09OkTAKBggqhkjOPQQDAgNIADBFAiEA026qDcGfTife\n" +
        "D75dbuT6MFz+sT8xuEkYNxyshK8vBMECIAa5lvbBATlTHSrb0FHg+jwFnQ2yYUE/\n" +
        "aFFhPrtFbwHn\n" +
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
        serviceAuthListener.setPsk("faaa0af3dd3f1e0379da046a3ab6ca44");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_PSK");
        clientAuthListener.setPsk("faaa0af3dd3f1e0379da046a3ab6ca45");
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
    public void testECDHEFailECDSA_DoNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(false, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(false, "ALLJOYN_ECDHE_ECDSA");
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {
        }
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }

    public void testECDHEFailECDSA_ClientNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(false, "ALLJOYN_ECDHE_ECDSA");
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {
        }
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFailECDSA_ServerNotSendKeys() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(false, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {
        }
        assertFalse(clientAuthListener.isAuthenticated());
    }

    public void testECDHEFailECDSA_ServerFailVerification() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        serviceAuthListener.setFailVerification(true);
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {
        }
        assertFalse(serviceAuthListener.isAuthenticated());
        assertFalse(clientAuthListener.isAuthenticated());
    }
    public void testECDHEFailECDSA_ClientFailVerification() throws Exception {
        ECDHEKeyXListener serviceAuthListener = new ServerECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        ECDHEKeyXListener clientAuthListener = new ClientECDHEKeyXListener(true, "ALLJOYN_ECDHE_ECDSA");
        clientAuthListener.setFailVerification(true);
        try {
            doPing(serviceAuthListener, clientAuthListener);
        }
        catch (Exception e) {
        }
        assertFalse(clientAuthListener.isAuthenticated());
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
