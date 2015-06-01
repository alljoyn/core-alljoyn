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

class PermissionMgmtObj : public BusObject {

    friend class PermissionManager;

  public:

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
        TRUST_ANCHOR_CA = 0,           ///< certificate authority
        TRUST_ANCHOR_ADMIN_GROUP = 1,  ///< admin group authority
        TRUST_ANCHOR_MEMBERSHIP = 2    ///< Trust anchor for membership cert
    } TrustAnchorType;

    struct TrustAnchor {
        TrustAnchorType use;
        qcc::KeyInfoNISTP256 keyInfo;
        qcc::GUID128 securityGroupId;

        TrustAnchor() : use(TRUST_ANCHOR_CA), securityGroupId(0)
        {
            qcc::ECCPublicKey pubKey;
            memset(&pubKey, 0, sizeof(qcc::ECCPublicKey));
            keyInfo.SetPublicKey(&pubKey);
        }
        TrustAnchor(TrustAnchorType use) : use(use), securityGroupId(0)
        {
            qcc::ECCPublicKey pubKey;
            memset(&pubKey, 0, sizeof(qcc::ECCPublicKey));
            keyInfo.SetPublicKey(&pubKey);
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
     */
    PermissionMgmtObj(BusAttachment& bus);

    /**
     * virtual destructor
     */
    virtual ~PermissionMgmtObj();

    /**
     * check whether the peer public key is an admin
     * @param publicKey the peer's public key
     * return true if the peer public key is an admin; false, otherwise.
     */
    bool IsAdmin(const qcc::ECCPublicKey* publicKey);

    /**
     * check whether the membership cert chain is for the admin security group
     * @param certChain the membership cert chain
     * return true if it is an admin security group; false, otherwise.
     */
    bool IsAdminGroup(const std::vector<qcc::MembershipCertificate*> certChain);

    /**
     * Called by the message bus when the object has been successfully registered. The object can
     * perform any initialization such as adding match rules at this time.
     */

    virtual void ObjectRegistered(void);

    /**
     * Generates the message args to send the membership data to the peer.
     * @param args[out] the output array of message args.  The caller must delete the array of message args after use.
     * @param count[out] the size of the output array array of message args.
     * @return
     *         - ER_OK if successful.
     *         - an error status otherwise.
     */

    QStatus GenerateSendMemberships(MsgArg** args, size_t* count);

    /**
     * Parse the message received from the PermissionMgmt's SendMembership method.
     * @param args the message
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
     * Set the permission manifest for the application.
     * @params rules the permission rules.
     * @params count the number of permission rules
     */
    QStatus SetManifest(PermissionPolicy::Rule* rules, size_t count);

    /**
     * Retrieve the claimable state of the application.
     * @return the claimable state
     */
    PermissionConfigurator::ClaimableState GetClaimableState();

    /**
     * Set the claimable state to be claimable or not.  The resulting claimable
     * state would be either STATE_UNCLAIMABLE or STATE_CLAIMABLE depending on
     * the value of the input flag.  This action is not allowed when the current
     * state is STATE_CLAIMED.
     * @param claimable flag
     * @return
     *      - #ER_OK if action is allowed.
     *      - #ER_INVALID_CLAIMABLE_STATE if current state is STATE_CLAIMED
     *      - #ER_NOT_IMPLEMENTED if the method is not implemented
     */
    QStatus SetClaimable(bool claimable);

    /**
     * Reset the Permission module by removing all the trust anchors, DSA keys,
     * installed policy and certificates.
     * @return ER_OK if successful; otherwise, an error code.
     */
    QStatus Reset();

    /**
     * Get the connected peer ECC public key if the connection uses the
     * ECDHE_ECDSA key exchange.
     * @param guid the peer guid
     * @param[out] the buffer to hold the ECC public key.
     * @return ER_OK if successful; otherwise, error code.
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

    /**
     * Handle a bus request to read a property from this object.
     *
     * @param ifcName    Identifies the interface that the property is defined on
     * @param propName  Identifies the the property to get
     * @param[out] val        Returns the property value. The type of this value is the actual value
     *                   type.
     * @return ER_OK for success, error code otherwise.
     */
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);

    QStatus InstallTrustAnchor(TrustAnchor* trustAnchor);
    QStatus StoreIdentityCertChain(MsgArg& certArg);
    QStatus StoreManifest(MsgArg& manifestArg);
    QStatus StorePolicy(PermissionPolicy& policy);
    QStatus StoreMembership(const MsgArg& certArg);
    void InstallMembership(const InterfaceDescription::Member* member, Message& msg);
    QStatus StoreMembershipAuthData(const MsgArg* msgArg);
    void InstallMembershipAuthData(const InterfaceDescription::Member* member, Message& msg);
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

  private:

    typedef enum {
        ENTRY_TRUST_ANCHOR,        ///< Trust anchor.
        ENTRY_POLICY,              ///< Local policy data
        ENTRY_MEMBERSHIPS,         ///< the list of membership certificates and associated policies
        ENTRY_IDENTITY,            ///< the identity cert
        ENTRY_MANIFEST_TEMPLATE,   ///< The manifest template
        ENTRY_MANIFEST,            ///< The manifest data
        ENTRY_CONFIGURATION        ///< The configuration data
    } ACLEntryType;

    struct Configuration {
        uint8_t version;
        uint8_t claimableState;

        Configuration() : version(1)
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
    void Claim(const InterfaceDescription::Member* member, Message& msg);
    void InstallPolicy(const InterfaceDescription::Member* member, Message& msg);
    QStatus GetACLKey(ACLEntryType aclEntryType, KeyStore::Key& guid);
    QStatus StoreTrustAnchors();
    QStatus LoadTrustAnchors();
    QStatus RemoveTrustAnchor(TrustAnchor* trustAnchor);

    QStatus GetPeerGUID(Message& msg, qcc::GUID128& guid);

    QStatus RetrievePolicy(PermissionPolicy& policy);
    void RemovePolicy(const InterfaceDescription::Member* member, Message& msg);
    void GetPolicy(const InterfaceDescription::Member* member, Message& msg);
    QStatus NotifyConfig();
    void InstallIdentity(const InterfaceDescription::Member* member, Message& msg);
    QStatus GetIdentityBlob(qcc::KeyBlob& kb);
    QStatus GetIdentityLeafCert(qcc::IdentityCertificate& cert);
    void GetIdentity(const InterfaceDescription::Member* member, Message& msg);
    void RemoveMembership(const InterfaceDescription::Member* member, Message& msg);
    void GetManifest(const InterfaceDescription::Member* member, Message& msg);
    bool ValidateCertChain(const qcc::String& certChainPEM, bool& authorized);
    QStatus LocateMembershipEntry(const qcc::String& serialNum, const qcc::String& issuerAki, KeyStore::Key& membershipKey, bool searchLeafCertOnly);
    QStatus LocateMembershipEntry(const qcc::String& serialNum, const qcc::String& issuerAki, KeyStore::Key& membershipKey);
    QStatus LoadAndValidateAuthData(const qcc::String& serial, const qcc::String& issuerAki, MsgArg& authDataArg, PermissionPolicy& authorization, KeyStore::Key& membershipKey);
    void ClearMembershipCertMap(MembershipCertMap& certMap);
    QStatus GetAllMembershipCerts(MembershipCertMap& certMap, bool loadCert);
    QStatus GetAllMembershipCerts(MembershipCertMap& certMap);
    void ClearTrustAnchors();
    void PolicyChanged(PermissionPolicy* policy);
    QStatus StoreConfiguration(const Configuration& config);
    QStatus GetConfiguration(Configuration& config);
    void Reset(const InterfaceDescription::Member* member, Message& msg);
    QStatus PerformReset(bool keepForClaim);
    QStatus SameSubjectPublicKey(qcc::CertificateX509& cert, bool& outcome);
    QStatus LocalMembershipsChanged();
    bool IsTrustAnchor(TrustAnchorType taType, const qcc::ECCPublicKey* publicKey);
    QStatus GetTrustAnchorsFromAllMemberships(TrustAnchorList& taList);
    QStatus ManageMembershipTrustAnchors(PermissionPolicy* policy);
    QStatus GetDSAPrivateKey(qcc::ECCPrivateKey& privateKey);

    /**
     * Bind to an exclusive port for PermissionMgmt object.
     */
    QStatus BindPort();

    BusAttachment& bus;
    CredentialAccessor* ca;
    const InterfaceDescription::Member* notifySignalName;
    PermissionConfigurator::ClaimableState claimableState;
    uint32_t serialNum;
    TrustAnchorList trustAnchors;
    _PeerState::GuildMap guildMap;
    PortListener* portListener;
};

}
#endif
