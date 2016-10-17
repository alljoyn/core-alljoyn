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

import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.HashMap;
import java.io.File;
import java.nio.ByteBuffer;
import java.util.UUID;
import java.lang.IllegalArgumentException;

import junit.framework.TestCase;

import org.alljoyn.bus.annotation.BusSignalHandler;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Introspectable;
import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.CryptoECC;
import org.alljoyn.bus.PermissionConfigurator.ApplicationState;
import org.alljoyn.bus.BusAttachment.RemoteMessage;
import org.alljoyn.bus.EchoChirpInterface;

public class SecurityManagementTest extends TestCase {
    private static final long expiredInSecs = 3600;
    private BusAttachment managerBus;
    private BusAttachment peer1Bus;
    private BusAttachment peer2Bus;
    private Map<String, ApplicationState> stateMap;
    private String managerBusUniqueName;
    private String peer1BusUniqueName;
    private String peer2BusUniqueName;
    /* Key: busName, Value: sessionId */
    private Map<String,Integer> hostedSessions;

    private PermissionConfigListenerImpl managerPclistener;
    private PermissionConfigListenerImpl peer1Pclistener;
    private PermissionConfigListenerImpl peer2Pclistener;

    private int receivedEcho = 0;
    private int receivedChirp = 0;
    private int receivedGetProp1 = 0;
    private int receivedSetProp1 = 0;
    private int receivedGetProp2 = 0;
    private int receivedSetProp2 = 0;

    private static final String defaultManifestXml = "<manifest>" +
        "<node>" +
        "<interface>" +
        "<any>" +
        "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
        "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>" +
        "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" +
        "</any>" +
        "</interface>" +
        "</node>" +
        "</manifest>";

    private static final String ecdsaPrivateKeyPEM =
        "-----BEGIN EC PRIVATE KEY-----\n" +
        "MDECAQEEIICSqj3zTadctmGnwyC/SXLioO39pB1MlCbNEX04hjeioAoGCCqGSM49\n" +
        "AwEH\n" +
        "-----END EC PRIVATE KEY-----";

    private static final String ecdsaCertChainX509PEM =
        "-----BEGIN CERTIFICATE-----\n" +
        "MIIBWjCCAQGgAwIBAgIHMTAxMDEwMTAKBggqhkjOPQQDAjArMSkwJwYDVQQDDCAw\n" +
        "ZTE5YWZhNzlhMjliMjMwNDcyMGJkNGY2ZDVlMWIxOTAeFw0xNTAyMjYyMTU1MjVa\n" +
        "Fw0xNjAyMjYyMTU1MjVaMCsxKTAnBgNVBAMMIDZhYWM5MjQwNDNjYjc5NmQ2ZGIy\n" +
        "NmRlYmRkMGM5OWJkMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEP/HbYga30Afm\n" +
        "0fB6g7KaB5Vr5CDyEkgmlif/PTsgwM2KKCMiAfcfto0+L1N0kvyAUgff6sLtTHU3\n" +
        "IdHzyBmKP6MQMA4wDAYDVR0TBAUwAwEB/zAKBggqhkjOPQQDAgNHADBEAiAZmNVA\n" +
        "m/H5EtJl/O9x0P4zt/UdrqiPg+gA+wm0yRY6KgIgetWANAE2otcrsj3ARZTY/aTI\n" +
        "0GOQizWlQm8mpKaQ3uE=\n" +
        "-----END CERTIFICATE-----";

    private SessionOpts defaultSessionOpts;
    private Mutable.ShortValue defaultSessionPort;
    private Mutable.IntegerValue managerToManagerSessionId;
    private Mutable.IntegerValue managerToPeer1SessionId;
    private Mutable.IntegerValue managerToPeer2SessionId;

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class EchoChirpService implements BusObject, EchoChirpInterface {
        private int prop1 = 1;
        private int prop2 = 2;

        public String echo(String shout) throws BusException {
            receivedEcho++;
            return shout;
        }

        public void chirp(String tweet) throws BusException {
            receivedChirp++;
        }

        public int getProp1() throws BusException {
            receivedGetProp1++;
            return prop1;
        }

        public void setProp1(int arg) throws BusException  {
            receivedSetProp1++;
            prop1 = arg;
        }

        public int getProp2() throws BusException {
            receivedGetProp2++;
            return prop2;
        }

        public void setProp2(int arg) throws BusException  {
            receivedSetProp2++;
            prop2 = arg;
        }
    }

    @Override
    public void setUp() throws Exception {
        stateMap = new HashMap<String, ApplicationState>();
        hostedSessions = new HashMap<String,Integer>();
        defaultSessionOpts = new SessionOpts();
        defaultSessionPort = new Mutable.ShortValue((short) 42);
        managerToManagerSessionId = new Mutable.IntegerValue((short) 0);
        managerToPeer1SessionId = new Mutable.IntegerValue((short) 0);
        managerToPeer2SessionId = new Mutable.IntegerValue((short) 0);
    }

    @Override
    public void tearDown() throws Exception {
        if (peer1Bus != null) {
            if (peer1Bus.isConnected()) {
                peer1Bus.disconnect();
            }
            peer1Bus.release();
            peer1Bus= null;
        }
        peer1Pclistener = null;

        if (peer2Bus != null) {
            if (peer2Bus.isConnected()) {
                peer2Bus.disconnect();
            }
            peer2Bus.release();
            peer2Bus= null;
        }
        peer2Pclistener = null;

        if (managerBus != null) {
            if (managerBus.isConnected()) {
                managerBus.disconnect();
            }
            managerBus.release();
            managerBus = null;
        }
        managerPclistener = null;
    }

    /**
     * copied and interpreted from core SecurityManagementTest.cc
     */
    public void testBasic() throws Exception {
        System.out.println("Start testBasic()");

        // Create manager bus attachment, connect, and register auth listener
        managerBus = new BusAttachment(getClass().getName() + "Manager", RemoteMessage.Receive);
        managerBus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, managerBus.connect());
        managerPclistener = new PermissionConfigListenerImpl("manager");
        assertEquals(Status.OK, registerAuthListener(managerBus, "alljoyn",
            "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", new AuthListenerImpl("manager"), managerPclistener));

        // Create peer1 bus attachment, connect, and register auth listener
        peer1Bus = new BusAttachment(getClass().getName() + "Peer1", RemoteMessage.Receive);
        peer1Bus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, peer1Bus.connect());
        peer1Pclistener = new PermissionConfigListenerImpl("peer1");
        assertEquals(Status.OK, registerAuthListener(peer1Bus, "alljoynpb1",
            "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", new AuthListenerImpl("peer1"), peer1Pclistener));

        // Register bus object on peer1 bus
        EchoChirpService service = new EchoChirpService();
        assertEquals(Status.OK, peer1Bus.registerBusObject(service, "/service", false));  // secure=false 

        // Create peer2 bus attachment, connect, and register auth listener
        peer2Bus = new BusAttachment(getClass().getName() + "Peer2", RemoteMessage.Receive);
        peer2Bus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, peer2Bus.connect());
        peer2Pclistener = new PermissionConfigListenerImpl("peer2");
        assertEquals(Status.OK, registerAuthListener(peer2Bus, "alljoynpb2",
            "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", new AuthListenerImpl("peer2"), peer2Pclistener));

        if (true) {
            peer2Bus.useOSLogging(true);
            peer2Bus.setDaemonDebug("ALLJOYN_AUTH", 7);
            peer2Bus.setDebugLevel("ALLJOYN_AUTH", 7);
            peer2Bus.setDaemonDebug("CRYPTO", 7);
            peer2Bus.setDebugLevel("CRYPTO", 7);
            peer2Bus.setDaemonDebug("AUTH_KEY_EXCHANGER", 7);
            peer2Bus.setDebugLevel("AUTH_KEY_EXCHANGER", 7);
        }

        managerBusUniqueName = managerBus.getUniqueName();
        peer1BusUniqueName = peer1Bus.getUniqueName();
        peer2BusUniqueName = peer2Bus.getUniqueName();

        assertEquals(Status.OK, managerBus.registerApplicationStateListener(appStateListener));

        // Set permission configuator claim capabilities
        managerBus.getPermissionConfigurator().setClaimCapabilities(
            (short) (PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL));
        managerBus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);

        peer1Bus.getPermissionConfigurator().setClaimCapabilities(
            (short) (PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL));
        peer1Bus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);

        peer2Bus.getPermissionConfigurator().setClaimCapabilities(
            (short) (PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL));
        peer2Bus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);

        // bind ports
        assertEquals(Status.OK, managerBus.bindSessionPort(defaultSessionPort, defaultSessionOpts,
                new SecuritySessionPortListener()));
        assertEquals(Status.OK, peer1Bus.bindSessionPort(defaultSessionPort, defaultSessionOpts,
                new SecuritySessionPortListener()));
        assertEquals(Status.OK, peer2Bus.bindSessionPort(defaultSessionPort, defaultSessionOpts,
                new SecuritySessionPortListener()));

        // join sessions
        assertEquals(Status.OK, managerBus.joinSession(managerBusUniqueName, defaultSessionPort.value,
                managerToManagerSessionId, defaultSessionOpts, new SessionListener()));
        assertEquals(Status.OK, managerBus.joinSession(peer1BusUniqueName, defaultSessionPort.value,
                managerToPeer1SessionId, defaultSessionOpts, new SessionListener()));
        assertEquals(Status.OK, managerBus.joinSession(peer2BusUniqueName, defaultSessionPort.value,
                managerToPeer2SessionId, defaultSessionOpts, new SessionListener()));

        // verify manager and peer configurators are claimable
        SecurityApplicationProxy sapWithManager = new SecurityApplicationProxy(managerBus, managerBusUniqueName,
                managerToManagerSessionId.value);
        assertEquals(ApplicationState.CLAIMABLE, sapWithManager.getApplicationState());

        SecurityApplicationProxy sapWithPeer1 = new SecurityApplicationProxy(managerBus, peer1BusUniqueName,
                managerToPeer1SessionId.value);
        assertEquals(ApplicationState.CLAIMABLE, sapWithPeer1.getApplicationState());

        SecurityApplicationProxy sapWithPeer2 = new SecurityApplicationProxy(managerBus, peer2BusUniqueName,
                managerToPeer2SessionId.value);
        assertEquals(ApplicationState.CLAIMABLE, sapWithPeer2.getApplicationState());

//PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]) ???

        // Get manager key
        PermissionConfigurator managerConfigurator = managerBus.getPermissionConfigurator();
        assertEquals(ApplicationState.CLAIMABLE, managerConfigurator.getApplicationState());
        KeyInfoNISTP256 managerPublicKey = managerConfigurator.getSigningPublicKey();

        // Get peer1 key
        PermissionConfigurator pcPeer1 = peer1Bus.getPermissionConfigurator();
        assertEquals(ApplicationState.CLAIMABLE, pcPeer1.getApplicationState());
        KeyInfoNISTP256 peer1Key = pcPeer1.getSigningPublicKey();

        // Get peer2 key
        PermissionConfigurator pcPeer2 = peer2Bus.getPermissionConfigurator();
        assertEquals(ApplicationState.CLAIMABLE, pcPeer2.getApplicationState());
        KeyInfoNISTP256 peer2Key = pcPeer2.getSigningPublicKey();

        UUID issuerUUID = UUID.randomUUID();
        ByteBuffer issuerCN = ByteBuffer.wrap(new byte[16]);
        issuerCN.putLong(issuerUUID.getMostSignificantBits());
        issuerCN.putLong(issuerUUID.getLeastSignificantBits());

        // Create identityCert for manager, sign, and claim
        CertificateX509[] certChainMaster = new CertificateX509[1];
        createIdentityCert(managerBus, "0", issuerCN, issuerCN, managerPublicKey, "ManagerAlias", expiredInSecs, certChainMaster);

        CryptoECC cryptoECC = new CryptoECC();
        cryptoECC.generateDSAKeyPair();
        String signedManifestXml = SecurityApplicationProxy.signManifest(certChainMaster[0], cryptoECC.getDSAPrivateKey(),
                defaultManifestXml);

        String[] signedManifestXmls = new String[] {signedManifestXml};
        managerConfigurator.claim(managerPublicKey, issuerUUID, managerPublicKey, certChainMaster, signedManifestXmls);

        for (int ms = 0; ms < 2000; ms++) {
            Thread.sleep(5);
            if (stateMap.get(managerBusUniqueName) == ApplicationState.CLAIMED) {
                break;
            }
        }
        assertEquals(stateMap.get(managerBusUniqueName), ApplicationState.CLAIMED);

        // Create identityCert for peer1, sign, and claim
        CertificateX509[] certChainPeer1 = new CertificateX509[1];
        createIdentityCert(managerBus, "0", issuerCN, issuerCN, peer1Key, "Peer1Alias", expiredInSecs, certChainPeer1);

        String signedManifestXmlPeer1 = SecurityApplicationProxy.signManifest(certChainPeer1[0], cryptoECC.getDSAPrivateKey(),
                defaultManifestXml);

        String [] signedManifestXmlsPeer1 = new String[] {signedManifestXmlPeer1};
        sapWithPeer1.claim(managerPublicKey, issuerUUID, managerPublicKey, certChainPeer1, signedManifestXmlsPeer1);

        for (int ms = 0; ms < 2000; ms++) {
            Thread.sleep(5);
            if (stateMap.get(peer1BusUniqueName) == ApplicationState.CLAIMED) {
                break;
            }
        }
        assertEquals(stateMap.get(peer1BusUniqueName), ApplicationState.CLAIMED);

        // Create identityCert for peer2, sign, and claim
        CertificateX509[] certChainPeer2 = new CertificateX509[1];
        createIdentityCert(managerBus, "0", issuerCN, issuerCN, peer2Key, "Peer2Alias", expiredInSecs, certChainPeer2);

        String signedManifestXmlPeer2 = SecurityApplicationProxy.signManifest(certChainPeer2[0], cryptoECC.getDSAPrivateKey(),
                defaultManifestXml);

        String [] signedManifestXmlsPeer2 = new String[] {signedManifestXmlPeer2};
        sapWithPeer2.claim(managerPublicKey, issuerUUID, managerPublicKey, certChainPeer2, signedManifestXmlsPeer2);

        for (int ms = 0; ms < 2000; ms++) {
            Thread.sleep(5);
            if (stateMap.get(peer2BusUniqueName) == ApplicationState.CLAIMED) {
                break;
            }
        }
        assertEquals(stateMap.get(peer2BusUniqueName), ApplicationState.CLAIMED);

        // Create membership certificates
        CertificateX509[] managerMembershipCert = new CertificateX509[1];
        createMembershipCert(managerBus, "1", issuerCN, ByteBuffer.wrap(managerBusUniqueName.getBytes()),
                             managerPublicKey, issuerCN, true, expiredInSecs, managerMembershipCert);
        managerConfigurator.installMembership(managerMembershipCert);   // TODO: should we install on the manger SAP???

        CertificateX509[] peer1MembershipCert = new CertificateX509[1];
        createMembershipCert(managerBus, "1", issuerCN, ByteBuffer.wrap(peer1BusUniqueName.getBytes()),
                             peer1Key, issuerCN, true, expiredInSecs, peer1MembershipCert);
        pcPeer1.installMembership(peer1MembershipCert);

        CertificateX509[] peer2MembershipCert = new CertificateX509[1];
        createMembershipCert(managerBus, "1", issuerCN, ByteBuffer.wrap(peer2BusUniqueName.getBytes()),
                             peer2Key, issuerCN, true, expiredInSecs, peer2MembershipCert);
        pcPeer2.installMembership(peer2MembershipCert);

        // Secure connections
        assertEquals(Status.OK, peer1Bus.secureConnection(null,false));  // forceAuth=false
        assertEquals(Status.OK, peer2Bus.secureConnection(null,false));

        Mutable.IntegerValue peer2ToPeer1SessionId = new Mutable.IntegerValue((short) 0);
        assertEquals(Status.OK, peer2Bus.joinSession(peer1BusUniqueName, defaultSessionPort.value,
                peer2ToPeer1SessionId, defaultSessionOpts, new SessionListener()));

        // Call remote methods and properties on the service
        ProxyBusObject proxyObj = peer2Bus.getProxyBusObject(peer1BusUniqueName,
                "/service", BusAttachment.SESSION_ID_ANY, new Class<?>[] { EchoChirpInterface.class });
        EchoChirpInterface proxy = proxyObj.getInterface(EchoChirpInterface.class);

        assertEquals(0, receivedEcho);
        assertEquals("message", proxy.echo("message"));
        assertEquals(1, receivedEcho);

        assertEquals(0, receivedGetProp1);
        assertEquals(1, proxy.getProp1());
        assertEquals(1, receivedGetProp1);

        assertEquals(0, receivedSetProp1);
        proxy.setProp1(11);
        assertEquals(1, receivedSetProp1);
        assertEquals(11, proxy.getProp1());
        assertEquals(2, receivedGetProp1);

        // Reset peer and verify its application state has also changed
        assertEquals(ApplicationState.CLAIMED, pcPeer1.getApplicationState());
        pcPeer1.reset();
        assertEquals(ApplicationState.CLAIMABLE, pcPeer1.getApplicationState());

        // End management
        assertFalse(peer1Pclistener.endManagementReceived);
        pcPeer1.endManagement();
        assertTrue(peer1Pclistener.endManagementReceived);
    }

    public void testConfiguratorReset() throws Exception {
        System.out.println("Start testConfiguratorReset()");

        // Create manager bus attachment, connect, and register auth listener
        managerBus = new BusAttachment(getClass().getName() + "Manager", RemoteMessage.Receive);
        managerBus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, managerBus.connect());
        managerPclistener = new PermissionConfigListenerImpl("manager");
        assertEquals(Status.OK, registerAuthListener(managerBus, "alljoyn",
            "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK", new AuthListenerImpl("manager"), managerPclistener));

        // Create peer1 bus attachment, connect, and register auth listener
        peer1Bus = new BusAttachment(getClass().getName() + "Peer1", RemoteMessage.Receive);
        peer1Bus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, peer1Bus.connect());
        peer1Pclistener = new PermissionConfigListenerImpl("peer1");
        assertEquals(Status.OK, registerAuthListener(peer1Bus, "alljoynpb",
            "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK", new AuthListenerImpl("peer1"), peer1Pclistener));

        // Reset peer1 configurator and verify reset() was called
        PermissionConfigurator peer1Configurator = peer1Bus.getPermissionConfigurator();
        assertFalse(peer1Pclistener.factoryResetReceived);
        peer1Configurator.reset();
        assertTrue(peer1Pclistener.factoryResetReceived);

        // Reset manager configurator and verify reset() was called
        PermissionConfigurator managerConfigurator = managerBus.getPermissionConfigurator();
        assertFalse(managerPclistener.factoryResetReceived);
        managerConfigurator.reset();
        assertTrue(managerPclistener.factoryResetReceived);
    }

    /**
     * Register auth listener with a PermissionConfigurationListener included (which enables security 2.0).
     * Internally this also calls bus enablePeerSecurity().
     */
    private Status registerAuthListener(BusAttachment bus, String keystoreName, String mechanisms,
                                        AuthListener authListener, PermissionConfigurationListener pclistener)
                                        throws Exception {
        return bus.registerAuthListener(mechanisms, authListener, null, false, pclistener);
/*
        if (System.getProperty("os.name").startsWith("Windows")) {
            return bus.registerAuthListener(mechanisms, authListener, null, false, pclistener);
        } else if (System.getProperty("java.vm.name").startsWith("Dalvik")) {
             // On some Android devices File.createTempFile trys to create a file in
             // a location we do not have permission to write to. Resulting in a
             // java.io.IOException: Permission denied error.
             // This code assumes that the junit tests will have file IO permission
             // for /data/data/org.alljoyn.bus
            return bus.registerAuthListener(mechanisms, authListener,
                    "/data/data/org.alljoyn.bus/files/"+keystoreName+".ks", false, pclistener);
        } else {
            return bus.registerAuthListener(mechanisms, authListener,
                    File.createTempFile(keystoreName, "ks").getAbsolutePath(), false, pclistener);
        }
*/
    }

    private void createIdentityCert(BusAttachment issuerBus, String serial, ByteBuffer issuerCN, ByteBuffer subjectCN,
            KeyInfoNISTP256 subjectPubKey, String alias, long expiredInSecs, CertificateX509[] certChain)
            throws Exception {
        if (certChain == null || certChain.length < 1) throw new IllegalArgumentException("Invalid certChain array size");
        PermissionConfigurator configurator = issuerBus.getPermissionConfigurator();

        /* generate the leaf cert */
        certChain[0] = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        certChain[0].setSerial(serial.getBytes());
        certChain[0].setIssuerCN(issuerCN.array());
        certChain[0].setSubjectCN(subjectCN.array());
        certChain[0].setSubjectPublicKey(subjectPubKey.getPublicKey());
        certChain[0].setSubjectAltName(alias.getBytes());

        long validFrom = System.currentTimeMillis() / 1000;
        certChain[0].setValidity(validFrom, validFrom + expiredInSecs);

        configurator.signCertificate(certChain[0]);

        KeyInfoNISTP256 keyInfo = configurator.getSigningPublicKey();
        certChain[0].verify(keyInfo.getPublicKey());
    }

    private void createMembershipCert(BusAttachment signingBus, String serial, ByteBuffer issuerCN, ByteBuffer subjectCN,
            KeyInfoNISTP256 subjectPubKey, ByteBuffer alias, boolean delegate, long expiredInSecs, CertificateX509[] certChain)
            throws Exception {
        if (certChain == null || certChain.length < 1) throw new IllegalArgumentException("Invalid certChain array size");
        PermissionConfigurator configurator = signingBus.getPermissionConfigurator();

        /* generate the leaf cert */
        certChain[0] = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);
        certChain[0].setSerial(serial.getBytes());
        certChain[0].setIssuerCN(issuerCN.array());
        certChain[0].setSubjectCN(subjectCN.array());
        certChain[0].setSubjectPublicKey(subjectPubKey.getPublicKey());
        certChain[0].setSubjectAltName(alias.array());
        certChain[0].setCA(delegate);

        long validFrom = System.currentTimeMillis() / 1000;
        certChain[0].setValidity(validFrom, validFrom + expiredInSecs);

        configurator.signCertificate(certChain[0]);
    }

    private ApplicationStateListener appStateListener = new ApplicationStateListener() {
        public void state(String busName, KeyInfoNISTP256 publicKeyInfo, ApplicationState state) {
            System.out.println("ApplicationStateListener: received state() callback with bus=" +
                    busName + ", state=" + state.name());
            stateMap.put(busName, state);
        }
    };

    public class AuthListenerImpl implements AuthListener {
        private String name;

        public AuthListenerImpl(String name) {
            this.name = name;
        }

        public boolean requested(String mechanism, String authPeer, int count, String userName, AuthRequest[] requests) {
            System.out.println("AuthListener["+name+"]: received requested() - " + mechanism + ", " + authPeer);
/*          for (AuthRequest rqst: requests) {
                if (rqst instanceof PrivateKeyRequest) {
                    System.out.println("AuthListener["+name+"]: received PrivateKeyRequest");
                    ((PrivateKeyRequest)rqst).setPrivateKey(ecdsaPrivateKeyPEM);
                } else if (rqst instanceof CertificateRequest) {
                    System.out.println("AuthListener["+name+"]: received CertificateRequest");
                    ((CertificateRequest)rqst).setCertificateChain(ecdsaCertChainX509PEM);
                } else if (rqst instanceof VerifyRequest) {
                    System.out.println("AuthListener["+name+"]: received VerifyRequest");
                    String certPEM = ((VerifyRequest)rqst).getCertificateChain();
                    if (!certPEM.equals(ecdsaCertChainX509PEM)) {
                        System.out.println("    -- verifying cert failed");
                    }
                } else if (rqst instanceof ExpirationRequest) {
                    System.out.println("AuthListener["+name+"]: received ExpirationRequest");
                    ((ExpirationRequest)rqst).setExpiration(100);  // expired 100 seconds from now
                } else if (rqst instanceof PasswordRequest) {
                    System.out.println("AuthListener["+name+"]: received PasswordRequest");
                    ((PasswordRequest)rqst).setPassword("password".toCharArray());
                } else {
                    System.out.println("AuthListener["+name+"]: received other request - " + rqst);
                }
            }
*/
            return true;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {
            System.out.println("AuthListener["+name+"]: received completed(" + authenticated + ") - " + mechanism + ", " + authPeer);
        }
    }

    public class PermissionConfigListenerImpl implements PermissionConfigurationListener {
        private String name;
        public boolean factoryResetReceived = false;
        public boolean policyChangedReceived = false;
        public boolean startManagementReceived = false;
        public boolean endManagementReceived = false;

        public PermissionConfigListenerImpl(String name) {
            this.name = name;
        }

        public Status factoryReset() {
            System.out.println("PermissionConfigurationListener["+name+"]: received factoryReset()");
            factoryResetReceived = true;
            return Status.OK;
        }

        public void policyChanged() {
            System.out.println("PermissionConfigurationListener["+name+"]: received policyChanged()");
            policyChangedReceived = true;
        }

        public void startManagement() {
            System.out.println("PermissionConfigurationListener["+name+"]: received startManagement()");
            startManagementReceived = true;
        }

        public void endManagement() {
            System.out.println("PermissionConfigurationListener["+name+"]: received endManagement()");
            endManagementReceived = true;
        }
    }

    public class SecuritySessionPortListener extends SessionPortListener {
        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts) {
            return true;
        }

        public void sessionJoined(short sessionPort, int id, String joiner) {
            hostedSessions.put(joiner, id);
        }
    }

}
