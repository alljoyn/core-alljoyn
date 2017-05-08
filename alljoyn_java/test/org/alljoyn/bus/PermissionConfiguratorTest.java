/*
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
import static org.alljoyn.bus.Assert.*;

import java.lang.InterruptedException;
import java.nio.ByteBuffer;
import java.util.UUID;

import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.CryptoECC;

public class PermissionConfiguratorTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment busAttachment;
    private PermissionConfigurator permissionConfigurator;

    private BusAttachment peer1Bus;
    private PermissionConfigurator pcPeer1;

    private static final String defaultManifestTemplate = "<manifest>" +
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

    public void setUp() throws Exception {
        busAttachment = new BusAttachment("PermissionConfiguratorTest");
        busAttachment.connect();
        permissionConfigurator = busAttachment.getPermissionConfigurator();

        peer1Bus = new BusAttachment("peer1Bus");
        peer1Bus.connect();
        SecurityTestHelper.registerAuthListener(peer1Bus, pclistener);
        pcPeer1 = peer1Bus.getPermissionConfigurator();
    }

    public void tearDown() throws Exception {
        busAttachment.disconnect();
        busAttachment.release();
        busAttachment = null;
        permissionConfigurator = null;
    }

    public void testError() throws Exception {
        //Need to register AuthListener to be able to use permissionConfigurator
        try {
            permissionConfigurator.getApplicationState();
            fail("didn't fail from unregistered AuthListener");
        } catch (Exception e) {
            assertEquals(e.getMessage(), "ER_" + Status.FEATURE_NOT_AVAILABLE);
        }
    }

    public void testNotClaimable() throws Exception {
        assertEquals(Status.OK, SecurityTestHelper.registerAuthListener(busAttachment));
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, permissionConfigurator.getApplicationState());
    }

    public void testSettersAndGetters() throws Exception {
        assertEquals(Status.OK, SecurityTestHelper.registerAuthListener(busAttachment));

        permissionConfigurator.setManifestTemplateFromXml(defaultManifestTemplate);

        assertNotNull(permissionConfigurator.getManifestTemplateAsXml());
        permissionConfigurator.setApplicationState(PermissionConfigurator.ApplicationState.CLAIMED);
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED, permissionConfigurator.getApplicationState());

        permissionConfigurator.setApplicationState(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE);
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, permissionConfigurator.getApplicationState());

        permissionConfigurator.setApplicationState(PermissionConfigurator.ApplicationState.CLAIMABLE);
        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, permissionConfigurator.getApplicationState());

        permissionConfigurator.setApplicationState(PermissionConfigurator.ApplicationState.NEED_UPDATE);
        assertEquals(PermissionConfigurator.ApplicationState.NEED_UPDATE, permissionConfigurator.getApplicationState());

        permissionConfigurator.setClaimCapabilities(PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL);
        assertEquals(PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL, permissionConfigurator.getClaimCapabilities());

        permissionConfigurator.setClaimCapabilities((short) (PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL |
                PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_PSK));
        assertEquals(PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL |
                PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_PSK,
                permissionConfigurator.getClaimCapabilities());

        permissionConfigurator.setClaimCapabilityAdditionalInfo(PermissionConfigurator.CLAIM_CAPABILITY_ADDITIONAL_PSK_GENERATED_BY_SECURITY_MANAGER);
        assertEquals(PermissionConfigurator.CLAIM_CAPABILITY_ADDITIONAL_PSK_GENERATED_BY_SECURITY_MANAGER,
                permissionConfigurator.getClaimCapabilityAdditionalInfo());

        KeyInfoNISTP256 securityManagerKey = permissionConfigurator.getSigningPublicKey();
    }

    /**
     * Refactored from SecurityOfflineClaimTest.cc
     * Claim_using_PermissionConfigurator_session_successful
     */
    public void testGetAndUpdateIdentity() throws Exception {

        pcPeer1.setManifestTemplateFromXml(ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE,
                pcPeer1.getApplicationState());

        //Getting Parameters for claim call
        KeyInfoNISTP256 pcPeer1Key = pcPeer1.getSigningPublicKey();
        UUID guid = UUID.randomUUID();

        //Peer public key used to generate the identity certificate chain
        ECCPublicKey peer1PublicKey = pcPeer1Key.getPublicKey();

        //Create identityCertChain
        CertificateX509 identityCertChain[] = CertificateTestHelper.createIdentityCert("0",
                    guid,
                    peer1PublicKey,
                    "Alias",
                    3600,
                    peer1Bus);

        String signedManifest = pcPeer1.computeThumbprintAndSignManifestXml(
                identityCertChain[0],
                ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        /*
         * Claim Peer1
         * the certificate authority is self signed so the certificateAuthority
         * key is the same as the adminGroup key.
         * For this test the guid is a randomly generated GUID. As long as the
         * GUID is consistent it's unimportant that the GUID is random.
         * Use generated identity certificate signed by the securityManager
         * Since we are only interested in claiming the peer we are using an all
         * inclusive manifest.
         */
        pcPeer1.claim(pcPeer1Key,
                    guid,
                    pcPeer1Key,
                    identityCertChain,
                    new String[]{signedManifest});

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED, pcPeer1.getApplicationState());

        CertificateX509[] returnedCertChain = pcPeer1.getIdentity();
        assertEquals(1, returnedCertChain.length);

        assertArrayEquals(identityCertChain[0].encodeCertificateDER(), returnedCertChain[0].encodeCertificateDER());

        CertificateId certId = pcPeer1.getIdentityCertificateId();
        assertArrayEquals(identityCertChain[0].getSerial(), certId.getSerial().getBytes());

        //Create identityCertChain
        CertificateX509 identityCertChain2[] = CertificateTestHelper.createIdentityCert("01234",
                    guid,
                    peer1PublicKey,
                    "Alias",
                    3600,
                    peer1Bus);

        pcPeer1.updateIdentity(identityCertChain2, new String[]{signedManifest});

        CertificateId certId2 = pcPeer1.getIdentityCertificateId();
        assertEquals(certId2.getSerial(), new String(identityCertChain2[0].getSerial()));
        assertEquals(certId2.getIssuerKeyInfo().getPublicKey(), peer1PublicKey);

        returnedCertChain = pcPeer1.getIdentity();
        assertEquals(1, returnedCertChain.length);

        assertArrayEquals(identityCertChain2[0].encodeCertificateDER(), returnedCertChain[0].encodeCertificateDER());
    }

    public void testReset() throws Exception {

        testGetAndUpdateIdentity();

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED,
                pcPeer1.getApplicationState());

        policyChanged = false;
        pcPeer1.resetPolicy();
        assertTrue(policyChanged);

        factoryReset = false;
        pcPeer1.reset();
        assertTrue(factoryReset);

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE,
                pcPeer1.getApplicationState());

    }

    public void testGetConnectedPeerPublicKey() throws Exception {
        /**
         * see alljoyn_core/unit_test/PermissionMgmtUseCaseTest.cc
         * this test uses CredentialAccessor, a private class,
         * to use GetConnectedPeerPublicKey.
         */
    }

    public void testInstallMembershipsAndManifests() throws Exception {
        testGetAndUpdateIdentity();

        UUID guid = UUID.randomUUID();
        ECCPublicKey peer1PublicKey = pcPeer1.getSigningPublicKey().getPublicKey();

        //Create identityCertChain
        CertificateX509 identityCertChain[] = CertificateTestHelper.createIdentityCert("0",
                    guid,
                    peer1PublicKey,
                    "Alias",
                    3600,
                    peer1Bus);

        String signedManifest = pcPeer1.computeThumbprintAndSignManifestXml(
                identityCertChain[0],
                defaultManifestTemplate);

        pcPeer1.installManifests(new String[] {signedManifest}, true);

        assertEquals(pcPeer1.getMembershipSummaries().length, 0);

        CertificateX509[] membership = CertificateTestHelper.createMembershipCert(
                "0123",
                guid,
                false,
                peer1PublicKey,
                10000,
                peer1Bus);

        pcPeer1.installMembership(membership);
        assertEquals(pcPeer1.getMembershipSummaries().length, 1);

        CertificateX509[] membership2 = CertificateTestHelper.createMembershipCert(
                "01234",
                guid,
                false,
                peer1PublicKey,
                10000,
                peer1Bus);

        pcPeer1.installMembership(membership2);
        assertEquals(pcPeer1.getMembershipSummaries().length, 2);
    }

    public void testStartAndEndManagement() throws Exception {
        testGetAndUpdateIdentity();

        startManagement = false;
        pcPeer1.startManagement();
        assertTrue(startManagement);

        endManagement = false;
        pcPeer1.endManagement();
        assertTrue(endManagement);
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
}
