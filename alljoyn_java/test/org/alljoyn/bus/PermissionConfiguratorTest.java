/*
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

import junit.framework.TestCase;

import java.lang.InterruptedException;
import java.nio.ByteBuffer;
import java.util.UUID;
import java.io.File;
import java.util.Arrays;

import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.CryptoECC;

public class PermissionConfiguratorTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    private PermissionConfigurator permissionConfigurator;
    private BusAttachment busAttachment;
    private BusAttachment peer1Bus;
    private BusAttachment securityManagerBus;

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

    public void setUp() throws Exception {
        busAttachment = new BusAttachment("PermissionConfiguratorTest");
        busAttachment.connect();

        peer1Bus = new BusAttachment("peer1Bus");
        peer1Bus.connect();
        registerAuthListener(peer1Bus);

        securityManagerBus = new BusAttachment("securityManagerBus");
        securityManagerBus.connect();
        registerAuthListener(securityManagerBus);
    }

    public void tearDown() throws Exception {
        busAttachment.disconnect();
        busAttachment.release();
        busAttachment = null;
        permissionConfigurator = null;
    }

    private Status registerAuthListener(BusAttachment bus) throws Exception {
        Status status = Status.OK;
        if (System.getProperty("os.name").startsWith("Windows")) {
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL", null, null, false, pclistener);
        } else if (System.getProperty("java.vm.name").startsWith("Dalvik")) {
            /*
             * on some Android devices File.createTempFile trys to create a file in
             * a location we do not have permission to write to.  Resulting in a
             * java.io.IOException: Permission denied error.
             * This code assumes that the junit tests will have file IO permission
             * for /data/data/org.alljoyn.bus
             */
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL", null,
                            "/data/data/org.alljoyn.bus/files/alljoyn.ks", false, pclistener);
        } else {
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL", null,
                            File.createTempFile(bus.getUniqueName().replaceAll(":", ""), "ks").getAbsolutePath(), false, pclistener);
        }
        return status;
    }

    public void testVeryBasic() throws Exception {
        //Need to register AuthListener to be able to use permissionConfigurator
        permissionConfigurator = busAttachment.getPermissionConfigurator();
        try {
            permissionConfigurator.getApplicationState();
            fail("didn't fail from unregistered AuthListener");
        } catch (Exception e) {
            assertEquals(e.getMessage(), "ER_" + Status.FEATURE_NOT_AVAILABLE);
        }
    }

    public void testNotClaimable() throws Exception {
        assertEquals(Status.OK, registerAuthListener(busAttachment));
        SecurityApplicationProxy sap = new SecurityApplicationProxy(busAttachment,busAttachment.getUniqueName(),(short)0);
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, sap.getApplicationState());
    }

    public void testBasic() throws Exception {
        assertEquals(Status.OK, registerAuthListener(busAttachment));

        SecurityApplicationProxy sap = new SecurityApplicationProxy(busAttachment,busAttachment.getUniqueName(),(short)0);
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, sap.getApplicationState());

        permissionConfigurator = busAttachment.getPermissionConfigurator();
        permissionConfigurator.setManifestTemplateFromXml(defaultManifestTemplate);

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, sap.getApplicationState());

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

        PermissionConfigurator pcPeer1 = peer1Bus.getPermissionConfigurator();
        pcPeer1.setManifestTemplateFromXml(ManifestTestHelper.MANIFEST_ALL_INCLUSIVE);

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE,
                pcPeer1.getApplicationState());

        //Create admin group key
        PermissionConfigurator permissionConfigurator = securityManagerBus.getPermissionConfigurator();
        KeyInfoNISTP256 securityManagerKey = permissionConfigurator.getSigningPublicKey();

        //Random GUID used for the SecurityManager
        UUID securityManagerGuid = UUID.randomUUID();

        //Peer public key used to generate the identity certificate chain
        ECCPublicKey peer1PublicKey = pcPeer1.getSigningPublicKey().getPublicKey();

        //Create identityCertChain
        CertificateX509 identityCertChain[] = CertificateTestHelper.createIdentityCert("0",
                    securityManagerGuid,
                    peer1PublicKey,
                    "Alias",
                    3600,
                    securityManagerBus);

        String signedManifest = securityManagerBus.getPermissionConfigurator().computeThumbprintAndSignManifestXml(
                identityCertChain[0],
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
        pcPeer1.claim(securityManagerKey,
                    securityManagerGuid,
                    securityManagerKey,
                    identityCertChain,
                    new String[]{signedManifest});

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMED, pcPeer1.getApplicationState());

        CertificateX509[] returnedCertChain = pcPeer1.getIdentity();
        assertTrue(1 == returnedCertChain.length);

        byte[] encodedReturnedCert = returnedCertChain[0].encodeCertificateDER();
        assertNotNull(encodedReturnedCert);
        assertTrue(0 != encodedReturnedCert.length);

        CertificateId certId = pcPeer1.getIdentityCertificateId();
        assertNotNull(certId.getSerial());

        //Create identityCertChain
        CertificateX509 identityCertChain2[] = CertificateTestHelper.createIdentityCert("01234",
                    securityManagerGuid,
                    peer1PublicKey,
                    "Alias",
                    3600,
                    securityManagerBus);

        pcPeer1.updateIdentity(identityCertChain2, new String[]{signedManifest});

        CertificateId certId2 = pcPeer1.getIdentityCertificateId();
        assertEquals(certId2.getSerial(), new String(identityCertChain2[0].getSerial()));
        assertEquals(certId2.getIssuerKeyInfo().getPublicKey(), securityManagerBus.getPermissionConfigurator().getSigningPublicKey().getPublicKey());

        /*
         * intermittent error bad_alloc when these are included,
         * logged JIRA issue 3502 to investigate.
         * The crash occurs in ASN1.cc:378 when
         * asn += *val
         *
         * val->length() returned npos in a gdb session
         */
        //byte[] encodedSourceCert = identityCertChain[0].encodeCertificateDER();
        //assertTrue(Arrays.equals(encodedSourceCert, encodedReturnedCert));
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

