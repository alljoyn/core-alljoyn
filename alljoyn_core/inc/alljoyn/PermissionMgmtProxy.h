/**
 * @file
 * This contains the PermissionMgmtProxy class
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

#ifndef _ALLJOYN_PERMISSIONMGMTPROXY_H_
#define _ALLJOYN_PERMISSIONMGMTPROXY_H_

#include <alljoyn/BusAttachment.h>
#include <alljoyn/PermissionPolicy.h>
#include <qcc/CertificateECC.h>

namespace ajn {

/**
 * PermissionMgmtProxy gives proxy access to the org.allsee.Security.PermissionMgmt
 * interface.
 */
class PermissionMgmtProxy : public ProxyBusObject {
  public:
    /**
     * PermissionMgmtProxy Constructor
     *
     * @param  bus reference to BusAttachment
     * @param[in] busName Unique or well-known name of remote AllJoyn bus
     * @param[in] sessionId the session received after joining AllJoyn session
     */
    PermissionMgmtProxy(BusAttachment& bus, const char* busName, SessionId sessionId = 0);

    /**
     * PermissionMgmtProxy destructor
     */
    virtual ~PermissionMgmtProxy();

    /**
     * Claim the app. This will make the claimer the admin and certificate
     * authority. The KeyInfo object description is shown below.
     *
     * Access restriction: None if the app is not yet claimed. An error will be
     * raised if the app already has an admin and it is not the caller.
     *
     * @param[in] certificateAuthority   a KeyInfo object representing the
     *                                   public key of the certificate authority
     * @param[in] adminGroupId           the admin group Id
     * @param[in] adminGroup             a KeyInfo object representing the admin
     *                                   security group authority
     * @param[in] identityCertChain      the identity certificate chain for the
     *                                   claimed app.  The leaf cert is listed first
     * @param[in] identityCertChainSize  the size of the identity certificate
     *                                   chain.
     * @param[in] manifest               the manifest of the application
     * @param[in] manifestSize           the number of rules in the manifest
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus Claim(qcc::KeyInfoNISTP256& certificateAuthority, qcc::GUID128& adminGroupId, qcc::KeyInfoNISTP256& adminGroup, qcc::IdentityCertificate* identityCertChain, size_t identityCertChainSize, PermissionPolicy::Rule* manifest, size_t manifestSize);


    /**
     * Install the static policy to the app. It replaces any existing policy on
     * the app.
     *
     * Access restriction: Only an admin of the app is allowed to install a
     * policy.
     *
     * @param[in] authorization variant for the authorization data
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus InstallPolicy(PermissionPolicy& authorization);

    /**
     * Decrypt and install the static policy to the app. It replaces any
     * existing policy on the app.
     *
     * Access restriction: unspecified in interface description
     *
     * @param[in] encryptedAuthorization  byte array for the encrypted
     *                                    authorization data
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus InstallEncryptedPolicy(const MsgArg& encryptedAuthorizationArg);

    /**
     * Retrieve the currently installed policy from the app.
     *
     * Access restiction: Only an admin of the app is allowed to get a
     * policy.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetPolicy(PermissionPolicy* authorization);

    /**
     * Remove the currently installed policy from the app.
     *
     * Access restriction: Only an admin of the app is allowed remove a policy.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus RemovePolicy();

    /**
     * Install a membership cert to the app. The Certificate object description
     * is shown below.
     *
     * Access restriction: Only an admin of the app is allowed to install a
     * membership certificate.
     *
     * Note: only X.509 DER format certificates are currently supported.
     *
     * @param[in] certChainArg the certificate chain for the guild membership
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus InstallMembership(const MsgArg& certChainArg);

    /**
     * Install a membership authorization data to the app
     *
     * Access restriction: Only an admin of the app is allowed to install
     * membership authorization data. Only membership authorization with a
     * matching membership certificate is accepted. The hash digest of the
     * authorization is verified against the digest listed in the membership
     * certificate.
     *
     * @param[in] serialNum     a string representing the serial number of the
     *                          membership certificate
     * @param[in] issuerAki        the issuer authority key id
     * @param[in] authorization authorization data
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus InstallMembershipAuthData(const char* serialNum, const qcc::String& issuerAki, PermissionPolicy& authorization);

    /**
     * Remove a membership certificate from the app. Any corresponding
     * membership authorization data will be also be removed.
     *
     * Access restriction: Only an admin of the app is allowed to remove a
     * membership.
     *
     * @param[in] serialNum a string representing the serial number of the
     *                      membership certificate.
     * @param[in] issuerAki    the issuer authority key id
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus RemoveMembership(const char* serialNum, const qcc::String& issuerAki);

    /**
     * Install an identity cert to the app.
     *
     * Access restriction: Only an admin of the app is allowed install an
     * identify certification.
     *
     * Note: Only X.509 DER format certificate is currently supported.
     *
     * @param[in] certArg The certificate of the identity.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus InstallIdentity(const MsgArg& certArg);

    /**
     * Retrieve the identity cert from the app.
     *
     * Access restriction: none.
     * Note: Only X.509 DER format certificate is currently supported.
     *
     * @param[out] cert identity certificate
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetIdentity(qcc::IdentityCertificate* cert);

    /**
     * Retrieve the manifest data installed by the application developer.
     *
     * Access restriction: unspecified in interface description
     *
     * @param[out] rules the manifest rules
     * @param[out] count the number of rules
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetManifest(PermissionPolicy::Rule** rules, size_t* count);

    /**
     * Reset to its original state by deleting all the trust anchors, DSA keys,
     * installed policy and certificates.
     *
     * Access restriction: Only an admin of the app is allowed to reset the
     * state.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus Reset();

    /**
     * Retrieve the public key
     *
     * Access restriction: unspecified in interface description
     *
     * @param[out] pubKey the public key of the application.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetPublicKey(qcc::ECCPublicKey* pubKey);

    /**
     * GetVersion get the PermissionMgmt version
     *
     * @param[out] version of the service.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetVersion(uint16_t& version);

  private:
    /**
     * Helper function to check return value when a method call fails
     * This will check if the error contained in the status is
     * org.alljoy.Bus.ER_PERMISSION_DENIED
     * @param status the status returned from the MethodCall
     * @param msg the reply message containing the error.
     *
     * @return ture if status equals ER_PERMISSION_DENIED or if the status
     * equals ER_BUS_REPLY_IS_ERROR_MESSAGE and the message contains a
     * org.alljoy.Bus.ER_PERMISSION_DENIED string.
     */
    bool IsPermissionDeniedError(const Message& msg);

    /**
     * This method is used to convert a returned MsgArg to a
     * qcc::ECCPublicKey. This is used as a helper function in multiple
     * methods that return a Key.
     *
     * @param[in] arg MsgArg containing a data for an ECCPublicKey
     * @param[out] pubKey the ECCPublicKey obtained from the passed int MsgArg
     */
    QStatus RetrieveECCPublicKeyFromMsgArg(const MsgArg& arg, qcc::ECCPublicKey* pubKey);
};
}

#endif /* _ALLJOYN_PERMISSIONMGMTPROXY_H_ */
