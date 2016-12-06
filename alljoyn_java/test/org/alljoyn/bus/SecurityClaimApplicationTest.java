/**
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
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
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
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

public class SecurityClaimApplicationTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }
    private BusAttachment securityManagerBus;
    private BusAttachment peer1Bus;
    private BusAttachment peer2Bus;

    private static final String INTERFACE_NAME = "org.allseen.test.SecurityApplication.claim";

    @Override
    public void setUp() throws Exception {
        securityManagerBus = new BusAttachment("SecurityClaimApplicationManager", BusAttachment.RemoteMessage.Receive);
        peer1Bus = new BusAttachment("SecurityClaimApplicationPeer1", BusAttachment.RemoteMessage.Receive);
        peer2Bus = new BusAttachment("SecurityClaimApplicationPeer2", BusAttachment.RemoteMessage.Receive);

        Status status = securityManagerBus.connect();
        assertEquals(status, Status.OK);
        status = peer1Bus.connect();
        assertEquals(status, Status.OK);
        status = peer2Bus.connect();
        assertEquals(status, Status.OK);

        managerGuid = UUID.randomUUID();
    }

    @Override
    public void tearDown() throws Exception {

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

    private static final String MANIFEST_TEMPLATE =
        "<manifest>" +
        "<node name=\"*\">" +
        "<interface name=\"*\">" +
        "<any name=\"*\">" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>" +
        "</any>" +
        "</interface>" +
        "</node>" +
        "</manifest>";

    private UUID managerGuid;

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
    }

    public CertificateX509[] createIdentityCert(String serial, ECCPublicKey pubKey, String alias, long expiration, BusAttachment bus) throws BusException {
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

    private AuthListener defaultAuthListener = new AuthListener() {
        public boolean requested(String mechanism, String peerName, int count, String userName,
                      AuthRequest[] requests) { return true; }
        public void completed(String mechanism, String peerName, boolean authenticated) {}
    };

    public void testIsUnclaimableByDefault() throws BusException, IOException {
        securityManagerBus.registerAuthListener("ALLJOYN_ECDHE_NULL",
                defaultAuthListener,
                File.createTempFile("secManBus", "ks").getAbsolutePath());

        SecurityApplicationProxy saWithSecurityManager = new SecurityApplicationProxy(
                securityManagerBus,
                securityManagerBus.getUniqueName(),
                0);
        PermissionConfigurator.ApplicationState applicationStateSecurityManager = saWithSecurityManager.getApplicationState();
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, applicationStateSecurityManager);

        peer1Bus.registerAuthListener("ALLJOYN_ECDHE_NULL",
                defaultAuthListener,
                File.createTempFile("peer1Bus", "ks").getAbsolutePath());

        SecurityApplicationProxy saWithPeer1 = new SecurityApplicationProxy(securityManagerBus,
                peer1Bus.getUniqueName(),
                0);
        PermissionConfigurator.ApplicationState applicationStatePeer1 = saWithPeer1.getApplicationState();
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, applicationStatePeer1);

        peer2Bus.registerAuthListener("ALLJOYN_ECDHE_NULL",
                defaultAuthListener,
                File.createTempFile("peer2Bus", "ks").getAbsolutePath());

        SecurityApplicationProxy saWithPeer2 = new SecurityApplicationProxy(securityManagerBus, peer2Bus.getUniqueName(), 0);
        PermissionConfigurator.ApplicationState applicationStatePeer2 = saWithPeer2.getApplicationState();
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, applicationStatePeer2);
    }

    private boolean stateChanged = false;
    private ApplicationStateListener claim_AppStateListener = new ApplicationStateListener() {
        public void state(String busName, KeyInfoNISTP256 publicKeyInfo, PermissionConfigurator.ApplicationState state) {
            stateChanged = true;
        }
    };

    private static final String MANIFEST_ALL_INCLUSIVE =
        "<manifest>" +
        "<node name=\"*\">" +
        "<interface name=\"*\">" +
        "<method name=\"*\">" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>" +
        "</method>" +
        "<signal name=\"*\">" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>" +
        "</signal>" +
        "<property name=\"*\">" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>" +
        "</property>" +
        "</interface>" +
        "</node>" +
        "</manifest>";

    /*
     * Claim using ECDHE_NULL
     * Verify that claim is succesful using an ECDHE_NULL based session, where the
     * CA public key and the group public key are the same.
     *
     * Test Case:
     * Claim using ECDHE_NULL
     * caPublic key == adminGroupSecurityPublicKey
     * Identity = Single certificate signed by CA
     */
    public void testClaim_using_ECDHE_NULL_session_successful() throws Exception, IOException {
        securityManagerBus.registerApplicationStateListener(claim_AppStateListener);

        securityManagerBus.registerAuthListener("ALLJOYN_ECDHE_NULL",
                defaultAuthListener,
                File.createTempFile("secManBus", "ks").getAbsolutePath());

        /* The State signal is only emitted if manifest template is installed */
        securityManagerBus.getPermissionConfigurator().setManifestTemplateFromXml(MANIFEST_TEMPLATE);

        //Wait for the Application.State Signal.
        for (int msec = 0; msec < 10000; msec += 5) {
            if (stateChanged) {
                break;
            }
            Thread.sleep(5);
        }

        stateChanged = false;

        peer1Bus.registerAuthListener("ALLJOYN_ECDHE_NULL",
                defaultAuthListener,
                File.createTempFile("peer1Bus", "ks").getAbsolutePath());

        /* The State signal is only emitted if manifest template is installed */
        peer1Bus.getPermissionConfigurator().setManifestTemplateFromXml(MANIFEST_TEMPLATE);

        //Wait for the Application.State Signal.
        for (int msec = 0; msec < 10000; msec += 5) {
            if (stateChanged) {
                break;
            }
            Thread.sleep(5);
        }
        stateChanged = false;

        SecurityApplicationProxy sapWithPeer1 = new SecurityApplicationProxy(
                securityManagerBus,
                peer1Bus.getUniqueName(),
                0);

        PermissionConfigurator.ApplicationState applicationStatePeer1 = sapWithPeer1.getApplicationState();
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, applicationStatePeer1);

        //Create admin group key
        KeyInfoNISTP256 securityManagerKey = securityManagerBus.getPermissionConfigurator().getSigningPublicKey();

        // peer public key used to generate the identity certificate chain
        ECCPublicKey peer1PublicKey = sapWithPeer1.getEccPublicKey();

        //Create identityCertChain
        CertificateX509 identityCertChain[] = createIdentityCert("0",
                peer1PublicKey,
                "Alias",
                3600,
                securityManagerBus);
        String signedManifest = securityManagerBus.getPermissionConfigurator().computeThumbprintAndSignManifestXml(
                identityCertChain[0],
                MANIFEST_ALL_INCLUSIVE);

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
        sapWithPeer1.claim(securityManagerKey,
                    managerGuid,
                    securityManagerKey,
                    identityCertChain,
                    new String[]{signedManifest});

        //Wait for the Application.State Signal.
        for (int msec = 0; msec < 10000; msec += 5) {
            if (stateChanged) {
                break;
            }
            Thread.sleep(5);
        }

        assertTrue(stateChanged);
        applicationStatePeer1 = sapWithPeer1.getApplicationState();
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED, applicationStatePeer1);

        securityManagerBus.unregisterApplicationStateListener(claim_AppStateListener);
    }

    /*
     * TestCase:
     * After Reset operation, app should emit the state notification and the public
     * key should be preserved.
     *
     * Procedure:
     * Verify that when admin resets the app. bus, the state notification is emitted
     *     and is received by the secondary bus.
     * Verify that Secondary bus gets the state notification.
     * The state should be "Claimable"
     * publickey algorithm = 0
     * publickey curveIdentifier = 0
     * publickey xCo-ordinate and yCo-ordinate are populated and are non-empty and
     *     are preserved and are same as before.
     */
    public void test_get_application_state_signal_for_claimed_then_reset_peer() throws BusException, IOException {
        securityManagerBus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                defaultAuthListener,
                File.createTempFile("secManBus", "ks").getAbsolutePath(),
                false,
                null);
        peer1Bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                defaultAuthListener,
                File.createTempFile("peer1Bus", "ks").getAbsolutePath(),
                false,
                null);

        SecurityApplicationProxy sapWithPeer1 = new SecurityApplicationProxy(securityManagerBus, peer1Bus.getUniqueName(), 0);
        SecurityApplicationProxy sapWithManager = new SecurityApplicationProxy(securityManagerBus, securityManagerBus.getUniqueName(), 0);

        /* The State signal is only emitted if manifest template is installed */
        securityManagerBus.getPermissionConfigurator().setManifestTemplateFromXml(MANIFEST_TEMPLATE);
        peer1Bus.getPermissionConfigurator().setManifestTemplateFromXml(MANIFEST_TEMPLATE);

        //Create admin group key
        PermissionConfigurator permissionConfigurator = securityManagerBus.getPermissionConfigurator();
        KeyInfoNISTP256 securityManagerKey = permissionConfigurator.getSigningPublicKey();

        PermissionConfigurator peerPermissionConfigurator = peer1Bus.getPermissionConfigurator();
        KeyInfoNISTP256 peer1PublicKey = peerPermissionConfigurator.getSigningPublicKey();

        //Create identityCertChain
        CertificateX509 identityCertChain[] = createIdentityCert("0",
                securityManagerKey.getPublicKey(),
                "Alias",
                3600,
                securityManagerBus);

        String signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(identityCertChain[0], MANIFEST_ALL_INCLUSIVE);

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
        signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(identityCertChain[0], MANIFEST_ALL_INCLUSIVE);

        sapWithPeer1.claim(securityManagerKey,
                    managerGuid,
                    securityManagerKey,
                    identityCertChain,
                    new String[]{signedManifest});

        installMembershipOnManager(sapWithManager);

        peer1Bus.registerApplicationStateListener(claim_AppStateListener);

        // Call Reset

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED, peerPermissionConfigurator.getApplicationState());

        securityManagerBus.secureConnection(peer1Bus.getUniqueName(), true);
        peer1Bus.secureConnection(securityManagerBus.getUniqueName(), true);
        sapWithPeer1.endManagement();
        sapWithPeer1.startManagement();
        sapWithPeer1.reset();

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, peerPermissionConfigurator.getApplicationState());

        peer1Bus.unregisterApplicationStateListener(claim_AppStateListener);
    }
}