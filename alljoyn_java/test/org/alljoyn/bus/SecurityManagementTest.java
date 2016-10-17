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
    private BusAttachment peerBus;
    private BusAttachment managerBus;
    private Map<String, ApplicationState> stateMap;
    private String managerBusUniqueName;
    private String peerBusUniqueName;
    /* Key: busName, Value: sessionId */
    private Map<String,Integer> hostedSessions;

    PermissionConfigListenerImpl managerPclistener;
    PermissionConfigListenerImpl peerPclistener;

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

    private static final String defaultAboutDataXml =
        "<AboutData>" +
        "  <AppId>000102030405060708090A0B0C0D0E0C</AppId>" +
        "  <DefaultLanguage>en</DefaultLanguage>" +
        "  <DeviceName>My Device Name</DeviceName>" +
        "  <DeviceId>93c06771-c725-48c2-b1ff-6a2a59d445b8</DeviceId>" +
        "  <AppName>My Application Name</AppName>" +
        "  <Manufacturer>Company</Manufacturer>" +
        "  <ModelNumber>Wxfy388i</ModelNumber>" +
        "  <Description>A detailed description provided by the application.</Description>" +
        "  <DateOfManufacture>2014-01-08</DateOfManufacture>" +
        "  <SoftwareVersion>1.0.0</SoftwareVersion>" +
        "  <AjSoftwareVersion>1.0.0</AjSoftwareVersion>" +
        "  <HardwareVersion>1.0.0</HardwareVersion>" +
        "  <SupportUrl>www.example.com</SupportUrl>" +
        "</AboutData>";

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
    private Mutable.IntegerValue managerToPeerSessionId;

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class EchoChirpService implements BusObject, EchoChirpInterface {
        private int prop1 = 1;
        private int prop2 = 2;
        private int receivedEcho = 0;
        private int receivedChirp = 0;
        private int receivedGetProp1 = 0;
        private int receivedSetProp1 = 0;
        private int receivedGetProp2 = 0;
        private int receivedSetProp2 = 0;

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
        managerToPeerSessionId = new Mutable.IntegerValue((short) 0);
    }

    @Override
    public void tearDown() throws Exception {
        if (peerBus != null) {
            if (peerBus.isConnected()) {
                peerBus.disconnect();
            }
            peerBus.release();
            peerBus= null;
        }
        peerPclistener = null;

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
    public void testWithCertificate() throws Exception {
        System.out.println("Starting testWithCertificate()...");

        // Create manager bus attachment, connect, and register auth listener
        managerBus = new BusAttachment(getClass().getName() + "Manager", RemoteMessage.Receive);
        managerBus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, managerBus.connect());
        managerPclistener = new PermissionConfigListenerImpl();
        assertEquals(Status.OK, registerAuthListener(managerBus, "alljoyn",
                "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA", managerPclistener));

        // Register bus object on manager bus, and request name
        EchoChirpService service = new EchoChirpService();
        assertEquals(Status.OK, managerBus.registerBusObject(service, "/service", true));

        DBusProxyObj control = managerBus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                     control.RequestName("org.alljoyn.bus.SecurityManagerTest",
                                         DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        // Create peer bus attachment, connect, and register auth listener
        peerBus = new BusAttachment(getClass().getName() + "Peer", RemoteMessage.Receive);
        peerBus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, peerBus.connect());
        peerPclistener = new PermissionConfigListenerImpl();
        assertEquals(Status.OK, registerAuthListener(peerBus, "alljoynpb",
                "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA", peerPclistener));

        managerBusUniqueName = managerBus.getUniqueName();
        peerBusUniqueName = peerBus.getUniqueName();

        // Set permission configuator claim capabilities
        managerBus.getPermissionConfigurator().setClaimCapabilities(
            (short) (PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL
                         | PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_PSK 
                         | PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_ECDSA));
        managerBus.getPermissionConfigurator().setClaimCapabilityAdditionalInfo(
            (short) (PermissionConfigurator.CLAIM_CAPABILITY_ADDITIONAL_PSK_GENERATED_BY_SECURITY_MANAGER
                         | PermissionConfigurator.CLAIM_CAPABILITY_ADDITIONAL_PSK_GENERATED_BY_APPLICATION));
        managerBus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);

        peerBus.getPermissionConfigurator().setClaimCapabilities(
            (short) (PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL
                         | PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_PSK 
                         | PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_ECDSA));
        peerBus.getPermissionConfigurator().setClaimCapabilityAdditionalInfo(
            (short) (PermissionConfigurator.CLAIM_CAPABILITY_ADDITIONAL_PSK_GENERATED_BY_SECURITY_MANAGER
                         | PermissionConfigurator.CLAIM_CAPABILITY_ADDITIONAL_PSK_GENERATED_BY_APPLICATION));
        peerBus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);

        // bind ports
        assertEquals(Status.OK, managerBus.bindSessionPort(defaultSessionPort, defaultSessionOpts,
                new SecuritySessionPortListener()));
        assertEquals(Status.OK, peerBus.bindSessionPort(defaultSessionPort, defaultSessionOpts,
                new SecuritySessionPortListener()));

        // join sessions
        assertEquals(Status.OK, managerBus.joinSession(managerBusUniqueName, defaultSessionPort.value,
                managerToManagerSessionId, defaultSessionOpts, new SessionListener()));
        assertEquals(Status.OK, managerBus.joinSession(peerBusUniqueName, defaultSessionPort.value,
                managerToPeerSessionId, defaultSessionOpts, new SessionListener()));
        // verify manager and peer configurators are claimable
        PermissionConfigurator managerConfigurator = managerBus.getPermissionConfigurator();
        assertEquals(ApplicationState.CLAIMABLE, managerConfigurator.getApplicationState());

        SecurityApplicationProxy sapWithPeer = new SecurityApplicationProxy(managerBus, peerBusUniqueName,
                managerToPeerSessionId.value);
        assertEquals(ApplicationState.CLAIMABLE, sapWithPeer.getApplicationState());

        assertEquals(Status.OK, managerBus.registerApplicationStateListener(appStateListener));

        //Get manager key
        KeyInfoNISTP256 managerPublicKey = managerConfigurator.getSigningPublicKey();

        //Get peer key
        PermissionConfigurator pcPeer = peerBus.getPermissionConfigurator();
        assertEquals(ApplicationState.CLAIMABLE, pcPeer.getApplicationState());
        KeyInfoNISTP256 peerKey = pcPeer.getSigningPublicKey();

        UUID issuerUUID = UUID.randomUUID();
        ByteBuffer issuerCN = ByteBuffer.wrap(new byte[16]);
        issuerCN.putLong(issuerUUID.getMostSignificantBits());
        issuerCN.putLong(issuerUUID.getLeastSignificantBits());

        // Create identityCert for manager, sign, and claim
        CertificateX509[] certChain = new CertificateX509[1];
        createIdentityCert(managerBus, "0", issuerCN, issuerCN, managerPublicKey, "ManagerAlias", expiredInSecs, certChain);

        CryptoECC cryptoECC = new CryptoECC();
        cryptoECC.generateDSAKeyPair();
        String signedManifestXml = SecurityApplicationProxy.signManifest(certChain[0], cryptoECC.getDSAPrivateKey(), defaultManifestXml);

        String[] signedManifestXmls = new String[]{signedManifestXml};
        managerConfigurator.claim(managerPublicKey, issuerUUID, managerPublicKey, certChain, signedManifestXmls);

        for (int ms = 0; ms < 2000; ms++) {
            Thread.sleep(5);
            if (stateMap.get(managerBusUniqueName) == ApplicationState.CLAIMED) {
                break;
            }
        }
        assertEquals(stateMap.get(managerBusUniqueName), ApplicationState.CLAIMED);

        // Create identityCert for peer, sign, and claim
        CertificateX509[] certChainPeer = new CertificateX509[1];
        createIdentityCert(managerBus, "0", issuerCN, issuerCN, peerKey, "PeerAlias", expiredInSecs, certChainPeer);

        String signedManifestXml2 = SecurityApplicationProxy.signManifest(certChainPeer[0], cryptoECC.getDSAPrivateKey(), defaultManifestXml);

        String [] signedManifestXmls2 = new String[]{signedManifestXml2};

        sapWithPeer.claim(managerPublicKey, issuerUUID, managerPublicKey, certChainPeer, signedManifestXmls2);

        for (int ms = 0; ms < 2000; ms++) {
            Thread.sleep(5);
            if (stateMap.get(peerBusUniqueName) == ApplicationState.CLAIMED) {
                break;
            }
        }
        assertEquals(stateMap.get(peerBusUniqueName), ApplicationState.CLAIMED);

        // Create membership certificate on manager
        CertificateX509[] certChainMembership = new CertificateX509[1];
        createMembershipCert(managerBus, "1", issuerCN, ByteBuffer.wrap(managerBusUniqueName.getBytes()),
                             managerPublicKey, issuerCN, true, expiredInSecs, certChainMembership);
        managerConfigurator.installMembership(certChainMembership);

        assertEquals(Status.OK, peerBus.secureConnection(null,true));
        assertEquals(Status.OK, managerBus.secureConnection(null,true));

        // End management
        assertFalse(peerPclistener.endManagementReceived);
        pcPeer.endManagement();
        assertTrue(peerPclistener.endManagementReceived);

        // Reset peer and verify its application state has also changed
        assertEquals(ApplicationState.CLAIMED, pcPeer.getApplicationState());
        pcPeer.reset();
        assertEquals(ApplicationState.CLAIMABLE, pcPeer.getApplicationState());
    }

    public void testConfiguratorReset() throws Exception {
        // Create manager bus attachment, connect, and register auth listener
        managerBus = new BusAttachment(getClass().getName() + "Manager", RemoteMessage.Receive);
        managerBus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, managerBus.connect());
        managerPclistener = new PermissionConfigListenerImpl();
        assertEquals(Status.OK, registerAuthListener(managerBus, "alljoyn",
                "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK", managerPclistener));

        // Create peer bus attachment, connect, and register auth listener
        peerBus = new BusAttachment(getClass().getName() + "Peer", RemoteMessage.Receive);
        peerBus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, peerBus.connect());
        peerPclistener = new PermissionConfigListenerImpl();
        assertEquals(Status.OK, registerAuthListener(peerBus, "alljoynpb",
                "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK", peerPclistener));

        // Reset peer configurator and verify reset() was called
        PermissionConfigurator peerConfigurator = peerBus.getPermissionConfigurator();
        assertFalse(peerPclistener.factoryResetReceived);
        peerConfigurator.reset();
        assertTrue(peerPclistener.factoryResetReceived);

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
                                        PermissionConfigurationListener pclistener) throws Exception {
        final AuthListener authListener = new AuthListener() {
            public boolean requested(String mechanism, String authPeer, int count, String userName, AuthRequest[] requests) {
                for (AuthRequest rqst: requests) {
                    if (rqst instanceof PrivateKeyRequest) {
                        System.out.println("AuthListener: received PrivateKeyRequest");
                        ((PrivateKeyRequest)rqst).setPrivateKey(ecdsaPrivateKeyPEM);
                    } else if (rqst instanceof CertificateRequest) {
                        System.out.println("AuthListener: received CertificateRequest");
                        ((CertificateRequest)rqst).setCertificateChain(ecdsaCertChainX509PEM);
                    } else if (rqst instanceof VerifyRequest) {
                        System.out.println("AuthListener: received VerifyRequest");
                        String certPEM = ((VerifyRequest)rqst).getCertificateChain();
                        if (!certPEM.equals(ecdsaCertChainX509PEM)) {
                            System.out.println("    -- verifying cert failed");
                        }
                    } else if (rqst instanceof ExpirationRequest) {
                        System.out.println("AuthListener: received ExpirationRequest");
                        ((ExpirationRequest)rqst).setExpiration(100);  // expired 100 seconds from now
                    } else if (rqst instanceof PasswordRequest) {
                        System.out.println("AuthListener: received PasswordRequest");
                        ((PasswordRequest)rqst).setPassword("000000".toCharArray());
                    } else {
                        System.out.println("AuthListener: received other request - " + rqst);
                    }
                }
                return true;
            }

            public void completed(String mechanism, String authPeer, boolean authenticated) {}
        };

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

    public class PermissionConfigListenerImpl implements PermissionConfigurationListener {
        public boolean factoryResetReceived = false;
        public boolean policyChangedReceived = false;
        public boolean startManagementReceived = false;
        public boolean endManagementReceived = false;

        public Status factoryReset() {
                System.out.println("PermissionConfigurationListener: received factoryReset()");
                factoryResetReceived = true;
                return Status.OK;
        }

        public void policyChanged() {
                System.out.println("PermissionConfigurationListener: received policyChanged()");
                policyChangedReceived = true;
        }

        public void startManagement() {
                System.out.println("PermissionConfigurationListener: received startManagement()");
                startManagementReceived = true;
        }

        public void endManagement() {
                System.out.println("PermissionConfigurationListener: received endManagement()");
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
