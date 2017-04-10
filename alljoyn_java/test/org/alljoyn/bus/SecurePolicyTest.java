/**
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*/

package org.alljoyn.bus;

import junit.framework.TestCase;
import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.HashMap;
import java.io.File;
import java.nio.ByteBuffer;
import java.util.UUID;
import java.io.IOException;

import org.alljoyn.bus.annotation.BusSignalHandler;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Introspectable;
import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.CryptoECC;
import org.alljoyn.bus.SignalEmitter;

public class SecurePolicyTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String SERVICE_PATH_PEER2 = "/service";

    private BusAttachment securityManagerBus;
    private BusAttachment peer1Bus;
    private BusAttachment peer2Bus;

    private KeyInfoNISTP256 securityManagerKey;
    private SecurityApplicationProxy sapWithPeer1;
    private SecurityApplicationProxy sapWithPeer2;
    private PermissionConfigurator peer1PermissionConfigurator;
    private PermissionConfigurator peer2PermissionConfigurator;

    private Mutable.ShortValue defaultSessionPort;
    private SessionOpts defaultSessionOpts;
    private Mutable.IntegerValue managerToManagerSessionId;
    private Mutable.IntegerValue managerToPeer1SessionId;
    private Mutable.IntegerValue managerToPeer2SessionId;

    private Map<String, PermissionConfigurator.ApplicationState> stateMap;

    private int receivedEcho;
    private int receivedChirp;
    private int receivedGetProp1;
    private int receivedSetProp1;
    private int receivedGetProp2;
    private int receivedSetProp2;

    private SignalEmitter localEmitter;

    @Override
    public void setUp() throws Exception {
        securityManagerBus = new BusAttachment("SecurityManager", BusAttachment.RemoteMessage.Receive);
        peer1Bus = new BusAttachment("SecurityPeer1", BusAttachment.RemoteMessage.Receive);
        peer2Bus = new BusAttachment("SecurityPeer2", BusAttachment.RemoteMessage.Receive);

        Status status = securityManagerBus.connect();
        assertEquals(status, Status.OK);
        status = peer1Bus.connect();
        assertEquals(status, Status.OK);
        status = peer2Bus.connect();
        assertEquals(status, Status.OK);

        // Register bus object on peer2 bus
        EchoChirpService service = new EchoChirpService();
        assertEquals(Status.OK, peer2Bus.registerBusObject(service, SERVICE_PATH_PEER2, true));
        localEmitter = new SignalEmitter(service, BusAttachment.SESSION_ID_ALL_HOSTED, SignalEmitter.GlobalBroadcast.On);

        managerGuid = UUID.randomUUID();
        stateMap = new HashMap<String, PermissionConfigurator.ApplicationState>();

        defaultSessionPort = new Mutable.ShortValue((short) 42);
        defaultSessionOpts = new SessionOpts();
        managerToManagerSessionId = new Mutable.IntegerValue((short) 0);
        managerToPeer1SessionId = new Mutable.IntegerValue((short) 0);
        managerToPeer2SessionId = new Mutable.IntegerValue((short) 0);

        receivedEcho = 0;
        receivedChirp = 0;
        receivedGetProp1 = 0;
        receivedSetProp1 = 0;
        receivedGetProp2 = 0;
        receivedSetProp2 = 0;

        SecurityTestHelper.registerAuthListener(securityManagerBus,
                SecurityTestHelper.AUTH_NULL +
                " " +
                SecurityTestHelper.AUTH_ECDSA);
        SecurityTestHelper.registerAuthListener(peer1Bus,
                SecurityTestHelper.AUTH_NULL +
                " " +
                SecurityTestHelper.AUTH_ECDSA);
        SecurityTestHelper.registerAuthListener(peer2Bus,
                SecurityTestHelper.AUTH_NULL +
                " " +
                SecurityTestHelper.AUTH_ECDSA);

        // bind ports
        assertEquals(Status.OK, securityManagerBus.bindSessionPort(
                defaultSessionPort, defaultSessionOpts, new SessionPortListenerImpl()));
        assertEquals(Status.OK, peer1Bus.bindSessionPort(defaultSessionPort, defaultSessionOpts, new SessionPortListenerImpl()));
        assertEquals(Status.OK, peer2Bus.bindSessionPort(defaultSessionPort, defaultSessionOpts, new SessionPortListenerImpl()));

        // join sessions
        assertEquals(Status.OK, securityManagerBus.joinSession(securityManagerBus.getUniqueName(),
                defaultSessionPort.value, managerToManagerSessionId, new SessionOpts(), new SessionListener()));
        assertEquals(Status.OK, securityManagerBus.joinSession(peer1Bus.getUniqueName(),
                defaultSessionPort.value, managerToPeer1SessionId, new SessionOpts(), new SessionListener()));
        assertEquals(Status.OK, securityManagerBus.joinSession(peer2Bus.getUniqueName(),
                defaultSessionPort.value, managerToPeer2SessionId, new SessionOpts(), new SessionListener()));

        sapWithPeer1 = new SecurityApplicationProxy(securityManagerBus, peer1Bus.getUniqueName(), 0);
        sapWithPeer2 = new SecurityApplicationProxy(securityManagerBus, peer2Bus.getUniqueName(), 0);
        SecurityApplicationProxy sapWithManager = new SecurityApplicationProxy(securityManagerBus, securityManagerBus.getUniqueName(), 0);

        // The State signal is only emitted if manifest template is installed
        securityManagerBus.getPermissionConfigurator().setManifestTemplateFromXml(ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);
        peer1Bus.getPermissionConfigurator().setManifestTemplateFromXml(ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);
        peer2Bus.getPermissionConfigurator().setManifestTemplateFromXml(ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        // Create admin group key
        PermissionConfigurator permissionConfigurator = securityManagerBus.getPermissionConfigurator();
        securityManagerKey = permissionConfigurator.getSigningPublicKey();

        peer1PermissionConfigurator = peer1Bus.getPermissionConfigurator();
        KeyInfoNISTP256 peer1PublicKey = peer1PermissionConfigurator.getSigningPublicKey();

        peer2PermissionConfigurator = peer2Bus.getPermissionConfigurator();
        KeyInfoNISTP256 peer2PublicKey = peer2PermissionConfigurator.getSigningPublicKey();

        // Create identityCertChain
        CertificateX509 identityCertChain[] = createIdentityCert("0",
                securityManagerKey.getPublicKey(),
                "Alias",
                3600,
                securityManagerBus);

        String signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(identityCertChain[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        /*
         * Claim Peer1
         * the certificate authority is self signed so the certificateAuthority
         * key is the same as the adminGroup key.
         * For this test the adminGroupId is a randomly generated GUID. As long as the
         * GUID is consistent it's unimportant that the GUID is random.
         * Use generated identity certificate signed by the securityManager
         * Since we are only interested in claiming the peer we are using an all
         * inclusive manifest.
         */
        sapWithManager.claim(securityManagerKey,
                    managerGuid,
                    securityManagerKey,
                    identityCertChain,
                    new String[]{signedManifest});

        identityCertChain = createIdentityCert("0",
                peer1PublicKey.getPublicKey(),
                "Alias",
                3600,
                securityManagerBus);
        signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(identityCertChain[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);
        sapWithPeer1.claim(securityManagerKey,
                    managerGuid,
                    securityManagerKey,
                    identityCertChain,
                    new String[]{signedManifest});

        /*
         * Claim peer 2
         */
        identityCertChain = createIdentityCert("0",
                peer2PublicKey.getPublicKey(),
                "Alias",
                3600,
                securityManagerBus);
        signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(identityCertChain[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);
        sapWithPeer2.claim(securityManagerKey,
                    managerGuid,
                    securityManagerKey,
                    identityCertChain,
                    new String[]{signedManifest});

        installMembershipOnManager(sapWithManager);

        peer1Bus.registerApplicationStateListener(appStateListener);
        peer2Bus.registerApplicationStateListener(appStateListener);

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED, peer1PermissionConfigurator.getApplicationState());
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED, peer2PermissionConfigurator.getApplicationState());

        securityManagerBus.secureConnection(null, true);
        peer1Bus.secureConnection(securityManagerBus.getUniqueName(), true);
        peer2Bus.secureConnection(securityManagerBus.getUniqueName(), true);

    }

    @Override
    public void tearDown() throws Exception {
        peer1PermissionConfigurator.resetPolicy();
        peer2PermissionConfigurator.resetPolicy();

        peer1Bus.unregisterApplicationStateListener(appStateListener);
        peer2Bus.unregisterApplicationStateListener(appStateListener);

        if (securityManagerBus != null) {
            assertTrue(securityManagerBus.isConnected());
            securityManagerBus.disconnect();
            assertFalse(securityManagerBus.isConnected());
            securityManagerBus.release();
        }

        if (peer1Bus != null) {
            assertTrue(peer1Bus.isConnected());
            peer1Bus.disconnect();
            assertFalse(peer1Bus.isConnected());
            peer1Bus.release();
        }

        if (peer2Bus != null) {
            assertTrue(peer2Bus.isConnected());
            peer2Bus.disconnect();
            assertFalse(peer2Bus.isConnected());
            peer2Bus.release();
        }
    }

    private UUID managerGuid;

    private ApplicationStateListener appStateListener = new ApplicationStateListener() {
        public void state(String busName, KeyInfoNISTP256 publicKeyInfo, PermissionConfigurator.ApplicationState state) {
            stateMap.put(busName, state);
        }
    };

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

    public class SessionPortListenerImpl extends SessionPortListener {
        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts) {
            return true;
        }

        public void sessionJoined(short sessionPort, int id, String joiner) {}
    }

    public void installMembershipOnManager(SecurityApplicationProxy sapWithManagerBus) throws BusException {
        //Get manager key
        PermissionConfigurator pcManager = securityManagerBus.getPermissionConfigurator();
        KeyInfoNISTP256 managerKey = pcManager.getSigningPublicKey();

        String membershipSerial = "1";
        CertificateX509 managerMembershipCertificate [] = new CertificateX509[1];
        managerMembershipCertificate[0] = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);
        managerMembershipCertificate[0].setSerial(membershipSerial.getBytes());
        ByteBuffer bbIssuer = ByteBuffer.wrap(new byte[16]);
        bbIssuer.putLong(managerGuid.getMostSignificantBits());
        bbIssuer.putLong(managerGuid.getLeastSignificantBits());
        managerMembershipCertificate[0].setIssuerCN(bbIssuer.array());
        managerMembershipCertificate[0].setSubjectCN(securityManagerBus.getUniqueName().getBytes());
        managerMembershipCertificate[0].setSubjectPublicKey(managerKey.getPublicKey());
        managerMembershipCertificate[0].setSubjectAltName(bbIssuer.array());
        managerMembershipCertificate[0].setCA(false);
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 3600;
        managerMembershipCertificate[0].setValidity(validFrom, validTo);
        pcManager.signCertificate(managerMembershipCertificate[0]);

        sapWithManagerBus.installMembership(managerMembershipCertificate);
        CertificateId[] certId = pcManager.getMembershipSummaries();
        assertTrue(certId.length != 0);
    }

    public CertificateX509[] createIdentityCert(String serial, ECCPublicKey pubKey, String alias, long expiration,
            BusAttachment bus) throws BusException {
        PermissionConfigurator pc = bus.getPermissionConfigurator();
        CertificateX509 identityCertificate[] = new CertificateX509[1];
        identityCertificate[0] = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        identityCertificate[0].setSerial(serial.getBytes());
        ByteBuffer bbIssuer = ByteBuffer.wrap(new byte[16]);
        bbIssuer.putLong(managerGuid.getMostSignificantBits());
        bbIssuer.putLong(managerGuid.getLeastSignificantBits());
        identityCertificate[0].setIssuerCN(bbIssuer.array());
        identityCertificate[0].setSubjectCN(bus.getUniqueName().getBytes());
        identityCertificate[0].setSubjectPublicKey(pubKey);
        identityCertificate[0].setSubjectAltName(alias.getBytes());
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + expiration;
        identityCertificate[0].setValidity(validFrom, validTo);

        pc.signCertificate(identityCertificate[0]);
        identityCertificate[0].verify(pc.getSigningPublicKey().getPublicKey());
        return identityCertificate;
    }

    public void signalHandler(String string) throws BusException {
        receivedChirp++;
    }

    public void testAllowAny() throws Exception {
        // Update policies
        String policy = PolicyTestHelper.POLICY_BEGIN +
            PolicyTestHelper.CA_ACL +
            PolicyTestHelper.ACL_ANY +
            PolicyTestHelper.ACL_END +
            PolicyTestHelper.MEMBER_ACL +
            PolicyTestHelper.ACL_ANY +
            PolicyTestHelper.ACL_END +
            PolicyTestHelper.POLICY_END;

        policy = policy.replace(PolicyTestHelper.REPLACE_CA,
                CertificateX509.encodePublicKeyPEM(securityManagerKey.getPublicKey()));

        policy = policy.replace(PolicyTestHelper.REPLACE_MEMBER,
                CertificateX509.encodePublicKeyPEM(securityManagerKey.getPublicKey()));
        policy = policy.replace(PolicyTestHelper.REPLACE_MEMBER_GUID,
                managerGuid.toString().replace("-", ""));

        sapWithPeer1.updatePolicy(policy);
        sapWithPeer2.updatePolicy(policy);
        sapWithPeer1.endManagement();
        sapWithPeer2.endManagement();

        // Call proxy methods
        assertEquals(Status.OK, peer1Bus.secureConnection(peer2Bus.getUniqueName(), true));

        ProxyBusObject proxyObj = peer1Bus.getProxyBusObject(peer2Bus.getUniqueName(),
                SERVICE_PATH_PEER2, BusAttachment.SESSION_ID_ANY, new Class<?>[] { EchoChirpInterface.class });
        EchoChirpInterface proxy = proxyObj.getInterface(EchoChirpInterface.class);

        // Test proxy call without having joined session with peer2
        assertEquals(0, receivedEcho);
        assertEquals("message", proxy.echo("message"));
        assertEquals(1, receivedEcho);

        // Join session between peer1 and peer2
        Mutable.IntegerValue peer1ToPeer2SessionId = new Mutable.IntegerValue((short) 0);
        assertEquals(Status.OK, peer1Bus.joinSession(peer2Bus.getUniqueName(), defaultSessionPort.value,
                peer1ToPeer2SessionId, new SessionOpts(), new SessionListener()));

        // Test proxy call with having joined session with peer2
        assertEquals("message2", proxy.echo("message2"));
        assertEquals(2, receivedEcho);

        assertEquals(0, receivedGetProp1);
        assertEquals(1, proxy.getProp1());
        assertEquals(1, receivedGetProp1);

        assertEquals(0, receivedSetProp1);
        proxy.setProp1(11);
        assertEquals(1, receivedSetProp1);
        assertEquals(11, proxy.getProp1());
        assertEquals(2, receivedGetProp1);

        assertEquals(Status.OK, peer1Bus.registerSignalHandler(
                    "org.allseen.bus.EchoChirpInterface",
                    "chirp",
                    this,
                    getClass().getMethod("signalHandler",
                    String.class)));

        EchoChirpInterface chirper = localEmitter.getInterface(EchoChirpInterface.class);

        // Test proxy call with having joined session with peer2
        assertEquals(0, receivedChirp);
        chirper.chirp("message3");
        for (int x = 0; (x < 50) && (receivedChirp == 0); x++) {
            Thread.sleep(100);
        }
        assertEquals(1, receivedChirp);
    }

    public void testAllowMethods() throws Exception {
        // Update policies
        String policy = PolicyTestHelper.POLICY_BEGIN +
            PolicyTestHelper.CA_ACL +
            PolicyTestHelper.ACL_METHOD +
            PolicyTestHelper.ACL_END +
            PolicyTestHelper.MEMBER_ACL +
            PolicyTestHelper.ACL_ANY +
            PolicyTestHelper.ACL_END +
            PolicyTestHelper.POLICY_END;

        policy = policy.replace(PolicyTestHelper.REPLACE_CA,
                CertificateX509.encodePublicKeyPEM(securityManagerKey.getPublicKey()));

        policy = policy.replace(PolicyTestHelper.REPLACE_MEMBER,
                CertificateX509.encodePublicKeyPEM(securityManagerKey.getPublicKey()));
        policy = policy.replace(PolicyTestHelper.REPLACE_MEMBER_GUID,
                managerGuid.toString().replace("-", ""));

        sapWithPeer1.updatePolicy(policy);
        sapWithPeer2.updatePolicy(policy);
        sapWithPeer1.endManagement();
        sapWithPeer2.endManagement();

        // Call proxy methods
        assertEquals(Status.OK, peer1Bus.secureConnection(peer2Bus.getUniqueName(), true));

        ProxyBusObject proxyObj = peer1Bus.getProxyBusObject(peer2Bus.getUniqueName(),
                SERVICE_PATH_PEER2, BusAttachment.SESSION_ID_ANY, new Class<?>[] { EchoChirpInterface.class });
        EchoChirpInterface proxy = proxyObj.getInterface(EchoChirpInterface.class);

        // Test proxy call without having joined session with peer2
        assertEquals(0, receivedEcho);
        assertEquals("message", proxy.echo("message"));
        assertEquals(1, receivedEcho);

        // Join session between peer1 and peer2
        Mutable.IntegerValue peer1ToPeer2SessionId = new Mutable.IntegerValue((short) 0);
        assertEquals(Status.OK, peer1Bus.joinSession(peer2Bus.getUniqueName(), defaultSessionPort.value,
                peer1ToPeer2SessionId, new SessionOpts(), new SessionListener()));

        // Test proxy call with having joined session with peer2
        assertEquals("message2", proxy.echo("message2"));
        assertEquals(2, receivedEcho);

        try {
            proxy.getProp1();
            fail("Properties are not allowed, and the getProp call succeeded");
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_" + Status.PERMISSION_DENIED);
        }

        try {
            proxy.setProp1(11);
            fail("Properties are not allowed, and the setProp call succeeded");
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_" + Status.PERMISSION_DENIED);
        }

        assertEquals(Status.OK, peer1Bus.registerSignalHandler(
                    "org.allseen.bus.EchoChirpInterface",
                    "chirp",
                    this,
                    getClass().getMethod("signalHandler",
                    String.class)));

        EchoChirpInterface chirper = localEmitter.getInterface(EchoChirpInterface.class);

        chirper.chirp("message3");
        Thread.sleep(1000);
        assertEquals(0, receivedChirp);
    }

    public void testAllowProperties() throws Exception {
        // Update policies
        String policy = PolicyTestHelper.POLICY_BEGIN +
            PolicyTestHelper.CA_ACL +
            PolicyTestHelper.ACL_PROPERTY +
            PolicyTestHelper.ACL_END +
            PolicyTestHelper.MEMBER_ACL +
            PolicyTestHelper.ACL_ANY +
            PolicyTestHelper.ACL_END +
            PolicyTestHelper.POLICY_END;

        policy = policy.replace(PolicyTestHelper.REPLACE_CA,
                CertificateX509.encodePublicKeyPEM(securityManagerKey.getPublicKey()));

        policy = policy.replace(PolicyTestHelper.REPLACE_MEMBER,
                CertificateX509.encodePublicKeyPEM(securityManagerKey.getPublicKey()));
        policy = policy.replace(PolicyTestHelper.REPLACE_MEMBER_GUID,
                managerGuid.toString().replace("-", ""));

        sapWithPeer1.updatePolicy(policy);
        sapWithPeer2.updatePolicy(policy);
        sapWithPeer1.endManagement();
        sapWithPeer2.endManagement();

        // Call proxy methods
        assertEquals(Status.OK, peer1Bus.secureConnection(peer2Bus.getUniqueName(), true));

        ProxyBusObject proxyObj = peer1Bus.getProxyBusObject(peer2Bus.getUniqueName(),
                SERVICE_PATH_PEER2, BusAttachment.SESSION_ID_ANY, new Class<?>[] { EchoChirpInterface.class });
        EchoChirpInterface proxy = proxyObj.getInterface(EchoChirpInterface.class);

        // Join session between peer1 and peer2
        Mutable.IntegerValue peer1ToPeer2SessionId = new Mutable.IntegerValue((short) 0);
        assertEquals(Status.OK, peer1Bus.joinSession(peer2Bus.getUniqueName(), defaultSessionPort.value,
                peer1ToPeer2SessionId, new SessionOpts(), new SessionListener()));

        // Test proxy call with having joined session with peer2
        try {
            proxy.echo("message2");
            fail("Methods are not allowed, and the echo method succeeded");
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_" + Status.PERMISSION_DENIED);
        }

        assertEquals(0, receivedGetProp1);
        assertEquals(1, proxy.getProp1());
        assertEquals(1, receivedGetProp1);

        assertEquals(0, receivedSetProp1);
        proxy.setProp1(11);
        assertEquals(1, receivedSetProp1);
        assertEquals(11, proxy.getProp1());
        assertEquals(2, receivedGetProp1);

        assertEquals(Status.OK, peer1Bus.registerSignalHandler(
                    "org.allseen.bus.EchoChirpInterface",
                    "chirp",
                    this,
                    getClass().getMethod("signalHandler",
                    String.class)));

        EchoChirpInterface chirper = localEmitter.getInterface(EchoChirpInterface.class);

        chirper.chirp("message3");
        Thread.sleep(1000);
        assertEquals(0, receivedChirp);
    }

    public void testAllowSignals() throws Exception {
        // Update policies
        String policy = PolicyTestHelper.POLICY_BEGIN +
            PolicyTestHelper.CA_ACL +
            PolicyTestHelper.ACL_SIGNAL +
            PolicyTestHelper.ACL_END +
            PolicyTestHelper.MEMBER_ACL +
            PolicyTestHelper.ACL_ANY +
            PolicyTestHelper.ACL_END +
            PolicyTestHelper.POLICY_END;

        policy = policy.replace(PolicyTestHelper.REPLACE_CA,
                CertificateX509.encodePublicKeyPEM(securityManagerKey.getPublicKey()));

        policy = policy.replace(PolicyTestHelper.REPLACE_MEMBER,
                CertificateX509.encodePublicKeyPEM(securityManagerKey.getPublicKey()));
        policy = policy.replace(PolicyTestHelper.REPLACE_MEMBER_GUID,
                managerGuid.toString().replace("-", ""));

        sapWithPeer1.updatePolicy(policy);
        sapWithPeer2.updatePolicy(policy);
        sapWithPeer1.endManagement();
        sapWithPeer2.endManagement();

        // Call proxy methods
        assertEquals(Status.OK, peer1Bus.secureConnection(peer2Bus.getUniqueName(), true));

        ProxyBusObject proxyObj = peer1Bus.getProxyBusObject(peer2Bus.getUniqueName(),
                SERVICE_PATH_PEER2, BusAttachment.SESSION_ID_ANY, new Class<?>[] { EchoChirpInterface.class });
        EchoChirpInterface proxy = proxyObj.getInterface(EchoChirpInterface.class);

        // Join session between peer1 and peer2
        Mutable.IntegerValue peer1ToPeer2SessionId = new Mutable.IntegerValue((short) 0);
        assertEquals(Status.OK, peer1Bus.joinSession(peer2Bus.getUniqueName(), defaultSessionPort.value,
                peer1ToPeer2SessionId, new SessionOpts(), new SessionListener()));

        // Test proxy call with having joined session with peer2
        try {
            proxy.echo("message2");
            fail("Methods are not allowed, and the echo method succeeded");
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_" + Status.PERMISSION_DENIED);
        }

        try {
            proxy.getProp1();
            fail("Properties are not allowed, and the getProp call succeeded");
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_" + Status.PERMISSION_DENIED);
        }

        try {
            proxy.setProp1(11);
            fail("Properties are not allowed, and the setProp call succeeded");
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_" + Status.PERMISSION_DENIED);
        }
        assertEquals(Status.OK, peer1Bus.registerSignalHandler(
                    "org.allseen.bus.EchoChirpInterface",
                    "chirp",
                    this,
                    getClass().getMethod("signalHandler",
                    String.class)));

        EchoChirpInterface chirper = localEmitter.getInterface(EchoChirpInterface.class);

        chirper.chirp("message3");
        for (int x = 0; (x < 50) && (receivedChirp == 0); x++) {
            Thread.sleep(100);
        }
        assertEquals(1, receivedChirp);
    }
}
