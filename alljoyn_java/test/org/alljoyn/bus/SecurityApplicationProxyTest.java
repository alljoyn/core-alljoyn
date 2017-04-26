/******************************************************************************
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
******************************************************************************/
package org.alljoyn.bus;

import junit.framework.TestCase;
import static org.alljoyn.bus.Assert.*;

import java.lang.InterruptedException;
import java.nio.ByteBuffer;
import java.util.UUID;
import java.io.File;
import java.util.Arrays;

import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.CryptoECC;

public class SecurityApplicationProxyTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment busAttachment;
    private BusAttachment peer1Bus;

    /**
     * permissionConfigurator for busAttachment.
     *
     * @see busAttachment
     */
    private PermissionConfigurator permissionConfigurator;

    private Mutable.ShortValue defaultSessionPort;
    private SessionOpts defaultSessionOpts;

    private String defaultManifestTemplate = "<manifest>" +
    "<node>" +
    "<interface name=\"org.alljoyn.security2.test\">" +
    "<method name=\"Up\">" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
    "</method>" +
    "<method name=\"Down\">" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
    "</method>" +
    "</interface>" +
    "<interface name=\"org.allseenalliance.control.Mouse*\">" +
    "<any>" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
    "</any>" +
    "</interface>" +
    "</node>" +
    "</manifest>";

    private static final short PORT = 42;

    private static final UUID guid = UUID.randomUUID();
    public void setUp() throws Exception {
        busAttachment = new BusAttachment("PermissionConfiguratorTest");
        busAttachment.connect();
        registerAuthListener(busAttachment);

        permissionConfigurator = busAttachment.getPermissionConfigurator();
        permissionConfigurator.setManifestTemplateFromXml(defaultManifestTemplate);

        peer1Bus = new BusAttachment("peer1Bus");
        peer1Bus.connect();
        registerAuthListener(peer1Bus);
        peer1Bus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestTemplate);

        defaultSessionPort = new Mutable.ShortValue(PORT);
        defaultSessionOpts = new SessionOpts();
    }

    public void tearDown() throws Exception {
        busAttachment.disconnect();
        busAttachment.release();
        busAttachment = null;
        permissionConfigurator = null;

        peer1Bus.disconnect();
        peer1Bus.release();
        peer1Bus = null;
    }

    private Status registerAuthListener(BusAttachment bus) throws Exception {
        Status status = Status.OK;
        if (System.getProperty("os.name").startsWith("Windows")) {
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", mAuthListener, null, false, pclistener);
        } else if (System.getProperty("java.vm.name").startsWith("Dalvik")) {
            /*
             * on some Android devices File.createTempFile trys to create a file in
             * a location we do not have permission to write to.  Resulting in a
             * java.io.IOException: Permission denied error.
             * This code assumes that the junit tests will have file IO permission
             * for /data/data/org.alljoyn.bus
             */
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", mAuthListener,
                            "/data/data/org.alljoyn.bus/files/alljoyn.ks", false, pclistener);
        } else {
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", mAuthListener,
                            File.createTempFile(bus.getUniqueName().replaceAll(":", ""), "ks").getAbsolutePath(), false, pclistener);
        }
        return status;
    }

    public void testGetters() throws Exception {
        SecurityApplicationProxy sap = new SecurityApplicationProxy(busAttachment,
                busAttachment.getUniqueName(),
                (short) 0);
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, sap.getApplicationState());

        assertEquals(1, sap.getSecurityApplicationVersion());

        byte[] temp = new byte[]{-32, 103, 61, -16, 63, 63, 29, -39, 39, 110, -31, -18, -42, 2, -32, -109, 85, 105, 74, -50, -50, -76, 104, 0, -120, -113, 99, 114, -44, 64, -71, 71};
        assertArrayEquals(temp, sap.getManifestTemplateDigest());

        assertNotNull(sap.getManifestTemplate());

        assertEquals(PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL |
                PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_SPEKE |
                PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_PSK,
                sap.getClaimCapabilities());

        assertEquals(0, sap.getClaimCapabilityAdditionalInfo());
        assertEquals(1, sap.getClaimableApplicationVersion());
    }

    /**
     * busAttachment will claim peer1 bus through
     * security application proxy.
     * Afterwards, this test will test getters
     */
    public void testGettersAfterClaiming() throws Exception {
        Mutable.IntegerValue sessionId = new Mutable.IntegerValue(0);

        assertEquals(Status.OK, peer1Bus.bindSessionPort(
                    defaultSessionPort,
                    defaultSessionOpts,
                    new SessionPortListenerImpl()));

        assertEquals(Status.OK, busAttachment.joinSession(
                    peer1Bus.getUniqueName(),
                    defaultSessionPort.value,
                    sessionId,
                    defaultSessionOpts,
                    new SessionListener()));

        KeyInfoNISTP256 busKey = permissionConfigurator.getSigningPublicKey();
        ECCPublicKey busPublicKey = busKey.getPublicKey();

        //Create identityCertChain
        CertificateX509 busIdentityCertChain[] = CertificateTestHelper.createIdentityCert("0",
                    guid,
                    busPublicKey,
                    "Alias",
                    3600,
                    busAttachment);

        String signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(
                busIdentityCertChain[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        permissionConfigurator.claim(busKey,
                    guid,
                    busKey,
                    busIdentityCertChain,
                    new String[]{signedManifest});

        //Create membershipCertChain
        CertificateX509 membershipCert[] = CertificateTestHelper.createMembershipCert("0",
                    guid,
                    false,
                    busPublicKey,
                    3600,
                    busAttachment);

        permissionConfigurator.installMembership(membershipCert);

        SecurityApplicationProxy sap = new SecurityApplicationProxy(busAttachment,
                peer1Bus.getUniqueName(),
                sessionId.value);
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, sap.getApplicationState());

        ECCPublicKey peerPublicKey = peer1Bus.getPermissionConfigurator()
            .getSigningPublicKey()
            .getPublicKey();

        //Create identityCertChain for claiming peer1
        CertificateX509 peer1IdentityCertChain[] = CertificateTestHelper.createIdentityCert("0",
                    guid,
                    peerPublicKey,
                    "Alias",
                    3600,
                    busAttachment);

        signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(
                peer1IdentityCertChain[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        sap.claim(busKey,
                    guid,
                    busKey,
                    peer1IdentityCertChain,
                    new String[]{signedManifest});

        busAttachment.secureConnection(null, true);
        sap.endManagement();
        sap.startManagement();
        assertEquals(1, sap.getSecurityApplicationVersion());

        byte[] temp = new byte[]{-32, 103, 61, -16, 63, 63, 29, -39, 39, 110, -31, -18, -42, 2, -32, -109, 85, 105, 74, -50, -50, -76, 104, 0, -120, -113, 99, 114, -44, 64, -71, 71};
        busAttachment.secureConnection(null, true);
        assertArrayEquals(temp, sap.getManifestTemplateDigest());

        assertNotNull(sap.getManifestTemplate());

        assertEquals(PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL |
                PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_SPEKE |
                PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_PSK,
                sap.getClaimCapabilities());

        assertEquals(0, sap.getClaimCapabilityAdditionalInfo());
        assertEquals(1, sap.getClaimableApplicationVersion());

        CertificateX509[] manufacturerCert = sap.getManufacturerCertificate();
        assertEquals(0, sap.getPolicyVersion());
        CertificateX509[] identity = sap.getIdentity();
        assertArrayEquals(identity[0].encodeCertificateDER(),
                peer1IdentityCertChain[0].encodeCertificateDER());

        assertEquals(2, sap.getManagedApplicationVersion());

        sap.computeManifestDigest(ManifestTestHelper.MANIFEST_ALL_INCLUSIVE,
                peer1IdentityCertChain[0]);

        CertificateX509 peer1IdentityCertChain2[] = CertificateTestHelper.createIdentityCert("12345",
                    guid,
                    peerPublicKey,
                    "Alias",
                    3600,
                    busAttachment);
        signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(
                peer1IdentityCertChain2[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);


        sap.updateIdentity(peer1IdentityCertChain2, new String[]{signedManifest});

        identity = sap.getIdentity();
        assertArrayEquals(identity[0].encodeCertificateDER(),
                peer1IdentityCertChain2[0].encodeCertificateDER());
    }

    public void testInstallAndRemoveMembership() throws Exception {
        Mutable.IntegerValue sessionId = new Mutable.IntegerValue(0);

        assertEquals(Status.OK, peer1Bus.bindSessionPort(
                    defaultSessionPort,
                    defaultSessionOpts,
                    new SessionPortListenerImpl()));

        assertEquals(Status.OK, busAttachment.joinSession(
                    peer1Bus.getUniqueName(),
                    defaultSessionPort.value,
                    sessionId,
                    defaultSessionOpts,
                    new SessionListener()));

        KeyInfoNISTP256 busKey = permissionConfigurator.getSigningPublicKey();
        ECCPublicKey busPublicKey = busKey.getPublicKey();

        //Create identityCertChain
        CertificateX509 busIdentityCertChain[] = CertificateTestHelper.createIdentityCert("0",
                    guid,
                    busPublicKey,
                    "Alias",
                    3600,
                    busAttachment);

        String signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(
                busIdentityCertChain[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        permissionConfigurator.claim(busKey,
                    guid,
                    busKey,
                    busIdentityCertChain,
                    new String[]{signedManifest});

        //Create membershipCertChain
        CertificateX509 membershipCert[] = CertificateTestHelper.createMembershipCert("0",
                    guid,
                    false,
                    busPublicKey,
                    3600,
                    busAttachment);

        permissionConfigurator.installMembership(membershipCert);

        SecurityApplicationProxy sap = new SecurityApplicationProxy(busAttachment,
                peer1Bus.getUniqueName(),
                sessionId.value);
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, sap.getApplicationState());

        ECCPublicKey peerPublicKey = peer1Bus.getPermissionConfigurator()
            .getSigningPublicKey()
            .getPublicKey();

        //Create identityCertChain for claiming peer1
        CertificateX509 peer1IdentityCertChain[] = CertificateTestHelper.createIdentityCert("0",
                    guid,
                    peerPublicKey,
                    "Alias",
                    3600,
                    busAttachment);

        signedManifest = permissionConfigurator.computeThumbprintAndSignManifestXml(
                peer1IdentityCertChain[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        sap.claim(busKey,
                    guid,
                    busKey,
                    peer1IdentityCertChain,
                    new String[]{signedManifest});

        busAttachment.secureConnection(null, true);
        sap.endManagement();
        sap.startManagement();


        CertificateX509 membershipCert2[] = CertificateTestHelper.createMembershipCert("1",
                    UUID.randomUUID(),
                    false,
                    peer1Bus.getPermissionConfigurator().getSigningPublicKey().getPublicKey(),
                    3600,
                    busAttachment);

        CertificateId[] certId = peer1Bus.getPermissionConfigurator().getMembershipSummaries();
        assertEquals(certId.length, 0);
        sap.installMembership(membershipCert2);
        certId = peer1Bus.getPermissionConfigurator().getMembershipSummaries();
        assertEquals(certId.length, 1);

        sap.removeMembership("1", permissionConfigurator.getSigningPublicKey());
        certId = peer1Bus.getPermissionConfigurator().getMembershipSummaries();
        assertEquals(certId.length, 0);

        sap.resetPolicy();
    }

    private boolean factoryReset = false;
    private boolean policyChanged = false;
    private boolean startManagement = false;
    private boolean endManagement = false;
    private PermissionConfigurationListener pclistener = new PermissionConfigurationListener() {

        public Status factoryReset() {
            factoryReset = true;
            return Status.OK;
        }

        public void policyChanged() {
            policyChanged = true;
        }

        public void startManagement() {
            startManagement = true;
        }

        public void endManagement() {
            endManagement = true;
        }
    };

    public class SessionPortListenerImpl extends SessionPortListener {
        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts) {
            return true;
        }

        public void sessionJoined(short sessionPort, int id, String joiner) {}
    }

    private AuthListener mAuthListener = new AuthListener() {
        @Override
        public boolean requested(String mech, String authPeer, int count, String userName, AuthRequest[] requests) {
            return true;
        }

        @Override
        public void completed(String mech, String peer, boolean authenticated) {}
    };
}

