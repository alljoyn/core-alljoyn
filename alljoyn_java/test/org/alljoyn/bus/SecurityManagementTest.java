/**
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */

package org.alljoyn.bus;

import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.HashMap;
import java.io.File;
import java.nio.ByteBuffer;
import java.util.UUID;

import junit.framework.TestCase;

import org.alljoyn.bus.annotation.BusSignalHandler;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Introspectable;
import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.CryptoECC;

public class SecurityManagementTest extends TestCase {
    private static final long expiredInSecs = 3600;
    private BusAttachment peerBus;
    private BusAttachment managerBus;
    private Map<String, PermissionConfigurator.ApplicationState> stateMap;
    private String managerBusUniqueName;
    private String peerBusUniqueName;
    /**
     * hostedSessions holds <busName, sessionId>
     */
    private Map<String,Integer> hostedSessions;

    private boolean factoryResetReceived;

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

    private SessionOpts defaultSessionOpts;
    private Mutable.ShortValue defaultSessionPort;
    private Mutable.IntegerValue managerToManagerSessionId;
    private Mutable.IntegerValue managerToPeerSessionId;

    static {
        System.loadLibrary("alljoyn_java");
    }

    @Override
    public void setUp() throws Exception {
        peerBus = null;
        stateMap = new HashMap<String, PermissionConfigurator.ApplicationState>();
        hostedSessions = new HashMap<String,Integer>();
        defaultSessionOpts = new SessionOpts();
        defaultSessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        defaultSessionOpts.isMultipoint = false;
        defaultSessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        defaultSessionOpts.transports = SessionOpts.TRANSPORT_ANY;
        defaultSessionPort = new Mutable.ShortValue((short) 42);
        managerToManagerSessionId = new Mutable.IntegerValue((short) 0);
        managerToPeerSessionId = new Mutable.IntegerValue((short) 0);
        factoryResetReceived = false;
    }

    @Override
    public void tearDown() throws Exception {

        if (peerBus != null) {
            assertTrue(peerBus.isConnected());
            peerBus.disconnect();
            assertFalse(peerBus.isConnected());
            peerBus.release();
            peerBus = null;
        }

    }

    /**
     * copied and interpreted from core SecurityManagementTest.cc
     */
    public void testBasic() throws Exception {
        peerBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        peerBus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, peerBus.connect());

        managerBus = new BusAttachment(getClass().getName() + "manager", BusAttachment.RemoteMessage.Receive);
        managerBus.registerKeyStoreListener(new InMemoryKeyStoreListener());
        assertEquals(Status.OK, managerBus.connect());

        managerBusUniqueName = managerBus.getUniqueName();
        peerBusUniqueName = peerBus.getUniqueName();

        assertEquals(Status.OK, managerBus.registerAuthListener("ALLJOYN_ECDHE_NULL", authListener, null, false, pclistener));
        assertEquals(Status.OK, peerBus.registerAuthListener("ALLJOYN_ECDHE_NULL", authListener, null, false, pclistener));

        managerBus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);
        peerBus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);

        /**
         * copied from the c++ counterpart.
         * not sure what is expected from this call, or how we are going to use it later
         * EXPECT_EQ(ER_OK, peer2Bus.CreateInterfacesFromXml(interface.c_str()));
         * EXPECT_EQ(ER_OK, peer3Bus.CreateInterfacesFromXml(interface.c_str()));
         */

        assertEquals(Status.OK, managerBus.bindSessionPort(defaultSessionPort, defaultSessionOpts, new SessionPortListener() {
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts) {
                return true;
            }
            public void sessionJoined(short sessionPort, int id, String joiner) {
                hostedSessions.put(joiner, id);
            }
        }));

        assertEquals(Status.OK, peerBus.bindSessionPort(defaultSessionPort, defaultSessionOpts, new SessionPortListener() {
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts) {
                return true;
            }
            public void sessionJoined(short sessionPort, int id, String joiner) {
                hostedSessions.put(joiner, id);
            }
        }));

        assertEquals(Status.OK, managerBus.joinSession(managerBusUniqueName, defaultSessionPort.value, managerToManagerSessionId, defaultSessionOpts, new JoinSessionSessionListener()));
        assertEquals(Status.OK, managerBus.joinSession(peerBusUniqueName, defaultSessionPort.value, managerToPeerSessionId, defaultSessionOpts, new JoinSessionSessionListener()));

        PermissionConfigurator managerConfigurator = managerBus.getPermissionConfigurator();
        PermissionConfigurator.ApplicationState applicationStateManager = managerConfigurator.getApplicationState();
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, applicationStateManager);

        SecurityApplicationProxy sapWithPeer = new SecurityApplicationProxy(managerBus, peerBusUniqueName, managerToPeerSessionId.value);
        PermissionConfigurator.ApplicationState applicationStatePeer;
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, sapWithPeer.getApplicationState());

        assertEquals(Status.OK, managerBus.registerApplicationStateListener(appStateListener));

        KeyInfoNISTP256 managerPublicKey = managerConfigurator.getSigningPublicKey();

        //Create peer key
        PermissionConfigurator pcPeer = peerBus.getPermissionConfigurator();
        KeyInfoNISTP256 peerKey = pcPeer.getSigningPublicKey();

        // Create identityCert
        CertificateX509[] certChain = new CertificateX509[1];

        UUID issuerCN = UUID.randomUUID();
        ByteBuffer bb = ByteBuffer.wrap(new byte[16]);
        bb.putLong(issuerCN.getMostSignificantBits());
        bb.putLong(issuerCN.getLeastSignificantBits());

        /* generate the leaf cert */
        certChain[0] = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        certChain[0].setSerial("0".getBytes());
        certChain[0].setIssuerCN(bb.array());
        certChain[0].setSubjectCN(bb.array());
        certChain[0].setSubjectPublicKey(managerPublicKey.getPublicKey());
        certChain[0].setSubjectAltName("ManagerAlias".getBytes());

        long validFrom = System.currentTimeMillis() / 1000;
        certChain[0].setValidity(validFrom, validFrom + expiredInSecs);

        managerConfigurator.signCertificate(certChain[0]);

        KeyInfoNISTP256 keyInfo = managerConfigurator.getSigningPublicKey();
        certChain[0].verify(keyInfo.getPublicKey());

        CryptoECC cryptoECC = new CryptoECC();
        cryptoECC.generateDSAKeyPair();
        String signedManifestXml = SecurityApplicationProxy.signManifest(certChain[0], cryptoECC.getDSAPrivateKey(), defaultManifestXml);

        String[] signedManifestXmls = new String[]{signedManifestXml};
        managerConfigurator.claim(managerPublicKey, issuerCN, managerPublicKey, certChain, signedManifestXmls);

        for (int ms = 0; ms < 2000; ms++) {
            Thread.sleep(5);
            if (stateMap.get(managerBusUniqueName) == PermissionConfigurator.ApplicationState.CLAIMED) {
                break;
            }
        }
        assertEquals(stateMap.get(managerBusUniqueName), PermissionConfigurator.ApplicationState.CLAIMED);

        // Create identityCert for peer
        CertificateX509[] certChainPeer = new CertificateX509[1];

        /* generate the leaf cert */
        certChainPeer[0] = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        certChainPeer[0].setSerial("0".getBytes());
        certChainPeer[0].setIssuerCN(bb.array());
        certChainPeer[0].setSubjectCN(bb.array());
        certChainPeer[0].setSubjectPublicKey(peerKey.getPublicKey());
        certChainPeer[0].setSubjectAltName("Peer1Alias".getBytes());

        validFrom = System.currentTimeMillis() / 1000;
        certChainPeer[0].setValidity(validFrom, validFrom + expiredInSecs);

        pcPeer.signCertificate(certChainPeer[0]);

        KeyInfoNISTP256 keyInfo2 = pcPeer.getSigningPublicKey();
        certChainPeer[0].verify(keyInfo2.getPublicKey());

        String signedManifestXml2 = SecurityApplicationProxy.signManifest(certChainPeer[0], cryptoECC.getDSAPrivateKey(), defaultManifestXml);

        String [] signedManifestXmls2 = new String[]{signedManifestXml2};

        //Manager claims Peers
        sapWithPeer.claim(managerPublicKey, issuerCN, managerPublicKey, certChainPeer, signedManifestXmls2);

        for (int ms = 0; ms < 2000; ms++) {
            Thread.sleep(5);
            if (stateMap.get(peerBusUniqueName) == PermissionConfigurator.ApplicationState.CLAIMED) {
                break;
            }
        }
        assertEquals(stateMap.get(peerBusUniqueName), PermissionConfigurator.ApplicationState.CLAIMED);

        CertificateX509[] certChainMembership = new CertificateX509[1];

        /* generate the leaf cert */
        certChainMembership[0] = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);
        certChainMembership[0].setSerial("1".getBytes());
        certChainMembership[0].setIssuerCN(bb.array());
        certChainMembership[0].setSubjectCN(managerBusUniqueName.getBytes());
        certChainMembership[0].setSubjectPublicKey(managerPublicKey.getPublicKey());
        certChainMembership[0].setSubjectAltName(bb.array());
        certChainMembership[0].setCA(true);

        validFrom = System.currentTimeMillis() / 1000;
        certChainMembership[0].setValidity(validFrom, validFrom + expiredInSecs);

        managerConfigurator.signCertificate(certChainMembership[0]);
        managerConfigurator.installMembership(certChainMembership);

        // Reset peer and verify its application state has also changed
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED, pcPeer.getApplicationState());
        pcPeer.reset();
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, pcPeer.getApplicationState());
    }

    public void testConfiguratorReset() throws Exception {
        // Create manager bus attachment, connect, and register auth listener
        managerBus = new BusAttachment(getClass().getName() + "Manager", BusAttachment.RemoteMessage.Receive);
        managerBus.registerKeyStoreListener(new NullKeyStoreListener());
        assertEquals(Status.OK, managerBus.connect());
        assertEquals(Status.OK, managerBus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                authListener, null, false, pclistener));

        // Create peer bus attachment, connect, and register auth listener
        peerBus = new BusAttachment(getClass().getName() + "Peer", BusAttachment.RemoteMessage.Receive);
        peerBus.registerKeyStoreListener(new NullKeyStoreListener());
        assertEquals(Status.OK, peerBus.connect());
        assertEquals(Status.OK, peerBus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                authListener, null, false, pclistener));

        // Reset peer configurator and verify reset() was called
        PermissionConfigurator peerConfigurator = peerBus.getPermissionConfigurator();
        factoryResetReceived = false;
        peerConfigurator.reset();
        assertTrue(factoryResetReceived);

        // Reset manager configurator and verify reset() was called
        PermissionConfigurator managerConfigurator = managerBus.getPermissionConfigurator();
        factoryResetReceived = false;
        managerConfigurator.reset();
        assertTrue(factoryResetReceived);
    }

    private ApplicationStateListener appStateListener = new ApplicationStateListener() {

        public void state(String busName, KeyInfoNISTP256 publicKeyInfo, PermissionConfigurator.ApplicationState state) {
            System.out.println("state callback was called on this bus " + busName);
            stateMap.put(busName, state);
        }

    };

    private PermissionConfigurationListener pclistener = new PermissionConfigurationListener() {

        public Status factoryReset() {
            factoryResetReceived = true;
            return Status.OK;
        }

        public void policyChanged() {
        }

        public void startManagement() {
        }

        public void endManagement() {
        }
    };

    public class JoinSessionSessionListener extends SessionListener{
    }

    private AuthListener authListener = new AuthListener() {
        public boolean requested(String mechanism, String authPeer, int count, String userName,
                AuthRequest[] requests) {
            return true;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {}
    };

    public class InMemoryKeyStoreListener implements KeyStoreListener {
        private byte[] keys;

        public byte[] getKeys() { return keys; }
        public char[] getPassword() { return "password".toCharArray(); }
        public void putKeys(byte[] keys) { this.keys = keys; }
    }

}