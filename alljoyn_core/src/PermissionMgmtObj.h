#ifndef _ALLJOYN_PERMISSION_MGMT_OBJ_H
#define _ALLJOYN_PERMISSION_MGMT_OBJ_H
/**
 * @file
 * This file defines the Permission DB classes that provide the interface to
 * parse the authorization data
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

#ifndef __cplusplus
#error Only include PermissionMgmtObj.h in C++ code.
#endif

#include <alljoyn/BusObject.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/GUID.h>
#include <qcc/KeyBlob.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <alljoyn/PermissionPolicy.h>
#include "CredentialAccessor.h"
#include "ProtectedAuthListener.h"
#include "PeerState.h"
#include "KeyStore.h"

namespace ajn {

class MessageEncryptionNotification {
  public:
    MessageEncryptionNotification()
    {
    }
    virtual ~MessageEncryptionNotification()
    {
    }

    /**
     * Notify the observer when the message encryption step is complete.
     */
    virtual void EncryptionComplete()
    {
    }
};

class PermissionMgmtObj : public BusObject {

    friend class PermissionManager;

  public:

    /**
     * Error name Permission Denied.  Error raised when the message is not
     * authorized.
     */
    static const char* ERROR_PERMISSION_DENIED;

    /**
     * Error name Invalid Certificate.  Error raised when the certificate or
     * cerficate chain is not valid.
     */
    static const char* ERROR_INVALID_CERTIFICATE;

    /**
     * Error name Invalid certificate usage.  Error raised when the extended
     * key usage (EKU) is not AllJoyn specific.
     */
    static const char* ERROR_INVALID_CERTIFICATE_USAGE;

    /**
     * Error name Digest Mismatch.  Error raised when the digest of the
     * manifest does not match the digest listed in the identity certificate.
     */
    static const char* ERROR_DIGEST_MISMATCH;

    /**
     * Error name Policy Not Newer.  Error raised when the new policy does not
     * have a greater version number than the existing policy.
     */
    static const char* ERROR_POLICY_NOT_NEWER;

    /**
     * Error name Duplicate Certificate.  Error raised when the certificate
     * is already installed.
     */
    static const char* ERROR_DUPLICATE_CERTIFICATE;

    /**
     * Error name Certificate Not Found.  Error raised when the certificate
     * is not found.
     */
    static const char* ERROR_CERTIFICATE_NOT_FOUND;

    /**
     * For the SendMemberships call, the app sends one cert chain at time since
     * thin client peer may not be able to handle large amount of data.  The app
     * reads back the membership cert chain from the peer.  It keeps looping until
     * both sides exchanged all the relevant membership cert chains.
     * A send code of
     *     SEND_MEMBERSHIP_NONE indicates the peer does not have any membership
     *          cert chain or already sent all of its membership cert chain in
     *          previous replies.
     */
    static const uint8_t SEND_MEMBERSHIP_NONE = 0;
    /**
     *     SEND_MEMBERSHIP_MORE indicates the peer will send more membership
     *          cert chains.
     */
    static const uint8_t SEND_MEMBERSHIP_MORE = 1;
    /**
     *     SEND_MEMBERSHIP_LAST indicates the peer sends the last membership
     *          cert chain.
     */
    static const uint8_t SEND_MEMBERSHIP_LAST = 2;

    class KeyExchangeListener : public ProtectedAuthListener {
      public:
        KeyExchangeListener()
        {
        }
        void SetPermissionMgmtObj(PermissionMgmtObj* pmo)
        {
            this->pmo = pmo;
        }

        /**
         * Simply wraps the call of the same name to the inner AuthListener
         */
        bool RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials);

        /**
         * Simply wraps the call of the same name to the inner ProtectedAuthListener
         */
        bool VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials);

      private:
        PermissionMgmtObj* pmo;
    };

    typedef enum {
        TRUST_ANCHOR_CA = 0,            ///< certificate authority
        TRUST_ANCHOR_SG_AUTHORITY = 1,  ///< security group authority
    } TrustAnchorType;

    struct TrustAnchor {
        TrustAnchorType use;
        qcc::KeyInfoNISTP256 keyInfo;
        qcc::GUID128 securityGroupId;

        TrustAnchor() : use(TRUST_ANCHOR_CA), securityGroupId(0)
        {
        }
        TrustAnchor(TrustAnchorType use) : use(use), securityGroupId(0)
        {
        }
        TrustAnchor(TrustAnchorType use, qcc::KeyInfoNISTP256& keyInfo) : use(use), keyInfo(keyInfo), securityGroupId(0)
        {
        }
    };

    /**
     * The list of trust anchors
     */
    typedef std::vector<TrustAnchor*> TrustAnchorList;

    /**
     * Constructor
     *
     * Must call Init() before using this BusObject.
     */
    PermissionMgmtObj(BusAttachment& bus, const char* objectPath);

    /**
     * virtual destructor
     */
    virtual ~PermissionMgmtObj();

    /**
     * Initialize and Register this BusObject with to the BusAttachment.
     *
     * @return
     *  - #ER_OK on success
     *  - An error status otherwise
     */
    virtual QStatus Init();

    /**
     * Generates the message args to send the membership data to the peer.
     * @param args[out] the vector of the membership cert chain args.
     * @param remotePeerGuid[in] the peer's authentication GUID.
     * @return
     *         - ER_OK if successful.
     *         - an error status otherwise.
     */

    QStatus GenerateSendMemberships(std::vector<std::vector<MsgArg*> >& args, const qcc::GUID128& remotePeerGuid);

    /**
     * Parse the message received from the PermissionMgmt's SendMembership method.
     * @param msg the message
     * @param[in,out] done output flag to indicate that process is complete
     * @return
     *         - ER_OK if successful.
     *         - an error status otherwise.
     */
    QStatus ParseSendMemberships(Message& msg, bool& done);
    QStatus ParseSendMemberships(Message& msg)
    {
        bool done = false;
        return ParseSendMemberships(msg, done);
    }

    /**
     * Parse the message received from the org.alljoyn.bus.Peer.Authentication's
     * SendManifest method.
     * @param msg the message
     * @param peerState the peer state
     * @return ER_OK if successful; otherwise, an error code.
     */
    QStatus ParseSendManifest(Message& msg, PeerState& peerState);

    /**
     * Is there any trust anchor installed?
     * @return true if there is at least one trust anchors installed; false, otherwise.
     */
    bool HasTrustAnchors()
    {
        return !trustAnchors.empty();
    }

    /**
     * Retrieve the list of trust anchors.
     * @return the list of trust anchors
     */

    const TrustAnchorList& GetTrustAnchors()
    {
        return trustAnchors;
    }

    /**
     * Helper function to release the allocated memory for the trust anchor list.
     * @param list the list to be clear of allocated memory.
     */
    static void ClearTrustAnchorList(TrustAnchorList& list);

    /**
     * Help function to store DSA keys in the key store.
     * @param ca the credential accesor object
     * @param privateKey the DSA private key
     * @param publicKey the DSA public key
     * @return ER_OK if successful; otherwise, error code.
     */

    static QStatus StoreDSAKeys(CredentialAccessor* ca, const qcc::ECCPrivateKey* privateKey, const qcc::ECCPublicKey* publicKey);

    /**
     * Set the permission manifest template for the application.
     * @params rules the permission rules.
     * @params count the number of permission rules
     */
    QStatus SetManifestTemplate(const PermissionPolicy::Rule* rules, size_t count);

    /**
     * Retrieve the claimable state of the application.
     * @return the claimable state
     */
    PermissionConfigurator::ApplicationState GetApplicationState();

    /**
     * Set the application state.  The state can't be changed from CLAIMED to
     * CLAIMABLE.
     * @param newState The new application state
     * @return
     *      - #ER_OK if action is allowed.
     *      - #ER_INVALID_APPLICATION_STATE if the state can't be changed
     *      - #ER_NOT_IMPLEMENTED if the method is not implemented
     */
    QStatus SetApplicationState(PermissionConfigurator::ApplicationState state);

    /**
     * Reset the Permission module by removing all the trust anchors, DSA keys,
     * installed policy and certificates.
     * @return ER_OK if successful; otherwise, an error code.
     */
    QStatus Reset();

    /**
     * Get the connected peer authentication metadata.
     * @param guid the peer guid
     * @param[out] authMechanism the authentication mechanism (ie the key exchange name)
     * @param[out] publicKeyFound the flag indicating whether the peer has an ECC public key
     * @param[out] publicKey the buffer to hold the ECC public key.  Pass NULL
     *                       to skip.
     * @param[out] manifestDigest the buffer to hold the manifest digest. Pass
     *                            NULL to skip.
     * @param[out] issuerPublicKeys the vector for the list of issuer public
     *                               keys.
     * @return ER_OK if successful; otherwise, error code.
     */
    QStatus GetConnectedPeerAuthMetadata(const qcc::GUID128& guid, qcc::String& authMechanism, bool& publicKeyFound, qcc::ECCPublicKey* publicKey, uint8_t* manifestDigest, std::vector<qcc::ECCPublicKey>& issuerPublicKeys);

    /**
     * Get the connected peer ECC public key if the connection uses the
     * ECDHE_ECDSA key exchange.
     * @param guid the peer guid
     * @param[out] publicKey the buffer to hold the ECC public key.  Pass NULL
     *                       to skip.
     * @param[out] manifestDigest the buffer to hold the manifest digest. Pass
     *                            NULL to skip.
     * @param[out] issuerPublicKeys the vector for the list of issuer public
     *                               keys.
     * @return ER_OK if successful; otherwise, error code.
     */
    QStatus GetConnectedPeerPublicKey(const qcc::GUID128& guid, qcc::ECCPublicKey* publicKey, uint8_t* manifestDigest, std::vector<qcc::ECCPublicKey>& issuerPublicKeys);

    /**
     * Get the connected peer ECC public key if the connection uses the
     * ECDHE_ECDSA key exchange.
     * @param guid the peer guid
     * @param[out] publicKey the buffer to hold the ECC public key.
     */
    QStatus GetConnectedPeerPublicKey(const qcc::GUID128& guid, qcc::ECCPublicKey* publicKey);

    /**
     * Retrieve the membership certificate map.
     * @return the membership certificate map.
     */
    _PeerState::GuildMap& GetGuildMap()
    {
        return guildMap;
    }

    /**
     * Load the internal data from the key store
     */
    void Load();

    QStatus InstallTrustAnchor(TrustAnchor* trustAnchor);
    QStatus StoreIdentityCertChain(MsgArg& certArg);
    QStatus StorePolicy(PermissionPolicy& policy, bool defaultPolicy = false);
    QStatus StoreMembership(const MsgArg& certArg);
    QStatus StoreManifest(MsgArg& manifestArg);
    /**
     * Generate the SHA-256 digest for the manifest data.
     * @param bus the bus attachment
     * @param rules the rules for the manifest
     * @param count the number of rules
     * @param[out] digest the buffer to store the digest
     * @param[out] digestSize the size of the buffer.  Expected to be Crypto_SHA256::DIGEST_SIZE
     * @return ER_OK for success; error code otherwise.
     */

    static QStatus GenerateManifestDigest(BusAttachment& bus, const PermissionPolicy::Rule* rules, size_t count, uint8_t* digest, size_t digestSize);

    /**
     * Retrieve the manifest from persistence store.
     * @param manifest[out] The variable to hold the manifest.
     *                      Set to NULL to find out the size of the manifest.
     * @param count[in,out] the number of rules in the manifest.
     *                      If manifest is NULL count will be set to the number
     *                      of rules in the manifest. If manifest is not NULL
     *                      count will return the number of rule placed in the
     *                      manifest. If count is smaller than the number of
     *                      rules found in the manifest ER_BUFFER_TOO_SMALL will
     *                      be returned.
     * @return ER_OK for success; error code otherwise.
     */
    QStatus RetrieveManifest(PermissionPolicy::Rule* manifest, size_t* count);
    /**
     * Reply to a method call with an error message.
     *
     * @param msg        The method call message
     * @param status     The status code for the error
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_OBJECT_NOT_REGISTERED if bus object has not yet been registered
     *      - An error status otherwise
     */
    QStatus MethodReply(const Message& msg, QStatus status);

    /**
     * The State signal is used to advertise the state of an application.  It is
     * sessionless, because the signal is intended to discover applications.
     * Discovery is not done by using 'About'.  Applications must add extra code
     * to provide About.
     *
     * Not all applications will do this as pure consumer applications don't
     * need to be discovered by other applications.  Still they need to be
     * discovered by the framework to support certain some core framework
     * features. Furthermore we want to avoid interference between core
     * framework events and application events.
     *
     * The application state is an enumeration representing the current state of
     * the application.
     *
     * The list of valid values:
     * | Value | Description                                                       |
     * |-------|-------------------------------------------------------------------|
     * | 0     | NotClaimable.  The application is not claimed and not accepting   |
     * |       | claim requests.                                                   |
     * | 1     | Claimable.  The application is not claimed and is accepting claim |
     * |       | requests.                                                         |
     * | 2     | Claimed. The application is claimed and can be configured.        |
     * | 3     | NeedUpdate. The application is claimed, but requires a            |
     * |       | configuration update (after a software upgrade).                  |
     *
     * @param[in] publicKeyInfo the application public key
     * @param[in] state the application state
     *
     * @return
     *   - #ER_OK on success
     *   - An error status otherwise.
     */
    virtual QStatus State(const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state) = 0;

    /**
     * Set the authentication mechanisms the application supports for the
     * claim process.  It is a bit mask.
     *
     * | Mask  | Description                                                   |
     * |-------|---------------------------------------------------------------|
     * | 0x1   | claiming via ECDHE_NULL                                       |
     * | 0x2   | claiming via ECDHE_PSK                                        |
     * | 0x4   | claiming via ECDHE_ECDSA                                      |
     *
     * @param[in] claimCapabilities The authentication mechanisms the application supports
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus SetClaimCapabilities(PermissionConfigurator::ClaimCapabilities claimCapabilities);

    /**
     * Set the additional information on the claim capabilities.
     * It is a bit mask.
     *
     * | Mask  | Description                                                   |
     * |-------|---------------------------------------------------------------|
     * | 0x1   | PSK generated by Security Manager                             |
     * | 0x2   | PSK generated by application                                  |
     *
     * @param[in] additionalInfo The additional info
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus SetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& additionalInfo);

    /**
     * Get the authentication mechanisms the application supports for the
     * claim process.
     *
     * @param[out] claimCapabilities The authentication mechanisms the application supports
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetClaimCapabilities(PermissionConfigurator::ClaimCapabilities& claimCapabilities);


    /**
     * Get the additional information on the claim capabilities.
     * @param[out] additionalInfo The additional info
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus GetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& additionalInfo);

    /**
     * Store a membership certificate chain.
     * @param certChain the array of CertificateX509 objects in the chain.
     * @param count the size of the certificate chain.
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    QStatus StoreMembership(const qcc::CertificateX509* certChain, size_t count);
    /**
     * Get the ECC public key from the keystore.
     * @param[out] publicKeyInfo the public key
     * @return ER_OK if successful; otherwise, error code.
     */
    QStatus GetPublicKey(qcc::KeyInfoNISTP256& publicKeyInfo);

    /**
     * Is ready for service?
     * @see Load()
     * @return true if it is ready for service; false, otherwise.
     */
    bool IsReady()
    {
        return ready;
    }

  protected:
    void Claim(const InterfaceDescription::Member* member, Message& msg);
    BusAttachment& bus;
    QStatus GetIdentity(MsgArg& arg);
    QStatus GetIdentityLeafCert(qcc::IdentityCertificate& cert);
    QStatus RetrieveIdentityCertificateId(qcc::String& serial, qcc::KeyInfoNISTP256& issuerKeyInfo);
    void Reset(const InterfaceDescription::Member* member, Message& msg);
    void InstallIdentity(const InterfaceDescription::Member* member, Message& msg);
    void InstallPolicy(const InterfaceDescription::Member* member, Message& msg);
    QStatus RetrievePolicy(PermissionPolicy& policy, bool defaultPolicy = false);
    QStatus GetPolicy(MsgArg& msgArg);
    QStatus RebuildDefaultPolicy(PermissionPolicy& defaultPolicy);
    QStatus GetDefaultPolicy(MsgArg& msgArg);
    void ResetPolicy(const InterfaceDescription::Member* member, Message& msg);
    void InstallMembership(const InterfaceDescription::Member* member, Message& msg);
    void RemoveMembership(const InterfaceDescription::Member* member, Message& msg);
    QStatus GetMembershipSummaries(MsgArg& arg);
    QStatus GetManifestTemplate(MsgArg& arg);
    QStatus GetManifestTemplateDigest(MsgArg& arg);
    PermissionConfigurator::ApplicationState applicationState;
    uint32_t policyVersion;
    uint16_t claimCapabilities;
    uint16_t claimCapabilityAdditionalInfo;

  private:

    typedef enum {
        ENTRY_DEFAULT_POLICY,      ///< Default policy data
        ENTRY_POLICY,              ///< Local policy data
        ENTRY_MEMBERSHIPS,         ///< the list of membership certificates and associated policies
        ENTRY_IDENTITY,            ///< the identity cert
        ENTRY_MANIFEST_TEMPLATE,   ///< The manifest template
        ENTRY_MANIFEST,            ///< The manifest data
        ENTRY_CONFIGURATION        ///< The configuration data
    } ACLEntryType;

    struct Configuration {
        uint8_t version;
        uint8_t applicationStateSet;
        uint8_t applicationState;
        uint16_t claimCapabilities;
        uint16_t claimCapabilityAdditionalInfo;

        Configuration() : version(1), applicationStateSet(0), applicationState(PermissionConfigurator::NOT_CLAIMABLE), claimCapabilities(PermissionConfigurator::CAPABLE_ECDHE_NULL), claimCapabilityAdditionalInfo(0)
        {
        }
    };

    typedef std::map<KeyStore::Key, qcc::MembershipCertificate*> MembershipCertMap;

    class PortListener : public SessionPortListener {

      public:

        PortListener() : SessionPortListener()
        {
        }

        bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
        {
            QCC_UNUSED(joiner);
            QCC_UNUSED(opts);
            return (ALLJOYN_SESSIONPORT_PERMISSION_MGMT == sessionPort);
        }
    };

    void GetPublicKey(const InterfaceDescription::Member* member, Message& msg);
    QStatus GetACLKey(ACLEntryType aclEntryType, KeyStore::Key& guid);
    QStatus StoreTrustAnchors();
    QStatus LoadTrustAnchors();

    QStatus StateChanged();

    QStatus GetIdentityBlob(qcc::KeyBlob& kb);
    bool ValidateCertChain(bool verifyIssuerChain, bool validateTrust, const qcc::CertificateX509* certChain, size_t count, bool enforceAKI = true);
    bool ValidateCertChainPEM(const qcc::String& certChainPEM, bool& authorized, bool enforceAKI = true);
    QStatus LocateMembershipEntry(const qcc::String& serialNum, const qcc::String& issuerAki, KeyStore::Key& membershipKey);
    void ClearMembershipCertMap(MembershipCertMap& certMap);
    QStatus GetAllMembershipCerts(MembershipCertMap& certMap, bool loadCert);
    QStatus GetAllMembershipCerts(MembershipCertMap& certMap);
    void ClearTrustAnchors();
    void PolicyChanged(PermissionPolicy* policy);
    QStatus StoreConfiguration(const Configuration& config);
    QStatus GetConfiguration(Configuration& config);
    QStatus PerformReset(bool keepForClaim);
    QStatus SameSubjectPublicKey(const qcc::CertificateX509& cert, bool& outcome);
    bool IsTrustAnchor(const qcc::ECCPublicKey* publicKey);
    QStatus ManageTrustAnchors(PermissionPolicy* policy);
    QStatus GetDSAPrivateKey(qcc::ECCPrivateKey& privateKey);
    QStatus RetrieveIdentityCertChain(MsgArg** certArgs, size_t* count);
    QStatus RetrieveIdentityCertChainPEM(qcc::String& pem);
    QStatus StoreApplicationState();
    QStatus LoadManifestTemplate(PermissionPolicy& policy);
    bool HasDefaultPolicy();
    bool IsRelevantMembershipCert(std::vector<MsgArg*>& membershipChain, std::vector<qcc::ECCPublicKey> peerIssuers);
    QStatus LookForManifestTemplate(bool& exist);

    /**
     * Bind to an exclusive port for PermissionMgmt object.
     */
    QStatus BindPort();

    CredentialAccessor* ca;
    TrustAnchorList trustAnchors;
    _PeerState::GuildMap guildMap;
    PortListener* portListener;
    MessageEncryptionNotification* callbackToClearSecrets;
    bool ready;
};

}
#endif
