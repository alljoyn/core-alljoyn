/**
 * @file
 * This contains the SecurityApplicationProxy class
 */
/******************************************************************************
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
 ******************************************************************************/

#ifndef _ALLJOYN_SECURITYAPPLICATIONPROXY_H
#define _ALLJOYN_SECURITYAPPLICATIONPROXY_H

#include <alljoyn/BusAttachment.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/PermissionConfigurator.h>
#include <qcc/Crypto.h>
#include <qcc/CertificateECC.h>

namespace ajn {

/**
 * SecurityApplicationProxy gives proxy access to the following interfaces.
 *  - org.alljoyn.Bus.Security.Application
 *  - org.alljoyn.Bus.Security.ClaimableApplication
 *  - org.alljoyn.Bus.Security.ManagedApplication
 */
class SecurityApplicationProxy : public ProxyBusObject {
  public:
    /**
     * SecurityApplicationProxy Constructor
     *
     * @param  bus reference to BusAttachment
     * @param[in] busName Unique or well-known name of remote AllJoyn bus
     * @param[in] sessionId the session received after joining AllJoyn session
     */
    SecurityApplicationProxy(BusAttachment& bus, AJ_PCSTR busName, SessionId sessionId = 0);

    /**
     * ClaimableApplicationProxy destructor
     */
    virtual ~SecurityApplicationProxy();

    /**
     * GetVersion get the org.alljoyn.Bus.Security.Application version
     *
     * @param[out] version of the service.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetSecurityApplicationVersion(uint16_t& version);

    /**
     * GetApplicationState get An enumeration representing the current state of
     * the application.  The list of valid values:
     *
     * | Value         | Description                                                      |
     * |---------------|------------------------------------------------------------------|
     * | NOT_CLAIMABLE | The application is not claimed and not accepting claim requests. |
     * | CLAIMABLE     | The application is not claimed and is accepting claim requests.  |
     * | CLAIMED       | The application is claimed and can be configured.                |
     * | NEED_UPDATE   | The application is claimed, but requires a configuration update. |
     *
     * @param[out] applicationState of the service.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetApplicationState(PermissionConfigurator::ApplicationState& applicationState);

    /**
     * Get the SHA-256 digest of the manifest template.
     *
     * @param[out] digest the buffer to hold the SHA-256 digest of the
     *                       manifest template.
     * @param expectedSize the size of the buffer.  The expected size must be
     *                     equal to qcc::Crypto_SHA256::DIGEST_SIZE or 32.
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_BAD_ARG_2 if the expected size is not equal to qcc::Crypto_SHA256::DIGEST_SIZE
     *  - an error status indicating failure
     */
    QStatus GetManifestTemplateDigest(uint8_t* digest, size_t expectedSize);

    /**
     * The Elliptic Curve Cryptography public key used by the application's keystore
     * to identify itself. The public key persists across any
     * ManagedApplication.Reset() call.  However, if the keystore is cleared via the
     * BusAttachment::ClearKeyStore() or using Config.FactoryReset(), the public key
     * will be regenerated.
     *
     * @param[out] eccPublicKey The Elliptic Curve Cryptography public key
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetEccPublicKey(qcc::ECCPublicKey& eccPublicKey);

    /**
     * The manufacturer certificate chain. The leaf cert is listed first. The signing
     * certs follow.  This chain is installed by the manufacturer at production time.
     * If no manufacturer certificate is available then this is an empty array.
     *
     * @param[out] certificate The manufacturer certificate chain.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetManufacturerCertificate(MsgArg& certificate);

    /**
     * The manifest template defined by the application developer.  The manifest
     * contains the permission rules the application requires to operate.
     *
     * @param[out] rules The permissions expressed as rules this application
     *                   requires to operate.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetManifestTemplate(MsgArg& rules);

    /**
     * Returns the XML version of the manifest template. The returned value
     * is managed by the caller and has to be freed using DestroyManifestTemplate.
     *
     * @param[out] manifestTemplateXml  The manifest template in XML format.
     *
     * @return
     *  - #ER_OK if successful.
     *  - An error status indicating failure.
     */
    QStatus GetManifestTemplate(AJ_PSTR* manifestTemplateXml);

    /**
     * Destroys the manifest template XML created by GetManifestTemplate.
     *
     * @param[in] manifestTemplateXml    The manifest temaplte in XML format.
     */
    static void DestroyManifestTemplate(AJ_PSTR manifestTemplateXml);

    /**
     * The authentication mechanisms the application supports for the claim process.
     * It is a bit mask.
     *
     * | Mask                  | Description              |
     * |-----------------------|--------------------------|
     * | CAPABLE_ECDHE_NULL    | claiming via ECDHE_NULL  |
     * | CAPABLE_ECDHE_PSK     | claiming via ECDHE_PSK   |
     * | CAPABLE_ECDHE_SPEKE   | claiming via ECDHE_SPEKE |
     * | CAPABLE_ECDHE_ECDSA   | claiming via ECDHE_ECDSA |
     *
     * @param[out] claimCapabilities The authentication mechanisms the application supports
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetClaimCapabilities(PermissionConfigurator::ClaimCapabilities& claimCapabilities);

    /**
     * The additional information information on the claim capabilities.
     * It is a bit mask.
     *
     * | Mask                              | Description                                   |
     * |-----------------------------------|-----------------------------------------------|
     * | PSK_GENERATED_BY_SECURITY_MANAGER | PSK or password generated by Security Manager |
     * | PSK_GENERATED_BY_APPLICATION      | PSK or password generated by application      |
     *
     * @param[out] claimCapabilitiesAdditionalInfo The additional information
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& claimCapabilitiesAdditionalInfo);

    /**
     * Claim the app. This will make the claimer the admin and certificate
     * authority. The KeyInfo object description is shown below.
     *
     * It is highly recommended that element 0 of identityCertificateChain, the peer's
     * end entity certificate, be of type qcc::IdentityCertificate, so that the correct
     * Extended Key Usage (EKU) is set. The remaining certificates in the chain can be
     * of this or the base CertificateX509 type.
     *
     * Access restriction: None if the app is not yet claimed. An error will be
     * raised if the app has already been claimed.
     *
     * @param[in] certificateAuthority   a KeyInfo object representing the
     *                                   public key of the certificate authority
     * @param[in] adminGroupId           the admin group Id
     * @param[in] adminGroup             a KeyInfo object representing the admin
     *                                   security group authority
     * @param[in] identityCertChain      the identity certificate chain for the
     *                                   claimed app.  The leaf cert is listed first
     * @param[in] identityCertChainCount the number of the identity certificates in the
     *                                   chain.
     * @param[in] manifests              the signed manifests to install on the application
     * @param[in] manifestCount          the number of manifests
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the application is not claimable
     *  - #ER_INVALID_CERTIFICATE Error raised when the identity certificate
     *                            chain is not valid
     *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage
     *                                  is not AllJoyn specific
     *  - #ER_DIGEST_MISMATCH Error raised when the manifest is not accepted by the application.
     *  - an error status indicating failure
     */
    QStatus Claim(const qcc::KeyInfoNISTP256& certificateAuthority,
                  const qcc::GUID128& adminGroupId,
                  const qcc::KeyInfoNISTP256& adminGroup,
                  const qcc::CertificateX509* identityCertChain, size_t identityCertChainCount,
                  const Manifest* manifests, size_t manifestCount);

    /**
     * Claim the app using a manifests in XML format. This will make
     * the claimer the admin and certificate authority. The KeyInfo
     * object description is shown below.
     *
     * Access restriction: None if the app is not yet claimed. An error will be
     * raised if the app has already been claimed.
     *
     * @param[in]   certificateAuthority    A KeyInfo object representing the
     *                                      public key of the certificate authority.
     * @param[in]   adminGroupId            The admin group Id.
     * @param[in]   adminGroup              A KeyInfo object representing the admin
     *                                      security group authority.
     * @param[in]   identityCertChain       The identity certificate chain for the
     *                                      claimed app.  The leaf cert is listed first
     *                                      followed by each intermediate Certificate
     *                                      Authority's certificate, ending in the trusted
     *                                      root's certificate.
     * @param[in]   identityCertChainCount  The count of the identity certificate
     *                                      chain elements.
     * @param[in]   manifestsXmls           An array of null-terminated strings containing
     *                                      the application's manifests in XML format.
     * @param[in]   manifestsCount          Count of "manifestsXmls" array elements.
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the application is not claimable
     *  - #ER_INVALID_CERTIFICATE Error raised when the identity certificate
     *                            chain is not valid
     *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage
     *                                  is not AllJoyn specific.
     *  - #ER_DIGEST_MISMATCH Error raised when none of the provided signed manifests are
     *                                         valid for the given identity certificate.
     *  - an error status indicating failure
     */
    QStatus Claim(const qcc::KeyInfoNISTP256& certificateAuthority,
                  const qcc::GUID128& adminGroupId,
                  const qcc::KeyInfoNISTP256& adminGroup,
                  const qcc::CertificateX509* identityCertChain, size_t identityCertChainCount,
                  AJ_PCSTR* manifestsXmls, size_t manifestsCount);

    /**
     * The ClaimableApplication version
     *
     * @param[out] version of the service.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetClaimableApplicationVersion(uint16_t& version);

    /**
     * This method allows an admin to reset the application to its original state
     * prior to claim.  The application's security 2.0 related configuration is
     * discarded.  The application is no longer claimed.
     *
     * If the keystore is cleared by the BusAttachment::ClearKeyStore() call, this
     * Reset() method call is not required.  The Configuration service's
     * FactoryReset() call in fact clears the keystore, so this Reset() call is not
     * required in that scenario.
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - an error status indicating failure
     */
    QStatus Reset();

    /**
     * This method allows an admin to update the application's identity certificate
     * chain and its manifests using the Manifest objects.
     *
     * After having a new identity certificate installed, the target bus clears
     * out all of its peer's secret and session keys, so the next call will get
     * security violation. After calling UpdateIdentity, SecureConnection(true)
     * should be called to force the peers to create a new set of secret and
     * session keys.
     *
     * It is highly recommended that element 0 of identityCertificateChain, the peer's
     * end entity certificate, be of type qcc::IdentityCertificate. Other certs can be
     * of this or the base CertificateX509 type.
     *
     * The target peer also clears all manifests it has already stored, and so all
     * manifests the peer needs must be sent again. Use GetManifests to retrieve
     * the currently-installed manifests before calling UpdateIdentity to reuse them.
     *
     * Manifests must already be signed by the authority that issued the identity
     * certificate chain.
     *
     * @see ProxyBusObject.SecureConnection(bool)
     * @see SecurityApplicationProxy.GetManifests(std::vector<Manifest>&)
     *
     * @param[in] identityCertificateChain             the identity certificate
     * @param[in] identityCertificateChainCount        the number of identity certificates
     * @param[in] manifests                            the signed manifests to install on the application
     * @param[in] manifestCount                        the number of manifests
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - #ER_INVALID_CERTIFICATE Error raised when the identity certificate chain is not valid
     *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage is not AllJoyn specific
     *  - #ER_DIGEST_MISMATCH Error raised when the digest is not accepted
     *  - an error status indicating failure
     */
    QStatus UpdateIdentity(const qcc::CertificateX509* identityCertificateChain, size_t identityCertificateChainCount,
                           const Manifest* manifest, size_t manifestCount);

    /**
     * This method allows an admin to update the application's identity certificate
     * chain and its manifests using manifests in XML format.
     *
     * After having a new identity certificate installed, the target bus clears
     * out all of its peer's secret and session keys, so the next call will get
     * security violation. After calling UpdateIdentity, SecureConnection(true)
     * should be called to force the peers to create a new set of secret and
     * session keys.
     *
     * @see ProxyBusObject.SecureConnection(bool)
     *
     * @param[in] identityCertificateChain       the identity certificate
     * @param[in] identityCertificateChainCount  the number of identity certificates
     * @param[in] manifestsXmls                  An array of null-terminated strings containing the application's
     *                                           manifests in XML format.
     * @param[in] manifestsCount                 Count of "manifestsXmls" array elements.
     *
     * @return
     *  - #ER_OK                        If successful.
     *  - #ER_PERMISSION_DENIED         Error raised when the caller does not have permission.
     *  - #ER_INVALID_CERTIFICATE       Error raised when the identity certificate chain is not valid.
     *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage is not AllJoyn specific.
     *  - #ER_DIGEST_MISMATCH           Error raised when none of the provided signed manifests are
     *                                  valid for the given identity certificate.
     *  - An error status indicating failure
     */
    QStatus UpdateIdentity(const qcc::CertificateX509* identityCertificateChain, size_t identityCertificateChainCount,
                           AJ_PCSTR* manifestsXmls, size_t manifestsCount);

    /**
     * This method allows an admin to install the permission policy to the
     * application.  Any existing policy will be replaced if the new policy version
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
     * @see ProxyBusObject.SecureConnection(bool)
     *
     * @param[in] policy the new policy.
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - #ER_POLICY_NOT_NEWER Error raised when the new policy does not have a
     *                         greater version number than the existing policy.
     *  - an error status indicating failure
     */
    QStatus UpdatePolicy(const PermissionPolicy& policy);

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
     * @see ProxyBusObject.SecureConnection(bool)
     *
     * @param[in] policy    The new policy in XML format. For the policy XSD refer to
     *                      alljoyn_core/docs/policy.xsd.
     *
     * @return
     *  - #ER_OK if successful.
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission.
     *  - #ER_POLICY_NOT_NEWER  Error raised when the new policy does not have a
     *                          greater version number than the existing policy.
     *  - #ER_XML_MALFORMED     If the policy was not in the valid XML format.
     *  - an error status indicating failure.
     */
    QStatus UpdatePolicy(AJ_PCSTR policyXml);

    /**
     * This method allows an admin to remove the currently installed policy.  The
     * application reverts back to the default policy generated during the claiming
     * process.
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - an error status indicating failure
     */
    QStatus ResetPolicy();

    /**
     * This method allows the admin to install a membership cert chain to the
     * application.
     *
     * It is highly recommended that element 0 of certificateChain, the peer's
     * end entity certificate, be of type qcc::MembershipCertificate, so that the correct
     * Extended Key Usage (EKU) is set. The remaining certificates in the chain can be
     * of this or the base CertificateX509 type.
     *
     * @param[in] certificateChain      The membership certificate chain. It can be a
     *                                  single certificate if it is issued by the security
     *                                  group authority.
     * @param[in] certificateChainCount The number of certificates in the certificate chain.
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - #ER_DUPLICATE_CERTIFICATE Error raised when the membership certificate
     *                              is already installed.
     *  - #ER_INVALID_CERTIFICATE Error raised when the membership certificate is not valid.
     *  - an error status indicating failure
     */
    QStatus InstallMembership(const qcc::CertificateX509* certificateChain, size_t certificateChainSize);

    /**
     * This method allows an admin to remove a membership certificate chain from the
     * application.
     *
     * @param[in] serial the serial number
     * @param[in] issuerKeyInfo the issuer key information including aki and public key of the membership leaf certificate
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - #ER_CERTIFICATE_NOT_FOUND Error raised when the certificate is not found
     *  - an error status indicating failure
     */
    QStatus RemoveMembership(const qcc::String& serial, const qcc::KeyInfoNISTP256& issuerKeyInfo);

    /**
     * This method allows an admin to install one or more additional signed manifests to the application.
     *
     * @param[in] manifests array of signed manifests to append to the application's existing set of manifests
     * @param[in] manifestCount number of manifests in the array
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - #ER_DIGEST_MISMATCH Error raised when none of the manifests are accepted by the application
     *  - an error status indicating failure
     */
    QStatus InstallManifests(const Manifest* manifests, size_t manifestCount);

    /**
     * Get the ManagedApplication version
     *
     * @param[out] version of the service.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetManagedApplicationVersion(uint16_t& version);

    /**
     * Get the identify certificate chain.
     *
     * @param[out] identityCertificate the identify certificate chain.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetIdentity(MsgArg& identityCertificate);

    /**
     * Get the manifests
     *
     * @param[out] manifests the manifest
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetManifests(std::vector<Manifest>& manifest);

    /**
     * Get the serial number and issuer of the currently installed identity certificate.
     *
     * @param[out] serial the identity certificate serial number
     * @param[out] issuerKeyInfo the issuer key info including the AKI and public key
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetIdentityCertificateId(qcc::String& serial, qcc::KeyInfoNISTP256& issuerKeyInfo);

    /**
     * Get the version number of the currently installed policy.
     *
     * @param[out] policyVersion the PolicyVersion
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetPolicyVersion(uint32_t& policyVersion);

    /**
     * Get the currently installed policy.
     *
     * @param[out] policy the currently installed policy
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetPolicy(PermissionPolicy& policy);

    /**
     * Get the default policy regardless of the currently installed policy.
     *
     * @param[out] defaultPolicy the default policy
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetDefaultPolicy(PermissionPolicy& defaultPolicy);

    /**
     * The list of serial numbers and issuers of the currently installed membership
     * certificates.  If the issuer's public key is not available in the case
     * where the membership is a single cert, the issuer public key field is
     * empty.
     *
     * @param[out] membershipSummaries the Membership Summaries
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetMembershipSummaries(MsgArg& membershipSummaries);

    /**
     * This method notifies the application about the fact that the Security Manager
     * will start to make changes to the application's security settings.
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - some other error status indicating failure
     */
    QStatus StartManagement();

    /**
     * This method notifies the application about the fact that the Security Manager
     * has finished making changes to the application's security settings.
     *
     * @return
     *  - #ER_OK if successful
     *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
     *  - some other error status indicating failure
     */
    QStatus EndManagement();

    /**
     * Populate the array of identity certificates with data from the msg arg
     * @param arg the message arg with signature a(yay)
     * @param[in,out] certs the array of Identity certificates
     * @param expectedSize the size of the certificate array
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    static QStatus MsgArgToIdentityCertChain(const MsgArg& arg, qcc::CertificateX509* certs, size_t expectedSize);

    /**
     * Populate the array of serial numbers and issuer key infos with data from
     * the msg arg returned from GetMembershipSummaries.
     * @param arg the message arg with signature a(ayay(yyayay))
     * @param[in,out] serials the array of serial numbers
     * @param[in,out] issuerKeyInfos the array of issuer key info objects
     * @param expectedSize the size of the arrays
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    static QStatus MsgArgToCertificateIds(const MsgArg& arg, qcc::String* serials, qcc::KeyInfoNISTP256* issuerKeyInfos, size_t expectedSize);

    /**
     * Populate the array of rules certificates with data from the msg arg
     * @param arg the message arg with signature a(ssa(syy)
     * @param[in,out] rules the array of rules
     * @param expectedSize the size of the array of rules
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    static QStatus MsgArgToRules(const MsgArg& arg, PermissionPolicy::Rule* rules, size_t expectedSize);

  private:

    /**
     * Extracts a vector of Manifest objects from an array of signed
     * manifests in XML format.
     *
     * @param[in]    manifestsXmls   An array of signed manifests in XML format.
     * @param[in]    manifestsCount  Count of the "manifestsXmls" array elements.
     * @param[out]   manifests       Output vector containing the Manifest objects.
     *
     * @return
     *  - #ER_OK             If successful
     *  - #ER_XML_MALFORMED  If one of the signed manifests was in an invalid format.
     *  - Some other error status indicating failure.
     */
    QStatus ExtractManifests(AJ_PCSTR* manifestsXmls, size_t manifestsCount, std::vector<Manifest>& manifests);
};
}

#endif /* _ALLJOYN_SECURITYAPPLICATIONPROXY_H */
