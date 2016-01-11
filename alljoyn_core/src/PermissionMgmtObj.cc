/**
 * @file
 * This file implements the PermissionMgmt interface.
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

#include <map>
#include <alljoyn/AllJoynStd.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/Crypto.h>
#include <qcc/CertificateECC.h>
#include <qcc/CertificateHelper.h>
#include <qcc/StringUtil.h>
#include <qcc/Event.h>
#include "PermissionMgmtObj.h"
#include "PeerState.h"
#include "BusInternal.h"
#include "KeyInfoHelper.h"
#include "KeyExchanger.h"

#define QCC_MODULE "PERMISSION_MGMT"

/**
 * The ALLJOYN ECDHE auth mechanism name prefix pattern
 */
#define ECDHE_NAME_PREFIX_PATTERN "ALLJOYN_ECDHE*"

/**
 * The ALLJOYN ECDHE_ECDSA auth mechanism name pattern
 */
#define ECDHE_ECDSA_NAME_PATTERN "ALLJOYN_ECDHE_ECDSA"

using namespace std;
using namespace qcc;

namespace ajn {

/* the error names */
const char* PermissionMgmtObj::ERROR_PERMISSION_DENIED = "org.alljoyn.Bus.Security.Error.PermissionDenied";
const char* PermissionMgmtObj::ERROR_INVALID_CERTIFICATE = "org.alljoyn.Bus.Security.Error.InvalidCertificate";
const char* PermissionMgmtObj::ERROR_INVALID_CERTIFICATE_USAGE = "org.alljoyn.Bus.Security.Error.InvalidCertificateUsage";
const char* PermissionMgmtObj::ERROR_DIGEST_MISMATCH = "org.alljoyn.Bus.Security.Error.DigestMismatch";
const char* PermissionMgmtObj::ERROR_POLICY_NOT_NEWER = "org.alljoyn.Bus.Security.Error.PolicyNotNewer";
const char* PermissionMgmtObj::ERROR_DUPLICATE_CERTIFICATE = "org.alljoyn.Bus.Security.Error.DuplicateCertificate";
const char* PermissionMgmtObj::ERROR_CERTIFICATE_NOT_FOUND = "org.alljoyn.Bus.Security.Error.CertificateNotFound";
const char* PermissionMgmtObj::ERROR_MANAGEMENT_ALREADY_STARTED = "org.alljoyn.Bus.Security.Error.ManagementAlreadyStarted";
const char* PermissionMgmtObj::ERROR_MANAGEMENT_NOT_STARTED = "org.alljoyn.Bus.Security.Error.ManagementNotStarted";

/**
 * Interest in the encryption step of a message in order to clear the secrets.
 */

class ClearSecretsWhenMsgDeliveredNotification : public MessageEncryptionNotification {
  public:
    ClearSecretsWhenMsgDeliveredNotification(BusAttachment& bus) : MessageEncryptionNotification(), bus(bus)
    {
    }

    ~ClearSecretsWhenMsgDeliveredNotification()
    {
    }

    /**
     * Notified the message encryption step is complete.
     */
    void EncryptionComplete()
    {
        /**
         * Clear out the master secrets used in ECDSA key exchange
         */
        bus.GetInternal().GetKeyStore().Clear(ECDHE_ECDSA_NAME_PATTERN);
        /* clear all the peer states to force re-authentication */
        bus.GetInternal().GetPeerStateTable()->Clear();
    }

  private:
    /**
     * Assignment not allowed
     */
    ClearSecretsWhenMsgDeliveredNotification& operator=(const ClearSecretsWhenMsgDeliveredNotification& other);

    /**
     * Copy constructor not allowed
     */
    ClearSecretsWhenMsgDeliveredNotification(const ClearSecretsWhenMsgDeliveredNotification& other);

    BusAttachment& bus;
};

static QStatus RetrieveAndGenDSAPublicKey(CredentialAccessor* ca, KeyInfoNISTP256& keyInfo);

PermissionMgmtObj::PermissionMgmtObj(BusAttachment& bus, const char* objectPath) :
    BusObject(objectPath), bus(bus), claimCapabilities(PermissionConfigurator::CLAIM_CAPABILITIES_DEFAULT),
    claimCapabilityAdditionalInfo(0), portListener(NULL), callbackToClearSecrets(NULL), ready(false), managementStarted(false)
{
}

QStatus PermissionMgmtObj::Init()
{
    ca = new CredentialAccessor(bus);
    bus.GetInternal().GetPermissionManager().SetPermissionMgmtObj(this);
    callbackToClearSecrets = new ClearSecretsWhenMsgDeliveredNotification(bus);
    return bus.RegisterBusObject(*this, true);
}

void PermissionMgmtObj::Load()
{
    KeyInfoNISTP256 keyInfo;
    RetrieveAndGenDSAPublicKey(ca, keyInfo);

    Configuration config;
    QStatus status = GetConfiguration(config);
    if (ER_OK == status) {
        applicationState = static_cast<PermissionConfigurator::ApplicationState> (config.applicationState);
        claimCapabilities = config.claimCapabilities;
        claimCapabilityAdditionalInfo = config.claimCapabilityAdditionalInfo;
    }

    ready = true;

    /* Bind to the reserved port for PermissionMgmt */
    BindPort();

    /* notify others about policy changed */
    PermissionPolicy* policy = new PermissionPolicy();
    status = RetrievePolicy(*policy);
    if (ER_OK == status) {
        policyVersion = policy->GetVersion();
    } else {
        policyVersion = 0;
        delete policy;
        policy = NULL;
    }
    PolicyChanged(policy);
    bool hasManifestTemplate;
    status = LookForManifestTemplate(hasManifestTemplate);
    /* emit application state signal only when there is an installed manifest
     * template. */
    if ((ER_OK == status) && hasManifestTemplate) {
        if (!config.applicationStateSet) {
            /* if the application is not set yet, then set it as claimable */
            applicationState = PermissionConfigurator::CLAIMABLE;
            StoreApplicationState();
        }
        StateChanged();
    }
}


PermissionMgmtObj::~PermissionMgmtObj()
{
    delete ca;
    ClearTrustAnchors();
    _PeerState::ClearGuildMap(guildMap);
    if (portListener) {
        if (bus.IsStarted()) {
            bus.UnbindSessionPort(ALLJOYN_SESSIONPORT_PERMISSION_MGMT);
        }
        delete portListener;
    }
    bus.UnregisterBusObject(*this);
    delete callbackToClearSecrets;
}

void PermissionMgmtObj::PolicyChanged(PermissionPolicy* policy)
{
    qcc::GUID128 localGUID;
    ca->GetGuid(localGUID);
    bus.GetInternal().GetPermissionManager().SetPolicy(policy);
    ManageTrustAnchors(policy);

    /* Finally, inform the application that it's security policy has changed. */
    bus.GetInternal().CallPolicyChangedCallback();
}

static QStatus RetrieveAndGenDSAPublicKey(CredentialAccessor* ca, KeyInfoNISTP256& keyInfo)
{
    bool genKeys = false;
    ECCPublicKey pubKey;
    QStatus status = ca->GetDSAPublicKey(pubKey);
    if (status != ER_OK) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            genKeys = true;
        } else {
            return status;
        }
    }
    if (genKeys) {
        Crypto_ECC ecc;
        status = ecc.GenerateDSAKeyPair();
        if (status != ER_OK) {
            return ER_CRYPTO_KEY_UNAVAILABLE;
        }
        status = PermissionMgmtObj::StoreDSAKeys(ca, ecc.GetDSAPrivateKey(), ecc.GetDSAPublicKey());
        if (status != ER_OK) {
            return ER_CRYPTO_KEY_UNAVAILABLE;
        }
        /* retrieve the public key */
        status = ca->GetDSAPublicKey(pubKey);
        if (status != ER_OK) {
            return status;
        }
    }

    qcc::GUID128 localGUID;
    ca->GetGuid(localGUID);
    keyInfo.SetKeyId(localGUID.GetBytes(), GUID128::SIZE);
    keyInfo.SetPublicKey(&pubKey);
    return ER_OK;
}

QStatus PermissionMgmtObj::GetACLKey(ACLEntryType aclEntryType, KeyStore::Key& key)
{
    key.SetType(KeyStore::Key::LOCAL);
    /* each local key will be indexed by an hardcode randomly generated GUID. */
    if (aclEntryType == ENTRY_DEFAULT_POLICY) {
        key.SetGUID(GUID128(qcc::String("D946354436F2F3C79EAF0636D947E8AC")));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_POLICY) {
        key.SetGUID(GUID128(qcc::String("F5CB9E723D7D4F1CFF985F4DD0D5E388")));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_MEMBERSHIPS) {
        key.SetGUID(GUID128(qcc::String("42B0C7F35695A3220A46B3938771E965")));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_IDENTITY) {
        key.SetGUID(GUID128(qcc::String("4D8B9E901D7BE0024A331609BBAA4B02")));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_MANIFEST_TEMPLATE) {
        key.SetGUID(GUID128(qcc::String("2962ADEAE004074C8C0D598C5387FEB3")));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_MANIFEST) {
        key.SetGUID(GUID128(qcc::String("5FFFB6E33C19F27A8A5535D45382B9E8")));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_CONFIGURATION) {
        key.SetGUID(GUID128(qcc::String("4EDDBBCE46E0542C24BEC5BF9717C971")));
        return ER_OK;
    }
    return ER_CRYPTO_KEY_UNAVAILABLE;      /* not available */
}

QStatus PermissionMgmtObj::MethodReply(const Message& msg, QStatus status)
{
    const char* errorName = nullptr;
    switch (status) {
    case ER_PERMISSION_DENIED:          errorName = ERROR_PERMISSION_DENIED; break;

    case ER_INVALID_CERTIFICATE:        errorName = ERROR_INVALID_CERTIFICATE; break;

    case ER_INVALID_CERTIFICATE_USAGE:  errorName = ERROR_INVALID_CERTIFICATE_USAGE; break;

    case ER_DUPLICATE_CERTIFICATE:      errorName = ERROR_DUPLICATE_CERTIFICATE; break;

    case ER_DIGEST_MISMATCH:            errorName = ERROR_DIGEST_MISMATCH; break;

    case ER_POLICY_NOT_NEWER:           errorName = ERROR_POLICY_NOT_NEWER; break;

    case ER_MANAGEMENT_ALREADY_STARTED: errorName = ERROR_MANAGEMENT_ALREADY_STARTED; break;

    case ER_MANAGEMENT_NOT_STARTED:     errorName = ERROR_MANAGEMENT_NOT_STARTED; break;

    default:
        QCC_DbgPrintf(("Replying with status code = %#x", status));
        return BusObject::MethodReply(msg, status);
    }

    QCC_DbgPrintf(("Replying with error = %s", errorName));
    return BusObject::MethodReply(msg, errorName);
}

static bool CanBeCAForIdentity(PermissionMgmtObj::TrustAnchorType taType)
{
    if (taType == PermissionMgmtObj::TRUST_ANCHOR_CA) {
        return true;
    }
    if (taType == PermissionMgmtObj::TRUST_ANCHOR_SG_AUTHORITY) {
        return true;
    }
    return false;
}

static PermissionMgmtObj::TrustAnchorList LocateTrustAnchor(PermissionMgmtObj::TrustAnchorList& trustAnchors, const qcc::String& aki)
{
    PermissionMgmtObj::TrustAnchorList retList;
    trustAnchors.Lock(MUTEX_CONTEXT);

    for (PermissionMgmtObj::TrustAnchorList::const_iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if (!CanBeCAForIdentity((*it)->use)) {
            continue;
        }
        if ((aki.size() == (*it)->keyInfo.GetKeyIdLen()) &&
            (memcmp(aki.data(), (*it)->keyInfo.GetKeyId(), aki.size()) == 0)) {
            retList.push_back(*it);
        }
    }

    trustAnchors.Unlock(MUTEX_CONTEXT);
    return retList;
}

bool PermissionMgmtObj::IsTrustAnchor(const ECCPublicKey* publicKey)
{
    bool isTrustAnchor = false;
    trustAnchors.Lock(MUTEX_CONTEXT);

    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if (CanBeCAForIdentity((*it)->use) &&
            (*((*it)->keyInfo.GetPublicKey()) == *publicKey)) {
            isTrustAnchor = true;
            break;
        }
    }

    trustAnchors.Unlock(MUTEX_CONTEXT);
    return isTrustAnchor;
}

static bool AddTrustAnchorIfNotDuplicate(std::shared_ptr<PermissionMgmtObj::TrustAnchor> ta, PermissionMgmtObj::TrustAnchorList& trustAnchors)
{
    bool duplicate = false;
    trustAnchors.Lock(MUTEX_CONTEXT);

    /* check for duplicate trust anchor */
    for (PermissionMgmtObj::TrustAnchorList::const_iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if ((*it)->use != ta->use) {
            continue;
        }
        if (((*it)->use == PermissionMgmtObj::TRUST_ANCHOR_SG_AUTHORITY) &&
            ((*it)->securityGroupId != ta->securityGroupId)) {
            continue;
        }
        if (*(*it)->keyInfo.GetPublicKey() == *(ta->keyInfo.GetPublicKey())) {
            duplicate = true;
            break;
        }
    }

    if (!duplicate) {
        trustAnchors.push_back(ta);
    }

    trustAnchors.Unlock(MUTEX_CONTEXT);
    return false;
}

static void LoadSGAuthoritiesAndCAs(const PermissionPolicy& policy, PermissionMgmtObj::TrustAnchorList& taList)
{
    const PermissionPolicy::Acl* acls = policy.GetAcls();
    for (size_t cnt = 0; cnt < policy.GetAclsSize(); cnt++) {
        const PermissionPolicy::Peer* peers = acls[cnt].GetPeers();
        for (size_t idx = 0; idx < acls[cnt].GetPeersSize(); idx++) {
            if (((peers[idx].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) ||
                 (peers[idx].GetType() == PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY)) && peers[idx].GetKeyInfo()) {
                if (KeyInfoHelper::InstanceOfKeyInfoNISTP256(*peers[idx].GetKeyInfo())) {
                    std::shared_ptr<PermissionMgmtObj::TrustAnchor> ta;
                    if (peers[idx].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
                        ta = std::make_shared<PermissionMgmtObj::TrustAnchor>(PermissionMgmtObj::TRUST_ANCHOR_SG_AUTHORITY, *(KeyInfoNISTP256*)peers[idx].GetKeyInfo());
                    } else {
                        ta = std::make_shared<PermissionMgmtObj::TrustAnchor>(PermissionMgmtObj::TRUST_ANCHOR_CA, *(KeyInfoNISTP256*)peers[idx].GetKeyInfo());
                    }

                    if (ta->keyInfo.GetKeyIdLen() == 0) {
                        KeyInfoHelper::GenerateKeyId(ta->keyInfo);
                    }
                    if (peers[idx].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
                        ta->securityGroupId = peers[idx].GetSecurityGroupId();
                    }

                    AddTrustAnchorIfNotDuplicate(ta, taList);
                }
            }
        }
    }
}

void PermissionMgmtObj::ClearTrustAnchors()
{
    trustAnchors.Lock(MUTEX_CONTEXT);
    trustAnchors.clear();
    trustAnchors.Unlock(MUTEX_CONTEXT);
}

QStatus PermissionMgmtObj::StoreDSAKeys(CredentialAccessor* ca, const ECCPrivateKey* privateKey, const ECCPublicKey* publicKey)
{
    size_t exportedPrivateKeySize = privateKey->GetSize();
    size_t exportedPublicKeySize = publicKey->GetSize();
    uint8_t* exportedKey = new uint8_t[max(exportedPrivateKeySize, exportedPublicKeySize)];
    QCC_VERIFY(ER_OK == privateKey->Export(exportedKey, &exportedPrivateKeySize));
    KeyBlob dsaPrivKb(exportedKey, exportedPrivateKeySize, KeyBlob::DSA_PRIVATE);
    qcc::ClearMemory(exportedKey, exportedPrivateKeySize);
    KeyStore::Key key;
    ca->GetLocalKey(KeyBlob::DSA_PRIVATE, key);
    QStatus status = ca->StoreKey(key, dsaPrivKb);
    if (status != ER_OK) {
        delete[] exportedKey;
        return status;
    }

    QCC_VERIFY(ER_OK == publicKey->Export(exportedKey, &exportedPublicKeySize));
    KeyBlob dsaPubKb(exportedKey, exportedPublicKeySize, KeyBlob::DSA_PUBLIC);
    delete[] exportedKey;
    ca->GetLocalKey(KeyBlob::DSA_PUBLIC, key);
    return ca->StoreKey(key, dsaPubKb);
}

QStatus PermissionMgmtObj::GetPublicKey(KeyInfoNISTP256& publicKeyInfo)
{
    return RetrieveAndGenDSAPublicKey(ca, publicKeyInfo);
}

/**
 * Generate the default policy.
 * 1. Admin group has full access
 * 2. Outgoing messages are authorized
 * 3. Incoming messages are denied
 * 4. Allow for self-installation of membership certificates
 * @param certificateAuthority the certificate authority
 * @param adminGroupGUID the admin group GUID
 * @param adminGroupAuthority the admin group authority
 * @param localPublicKey the local public key
 * @param[out] policy the permission policy
 */

static void GenerateDefaultPolicy(const KeyInfoNISTP256& certificateAuthority, const GUID128& adminGroupGUID, const KeyInfoNISTP256& adminGroupAuthority, const ECCPublicKey* localPublicKey, PermissionPolicy& policy)
{
    policy.SetVersion(0);

    /* add the acls section */
    PermissionPolicy::Acl acls[4];
    /* acls record 0  Certificate authority */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
        peers[0].SetKeyInfo(&certificateAuthority);
        acls[0].SetPeers(1, peers);
    }
    /* acls record 1  ADMIN GROUP */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
        peers[0].SetSecurityGroupId(adminGroupGUID);
        peers[0].SetKeyInfo(&adminGroupAuthority);
        acls[1].SetPeers(1, peers);

        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName("*");
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("*");
        prms[0].SetActionMask(
            PermissionPolicy::Rule::Member::ACTION_PROVIDE |
            PermissionPolicy::Rule::Member::ACTION_OBSERVE |
            PermissionPolicy::Rule::Member::ACTION_MODIFY
            );
        rules[0].SetMembers(1, prms);
        acls[1].SetRules(1, rules);
    }
    /* acls record 2  LOCAL PUBLIC KEY */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
        KeyInfoNISTP256 keyInfo;
        keyInfo.SetPublicKey(localPublicKey);
        KeyInfoHelper::GenerateKeyId(keyInfo);
        peers[0].SetKeyInfo(&keyInfo);
        acls[2].SetPeers(1, peers);

        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName("org.alljoyn.Bus.Security.ManagedApplication");
        PermissionPolicy::Rule::Member prms[1];
        prms[0].SetMemberName("InstallMembership");
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(1, prms);
        acls[2].SetRules(1, rules);
    }
    /* acls record 3  any trusted user */
    {
        PermissionPolicy::Peer peers[1];
        peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
        acls[3].SetPeers(1, peers);

        PermissionPolicy::Rule rules[1];
        rules[0].SetObjPath("*");
        rules[0].SetInterfaceName("*");
        PermissionPolicy::Rule::Member prms[3];
        prms[0].SetMemberName("*");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        prms[1].SetMemberName("*");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        prms[2].SetMemberName("*");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        rules[0].SetMembers(3, prms);
        acls[3].SetRules(1, rules);
    }
    policy.SetAcls(4, acls);
}

void PermissionMgmtObj::Claim(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_DbgTrace(("PermissionMgmtObj::%s", __FUNCTION__));
    QCC_UNUSED(member);
    if (applicationState == PermissionConfigurator::NOT_CLAIMABLE) {
        BusObject::MethodReply(msg, ERROR_PERMISSION_DENIED, "Unclaimable");
        return;
    }
    if ((applicationState == PermissionConfigurator::CLAIMED) ||
        (applicationState == PermissionConfigurator::NEED_UPDATE)) {
        BusObject::MethodReply(msg, ERROR_PERMISSION_DENIED, "Already claimed");
        return;
    }
    size_t numArgs;
    MsgArg* args;
    msg->GetArgs((size_t&) numArgs, (const MsgArg*&) args);
    if (7 != numArgs) {
        MethodReply(msg, ER_BAD_ARG_COUNT);
        return;
    }

    PermissionPolicy* defaultPolicy = NULL;
    ECCPublicKey pubKey;
    TrustAnchor* adminGroupAuthority = NULL;
    TrustAnchor* certificateAuthority = new TrustAnchor(TRUST_ANCHOR_CA);
    QStatus status = KeyInfoHelper::MsgArgToKeyInfoNISTP256PubKey(args[0], certificateAuthority->keyInfo);
    if (ER_OK != status) {
        goto DoneValidation;
    }
    if (certificateAuthority->keyInfo.empty()) {
        status = ER_BAD_ARG_1;
        goto DoneValidation;
    }
    status = KeyInfoHelper::MsgArgToKeyInfoKeyId(args[1], certificateAuthority->keyInfo);
    if (ER_OK != status) {
        goto DoneValidation;
    }
    if (certificateAuthority->keyInfo.GetKeyIdLen() == 0) {
        status = ER_BAD_ARG_2;
        goto DoneValidation;
    }

    adminGroupAuthority = new TrustAnchor(TRUST_ANCHOR_SG_AUTHORITY);
    /* get the admin security group id */
    uint8_t* buf;
    size_t len;
    status = args[2].Get("ay", &len, &buf);
    if (ER_OK != status) {
        goto DoneValidation;
    }
    if (len != GUID128::SIZE) {
        status = ER_INVALID_DATA;
        goto DoneValidation;
    }
    adminGroupAuthority->securityGroupId.SetBytes(buf);

    /* get the group authority public key */
    status = KeyInfoHelper::MsgArgToKeyInfoNISTP256PubKey(args[3], adminGroupAuthority->keyInfo);
    if (ER_OK != status) {
        goto DoneValidation;
    }
    if (adminGroupAuthority->keyInfo.empty()) {
        status = ER_BAD_ARG_4;
        goto DoneValidation;
    }
    status = KeyInfoHelper::MsgArgToKeyInfoKeyId(args[4], adminGroupAuthority->keyInfo);
    if (ER_OK != status) {
        goto DoneValidation;
    }
    if (adminGroupAuthority->keyInfo.GetKeyIdLen() == 0) {
        status = ER_BAD_ARG_5;
        goto DoneValidation;
    }

    /* clear most of the key entries with the exception of the DSA keys and manifest */
    PerformReset(true);

    /* generate the default policy */
    defaultPolicy = new PermissionPolicy();
    ca->GetDSAPublicKey(pubKey);
    GenerateDefaultPolicy(certificateAuthority->keyInfo, adminGroupAuthority->securityGroupId, adminGroupAuthority->keyInfo, &pubKey, *defaultPolicy);
    /* load the trust anchors from the default policy for validation of the
        identity cert chain */
    status = ManageTrustAnchors(defaultPolicy);
    if (ER_OK != status) {
        goto DoneValidation;
    }

    /* install the identity cert chain */
    status = StoreIdentityCertChain(args[5]);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::Claim failed to store identity certificate chain"));
        goto DoneValidation;
    }
    /* install the manifest */
    status = StoreManifest(args[6]);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::Claim failed to store manifest"));
        goto DoneValidation;
    }

DoneValidation:
    delete adminGroupAuthority;
    delete certificateAuthority;
    if (ER_OK != status) {
        delete defaultPolicy;
        PerformReset(true);
        MethodReply(msg, status);
        return;
    }
    /* store the default policy for support of the property ManagedApplication::DefaultPolicy */
    status = StorePolicy(*defaultPolicy, true);
    if (ER_OK == status) {
        /* store the default policy as the initial local policy */
        status = StorePolicy(*defaultPolicy);
    }
    if (ER_OK != status) {
        delete defaultPolicy;
        PerformReset(true);
    }
    MethodReply(msg, status);

    if (ER_OK == status) {
        applicationState = PermissionConfigurator::CLAIMED;
        StoreApplicationState();
        policyVersion = defaultPolicy->GetVersion();
        PolicyChanged(defaultPolicy);
        StateChanged();
    }
}

void PermissionMgmtObj::InstallPolicy(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);

    PermissionPolicy* policy = new PermissionPolicy();
    QStatus status = policy->Import(PermissionPolicy::SPEC_VERSION, *msg->GetArg(0));
    if (ER_OK != status) {
        delete policy;
        MethodReply(msg, status);
        return;
    }

    /* is there any existing policy?  If so, make sure the new policy must have
       a serial number greater than the existing policy's serial number */
    PermissionPolicy existingPolicy;
    status = RetrievePolicy(existingPolicy);
    if (ER_OK == status) {
        if (policy->GetVersion() <= existingPolicy.GetVersion()) {
            MethodReply(msg, ER_POLICY_NOT_NEWER);
            delete policy;
            return;
        }
    }

    status = StorePolicy(*policy);
    if (ER_OK == status) {
        policyVersion = policy->GetVersion();
    }
    if (ER_OK == status) {
        /* notify me when the encryption completes for this message so the
           secrets can be removed */
        msg->SetMessageEncryptionNotification(callbackToClearSecrets);
    }
    MethodReply(msg, status);
    if (ER_OK == status) {
        PolicyChanged(policy);
    } else {
        delete policy;
    }
}

void PermissionMgmtObj::ResetPolicy(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    KeyStore::Key key;
    GetACLKey(ENTRY_POLICY, key);
    QStatus status = ca->DeleteKey(key);
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }
    /* restore the default policy */
    PermissionPolicy* defaultPolicy = new PermissionPolicy();
    status = RebuildDefaultPolicy(*defaultPolicy);
    if (ER_OK != status) {
        QCC_LogError(status, ("PermissionMgmtObj::ResetPolicy rebuild the default policy failed"));
        goto Exit;
    }
    status = StorePolicy(*defaultPolicy);
    if (ER_OK != status) {
        QCC_LogError(status, ("PermissionMgmtObj::ResetPolicy storing the default policy failed"));
        goto Exit;
    }
Exit:
    MethodReply(msg, status);
    if (ER_OK == status) {
        policyVersion = defaultPolicy->GetVersion();
        PolicyChanged(defaultPolicy);
    } else {
        delete defaultPolicy;
    }
}

QStatus PermissionMgmtObj::GetPolicy(MsgArg& msgArg)
{
    PermissionPolicy policy;
    QStatus status = RetrievePolicy(policy);
    if (ER_OK != status) {
        return status;
    }
    return policy.Export(msgArg);
}

bool PermissionMgmtObj::HasDefaultPolicy()
{
    PermissionPolicy policy;
    return (ER_OK == RetrievePolicy(policy, true));
}

QStatus PermissionMgmtObj::RebuildDefaultPolicy(PermissionPolicy& defaultPolicy)
{
    return RetrievePolicy(defaultPolicy, true);
}

QStatus PermissionMgmtObj::GetDefaultPolicy(MsgArg& msgArg)
{
    /* rebuild the default policy */
    PermissionPolicy defaultPolicy;
    QStatus status = RebuildDefaultPolicy(defaultPolicy);
    if (ER_OK != status) {
        return status;
    }
    return defaultPolicy.Export(msgArg);
}

QStatus PermissionMgmtObj::StorePolicy(PermissionPolicy& policy, bool defaultPolicy)
{
    uint8_t* buf = NULL;
    size_t size;
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    QStatus status = policy.Export(marshaller, &buf, &size);
    if (ER_OK != status) {
        return status;
    }
    /* store the message into the key store */
    KeyStore::Key policyKey;
    if (defaultPolicy) {
        GetACLKey(ENTRY_DEFAULT_POLICY, policyKey);
    } else {
        GetACLKey(ENTRY_POLICY, policyKey);
    }
    KeyBlob kb((uint8_t*) buf, size, KeyBlob::GENERIC);
    delete [] buf;

    return ca->StoreKey(policyKey, kb);
}

QStatus PermissionMgmtObj::RetrievePolicy(PermissionPolicy& policy, bool defaultPolicy)
{
    /* retrieve data from keystore */
    KeyStore::Key policyKey;
    if (defaultPolicy) {
        GetACLKey(ENTRY_DEFAULT_POLICY, policyKey);
    } else {
        GetACLKey(ENTRY_POLICY, policyKey);
    }
    KeyBlob kb;
    QStatus status = ca->GetKey(policyKey, kb);
    if (ER_OK != status) {
        return status;
    }
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    return policy.Import(marshaller, kb.GetData(), kb.GetSize());
}

QStatus PermissionMgmtObj::StateChanged()
{
    ECCPublicKey pubKey;
    QStatus status = ca->GetDSAPublicKey(pubKey);
    if (status != ER_OK) {
        return status;
    }
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(&pubKey);
    status = KeyInfoHelper::GenerateKeyId(keyInfo);
    if (status != ER_OK) {
        return status;
    }
    return State(keyInfo, applicationState);
}

static QStatus ValidateCertificateWithTrustAnchors(const CertificateX509& cert, PermissionMgmtObj::TrustAnchorList* taList)
{
    QStatus status = ER_UNKNOWN_CERTIFICATE;
    taList->Lock(MUTEX_CONTEXT);

    for (PermissionMgmtObj::TrustAnchorList::iterator it = taList->begin(); it != taList->end(); it++) {
        std::shared_ptr<PermissionMgmtObj::TrustAnchor> ta = *it;
        bool qualified = false;
        if (cert.GetType() == CertificateX509::MEMBERSHIP_CERTIFICATE) {
            if ((ta->use == PermissionMgmtObj::TRUST_ANCHOR_SG_AUTHORITY) &&
                (ta->securityGroupId == ((MembershipCertificate*) &cert)->GetGuild())) {
                qualified = true;
            }
        } else {
            qualified = true;
        }
        if (qualified && (cert.Verify(ta->keyInfo.GetPublicKey()) == ER_OK)) {
            status = ER_OK;  /* cert is verified */
            break;
        }
    }

    taList->Unlock(MUTEX_CONTEXT);
    return status;
}

static bool ValidateAKIInCertChain(const qcc::CertificateX509* certChain, size_t count)
{
    for (size_t cnt = 0; cnt < count; cnt++) {
        if (certChain[cnt].GetAuthorityKeyId().empty()) {
            return false;
        }
    }
    return true;
}

static QStatus ValidateMembershipCertificateChain(const CertificateX509* certChain, size_t count, PermissionMgmtObj::TrustAnchorList* taList)
{
    if (count == 0) {
        return ER_INVALID_CERTIFICATE;
    }

    if (!KeyExchangerECDHE_ECDSA::IsCertChainStructureValid(certChain, count)) {
        return ER_INVALID_CERTIFICATE;
    }
    if (!CertificateX509::ValidateCertificateTypeInCertChain(certChain, count)) {
        return ER_INVALID_CERTIFICATE;
    }
    if (!ValidateAKIInCertChain(certChain, count)) {
        return ER_INVALID_CERTIFICATE;
    }
    if (!taList) {
        return ER_OK;
    }
    for (size_t cnt = 0; cnt < count; cnt++) {
        if (ER_OK == ValidateCertificateWithTrustAnchors(certChain[cnt], taList)) {
            return ER_OK;
        }
    }
    return ER_INVALID_CERTIFICATE;
}

static QStatus ValidateMembershipCertificateChain(std::vector<CertificateX509*>& certs, PermissionMgmtObj::TrustAnchorList* taList)
{
    if (certs.size() == 0) {
        return ER_INVALID_CERTIFICATE;
    }

    /* build an array of base CertificateX509 instances for validation */
    CertificateX509* certChain = new CertificateX509[certs.size()];
    if (certChain == NULL) {
        return ER_OUT_OF_MEMORY;
    }
    for (size_t cnt = 0; cnt < certs.size(); cnt++) {
        certChain[cnt] = *(certs[cnt]);
    }
    QStatus status = ValidateMembershipCertificateChain(certChain, certs.size(), taList);
    delete [] certChain;
    return status;
}

static QStatus LoadCertificate(CertificateX509::EncodingType encoding, const uint8_t* encoded, size_t encodedLen, CertificateX509& cert)
{
    if (encoding == CertificateX509::ENCODING_X509_DER) {
        return cert.DecodeCertificateDER(String((const char*) encoded, encodedLen));
    } else if (encoding == CertificateX509::ENCODING_X509_DER_PEM) {
        return cert.DecodeCertificatePEM(String((const char*) encoded, encodedLen));
    }
    return ER_NOT_IMPLEMENTED;
}

QStatus PermissionMgmtObj::SameSubjectPublicKey(const CertificateX509& cert, bool& outcome)
{
    ECCPublicKey pubKey;
    QStatus status = ca->GetDSAPublicKey(pubKey);
    if (status != ER_OK) {
        return status;
    }
    outcome = (pubKey == *cert.GetSubjectPublicKey());
    return ER_OK;
}

QStatus PermissionMgmtObj::GetDSAPrivateKey(qcc::ECCPrivateKey& privateKey)
{
    return ca->GetDSAPrivateKey(privateKey);
}

QStatus PermissionMgmtObj::StoreIdentityCertChain(MsgArg& certArg)
{
    size_t certChainCount;
    MsgArg* certChain;
    QStatus status = certArg.Get("a(yay)", &certChainCount, &certChain);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertChain failed to retrieve certificate chain status 0x%x", status));
        return status;
    }
    if (certChainCount == 0) {
        return ER_INVALID_DATA;
    }
    /* store the identity cert chain into the key store */
    KeyStore::Key identityHead;
    GetACLKey(ENTRY_IDENTITY, identityHead);
    /* remove the existing identity cert chain */
    status = ca->DeleteKey(identityHead);
    if (ER_OK != status) {
        return status;
    }
    CertificateX509* certs = new CertificateX509[certChainCount];
    KeyStore::Key previousKey;
    for (size_t cnt = 0; cnt < certChainCount; cnt++) {
        uint8_t encoding;
        uint8_t* encoded;
        size_t encodedLen;
        status = certChain[cnt].Get("(yay)", &encoding, &encodedLen, &encoded);
        if (ER_OK != status) {
            goto ExitStoreIdentity;
        }
        if ((encoding != CertificateX509::ENCODING_X509_DER) && (encoding != CertificateX509::ENCODING_X509_DER_PEM)) {
            QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertChain does not support encoding %d", encoding));
            status = ER_NOT_IMPLEMENTED;
            goto ExitStoreIdentity;
        }
        status = LoadCertificate((CertificateX509::EncodingType) encoding, encoded, encodedLen, certs[cnt]);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertChain failed to validate certificate status 0x%x", status));
        }
    }
    if (!ValidateCertChain(true, false, certs, certChainCount)) {
        status = ER_INVALID_CERTIFICATE;
        goto ExitStoreIdentity;
    }
    /* now storing the cert chain */
    for (size_t cnt = 0; cnt < certChainCount; cnt++) {
        String der;
        status = certs[cnt].EncodeCertificateDER(der);
        KeyBlob kb((const uint8_t*) der.data(), der.size(), KeyBlob::GENERIC);
        if (cnt == 0) {
            /* the leaf cert public key must be the same as my public key */
            bool sameKey = false;
            status = SameSubjectPublicKey(certs[cnt], sameKey);
            if (ER_OK != status) {
                QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertChain failed to validate certificate subject public key status 0x%x", status));
                goto ExitStoreIdentity;
            }
            if (!sameKey) {
                QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertChain failed since certificate subject public key is not the same as target public key"));
                status = ER_UNKNOWN_CERTIFICATE;
                goto ExitStoreIdentity;
            }
            status = ca->StoreKey(identityHead, kb);
            if (ER_OK != status) {
                goto ExitStoreIdentity;
            }
            previousKey = identityHead;
        } else {
            /* add the next cert as an associate of the previous cert entry */
            GUID128 guid;
            KeyStore::Key key(KeyStore::Key::LOCAL, guid);
            status = ca->AddAssociatedKey(previousKey, key, kb);
            if (ER_OK != status) {
                goto ExitStoreIdentity;
            }
            previousKey = key;
        }
    }

ExitStoreIdentity:

    if (ER_OK != status) {
        /* clear out the identity entry and all of its associated key */
        ca->DeleteKey(identityHead);
    }
    delete [] certs;
    return status;
}

static QStatus RetrieveIdentityCertChainBlobs(CredentialAccessor* ca, KeyStore::Key identityHead, std::vector<KeyBlob*>& blobs)
{
    KeyBlob* kb = new KeyBlob();
    QStatus status = ca->GetKey(identityHead, *kb);
    if (ER_OK != status) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            status = ER_OK;
        }
        delete kb;
        return status;
    }
    blobs.push_back(kb);

    /* go thru the issuers */
    KeyStore::Key* keys = NULL;
    size_t numOfKeys = 0;
    status = ca->GetKeys(identityHead, &keys, &numOfKeys);
    if (ER_OK != status) {
        goto Exit;
    }
    for (size_t cnt = 0; cnt < numOfKeys; cnt++) {
        status = RetrieveIdentityCertChainBlobs(ca, keys[cnt], blobs);
        if (ER_OK != status) {
            goto Exit;
        }
    }
Exit:
    if (ER_OK != status) {
        for (std::vector<KeyBlob*>::iterator it = blobs.begin(); it != blobs.end(); it++) {
            delete *it;
        }
        blobs.clear();
    }
    delete [] keys;
    return status;
}

QStatus PermissionMgmtObj::RetrieveIdentityCertChain(MsgArg** certArgs, size_t* count)
{
    KeyStore::Key identityHead;
    GetACLKey(ENTRY_IDENTITY, identityHead);
    std::vector<KeyBlob*> blobs;
    QStatus status = RetrieveIdentityCertChainBlobs(ca, identityHead, blobs);
    if (ER_OK != status) {
        return status;
    }
    if (blobs.size() == 0) {
        return ER_CERTIFICATE_NOT_FOUND;
    }
    *count = blobs.size();
    *certArgs = new MsgArg[*count];
    for (size_t cnt = 0; cnt < *count; cnt++) {
        KeyBlob* kb = blobs[cnt];
        status = (*certArgs)[cnt].Set("(yay)", CertificateX509::ENCODING_X509_DER, kb->GetSize(), kb->GetData());
        if (ER_OK != status) {
            goto Exit;
        }
        (*certArgs)[cnt].Stabilize();
    }
Exit:
    if (ER_OK != status) {
        delete [] *certArgs;
        *certArgs = NULL;
        *count = 0;
    }
    for (std::vector<KeyBlob*>::iterator it = blobs.begin(); it != blobs.end(); it++) {
        delete *it;
    }
    blobs.clear();
    return status;
}

QStatus PermissionMgmtObj::RetrieveIdentityCertChainPEM(qcc::String& pem)
{
    KeyStore::Key identityHead;
    GetACLKey(ENTRY_IDENTITY, identityHead);
    std::vector<KeyBlob*> blobs;
    QStatus status = RetrieveIdentityCertChainBlobs(ca, identityHead, blobs);
    if (ER_OK != status) {
        return status;
    }
    if (blobs.size() == 0) {
        return ER_CERTIFICATE_NOT_FOUND;
    }
    for (size_t cnt = 0; cnt < blobs.size(); cnt++) {
        KeyBlob* kb = blobs[cnt];
        String der((const char*) kb->GetData(), kb->GetSize());
        String localPEM;
        status = CertificateX509::EncodeCertificatePEM(der, localPEM);
        if (ER_OK != status) {
            goto Exit;
        }
        if (cnt == 0) {
            pem = localPEM;
        } else {
            pem += "\n" + localPEM;
        }
    }
Exit:
    for (std::vector<KeyBlob*>::iterator it = blobs.begin(); it != blobs.end(); it++) {
        delete *it;
    }
    blobs.clear();
    return status;
}

QStatus PermissionMgmtObj::RetrieveIdentityCertificateId(qcc::String& serial, qcc::KeyInfoNISTP256& issuerKeyInfo)
{
    KeyStore::Key identityHead;
    GetACLKey(ENTRY_IDENTITY, identityHead);
    KeyBlob kb;
    QStatus status = ca->GetKey(identityHead, kb);
    if (ER_OK != status) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            status = ER_CERTIFICATE_NOT_FOUND;
        }
        return status;
    }
    IdentityCertificate leafCert;
    status = LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), leafCert);
    if (ER_OK != status) {
        return status;
    }
    if (leafCert.GetSerialLen() > 0) {
        serial.assign(reinterpret_cast<const char*>(leafCert.GetSerial()), leafCert.GetSerialLen());
    }
    issuerKeyInfo.SetKeyId((uint8_t*) leafCert.GetAuthorityKeyId().data(), leafCert.GetAuthorityKeyId().size());

    /* locate the next cert in the identity cert chain for the issuer public key */
    KeyStore::Key* keys = NULL;
    size_t numOfKeys = 0;
    status = ca->GetKeys(identityHead, &keys, &numOfKeys);
    if (ER_OK != status) {
        delete [] keys;
        return status;
    }
    if (numOfKeys > 0) {
        status = ca->GetKey(keys[0], kb);
        if (ER_OK != status) {
            goto Exit;
        }
        IdentityCertificate cert;
        status = LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
        if (ER_OK != status) {
            goto Exit;
        }
        issuerKeyInfo.SetPublicKey(cert.GetSubjectPublicKey());
    } else {
        /* The identity cert is a single cert.  Locate the trust anchors that
         * might sign this leaf cert */
        TrustAnchorList anchors = LocateTrustAnchor(trustAnchors, leafCert.GetAuthorityKeyId());

        if (anchors.empty()) {
            status = ER_UNKNOWN_CERTIFICATE;
            goto Exit;
        }
        for (TrustAnchorList::const_iterator it = anchors.begin(); it != anchors.end(); it++) {
            if (ER_OK == leafCert.Verify((*it)->keyInfo.GetPublicKey())) {
                issuerKeyInfo.SetPublicKey((*it)->keyInfo.GetPublicKey());
                break;
            }
        }
        anchors.clear();
    }
Exit:
    delete [] keys;
    return status;
}

void PermissionMgmtObj::InstallIdentity(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    /* save the current identity cert chain to rollback */
    MsgArg* currentCertArgs = NULL;
    size_t count = 0;
    RetrieveIdentityCertChain(&currentCertArgs, &count);

    QStatus status = StoreIdentityCertChain((MsgArg&) *msg->GetArg(0));
    if (ER_OK != status) {
        goto Exit;
    }
    status = StoreManifest((MsgArg&) *msg->GetArg(1));
Exit:
    if (ER_OK != status) {
        /* restore the identity cert chain */
        if (currentCertArgs != NULL) {
            MsgArg arg("a(yay)", count, currentCertArgs);
            QStatus aStatus = StoreIdentityCertChain(arg);
            if (ER_OK != aStatus) {
                QCC_LogError(aStatus, ("PermissionMgmtObj::InstallIdentity restoring the identity certificate failed"));
            }
        }
    }
    delete [] currentCertArgs;
    if (ER_OK == status) {
        /* notify me when the encryption completes for this message so the
           secrets can be removed */
        msg->SetMessageEncryptionNotification(callbackToClearSecrets);
    }
    MethodReply(msg, status);
    if (ER_OK == status) {
        /* now that there is a new identity and manifest */
        if (applicationState == PermissionConfigurator::NEED_UPDATE) {
            SetApplicationState(PermissionConfigurator::CLAIMED);
        }
    }
}

QStatus PermissionMgmtObj::GetIdentityBlob(KeyBlob& kb)
{
    /* Get the Identity blob from the key store */
    KeyStore::Key key;
    GetACLKey(ENTRY_IDENTITY, key);
    QStatus status = ca->GetKey(key, kb);
    if (ER_OK != status) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            return ER_CERTIFICATE_NOT_FOUND;
        } else {
            return status;
        }
    }
    return ER_OK;
}

QStatus PermissionMgmtObj::GetIdentity(MsgArg& arg)
{
    MsgArg* certArgs = NULL;
    size_t count = 0;
    QStatus status = RetrieveIdentityCertChain(&certArgs, &count);
    if (ER_OK != status) {
        delete [] certArgs;
        return status;
    }
    status = arg.Set("a(yay)", count, certArgs);
    if (ER_OK != status) {
        delete [] certArgs;
        return status;
    }
    arg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    return status;
}

QStatus PermissionMgmtObj::GetIdentityLeafCert(IdentityCertificate& cert)
{
    KeyBlob kb;
    QStatus status = GetIdentityBlob(kb);
    if (ER_OK != status) {
        return status;
    }
    return LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
}

QStatus PermissionMgmtObj::GenerateManifestDigest(BusAttachment& bus, const PermissionPolicy::Rule* rules, size_t count, uint8_t* digest, size_t digestSize)
{
    if (digestSize != Crypto_SHA256::DIGEST_SIZE) {
        return ER_INVALID_DATA;
    }
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    return marshaller.Digest(rules, count, digest, Crypto_SHA256::DIGEST_SIZE);
}

static QStatus GetManifestFromMessageArg(BusAttachment& bus, const MsgArg& manifestArg, PermissionPolicy::Rule** rules, size_t* count, uint8_t* digest)
{
    QStatus status = PermissionPolicy::ParseRules(manifestArg, rules, count);
    if (ER_OK != status) {
        QCC_DbgPrintf(("GetManifestFromMessageArg failed to retrieve rules from msgarg status 0x%x", status));
        return status;
    }
    return PermissionMgmtObj::GenerateManifestDigest(bus, *rules, *count, digest, Crypto_SHA256::DIGEST_SIZE);
}

QStatus PermissionMgmtObj::StoreManifest(MsgArg& manifestArg)
{
    size_t count = 0;
    PermissionPolicy::Rule* rules = NULL;
    IdentityCertificate cert;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    QStatus status = GetManifestFromMessageArg(bus, manifestArg, &rules, &count, digest);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreManifest failed to retrieve rules from msgarg status 0x%x", status));
        goto DoneValidation;
    }
    if (count == 0) {
        status = ER_INVALID_DATA;
        goto DoneValidation;
    }
    /* retrieve the identity leaf cert */
    status = GetIdentityLeafCert(cert);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreManifest failed to retrieve identify cert status 0x%x\n", status));
        status = ER_CERTIFICATE_NOT_FOUND;
        goto DoneValidation;
    }
    /* compare the digests */
    if (memcmp(digest, cert.GetDigest(), Crypto_SHA256::DIGEST_SIZE)) {
        status = ER_DIGEST_MISMATCH;
        goto DoneValidation;
    }

DoneValidation:
    if (ER_OK != status) {
        delete [] rules;
        return status;
    }
    /* store the manifest */
    PermissionPolicy policy;
    PermissionPolicy::Acl acls[1];
    acls[0].SetRules(count, rules);
    policy.SetAcls(1, acls);
    delete [] rules;
    rules = NULL;

    uint8_t* buf = NULL;
    size_t size;
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    status = policy.Export(marshaller, &buf, &size);
    if (ER_OK != status) {
        return status;
    }
    /* store the manifest into the key store */
    KeyStore::Key key;
    GetACLKey(ENTRY_MANIFEST, key);
    KeyBlob kb((uint8_t*) buf, size, KeyBlob::GENERIC);
    delete [] buf;
    return ca->StoreKey(key, kb);
}

static QStatus GetMembershipKey(CredentialAccessor* ca, KeyStore::Key& membershipHead, const String& serialNum, const String& issuerAki, KeyStore::Key& membershipKey)
{
    KeyStore::Key* keys = NULL;
    size_t numOfKeys;
    QStatus status = ca->GetKeys(membershipHead, &keys, &numOfKeys);
    if (ER_OK != status) {
        return status;
    }
    String tag = serialNum.substr(0, KeyBlob::MAX_TAG_LEN);
    bool found = false;
    status = ER_OK;
    for (size_t cnt = 0; cnt < numOfKeys; cnt++) {
        KeyBlob kb;
        status = ca->GetKey(keys[cnt], kb);
        if (ER_OK != status) {
            break;
        }
        /* check the tag */

        if (kb.GetTag() == tag) {
            /* maybe a match, check both serial number and issuer */
            MembershipCertificate cert;
            LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
            if ((cert.GetSerialLen() == serialNum.size()) &&
                (memcmp(cert.GetSerial(), serialNum.data(), serialNum.size()) == 0) &&
                (cert.GetAuthorityKeyId() == issuerAki)) {
                membershipKey = keys[cnt];
                found = true;
                break;
            }
        }
    }
    delete [] keys;
    if (ER_OK != status) {
        return status;
    }
    if (found) {
        return ER_OK;
    }
    return ER_BUS_KEY_UNAVAILABLE;  /* not found */
}

static QStatus LoadX509CertFromMsgArg(const MsgArg& arg, CertificateX509& cert)
{
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    QStatus status = arg.Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        return status;
    }
    if ((encoding != CertificateX509::ENCODING_X509_DER) && (encoding != CertificateX509::ENCODING_X509_DER_PEM)) {
        return ER_NOT_IMPLEMENTED;
    }
    status = LoadCertificate((CertificateX509::EncodingType) encoding, encoded, encodedLen, cert);
    if (ER_OK != status) {
        return ER_INVALID_CERTIFICATE;
    }
    return ER_OK;
}

void PermissionMgmtObj::InstallMembership(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QStatus status = StoreMembership(*(msg->GetArg(0)));
    MethodReply(msg, status);
}

QStatus PermissionMgmtObj::StoreMembership(const qcc::CertificateX509* certChain, size_t count)
{
    if (count == 0) {
        return ER_OK;
    }
    /* make sure the leaf cert subject public key is the same as the one in
     * the keystore.
     */
    bool sameKey = false;
    QStatus status = SameSubjectPublicKey(certChain[0], sameKey);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreMembership failed to validate certificate subject public key status 0x%x", status));
        return status;
    }
    if (!sameKey) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreMembership failed since certificate subject public key is not the same as target public key"));
        return ER_INVALID_CERTIFICATE;
    }
    status = ValidateMembershipCertificateChain(certChain, count, NULL);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreMembership failed to validate certificate chain 0x%x", status));
        return status;
    }
    GUID128 membershipGuid;
    KeyStore::Key membershipKey(KeyStore::Key::LOCAL, membershipGuid);
    for (size_t cnt = 0; cnt < count; cnt++) {
        String der;
        status = certChain[cnt].EncodeCertificateDER(der);
        if (ER_OK != status) {
            return status;
        }
        KeyBlob kb((const uint8_t*) der.data(), der.size(), KeyBlob::GENERIC);
        if (cnt == 0) {
            /* handle the leaf cert */
            String serialTag;
            if (certChain[cnt].GetSerialLen() > 0) {
                serialTag.assign(reinterpret_cast<const char*>(certChain[cnt].GetSerial()), certChain[cnt].GetSerialLen());
            }
            kb.SetTag(serialTag);

            /* store the Membership DER  into the key store */
            KeyStore::Key membershipHead;
            GetACLKey(ENTRY_MEMBERSHIPS, membershipHead);

            KeyBlob headerBlob;
            status = ca->GetKey(membershipHead, headerBlob);
            bool checkDup = true;
            if (status == ER_BUS_KEY_UNAVAILABLE) {
                /* create an empty header node */
                uint8_t numEntries = 1;
                headerBlob.Set(&numEntries, 1, KeyBlob::GENERIC);
                status = ca->StoreKey(membershipHead, headerBlob);
                if (ER_OK != status) {
                    return status;
                }
                checkDup = false;
            }
            /* check for duplicate */
            if (checkDup) {
                KeyStore::Key tmpKey;
                status = GetMembershipKey(ca, membershipHead, serialTag, certChain[cnt].GetAuthorityKeyId(), tmpKey);
                if (ER_OK == status) {
                    /* found a duplicate */
                    return ER_DUPLICATE_CERTIFICATE;
                }
            }
            /* add the membership cert as an associate entry to the membership list header node */
            status = ca->AddAssociatedKey(membershipHead, membershipKey, kb);
        } else {
            /* add the cert chain data as an associate of the membership entry */
            GUID128 guid;
            KeyStore::Key key(KeyStore::Key::LOCAL, guid);
            status = ca->AddAssociatedKey(membershipKey, key, kb);
            membershipKey = key;
        }
    }
    return status;
}

QStatus PermissionMgmtObj::StoreMembership(const MsgArg& msgArg)
{
    size_t certChainCount;
    MsgArg* certChain;
    QStatus status = msgArg.Get("a(yay)", &certChainCount, &certChain);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallMembership failed to retrieve certificate chain status 0x%x", status));
        return status;
    }

    CertificateX509* certs = new CertificateX509[certChainCount];
    if (certs == NULL) {
        return ER_OUT_OF_MEMORY;
    }
    for (size_t cnt = 0; cnt < certChainCount; cnt++) {
        QStatus status = LoadX509CertFromMsgArg(certChain[cnt], certs[cnt]);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionMgmtObj::InstallMembership failed to retrieve certificate [%d] status 0x%x", (int) cnt, status));
            goto Exit;
        }
    }
    status = StoreMembership(certs, certChainCount);
Exit:
    delete [] certs;
    return status;
}

QStatus PermissionMgmtObj::LocateMembershipEntry(const String& serialNum, const String& issuerAki, KeyStore::Key& membershipKey)
{
    /* look for memberships head in the key store */
    KeyStore::Key membershipHead;
    GetACLKey(ENTRY_MEMBERSHIPS, membershipHead);

    KeyBlob headerBlob;
    QStatus status = ca->GetKey(membershipHead, headerBlob);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return status;
    }
    return GetMembershipKey(ca, membershipHead, serialNum, issuerAki, membershipKey);
}

void PermissionMgmtObj::RemoveMembership(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    uint8_t* serialVal;
    size_t serialLen;
    uint8_t* akiVal;
    size_t akiLen;
    uint8_t algorithm;
    uint8_t curve;
    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    QStatus status = msg->GetArg(0)->Get("(ayay(yyayay))", &serialLen, &serialVal, &akiLen, &akiVal, &algorithm, &curve, &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if (algorithm != qcc::SigInfo::ALGORITHM_ECDSA_SHA_256) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if (curve != qcc::Crypto_ECC::ECC_NIST_P256) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if ((xLen != qcc::ECC_COORDINATE_SZ) || (yLen != qcc::ECC_COORDINATE_SZ)) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    ECCPublicKey publicKey;
    publicKey.Import(xCoord, xLen, yCoord, yLen);
    KeyInfoNISTP256 issuerKeyInfo;
    issuerKeyInfo.SetPublicKey(&publicKey);
    String issuerAki;
    if (akiLen == 0) {
        /* calculate it */
        if (ER_OK != CertificateX509::GenerateAuthorityKeyId(issuerKeyInfo.GetPublicKey(), issuerAki)) {
            MethodReply(msg, status);
            return;
        }
    } else {
        issuerAki.assign((const char*) akiVal, akiLen);
    }
    issuerKeyInfo.SetKeyId((uint8_t*) issuerAki.data(), issuerAki.size());
    String serial((const char*) serialVal, serialLen);
    KeyStore::Key membershipKey;
    MembershipCertificate cert;
    KeyBlob kb;
    status = LocateMembershipEntry(serial, issuerAki, membershipKey);
    if (ER_OK != status) {
        goto Exit;
    }
    /* retrieve the cert to validate before delete */
    status = ca->GetKey(membershipKey, kb);
    if (ER_OK != status) {
        goto Exit;
    }
    status = LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
    if (ER_OK != status) {
        goto Exit;
    }
    /* verify the cert with issuer's public key */
    status = cert.Verify(issuerKeyInfo.GetPublicKey());
    if (ER_OK != status) {
        status = ER_CERTIFICATE_NOT_FOUND;
        goto Exit;
    }

    /* found it so delete it */
    status = ca->DeleteKey(membershipKey);
Exit:
    if (ER_BUS_KEY_UNAVAILABLE == status) {
        /* could not find it.  */
        status = ER_CERTIFICATE_NOT_FOUND;
    }
    MethodReply(msg, status);
}

void PermissionMgmtObj::StartManagement(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QStatus status = ER_OK;

    if (!CompareAndExchange(&managementStarted, false, true)) {
        status = ER_MANAGEMENT_ALREADY_STARTED;
    } else {
        bus.GetInternal().CallStartManagementCallback();
    }

    MethodReply(msg, status);
}

void PermissionMgmtObj::EndManagement(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QStatus status = ER_OK;

    if (!CompareAndExchange(&managementStarted, true, false)) {
        status = ER_MANAGEMENT_NOT_STARTED;
    } else {
        bus.GetInternal().CallEndManagementCallback();
    }

    MethodReply(msg, status);
}

QStatus PermissionMgmtObj::GetAllMembershipCerts(MembershipCertMap& certMap, bool loadCert)
{
    /* look for memberships head in the key store */
    KeyStore::Key membershipHead;
    GetACLKey(ENTRY_MEMBERSHIPS, membershipHead);

    KeyBlob headerBlob;
    QStatus status = ca->GetKey(membershipHead, headerBlob);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return ER_OK;  /* nothing to do */
    }
    KeyStore::Key* keys = NULL;
    size_t numOfKeys;
    status = ca->GetKeys(membershipHead, &keys, &numOfKeys);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::GetAllMembershipCerts failed to retrieve the list of membership certificates.  Status 0x%x", status));
        return status;
    }
    if (numOfKeys == 0) {
        return ER_OK;
    }
    for (size_t cnt = 0; cnt < numOfKeys; cnt++) {
        KeyBlob kb;
        status = ca->GetKey(keys[cnt], kb);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionMgmtObj::GetAllMembershipCerts error looking for membership certificate"));
            delete [] keys;
            return status;
        }
        MembershipCertificate* cert = NULL;
        if (loadCert) {
            cert = new MembershipCertificate();
            status = LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), *cert);
            if (ER_OK != status) {
                QCC_DbgPrintf(("PermissionMgmtObj::GetAllMembershipCerts error loading membership certificate"));
                delete cert;
                delete [] keys;
                return status;
            }
        }
        certMap[keys[cnt]] = cert;
    }
    delete [] keys;
    return ER_OK;
}

QStatus PermissionMgmtObj::GetAllMembershipCerts(MembershipCertMap& certMap)
{
    return GetAllMembershipCerts(certMap, true);
}

void PermissionMgmtObj::ClearMembershipCertMap(MembershipCertMap& certMap)
{
    for (MembershipCertMap::iterator it = certMap.begin(); it != certMap.end(); it++) {
        delete it->second;
    }
    certMap.clear();
}

static void ClearArgVector(std::vector<MsgArg*>& argList)
{
    for (std::vector<MsgArg*>::iterator it = argList.begin(); it != argList.end(); it++) {
        delete *it;
    }
    argList.clear();
}

/**
 * The membership meta data is stored as
 *               +-------------------+
 *               | Membership Header |
 *               +-------------------+
 *                        |
 *                   +-----------+
 *                   | leaf cert |
 *                   +-----------+
 *                        |
 *                +-----------------+
 *                | cert 1 in chain |
 *                +-----------------+
 *                        |
 *                +-----------------+
 *                | cert 2 in chain |
 *                +-----------------+
 *
 */


static QStatus GenerateSendMembershipForCertEntry(BusAttachment& bus, CredentialAccessor* ca, KeyStore::Key& entryKey, std::vector<MsgArg*>& argList)
{
    KeyStore::Key* keys = NULL;
    size_t numOfKeys;
    QStatus status = ca->GetKeys(entryKey, &keys, &numOfKeys);
    if (ER_OK != status) {
        delete [] keys;
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            return ER_OK; /* nothing to generate */
        }
        return status;
    }

    std::vector<MsgArg*> addOns;
    /* go through the associated entries of this keys */
    for (size_t cnt = 0; cnt < numOfKeys; cnt++) {
        KeyBlob kb;
        status = ca->GetKey(keys[cnt], kb);
        if (ER_OK != status) {
            break;
        }
        MsgArg* msgArg = new MsgArg("(yay)", CertificateX509::ENCODING_X509_DER,  kb.GetSize(), kb.GetData());
        msgArg->Stabilize();
        msgArg->SetOwnershipFlags(MsgArg::OwnsArgs, true);
        addOns.push_back(msgArg);

        /* this cert chain node may have a parent cert */
        MembershipCertificate chainCert;
        status = LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), chainCert);
        if (ER_OK != status) {
            goto Exit;
        }
        status = GenerateSendMembershipForCertEntry(bus, ca, keys[cnt], addOns);
        if (ER_OK != status) {
            goto Exit;
        }
    }
Exit:
    if (ER_OK == status) {
        argList.reserve(argList.size() + addOns.size());
        argList.insert(argList.end(), addOns.begin(), addOns.end());
        addOns.clear();
    } else {
        ClearArgVector(addOns);
    }
    delete [] keys;
    return status;
}

QStatus PermissionMgmtObj::GetMembershipSummaries(MsgArg& arg)
{
    MembershipCertMap certMap;
    QStatus status = GetAllMembershipCerts(certMap);
    if (ER_OK != status) {
        return status;
    }
    if (certMap.size() == 0) {
        arg.Set("a(ayay(yyayay))", 0, NULL);
        return ER_OK;
    }

    MsgArg* membershipArgs = new MsgArg[certMap.size()];
    size_t cnt = 0;
    KeyStore::Key* issuerKeys = NULL;
    for (MembershipCertMap::iterator it = certMap.begin(); it != certMap.end(); it++) {
        KeyInfoNISTP256 issuerKeyInfo;
        KeyStore::Key leafKey = it->first;
        MembershipCertificate* leafCert = it->second;
        issuerKeyInfo.SetKeyId((const uint8_t*) leafCert->GetAuthorityKeyId().data(), leafCert->GetAuthorityKeyId().size());
        delete [] issuerKeys;
        issuerKeys = NULL;
        size_t numOfIssuerKeys = 0;
        bool locateIssuer = false;
        status = ca->GetKeys(leafKey, &issuerKeys, &numOfIssuerKeys);
        if (ER_OK != status) {
            if (ER_BUS_KEY_UNAVAILABLE == status) {
                locateIssuer = true;
            } else {
                goto Exit;
            }
        } else {
            if (numOfIssuerKeys > 0) {
                KeyBlob kb;
                status = ca->GetKey(issuerKeys[0], kb);
                if (ER_OK != status) {
                    goto Exit;
                }
                MembershipCertificate issuerCert;
                status = LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), issuerCert);
                if (ER_OK != status) {
                    goto Exit;
                }
                issuerKeyInfo.SetPublicKey(issuerCert.GetSubjectPublicKey());
                if (issuerKeyInfo.GetKeyIdLen() == 0) {
                    String aki;
                    if (ER_OK != CertificateX509::GenerateAuthorityKeyId(issuerKeyInfo.GetPublicKey(), aki)) {
                        goto Exit;
                    }
                    issuerKeyInfo.SetKeyId((const uint8_t*) aki.data(), aki.size());
                }
            } else {
                locateIssuer = true;
            }
        }
        if (locateIssuer) {
            /* membership cert is a single cert.  Need to locate the issuer from
             * the list of trust anchors using the cert's aki.
             */
            TrustAnchorList anchors = LocateTrustAnchor(trustAnchors, leafCert->GetAuthorityKeyId());

            for (TrustAnchorList::const_iterator it = anchors.begin(); it != anchors.end(); it++) {
                if (ER_OK == leafCert->Verify((*it)->keyInfo.GetPublicKey())) {
                    issuerKeyInfo.SetPublicKey((*it)->keyInfo.GetPublicKey());
                    break;
                }
            }
            anchors.clear();
        }
        size_t coordSize = issuerKeyInfo.GetPublicKey()->GetCoordinateSize();
        uint8_t* xData = new uint8_t[coordSize];
        uint8_t* yData = new uint8_t[coordSize];
        KeyInfoHelper::ExportCoordinates(*issuerKeyInfo.GetPublicKey(), xData, coordSize, yData, coordSize);

        status = membershipArgs[cnt].Set("(ayay(yyayay))",
                                         leafCert->GetSerialLen(), leafCert->GetSerial(),
                                         issuerKeyInfo.GetKeyIdLen(), issuerKeyInfo.GetKeyId(),
                                         issuerKeyInfo.GetAlgorithm(), issuerKeyInfo.GetCurve(),
                                         coordSize, xData, coordSize, yData);

        membershipArgs[cnt].Stabilize();
        delete [] xData;
        delete [] yData;
        cnt++;
    }
Exit:
    delete [] issuerKeys;
    if (ER_OK == status) {
        arg.Set("a(ayay(yyayay))", certMap.size(), membershipArgs);
        arg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    } else {
        delete [] membershipArgs;
    }
    ClearMembershipCertMap(certMap);
    return status;
}

bool PermissionMgmtObj::IsRelevantMembershipCert(std::vector<MsgArg*>& membershipChain, std::vector<ECCPublicKey> peerIssuers)
{
    /*
     * A membership cert is considered relevant to this exchange if any certificate in the membership chain was issued by an issuer
     * in the peer's identity cert chain.  We have only public keys to compare from the identity cert chain, the certificates themselves
     * are not persisted as auth metadata.
     */

    QStatus status;
    CertificateX509 cert;

    if (membershipChain.size() == 0) {
        return false;
    }

    /* Compare membership chain to peerIssuers. */
    for (size_t i = 0; i < membershipChain.size(); i++) {
        status = LoadX509CertFromMsgArg(membershipChain[i][0], cert);
        if (status != ER_OK) {
            QCC_DbgPrintf(("PermissionMgmtObj::IsRelevantMembershipCert failed to load certificate in membership chain"));
            return false;
        }

        for (size_t j = 0; j < peerIssuers.size(); j++) {
            if (cert.IsSubjectPublicKeyEqual(&(peerIssuers[j]))) {
                return true;
            }
        }
    }

    /* Check if this chain ends in a root, if so we're done, if not, check the installed roots. */
    if (cert.IsIssuerOf(cert)) {
        return false;
    }

    TrustAnchorList anchors = LocateTrustAnchor(trustAnchors, cert.GetAuthorityKeyId());

    if (anchors.size() == 0) {
        QCC_DbgPrintf(("PermissionMgmtObj::IsRelevantMembershipCert: Membership certificate present, but no trust anchors are installed"));
        return false;
    }

    for (TrustAnchorList::const_iterator it = anchors.begin(); it != anchors.end(); it++) {
        for (size_t j = 0; j < peerIssuers.size(); j++) {
            if (*(*it)->keyInfo.GetPublicKey() == peerIssuers[j]) {
                return true;
            }
        }
    }

    return false;
}

QStatus PermissionMgmtObj::GenerateSendMemberships(std::vector<std::vector<MsgArg*> >& args, const qcc::GUID128& remotePeerGuid)
{
    MembershipCertMap certMap;
    ECCPublicKey peerPublicKey;
    std::vector<ECCPublicKey> peerIssuers;
    bool publicKeyFound = false;
    qcc::String authMechanism;

    QStatus status = GetAllMembershipCerts(certMap);
    if (ER_OK != status) {
        return status;
    }
    if (certMap.size() == 0) {
        return ER_OK;
    }

    status = this->GetConnectedPeerAuthMetadata(remotePeerGuid, authMechanism, publicKeyFound, &peerPublicKey, NULL, peerIssuers);
    if ((status != ER_OK) || !publicKeyFound) {
        goto Exit;
    }

    for (MembershipCertMap::iterator it = certMap.begin(); it != certMap.end(); it++) {
        std::vector<MsgArg*> argList;
        MembershipCertificate* cert = it->second;
        String der;
        status = cert->EncodeCertificateDER(der);
        if (ER_OK != status) {
            goto Exit;
        }
        MsgArg* msgArg = new MsgArg("(yay)", CertificateX509::ENCODING_X509_DER, der.length(), der.c_str());
        msgArg->Stabilize();
        msgArg->SetOwnershipFlags(MsgArg::OwnsArgs, true);
        argList.push_back(msgArg);

        status = GenerateSendMembershipForCertEntry(bus, ca, (KeyStore::Key&)it->first, argList);
        if (ER_OK != status) {
            ClearArgVector(argList);
            goto Exit;
        }

        /* Now that we have the membership cert chain, determine whether it's relevant for this protocol instance. */
        if (this->IsRelevantMembershipCert(argList, peerIssuers)) {
            args.push_back(argList);
        } else {
            ClearArgVector(argList);
        }
    }

Exit:
    ClearMembershipCertMap(certMap);
    if (ER_OK != status) {
        _PeerState::ClearGuildArgs(args);
    }
    return status;
}

QStatus PermissionMgmtObj::ParseSendManifest(Message& msg, PeerState& peerState)
{
    size_t count = 0;
    PermissionPolicy::Rule* rules = NULL;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    std::vector<ECCPublicKey> issuerKeys;
    QStatus status = GetManifestFromMessageArg(bus, *(msg->GetArg(0)), &rules, &count, digest);
    if (ER_OK != status) {
        goto DoneValidation;
    }
    if (count == 0) {
        status = ER_OK; /* nothing to do */
        goto DoneValidation;
    }
    /* retrieve the peer's manifest digest from keystore */
    uint8_t storedDigest[Crypto_SHA256::DIGEST_SIZE];
    status = GetConnectedPeerPublicKey(peerState->GetGuid(), NULL, storedDigest, issuerKeys);
    if (ER_OK != status) {
        status = ER_MISSING_DIGEST_IN_CERTIFICATE;
        goto DoneValidation;
    }

    /* compare the digests */
    if (memcmp(digest, storedDigest, Crypto_SHA256::DIGEST_SIZE)) {
        status = ER_DIGEST_MISMATCH;
        goto DoneValidation;
    }
    /* store the manifest in the peer state object */
    delete [] peerState->manifest;
    peerState->manifest = rules;
    peerState->manifestSize = count;
    return ER_OK;  /* done */

DoneValidation:
    delete [] rules;
    return status;
}

QStatus PermissionMgmtObj::ParseSendMemberships(Message& msg, bool& done)
{
    done = false;
    uint8_t sendCode;
    QStatus status = msg->GetArg(0)->Get("y", &sendCode);
    if (ER_OK != status) {
        return status;
    }
    if (sendCode == SEND_MEMBERSHIP_NONE) {
        /* the send informs that it does not have any membership cert */
        done = true;
        return ER_OK;
    }

    MsgArg* certArgs;
    size_t count;
    status = msg->GetArg(1)->Get("a(yay)", &count, &certArgs);
    if (ER_OK != status) {
        return status;
    }
    if (count == 0) {
        return ER_OK;
    }

    PeerState peerState =  bus.GetInternal().GetPeerStateTable()->GetPeerState(msg->GetSender());
    _PeerState::GuildMetadata* meta = NULL;
    for (size_t idx = 0; idx < count; idx++) {
        CertificateX509* cert = new CertificateX509();
        if (idx == 0) {
            /* leaf cert */
            meta = new _PeerState::GuildMetadata();
            status = LoadX509CertFromMsgArg(certArgs[idx], *cert);
            if (ER_OK != status) {
                delete meta;
                delete cert;
                return status;
            }
            meta->certChain.push_back(cert);
            String serialTag;
            if (cert->GetSerialLen() > 0) {
                serialTag.assign(reinterpret_cast<const char*>(cert->GetSerial()), cert->GetSerialLen());
            }
            peerState->SetGuildMetadata(serialTag, cert->GetAuthorityKeyId(), meta);
        } else {
            status = LoadX509CertFromMsgArg(certArgs[idx], *cert);
            if (ER_OK != status) {
                delete cert;
                return status;
            }
            meta->certChain.push_back(cert);
        }
    }

    if (sendCode == SEND_MEMBERSHIP_LAST) {
        /* do the membership cert validation for the peer */
        ECCPublicKey peerPublicKey;
        status = GetConnectedPeerPublicKey(peerState->GetGuid(), &peerPublicKey);
        if (ER_OK != status) {
            _PeerState::ClearGuildMap(peerState->guildMap);
            done = true;
            return ER_OK;  /* could not validate */
        }

        while (!peerState->guildMap.empty()) {
            bool verified = true;
            for (_PeerState::GuildMap::iterator it = peerState->guildMap.begin(); it != peerState->guildMap.end(); it++) {
                _PeerState::GuildMetadata* metadata = it->second;
                if (metadata->certChain.size() == 0) {
                    /* remove this entry since it has no certs */
                    peerState->guildMap.erase(it);
                    delete metadata;
                    verified = false;
                    break;
                }
                /* validate the membership leaf cert's subject public key to
                    match the peer's public key to make sure it sends its own
                    certs. */
                if (peerPublicKey != *(metadata->certChain[0]->GetSubjectPublicKey())) {
                    /* remove this membership cert since it is not valid */
                    peerState->guildMap.erase(it);
                    delete metadata;
                    verified = false;
                    break;
                }

                /* build the vector of certs to verify.  The membership cert is the leaf node -- first item on the vector */
                /* check membership cert chain against the security group trust anchors */
                status = ValidateMembershipCertificateChain(metadata->certChain, &trustAnchors);

                if (ER_OK != status) {
                    /* remove this membership cert since it is not valid */
                    QCC_DbgPrintf(("PermissionMgmtObj::ParseSendMemberships invalidated peer's membership guild thus removing it from peer's guild list"));
                    peerState->guildMap.erase(it);
                    delete metadata;
                    verified = false;
                    break;
                }
            }
            if (verified) {
                break;  /* done */
            }
        }
        done = true;
    }
    return ER_OK;
}

QStatus PermissionMgmtObj::StoreConfiguration(const Configuration& config)
{
    /* store the message into the key store */
    KeyStore::Key configKey;
    GetACLKey(ENTRY_CONFIGURATION, configKey);
    KeyBlob kb((uint8_t*) &config, sizeof(Configuration), KeyBlob::GENERIC);
    return ca->StoreKey(configKey, kb);
}

QStatus PermissionMgmtObj::GetConfiguration(Configuration& config)
{
    KeyBlob kb;
    KeyStore::Key key;
    GetACLKey(ENTRY_CONFIGURATION, key);
    QStatus status = ca->GetKey(key, kb);
    if (ER_BUS_KEY_UNAVAILABLE == status) {
        /* generate the default configuration */
        if (HasDefaultPolicy()) {
            config.applicationStateSet = 1;
            config.applicationState = (uint8_t) PermissionConfigurator::CLAIMED;
        } else {
            config.applicationStateSet = 0;
            config.applicationState = (uint8_t) PermissionConfigurator::NOT_CLAIMABLE;
        }
        config.claimCapabilities = claimCapabilities;
        config.claimCapabilityAdditionalInfo = claimCapabilityAdditionalInfo;
        return StoreConfiguration(config);
    }
    if (ER_OK != status) {
        return status;
    }
    if (kb.GetSize() != sizeof(Configuration)) {
        return ER_INVALID_DATA;
    }
    memcpy(&config, kb.GetData(), kb.GetSize());
    return ER_OK;
}

PermissionConfigurator::ApplicationState PermissionMgmtObj::GetApplicationState()
{
    return applicationState;
}

QStatus PermissionMgmtObj::StoreApplicationState()
{
    Configuration config;
    QStatus status = GetConfiguration(config);
    if (ER_OK != status) {
        return status;
    }
    config.applicationState = (uint8_t) applicationState;
    config.applicationStateSet = 1;
    return StoreConfiguration(config);
}

QStatus PermissionMgmtObj::SetApplicationState(PermissionConfigurator::ApplicationState newState)
{
    if ((newState == PermissionConfigurator::CLAIMABLE) && (applicationState == PermissionConfigurator::CLAIMED)) {
        return ER_INVALID_APPLICATION_STATE;
    }

    /* save the existing state */
    PermissionConfigurator::ApplicationState savedState = applicationState;
    applicationState = newState;
    QStatus status = StoreApplicationState();
    if (ER_OK == status) {
        StateChanged();
    } else {
        applicationState = savedState;
    }
    return status;
}

QStatus PermissionMgmtObj::PerformReset(bool keepForClaim)
{
    bus.GetInternal().GetKeyStore().Reload();
    KeyStore::Key key;
    QStatus status;

    ClearTrustAnchors();
    if (!keepForClaim) {
        ca->GetLocalKey(KeyBlob::DSA_PRIVATE, key);
        status = ca->DeleteKey(key);
        if (status != ER_OK) {
            return status;
        }
        ca->GetLocalKey(KeyBlob::DSA_PUBLIC, key);
        status = ca->DeleteKey(key);
        if (status != ER_OK) {
            return status;
        }
        GetACLKey(ENTRY_MANIFEST_TEMPLATE, key);
        status = ca->DeleteKey(key);
        if (ER_OK != status) {
            return status;
        }

    }
    ca->GetLocalKey(KeyBlob::PEM, key);
    status = ca->DeleteKey(key);
    if (status != ER_OK) {
        return status;
    }

    GetACLKey(ENTRY_IDENTITY, key);
    status = ca->DeleteKey(key);
    if (ER_OK != status) {
        return status;
    }
    GetACLKey(ENTRY_POLICY, key);
    status = ca->DeleteKey(key);
    if (ER_OK != status) {
        return status;
    }

    GetACLKey(ENTRY_DEFAULT_POLICY, key);
    status = ca->DeleteKey(key);
    if (ER_OK != status) {
        return status;
    }

    GetACLKey(ENTRY_MEMBERSHIPS, key);
    status = ca->DeleteKey(key);
    if (ER_OK != status) {
        return status;
    }

    GetACLKey(ENTRY_MANIFEST, key);
    status = ca->DeleteKey(key);
    if (ER_OK != status) {
        return status;
    }

    GetACLKey(ENTRY_CONFIGURATION, key);
    status = ca->DeleteKey(key);
    if (ER_OK != status) {
        return status;
    }

    /* clear out all the peer entries for all ECDHE key exchanges */
    bus.GetInternal().GetKeyStore().Clear(ECDHE_NAME_PREFIX_PATTERN);

    applicationState = PermissionConfigurator::CLAIMABLE;
    policyVersion = 0;
    return status;
}

QStatus PermissionMgmtObj::Reset()
{
    /* First call out to the application to give it a chance to reset
     * any of its own persisted state, to avoid it leaking out to others.
     */
    QStatus status = bus.GetInternal().CallFactoryResetCallback();
    if (ER_OK != status) {
        return status;
    }

    /* Reset the security configuration. */
    status = PerformReset(true);
    if (ER_OK == status) {
        PolicyChanged(NULL);
        StateChanged();
    }
    return status;
}

void PermissionMgmtObj::Reset(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    MethodReply(msg, Reset());
}

QStatus PermissionMgmtObj::GetConnectedPeerAuthMetadata(const GUID128& guid, qcc::String& authMechanism, bool& publicKeyFound, qcc::ECCPublicKey* publicKey, uint8_t* manifestDigest, std::vector<ECCPublicKey>& issuerPublicKeys)
{
    CredentialAccessor ca(bus);
    KeyBlob kb;
    KeyStore::Key key(KeyStore::Key::REMOTE, guid);
    QStatus status = ca.GetKey(key, kb);
    if (ER_OK != status) {
        return status;
    }
    KeyBlob msBlob;
    publicKeyFound = false;
    status = KeyExchanger::ParsePeerSecretRecord(kb, msBlob, publicKey, manifestDigest, issuerPublicKeys, publicKeyFound);
    if (ER_OK != status) {
        return status;
    }
    authMechanism = msBlob.GetTag();
    return status;
}

QStatus PermissionMgmtObj::GetConnectedPeerPublicKey(const GUID128& guid, qcc::ECCPublicKey* publicKey, uint8_t* manifestDigest, std::vector<ECCPublicKey>& issuerPublicKeys)
{
    bool publicKeyFound = false;
    qcc::String authMechanism;
    QStatus status = GetConnectedPeerAuthMetadata(guid, authMechanism, publicKeyFound, publicKey, manifestDigest, issuerPublicKeys);
    if (ER_OK != status) {
        return status;
    }
    if (!publicKeyFound) {
        status = ER_BUS_KEY_UNAVAILABLE;
    }
    return status;
}

QStatus PermissionMgmtObj::GetConnectedPeerPublicKey(const GUID128& guid, qcc::ECCPublicKey* publicKey)
{
    std::vector<ECCPublicKey> issuerPublicKeys;
    return GetConnectedPeerPublicKey(guid, publicKey, NULL, issuerPublicKeys);
}

QStatus PermissionMgmtObj::SetManifestTemplate(const PermissionPolicy::Rule* rules, size_t count)
{
    if (count == 0) {
        return ER_OK;
    }
    PermissionPolicy policy;
    PermissionPolicy::Acl acls[1];
    PermissionPolicy::Rule* localRules = NULL;
    if (count > 0) {
        localRules = new PermissionPolicy::Rule[count];
        for (size_t cnt = 0; cnt < count; cnt++) {
            localRules[cnt] = rules[cnt];
        }
    }
    acls[0].SetRules(count, localRules);
    policy.SetAcls(1, acls);

    delete [] localRules;
    localRules = NULL;

    uint8_t* buf = NULL;
    size_t size;
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    QStatus status = policy.Export(marshaller, &buf, &size);
    if (ER_OK != status) {
        return status;
    }
    /* store the message into the key store */
    KeyStore::Key key;
    GetACLKey(ENTRY_MANIFEST_TEMPLATE, key);
    KeyBlob kb((uint8_t*) buf, size, KeyBlob::GENERIC);
    delete [] buf;

    status = ca->StoreKey(key, kb);
    if (ER_OK == status) {
        if (applicationState == PermissionConfigurator::NOT_CLAIMABLE) {
            /* Setting the manifest template triggers changing the application
             * state from the default value of not claimable to claimable
             * unless it has been intentionally set to be not claimable.
             */
            bool setClaimable = false;
            Configuration config;
            QStatus aStatus = GetConfiguration(config);
            if (ER_OK == aStatus) {
                if (!config.applicationStateSet) {
                    setClaimable = true;
                }
            }
            if (setClaimable) {
                applicationState = PermissionConfigurator::CLAIMABLE;
                StoreApplicationState();
                StateChanged();
            }
        }
    }
    return status;
}

QStatus PermissionMgmtObj::RetrieveManifest(PermissionPolicy::Rule* manifest, size_t* count)
{
    KeyBlob kb;
    KeyStore::Key key;
    GetACLKey(ENTRY_MANIFEST, key);
    QStatus status = ca->GetKey(key, kb);
    if (ER_OK != status) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            status = ER_MANIFEST_NOT_FOUND;
        }
        return status;
    }
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    PermissionPolicy policy;
    status = policy.Import(marshaller, kb.GetData(), kb.GetSize());
    if (ER_OK != status) {
        return status;
    }
    if (policy.GetAclsSize() == 0) {
        return ER_MANIFEST_NOT_FOUND;
    }
    const PermissionPolicy::Acl* acls = policy.GetAcls();
    if (manifest == NULL) {
        *count = acls[0].GetRulesSize();
    } else {
        if (*count < acls[0].GetRulesSize()) {
            status = ER_BUFFER_TOO_SMALL;
        } else {
            *count = acls[0].GetRulesSize();
        }
        for (size_t i = 0; i < *count; ++i) {
            manifest[i] = acls[0].GetRules()[i];
        }
    }
    return status;
}

QStatus PermissionMgmtObj::LoadManifestTemplate(PermissionPolicy& policy)
{
    KeyBlob kb;
    KeyStore::Key key;
    GetACLKey(ENTRY_MANIFEST_TEMPLATE, key);
    QStatus status = ca->GetKey(key, kb);
    if (ER_OK != status) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            return ER_MANIFEST_NOT_FOUND;
        }
        return status;
    }
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    status = policy.Import(marshaller, kb.GetData(), kb.GetSize());
    if (ER_OK != status) {
        return status;
    }
    if (policy.GetAclsSize() == 0) {
        return ER_MANIFEST_NOT_FOUND;
    }
    return ER_OK;
}

QStatus PermissionMgmtObj::LookForManifestTemplate(bool& exist)
{
    exist = false;
    KeyBlob kb;
    KeyStore::Key key;
    GetACLKey(ENTRY_MANIFEST_TEMPLATE, key);
    QStatus status = ca->GetKey(key, kb);
    if (ER_OK == status) {
        exist = true;
    } else if (ER_BUS_KEY_UNAVAILABLE == status) {
        status = ER_OK;
    }
    return status;
}

QStatus PermissionMgmtObj::GetManifestTemplate(MsgArg& arg)
{
    PermissionPolicy policy;
    QStatus status = LoadManifestTemplate(policy);
    if (ER_OK != status) {
        return status;
    }
    PermissionPolicy::Acl* acls = (PermissionPolicy::Acl*) policy.GetAcls();
    return PermissionPolicy::GenerateRules(acls[0].GetRules(), acls[0].GetRulesSize(), arg);
}

QStatus PermissionMgmtObj::GetManifestTemplateDigest(MsgArg& arg)
{
    PermissionPolicy policy;
    QStatus status = LoadManifestTemplate(policy);
    if (ER_OK != status) {
        return status;
    }
    PermissionPolicy::Acl* acls = (PermissionPolicy::Acl*) policy.GetAcls();
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    status = GenerateManifestDigest(bus, acls[0].GetRules(), acls[0].GetRulesSize(), digest, Crypto_SHA256::DIGEST_SIZE);
    if (ER_OK != status) {
        return status;
    }
    status = arg.Set("(yay)", qcc::SigInfo::ALGORITHM_ECDSA_SHA_256, Crypto_SHA256::DIGEST_SIZE, digest);
    if (ER_OK != status) {
        return status;
    }
    arg.Stabilize();
    return status;
}

bool PermissionMgmtObj::ValidateCertChain(bool verifyIssuerChain, bool validateTrust, const qcc::CertificateX509* certChain, size_t count, bool enforceAKI)
{
    if (verifyIssuerChain) {
        if (!KeyExchangerECDHE_ECDSA::IsCertChainStructureValid(certChain, count)) {
            return false;
        }
    }
    /* check for AKI existence */
    if (enforceAKI) {
        if (!ValidateAKIInCertChain(certChain, count)) {
            return false;
        }
    }
    bool valid = false;
    if (validateTrust) {
        /* go through the cert chain to see whether any of the issuer is the trust anchor */
        if (count == 1) {
            /* single cert is exchanged */
            /* locate the issuer */
            if (certChain[0].GetAuthorityKeyId().empty()) {
                if (IsTrustAnchor(certChain[0].GetSubjectPublicKey())) {
                    valid = (ER_OK == certChain[0].Verify(certChain[0].GetSubjectPublicKey()));
                }
            } else {
                TrustAnchorList anchors = LocateTrustAnchor(trustAnchors, certChain[0].GetAuthorityKeyId());

                if (!anchors.empty()) {
                    for (TrustAnchorList::const_iterator it = anchors.begin(); it != anchors.end(); it++) {
                        valid = (ER_OK == certChain[0].Verify((*it)->keyInfo.GetPublicKey()));
                        if (valid) {
                            break;
                        }
                    }
                    anchors.clear();
                } /* there is a trust anchor with given authority key id */
            }
        } else {
            /* the cert chain signature validation is already done by the KeyExchanger code.  Now just need to check issuers */
            for (size_t cnt = 1; cnt < count; cnt++) {
                if (IsTrustAnchor(certChain[cnt].GetSubjectPublicKey())) {
                    valid = true;
                    break;
                }
            }
        }
    } else {
        valid = true;
    }

    if (valid) {
        /* check the certificate type in the chain */
        valid = CertificateX509::ValidateCertificateTypeInCertChain(certChain, count);
    }
    return valid;
}

bool PermissionMgmtObj::ValidateCertChainPEM(const qcc::String& certChainPEM, bool& authorized, bool enforceAKI)
{
    /* get the trust anchor public key */
    bool handled = false;
    authorized = false;
    if (!HasTrustAnchors()) {
        /* there is no trust anchor to check.  So report as unhandled */
        return handled;
    }
    handled = true;

    /* parse the PEM to retrieve the cert chain */

    size_t count = 0;
    QStatus status = CertificateHelper::GetCertCount(certChainPEM, &count);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("PermissionMgmtObj::ValidateCertChain has error counting certs in the PEM"));
        return handled;
    }
    if (count == 0) {
        return handled;
    }
    CertificateX509* certChain = new CertificateX509[count];
    status = CertificateX509::DecodeCertChainPEM(certChainPEM, certChain, count);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("PermissionMgmtObj::ValidateCertChain has error loading certs in the PEM"));
        delete [] certChain;
        return handled;
    }
    authorized = ValidateCertChain(false, true, certChain, count, enforceAKI);
    delete [] certChain;
    return handled;
}

bool PermissionMgmtObj::KeyExchangeListener::RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials)
{
    if (strcmp("ALLJOYN_ECDHE_ECDSA", authMechanism) == 0) {
        /* Use the installed identity certificate instead of asking the application */
        qcc::String pem;
        QStatus status = pmo->RetrieveIdentityCertChainPEM(pem);
        if ((ER_OK == status) && (pem.size() > 0)) {
            bool handled = true;
            /* handle private key */
            if ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                qcc::ECCPrivateKey pk;
                if (ER_OK != pmo->GetDSAPrivateKey(pk)) {
                    handled = false;
                }
                String pem;
                CertificateX509::EncodePrivateKeyPEM(&pk, pem);
                credentials.SetPrivateKey(pem);
            }
            /* build the cert chain based on the identity cert */
            if ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                credentials.SetCertChain(pem);
            }
            if (handled) {
                return true;
            }
        }
    }
    return ProtectedAuthListener::RequestCredentials(authMechanism, peerName, authCount, userName, credMask, credentials);
}

bool PermissionMgmtObj::KeyExchangeListener::VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials)
{
    if (strcmp("ALLJOYN_ECDHE_ECDSA", authMechanism) == 0) {
        qcc::String certChainPEM = credentials.GetCertChain();
        if (certChainPEM.empty()) {
            return false;
        }
        bool authorized = false;
        bool enforceAKI = false; /* prior to security 2.0 app may have not AKI */
        bool handled = pmo->ValidateCertChainPEM(certChainPEM, authorized, enforceAKI);
        if (handled) {
            return authorized;
        }
    }
    return ProtectedAuthListener::VerifyCredentials(authMechanism, peerName, credentials);
}

QStatus PermissionMgmtObj::BindPort()
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort sessionPort = ALLJOYN_SESSIONPORT_PERMISSION_MGMT;
    if (portListener) {
        if (ER_OK == bus.UnbindSessionPort(sessionPort)) {
            delete portListener;
            portListener = NULL;
        }
    }

    if (portListener == NULL) {
        portListener = new PortListener();
    }
    QStatus status = bus.BindSessionPort(sessionPort, opts, *portListener);
    if (ER_OK != status) {
        delete portListener;
        portListener = NULL;
    }
    return status;
}

QStatus PermissionMgmtObj::ManageTrustAnchors(PermissionPolicy* policy)
{
    if (policy == NULL) {
        return ER_OK;
    }
    ClearTrustAnchors();
    LoadSGAuthoritiesAndCAs(*policy, trustAnchors);
    return ER_OK;
}

QStatus PermissionMgmtObj::SetClaimCapabilities(PermissionConfigurator::ClaimCapabilities caps)
{
    Configuration config;
    QStatus status = GetConfiguration(config);
    if (ER_OK != status) {
        return status;
    }
    config.claimCapabilities = caps;
    status = StoreConfiguration(config);
    if (ER_OK != status) {
        return status;
    }
    claimCapabilities = config.claimCapabilities;
    return ER_OK;
}

QStatus PermissionMgmtObj::SetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& additionalInfo)
{
    Configuration config;
    QStatus status = GetConfiguration(config);
    if (ER_OK != status) {
        return status;
    }
    config.claimCapabilityAdditionalInfo = additionalInfo;
    status = StoreConfiguration(config);
    if (ER_OK != status) {
        return status;
    }
    claimCapabilityAdditionalInfo = config.claimCapabilityAdditionalInfo;
    return ER_OK;
}

QStatus PermissionMgmtObj::GetClaimCapabilities(PermissionConfigurator::ClaimCapabilities& caps)
{
    caps = claimCapabilities;
    return ER_OK;
}

QStatus PermissionMgmtObj::GetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& additionalInfo)
{
    additionalInfo = claimCapabilityAdditionalInfo;
    return ER_OK;
}

bool PermissionMgmtObj::HasTrustAnchors()
{
    trustAnchors.Lock(MUTEX_CONTEXT);
    bool hasTrustAnchors = !trustAnchors.empty();
    trustAnchors.Unlock(MUTEX_CONTEXT);
    return hasTrustAnchors;
}

} /* namespace ajn */

