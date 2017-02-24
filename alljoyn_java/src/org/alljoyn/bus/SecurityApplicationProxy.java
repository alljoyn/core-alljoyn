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

import java.util.UUID;
import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.ECCPrivateKey;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.KeyInfoNISTP256;

public class SecurityApplicationProxy {

    /**
     * Allocate native resources.
     */
    private native void create(BusAttachment bus, String busName, int sessionId) throws BusException;

    /**
     * Hold on to the BusAttachment so we can DecRef it in the JNI
     * when this object is destroyed
     */
    private BusAttachment busattachment;

    /**
     * Release native resources.
     */
    private synchronized native void destroy(BusAttachment bus);

    /**
     * SecurityApplicationProxy constructor
     *
     * @param bus the BusAttachment that owns this ProxyBusObject
     * @param busName the unique or well-known name of the remote AllJoyn BusAttachment
     * @param sessionId the session ID this ProxyBusObject will use
     */
    public SecurityApplicationProxy(BusAttachment bus, String busName, int sessionId) throws BusException {
        create(bus, busName, sessionId);
        busattachment = bus;
    }

    /**
     * Let the Java garbage collector release resources.
     */
    @Override
    protected void finalize() throws Throwable {
        try {
            destroy(busattachment);
        } finally {
            super.finalize();
        }
    }

    /**
     * Get the version of the remote SecurityApplication interface
     * @return the version of the remote SecurityApplication interface
     * @throws BusException
     *   throws an exception indicating failure to make the remote method call
     */
    public native short getSecurityApplicationVersion() throws BusException;

    /**
     * Get the ApplicationState
     * @return the Application State
     * @throws BusException indicating failure to read ApplicationState property
     */
    public native PermissionConfigurator.ApplicationState getApplicationState() throws BusException; 

    /**
     * Get the ManifestTemplateDigest
     * @return the ManifestTemplateDigest
     * @throws BusException indicating failure to read ManifestTemplateDigest property
     */
    public native byte[] getManifestTemplateDigest() throws BusException;

    /**
     * @return the EccPublicKey
     * @throws BusException indicating failure to read EccPublicKey property
     */
    public native ECCPublicKey getEccPublicKey() throws BusException;

    /**
     * @return the ManufacturerCertificate
     * @throws BusException indicating failure to read ManufacturerCertificate property
     */
    public native CertificateX509[] getManufacturerCertificate() throws BusException;

    /**
     * @return the ManifestTemplate
     * @throws BusException indicating failure to read ManifestTemplate property
     */
    public native String getManifestTemplate() throws BusException;

    /**
     * @return the ClaimCapabilities
     * @throws BusException indicating failure to read ClaimCapabilities property
     */
    public native short getClaimCapabilities() throws BusException;

    /**
     * @return the ClaimCapabilitiesAdditionalInfo
     * @throws BusException indicating failure to read ClaimCapabilitiesAdditionalInfo property
     */
    public native short getClaimCapabilityAdditionalInfo() throws BusException;

    /**
     * Claim the app using a manifests in XML format. This will make
     * the claimer the admin and certificate authority. The KeyInfo
     * object description is shown below.
     *
     * Note: After this call the remote application should wait for the "EndManagement"
     * call before it can begin regular operation. Since "StartManagement" calls are
     * not possible before the application is claimed, that call is made internally
     * on the application's side before the claiming procedure begins.
     *
     * Access restriction: None if the app is not yet claimed. An error will be
     * raised if the app has already been claimed.
     *
     * @param   certificateAuthority    A KeyInfo object representing the
     *                                      public key of the certificate authority.
     * @param   adminGroupId            The admin group Id.
     * @param   adminGroup              A KeyInfo object representing the admin
     *                                      security group authority.
     * @param   identityCertChain       The identity certificate chain for the
     *                                      claimed app.  The leaf cert is listed first
     *                                      followed by each intermediate Certificate
     *                                      Authority's certificate, ending in the trusted
     *                                      root's certificate.
     * @param   identityCertChainCount  The count of the identity certificate
     *                                      chain elements.
     * @param   manifestsXmls           An array of null-terminated strings containing
     *                                      the application's manifests in XML format.
     * @param   manifestsCount          Count of "manifestsXmls" array elements.
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the application is not claimable
     *  - #ER_INVALID_CERTIFICATE Error raised when the identity certificate
     *                            chain is not valid
     *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage
     *                                  is not AllJoyn specific.
     *  - #ER_DIGEST_MISMATCH Error raised when none of the provided signed manifests are
     *                                         valid for the given identity certificate.
     *  - an error status indicating failure
     */
    private native void claim(KeyInfoNISTP256 certificateAuthority,
                  UUID adminGroupId, KeyInfoNISTP256 adminGroup,
                  CertificateX509[] identityCertChain, long identityCertChainCount,
                  String[] manifestsXmls, long manifestsCount) throws BusException;

    /**
     * Claim the app using a manifests in XML format. This will make
     * the claimer the admin and certificate authority. The KeyInfo
     * object description is shown below.
     *
     * Note: After this call the remote application should wait for the "EndManagement"
     * call before it can begin regular operation. Since "StartManagement" calls are
     * not possible before the application is claimed, that call is made internally
     * on the application's side before the claiming procedure begins.
     *
     * Access restriction: None if the app is not yet claimed. An error will be
     * raised if the app has already been claimed.
     *
     * @param   certificateAuthority    A KeyInfo object representing the
     *                                      public key of the certificate authority.
     * @param   adminGroupId            The admin group Id.
     * @param   adminGroup              A KeyInfo object representing the admin
     *                                      security group authority.
     * @param   identityCertChain       The identity certificate chain for the
     *                                      claimed app.  The leaf cert is listed first
     *                                      followed by each intermediate Certificate
     *                                      Authority's certificate, ending in the trusted
     *                                      root's certificate.
     * @param   manifestXmls           An array of null-terminated strings containing
     *                                      the application's manifests in XML format.
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the application is not claimable
     *  - #ER_INVALID_CERTIFICATE Error raised when the identity certificate
     *                            chain is not valid
     *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage
     *                                  is not AllJoyn specific.
     *  - #ER_DIGEST_MISMATCH Error raised when none of the provided signed manifests are
     *                                         valid for the given identity certificate.
     *  - an error status indicating failure
     */
    public void claim(KeyInfoNISTP256 certificateAuthority,
                  UUID adminGroupId, KeyInfoNISTP256 adminGroup,
                  CertificateX509[] identityCertChain, String[] manifestXmls) throws BusException {
        claim(certificateAuthority, adminGroupId, adminGroup, identityCertChain, identityCertChain.length,
                manifestXmls, manifestXmls.length);
    }

    /**
     * The ClaimableApplication version
     *
     * @return version of the service.
     *
     * @throws BusException
     *  - an error status indicating failure
     */
    public native short getClaimableApplicationVersion() throws BusException;

    /**
     * This method allows an admin to reset the application to its original state
     * prior to claim. The application's security 2.0 related configuration is
     * discarded. The application is no longer claimed.
     *
     * If the keystore is cleared by the BusAttachment::ClearKeyStore() call, this
     * Reset() method call is not required.  The Configuration service's
     * FactoryReset() call in fact clears the keystore, so this Reset() call is not
     * required in that scenario.
     *
     * Note: After this call the remote application will automatically call
     * "EndManagement" on itself.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - an error status indicating failure
     */
    public native void reset() throws BusException;

    /**
     * This method allows an admin to update the application's identity certificate
     * chain and its manifests using manifests in XML format.
     *
     * After having a new identity certificate installed, the target bus clears
     * out all of its peer's secret and session keys, so the next call will get
     * security violation. Calling updateIdentity causes existing sessions to be
     * recreated and session keys to be renegotiated.
     *
     * @param identityCertificateChain       the identity certificate
     * @param identityCertificateChainCount  the number of identity certificates
     * @param manifestsXmls                  An array of null-terminated strings containing the application's
     *                                           manifests in XML format.
     * @param manifestsCount                 Count of "manifestsXmls" array elements.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED         Error raised when the caller does not have permission.
     *  - #ER_INVALID_CERTIFICATE       Error raised when the identity certificate chain is not valid.
     *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage is not AllJoyn specific.
     *  - #ER_DIGEST_MISMATCH           Error raised when none of the provided signed manifests are
     *                                  valid for the given identity certificate.
     *  - An error status indicating failure
     */
    private native void updateIdentity(CertificateX509[] identityCertificateChain, long identityCertificateChainCount,
                           String[] manifestsXmls, long manifestsCount) throws BusException;

    /**
     * This method allows an admin to update the application's identity certificate
     * chain and its manifests using manifests in XML format.
     *
     * After having a new identity certificate installed, the target bus clears
     * out all of its peer's secret and session keys, so the next call will get
     * security violation. Calling updateIdentity causes existing sessions to
     * be created and session keys to be renegotiated.
     *
     * @param identityCertificateChain       the identity certificate
     * @param manifestXmls                  An array of null-terminated strings containing the application's
     *                                           manifests in XML format.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED         Error raised when the caller does not have permission.
     *  - #ER_INVALID_CERTIFICATE       Error raised when the identity certificate chain is not valid.
     *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage is not AllJoyn specific.
     *  - #ER_DIGEST_MISMATCH           Error raised when none of the provided signed manifests are
     *                                  valid for the given identity certificate.
     *  - An error status indicating failure
     */
    public void updateIdentity(CertificateX509[] identityCertificateChain,
                           String[] manifestXmls) throws BusException {
        updateIdentity(identityCertificateChain, identityCertificateChain.length, manifestXmls, manifestXmls.length);
    }

    /**
     * This method allows an admin to install the permission policy to the
     * application using an XML version of the policy.
     * Any existing policy will be replaced if the new policy version
     * number is greater than the existing policy's version number.
     *
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation. After calling UpdatePolicy, SecureConnection(true) should be
     * called to force the peers to create a new set of secret and session keys.
     *
     * Until ASACORE-2755 is fixed the caller must include all default policies
     * (containing information about the trust anchors) with each call, so they
     * would not be removed.
     *
     * @param policyXml    The new policy in XML format. For the policy XSD refer to
     *                      alljoyn_core/docs/policy.xsd.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission.
     *  - #ER_POLICY_NOT_NEWER  Error raised when the new policy does not have a
     *                          greater version number than the existing policy.
     *  - #ER_XML_MALFORMED     If the policy was not in the valid XML format.
     *  - an error status indicating failure.
     */
    public native void updatePolicy(String policyXml) throws BusException;

    /**
     * This method allows an admin to remove the currently installed policy.  The
     * application reverts back to the default policy generated during the claiming
     * process.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - an error status indicating failure
     */
    public native void resetPolicy() throws BusException;

    /**
     * This method allows the admin to install a membership cert chain to the
     * application.
     *
     * It is highly recommended that element 0 of certificateChain, the peer's
     * end entity certificate, be of type qcc::MembershipCertificate, so that the correct
     * Extended Key Usage (EKU) is set. The remaining certificates in the chain can be
     * of this or the base CertificateX509 type.
     *
     * @param certificateChain      The membership certificate chain. It can be a
     *                                  single certificate if it is issued by the security
     *                                  group authority.
     * @param certificateChainCount The number of certificates in the certificate chain.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - #ER_DUPLICATE_CERTIFICATE Error raised when the membership certificate
     *                              is already installed.
     *  - #ER_INVALID_CERTIFICATE Error raised when the membership certificate is not valid.
     *  - an error status indicating failure
     */
    private native void installMembership(CertificateX509[] certificateChain, long certificateChainCount) throws BusException;

    /**
     * This method allows the admin to install a membership cert chain to the
     * application.
     *
     * It is highly recommended that element 0 of certificateChain, the peer's
     * end entity certificate, be of type qcc::MembershipCertificate, so that the correct
     * Extended Key Usage (EKU) is set. The remaining certificates in the chain can be
     * of this or the base CertificateX509 type.
     *
     * @param certificateChain      The membership certificate chain. It can be a
     *                                  single certificate if it is issued by the security
     *                                  group authority.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - #ER_DUPLICATE_CERTIFICATE Error raised when the membership certificate
     *                              is already installed.
     *  - #ER_INVALID_CERTIFICATE Error raised when the membership certificate is not valid.
     *  - an error status indicating failure
     */
    public void installMembership(CertificateX509[] certificateChain) throws BusException {
        installMembership(certificateChain, certificateChain.length);
    }

    /**
     * This method allows an admin to remove a membership certificate chain from the
     * application.
     *
     * @param serial the serial number
     * @param issuerKeyInfo the issuer key information including aki and public key of the membership leaf certificate
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - #ER_CERTIFICATE_NOT_FOUND Error raised when the certificate is not found
     *  - an error status indicating failure
     */
    public native void removeMembership(String serial, KeyInfoNISTP256 issuerKeyInfo) throws BusException;

    /**
     * Get the ManagedApplication version
     *
     * @return version of the service.
     *
     * @throws BusException
     *  - an error status indicating failure
     */
    public native short getManagedApplicationVersion() throws BusException;

    /**
     * Get the identify certificate chain.
     *
     * @return CertificateX509 the identify certificate chain.
     *
     * @throws BusException
     *  - an error status indicating failure
     */
    public native CertificateX509[] getIdentity() throws BusException;

    /**
     * Get the version number of the currently installed policy.
     *
     * @return policyVersion the PolicyVersion
     *
     * @throws BusException
     *  - an error status indicating failure
     */
    public native int getPolicyVersion() throws BusException;

    /**
     * This method notifies the application about the fact that the Security Manager
     * will start to make changes to the application's security settings.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - some other error status indicating failure
     */
    public native void startManagement() throws BusException;

    /**
     * This method notifies the application about the fact that the Security Manager
     * has finished making changes to the application's security settings.
     *
     * @throws BusException
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - some other error status indicating failure
     */
    public native void endManagement() throws BusException;

    /**
     * Adds an identity certificate thumbprint to and signs the manifest XML.
     *
     * @param    identityCertificate The identity certificate of the remote application that will
     *                                   use the signed manifest.
     * @param    privateKey          The signing key. It must be the same one used to sign the
     *                                   identity certificate.
     * @param    unsignedManifestXml The unsigned manifest in XML format. The XML schema can be found
     *                                   under alljoyn_core/docs/manifest_template.xsd.
     * @return                       The signed manifest in XML format.
     *                                   The string is managed by the caller and must be later destroyed
     *                                   using DestroySignedManifest.
     * @throws BusException
     *  - an error status indicating failure
     */
    public native static String signManifest(CertificateX509 identityCertificate,
                                ECCPrivateKey privateKey,
                                String unsignedManifestXml) throws BusException;

    /**
     * Adds an identity certificate thumbprint and retrieves the digest of the manifest XML for signing.
     *
     * @param    unsignedManifestXml    The unsigned manifest in XML format. The XML schema can be found
     *                                      under alljoyn_core/docs/manifest_template.xsd.
     * @param    identityCertificate    The identity certificate of the remote application that will use
     *                                      the signed manifest.
     * @return   byte[]                 Pointer to receive the byte array containing the digest to be
     *                                      signed with ECDSA-SHA256. This array is managed by the caller and must
     *                                      be later destroyed using DestroyManifestDigest.
     *
     * @throws BusException
     *          - #ER_XML_MALFORMED If the unsigned manifest is not compliant with the required format.
     *          - Other error status indicating failure.
     */
    public native static byte[] computeManifestDigest(String unsignedManifestXml,
                                         CertificateX509 identityCertificate) throws BusException;

    /** The native connection handle. */
    private long handle;
}