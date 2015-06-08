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
#include "PermissionMgmtObj.h"
#include "PeerState.h"
#include "BusInternal.h"
#include "KeyInfoHelper.h"
#include "KeyExchanger.h"

#define QCC_MODULE "PERMISSION_MGMT"

/**
 * PermissionMgmt interface version number
 */
#define VERSION_NUM  1

/**
 * The manifest type ID
 */

#define MANIFEST_TYPE_ALLJOYN 1

/**
 * The ALLJOYN ECDHE auth mechanism name prefix pattern
 */
#define ECDHE_NAME_PREFIX_PATTERN "ALLJOYN_ECDHE*"

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

static QStatus RetrieveAndGenDSAPublicKey(CredentialAccessor* ca, KeyInfoNISTP256& keyInfo);

PermissionMgmtObj::PermissionMgmtObj(BusAttachment& bus, const char* objectPath) :
    BusObject(objectPath),
    bus(bus), portListener(NULL)
{
}

PermissionMgmtObj::PermissionMgmtObj(BusAttachment& bus) :
    BusObject(org::allseen::Security::PermissionMgmt::ObjectPath, false),
    bus(bus), portListener(NULL)
{
}

QStatus PermissionMgmtObj::Init()
{
    /** TODO: Removing the PermissionMgmt interface once the functions are
     * migrated to the new Security interfaces.
     */
    /* Add org.allseen.Security.PermissionMgmt interface */
    const InterfaceDescription* ifc = bus.GetInterface(org::allseen::Security::PermissionMgmt::InterfaceName);
    if (ifc) {
        AddInterface(*ifc);
        AddMethodHandler(ifc->GetMember("GetManifest"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::GetManifestTemplate));
    }
    ca = new CredentialAccessor(bus);
    bus.GetInternal().GetPermissionManager().SetPermissionMgmtObj(this);
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
    } else {
        if (ER_OK == LoadTrustAnchors()) {
            applicationState = PermissionConfigurator::CLAIMED;
        } else {
            applicationState = PermissionConfigurator::CLAIMABLE;
        }
        StoreApplicationState();
    }
    /* notify others */
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
}

void PermissionMgmtObj::PolicyChanged(PermissionPolicy* policy)
{
    qcc::GUID128 localGUID;
    ca->GetGuid(localGUID);
    bus.GetInternal().GetPermissionManager().SetPolicy(policy);
    ManageMembershipTrustAnchors(policy);
    StateChanged();
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
    if (aclEntryType == ENTRY_TRUST_ANCHOR) {
        key.SetGUID(GUID128(qcc::String("E866F6C2CB5C005256F2944A042C0758")));
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
    if (ER_PERMISSION_DENIED == status) {
        return BusObject::MethodReply(msg, ERROR_PERMISSION_DENIED);
    } else if (ER_INVALID_CERTIFICATE == status) {
        return BusObject::MethodReply(msg, ERROR_INVALID_CERTIFICATE);
    } else if (ER_INVALID_CERTIFICATE_USAGE == status) {
        return BusObject::MethodReply(msg, ERROR_INVALID_CERTIFICATE_USAGE);
    } else if (ER_DUPLICATE_CERTIFICATE == status) {
        return BusObject::MethodReply(msg, ERROR_DUPLICATE_CERTIFICATE);
    } else if (ER_DIGEST_MISMATCH == status) {
        return BusObject::MethodReply(msg, ERROR_DIGEST_MISMATCH);
    } else if (ER_POLICY_NOT_NEWER == status) {
        return BusObject::MethodReply(msg, ERROR_POLICY_NOT_NEWER);
    } else {
        return BusObject::MethodReply(msg, status);
    }
}

static bool CanBeCAForIdentity(PermissionMgmtObj::TrustAnchorType taType)
{
    if (taType == PermissionMgmtObj::TRUST_ANCHOR_CA) {
        return true;
    }
    if (taType == PermissionMgmtObj::TRUST_ANCHOR_SG_AUTHORITY) {
        return true;
    }
    if (taType == PermissionMgmtObj::TRUST_ANCHOR_RESTRICTED_CA) {
        return true;
    }
    return false;
}

/* Get the admin group authority information from the list of trust anchors. */
static QStatus GetAdminGroupAuthority(const PermissionMgmtObj::TrustAnchorList& trustAnchors, GUID128& adminGroupGUID, KeyInfoNISTP256& adminGroupAuthority)
{
    for (PermissionMgmtObj::TrustAnchorList::const_iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if ((*it)->use == PermissionMgmtObj::TRUST_ANCHOR_ADMIN_GROUP) {
            adminGroupGUID = (*it)->securityGroupId;
            adminGroupAuthority = (*it)->keyInfo;
            return ER_OK;
        }
    }
    return ER_FAIL;
}
static PermissionMgmtObj::TrustAnchorList LocateTrustAnchor(PermissionMgmtObj::TrustAnchorList& trustAnchors, const qcc::String& aki)
{
    PermissionMgmtObj::TrustAnchorList retList;
    for (PermissionMgmtObj::TrustAnchorList::const_iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if (!CanBeCAForIdentity((*it)->use)) {
            continue;
        }
        if ((aki.size() == (*it)->keyInfo.GetKeyIdLen()) &&
            (memcmp(aki.data(), (*it)->keyInfo.GetKeyId(), aki.size()) == 0)) {
            retList.push_back(*it);
        }
    }
    return retList;
}

bool PermissionMgmtObj::IsTrustAnchor(const ECCPublicKey* publicKey)
{
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if (CanBeCAForIdentity((*it)->use) &&
            (memcmp((*it)->keyInfo.GetPublicKey(), publicKey, sizeof(ECCPublicKey)) == 0)) {
            return true;
        }
    }
    return false;
}

bool PermissionMgmtObj::IsTrustAnchor(TrustAnchorType taType, const ECCPublicKey* publicKey)
{
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if ((taType == (*it)->use) &&
            (memcmp((*it)->keyInfo.GetPublicKey(), publicKey, sizeof(ECCPublicKey)) == 0)) {
            return true;
        }
    }
    return false;
}

bool PermissionMgmtObj::IsAdmin(const ECCPublicKey* publicKey)
{
    return IsTrustAnchor(TRUST_ANCHOR_ADMIN_GROUP, publicKey);
}

bool PermissionMgmtObj::IsAdminGroup(const std::vector<MembershipCertificate*> certChain)
{
    for (std::vector<MembershipCertificate*>::const_iterator certIt = certChain.begin(); certIt != certChain.end(); certIt++) {
        for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
            if (((*it)->use == TRUST_ANCHOR_ADMIN_GROUP) &&
                ((*it)->securityGroupId == (*certIt)->GetGuild())) {
                /* match security group ID */
                if ((*certIt)->GetAuthorityKeyId().size() > 0) {
                    /* cert has aki */
                    if (((*it)->keyInfo.GetKeyIdLen() == (*certIt)->GetAuthorityKeyId().size()) &&
                        (memcmp((*it)->keyInfo.GetKeyId(), (*certIt)->GetAuthorityKeyId().data(), (*it)->keyInfo.GetKeyIdLen()) == 0)) {
                        /* same aki.  Verify the cert using trust anchor */
                        if (ER_OK == (*certIt)->Verify((*it)->keyInfo.GetPublicKey())) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}


void PermissionMgmtObj::ClearTrustAnchorList(TrustAnchorList& list)
{
    for (TrustAnchorList::iterator it = list.begin(); it != list.end(); it++) {
        delete *it;
    }
    list.clear();
}

/**
 * remove all occurences of trust anchor with given use type
 */
static void ClearTrustAnchorListByUse(PermissionMgmtObj::TrustAnchorType use, PermissionMgmtObj::TrustAnchorList& list)
{
    PermissionMgmtObj::TrustAnchorList::iterator it = list.begin();
    while (it != list.end()) {
        if ((*it)->use == use) {
            delete *it;
            it = list.erase(it);
        } else {
            it++;
        }
    }
}

static void LoadSGAuthoritiesAndRestrictedCAs(bool getSGAuthoritiesOnly, const PermissionPolicy& policy, PermissionMgmtObj::TrustAnchorList& taList)
{
    const PermissionPolicy::Acl* acls = policy.GetAcls();
    for (size_t cnt = 0; cnt < policy.GetAclsSize(); cnt++) {
        const PermissionPolicy::Peer* peers = acls[cnt].GetPeers();
        for (size_t idx = 0; idx < acls[cnt].GetPeersSize(); idx++) {
            bool typeMatched = false;
            if (peers[idx].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
                typeMatched = true;
            } else if (!getSGAuthoritiesOnly && (peers[idx].GetType() == PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY)) {
                typeMatched = true;
            }
            if (typeMatched && peers[idx].GetKeyInfo()) {
                if (KeyInfoHelper::InstanceOfKeyInfoNISTP256(*peers[idx].GetKeyInfo())) {
                    PermissionMgmtObj::TrustAnchor* ta = new PermissionMgmtObj::TrustAnchor((peers[idx].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP ? PermissionMgmtObj::TRUST_ANCHOR_SG_AUTHORITY : PermissionMgmtObj::TRUST_ANCHOR_RESTRICTED_CA), *(KeyInfoNISTP256*) peers[idx].GetKeyInfo());
                    if (ta->keyInfo.GetKeyIdLen() == 0) {
                        KeyInfoHelper::GenerateKeyId(ta->keyInfo);
                    }
                    taList.push_back(ta);
                }
            }
        }
    }
}

static void LoadSecurityGroupAuthorities(const PermissionPolicy& policy, PermissionMgmtObj::TrustAnchorList& taList)
{
    LoadSGAuthoritiesAndRestrictedCAs(true, policy, taList);
}

void PermissionMgmtObj::ClearTrustAnchors()
{
    ClearTrustAnchorList(trustAnchors);
}

QStatus PermissionMgmtObj::InstallTrustAnchor(TrustAnchor* trustAnchor)
{
    LoadTrustAnchors();
    /* check for duplicate trust anchor */
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if ((*it)->use != trustAnchor->use) {
            continue;
        }
        if (memcmp((*it)->keyInfo.GetPublicKey(), trustAnchor->keyInfo.GetPublicKey(), sizeof(ECCPublicKey)) == 0) {
            return ER_DUPLICATE_KEY;  /* duplicate */
        }
    }
    /* set the authority key identifier if it not already set */
    if (trustAnchor->keyInfo.GetKeyIdLen() == 0) {
        KeyInfoHelper::GenerateKeyId(trustAnchor->keyInfo);
    }
    TrustAnchor* anchor = new TrustAnchor(*trustAnchor);
    trustAnchors.push_back(anchor);
    return StoreTrustAnchors();
}

QStatus PermissionMgmtObj::StoreTrustAnchors()
{
    QCC_DbgPrintf(("PermissionMgmtObj::StoreTrustAnchors to keystore (guid %s)\n",
                   bus.GetInternal().GetKeyStore().GetGuid().c_str()));
    /* the format of the persistent buffer:
     * count
     * size:trust anchor
     * size:trust anchor
     * ...
     */
    /* calculate the buffer size */
    size_t bufferSize = sizeof(uint8_t);  /* the number of trust anchors */
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        bufferSize += sizeof(uint32_t);  /* account for the item size */
        bufferSize += sizeof(uint8_t);  /* account for the use field */
        bufferSize += (*it)->keyInfo.GetExportSize();
    }
    uint8_t* buffer = new uint8_t[bufferSize];
    uint8_t* pBuf = buffer;
    /* the count field */
    *pBuf = (uint8_t) trustAnchors.size();
    pBuf += sizeof(uint8_t);
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        uint32_t* itemSize = (uint32_t*) pBuf;
        *itemSize = (uint32_t) (sizeof(uint8_t) + (*it)->keyInfo.GetExportSize());
        pBuf += sizeof(uint32_t);
        *pBuf = (uint8_t) (*it)->use;
        pBuf++;
        (*it)->keyInfo.Export(pBuf);
        pBuf += (*itemSize - 1);
    }
    KeyStore::Key trustAnchorKey;
    GetACLKey(ENTRY_TRUST_ANCHOR, trustAnchorKey);
    KeyBlob kb(buffer, bufferSize, KeyBlob::GENERIC);
    kb.SetExpiration(0xFFFFFFFF);  /* never expired */

    QStatus status = ca->StoreKey(trustAnchorKey, kb);
    delete [] buffer;
    return status;
}

QStatus PermissionMgmtObj::LoadTrustAnchors()
{
    QCC_DbgPrintf(("PermissionMgmtObj::LoadTrustAnchors from keystore (guid %s)\n",
                   bus.GetInternal().GetKeyStore().GetGuid().c_str()));
    KeyStore::Key trustAnchorKey;
    GetACLKey(ENTRY_TRUST_ANCHOR, trustAnchorKey);
    KeyBlob kb;
    QStatus status = ca->GetKey(trustAnchorKey, kb);
    if (ER_OK != status) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            return ER_NO_TRUST_ANCHOR;
        }
        return status;
    }
    /* the format of the persistent buffer:
     * count
     * size:trust anchor
     * size:trust anchor
     * ...
     */

    ClearTrustAnchors();

    if (kb.GetSize() == 0) {
        return ER_NO_TRUST_ANCHOR; /* no trust anchor */
    }
    uint8_t* pBuf = (uint8_t*) kb.GetData();
    uint8_t count = *pBuf;
    size_t bytesRead = sizeof(uint8_t);
    if (bytesRead > kb.GetSize()) {
        ClearTrustAnchors();
        return ER_NO_TRUST_ANCHOR; /* no trust anchor */
    }
    pBuf += sizeof(uint8_t);
    for (size_t cnt = 0; cnt < count; cnt++) {
        uint32_t itemSize = *(uint32_t*) pBuf;
        bytesRead += sizeof(uint32_t);
        if (bytesRead > kb.GetSize()) {
            ClearTrustAnchors();
            return ER_NO_TRUST_ANCHOR; /* no trust anchor */
        }
        pBuf += sizeof(uint32_t);
        bytesRead += itemSize;
        if (bytesRead > kb.GetSize()) {
            ClearTrustAnchors();
            return ER_NO_TRUST_ANCHOR; /* no trust anchor */
        }
        TrustAnchor* ta = new TrustAnchor();
        ta->use = (TrustAnchorType) * pBuf;
        pBuf++;
        itemSize--;
        ta->keyInfo.Import(pBuf, itemSize);
        trustAnchors.push_back(ta);
        pBuf += itemSize;
    }
    return ER_OK;
}

QStatus PermissionMgmtObj::RemoveTrustAnchor(TrustAnchor* trustAnchor)
{
    LoadTrustAnchors();
    bool dirty = false;
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if (((*it)->use == trustAnchor->use) && (memcmp((*it)->keyInfo.GetPublicKey(), trustAnchor->keyInfo.GetPublicKey(), sizeof(ECCPublicKey)) == 0)) {
            delete *it;
            trustAnchors.erase(it);
            dirty = true;
            break;
        }
    }
    QStatus status = ER_OK;
    if (dirty) {
        status = StoreTrustAnchors();
    }
    return status;
}

QStatus PermissionMgmtObj::GetPeerGUID(Message& msg, qcc::GUID128& guid)
{
    PeerStateTable* peerTable = bus.GetInternal().GetPeerStateTable();
    qcc::String peerName = msg->GetSender();
    if (peerTable->IsKnownPeer(peerName)) {
        guid = peerTable->GetPeerState(peerName)->GetGuid();
        return ER_OK;
    } else {
        return ER_BUS_NO_PEER_GUID;
    }
}

QStatus PermissionMgmtObj::StoreDSAKeys(CredentialAccessor* ca, const ECCPrivateKey* privateKey, const ECCPublicKey* publicKey)
{

    KeyBlob dsaPrivKb((const uint8_t*) privateKey, sizeof(ECCPrivateKey), KeyBlob::DSA_PRIVATE);
    KeyStore::Key key;
    ca->GetLocalKey(KeyBlob::DSA_PRIVATE, key);
    QStatus status = ca->StoreKey(key, dsaPrivKb);
    if (status != ER_OK) {
        return status;
    }

    KeyBlob dsaPubKb((const uint8_t*) publicKey, sizeof(ECCPublicKey), KeyBlob::DSA_PUBLIC);
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
 * @param adminGroupGUID the admin group GUID
 * @param adminGroupAuthority the admin group authority
 * @param localPublicKey the local public key
 * @param[out] policy the permission policy
 */

static void GenerateDefaultPolicy(const GUID128& adminGroupGUID, const KeyInfoNISTP256& adminGroupAuthority, const ECCPublicKey* localPublicKey, PermissionPolicy& policy)
{
    policy.SetVersion(0);

    /* add the acls section */
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[3];

    /* acls record 0  ADMIN GROUP */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(adminGroupGUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(adminGroupAuthority.GetKeyId(), adminGroupAuthority.GetKeyIdLen());
    keyInfo->SetPublicKey(adminGroupAuthority.GetPublicKey());
    peers[0].SetKeyInfo(keyInfo);
    acls[0].SetPeers(1, peers);

    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_PROVIDE |
        PermissionPolicy::Rule::Member::ACTION_OBSERVE |
        PermissionPolicy::Rule::Member::ACTION_MODIFY
        );
    rules[0].SetMembers(1, prms);
    acls[0].SetRules(1, rules);

    /* acls record 1  LOCAL PUBLIC KEY */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetPublicKey(localPublicKey);
    KeyInfoHelper::GenerateKeyId(*keyInfo);
    peers[0].SetKeyInfo(keyInfo);
    acls[1].SetPeers(1, peers);

    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("org.alljoyn.Bus.Security.ManagedApplication");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("InstallMembership");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    acls[1].SetRules(1, rules);

    /* acls record 2  any trusted user */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[2].SetPeers(1, peers);

    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("*");
    prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[0].SetMembers(2, prms);
    acls[2].SetRules(1, rules);

    policy.SetAcls(3, acls);
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

    TrustAnchor* adminGroupAuthority = NULL;
    TrustAnchor* certificateAuthority = new TrustAnchor(TRUST_ANCHOR_CA);
    QStatus status = KeyInfoHelper::MsgArgToKeyInfoNISTP256PubKey(args[0], certificateAuthority->keyInfo);
    if (ER_OK != status) {
        goto DoneValidation;
    }
    status = KeyInfoHelper::MsgArgToKeyInfoKeyId(args[1], certificateAuthority->keyInfo);
    if (ER_OK != status) {
        goto DoneValidation;
    }

    adminGroupAuthority = new TrustAnchor(TRUST_ANCHOR_ADMIN_GROUP);
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
    status = KeyInfoHelper::MsgArgToKeyInfoKeyId(args[4], adminGroupAuthority->keyInfo);
    if (ER_OK != status) {
        goto DoneValidation;
    }

    /* clear most of the key entries with the exception of the DSA keys and manifest */
    PerformReset(true);

    /* install certificate authority */
    status = InstallTrustAnchor(certificateAuthority);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::Claim failed to store certificate authority"));
        status = ER_PERMISSION_DENIED;
        goto DoneValidation;
    }
    /* install admin group authority */
    status = InstallTrustAnchor(adminGroupAuthority);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::Claim failed to store admin group authority"));
        status = ER_PERMISSION_DENIED;
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
    delete certificateAuthority;
    if (ER_OK != status) {
        delete adminGroupAuthority;
        MethodReply(msg, status);
        return;
    }

    /* generate the default policy */
    PermissionPolicy* defaultPolicy = new PermissionPolicy();
    ECCPublicKey pubKey;
    ca->GetDSAPublicKey(pubKey);
    GenerateDefaultPolicy(adminGroupAuthority->securityGroupId, adminGroupAuthority->keyInfo, &pubKey, *defaultPolicy);
    delete adminGroupAuthority;
    status = StorePolicy(*defaultPolicy);
    if (ER_OK == status) {
        policyVersion = defaultPolicy->GetVersion();
        PolicyChanged(defaultPolicy);
    } else {
        delete defaultPolicy;
    }

    MethodReply(msg, status);

    if (ER_OK == status) {
        applicationState = PermissionConfigurator::CLAIMED;
        StoreApplicationState();
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

QStatus PermissionMgmtObj::RebuildDefaultPolicy(PermissionPolicy& defaultPolicy)
{
    /* generate the default policy */
    ECCPublicKey pubKey;
    ca->GetDSAPublicKey(pubKey);
    GUID128 adminGroupGUID;
    KeyInfoNISTP256 adminGroupAuthority;
    QStatus status = GetAdminGroupAuthority(trustAnchors, adminGroupGUID, adminGroupAuthority);
    if (ER_OK != status) {
        return status;
    }
    GenerateDefaultPolicy(adminGroupGUID, adminGroupAuthority, &pubKey, defaultPolicy);
    return ER_OK;
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

QStatus PermissionMgmtObj::StorePolicy(PermissionPolicy& policy)
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
    GetACLKey(ENTRY_POLICY, policyKey);
    KeyBlob kb((uint8_t*) buf, size, KeyBlob::GENERIC);
    delete [] buf;

    return ca->StoreKey(policyKey, kb);
}

QStatus PermissionMgmtObj::RetrievePolicy(PermissionPolicy& policy)
{
    /* retrieve data from keystore */
    KeyStore::Key policyKey;
    GetACLKey(ENTRY_POLICY, policyKey);
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

static QStatus ValidateCertificate(CertificateX509& cert, PermissionMgmtObj::TrustAnchorList* taList)
{
    /* check validity period */
    if (ER_OK != cert.VerifyValidity()) {
        return ER_INVALID_CERTIFICATE;
    }
    for (PermissionMgmtObj::TrustAnchorList::iterator it = taList->begin(); it != taList->end(); it++) {
        if (cert.Verify((*it)->keyInfo.GetPublicKey()) == ER_OK) {
            return ER_OK;  /* cert is verified */
        }
    }
    return ER_UNKNOWN_CERTIFICATE;
}

static QStatus ValidateMembershipCertificate(MembershipCertificate& cert, PermissionMgmtObj::TrustAnchorList* taList)
{
    /* check validity period */
    if (ER_OK != cert.VerifyValidity()) {
        return ER_INVALID_CERTIFICATE;
    }
    for (PermissionMgmtObj::TrustAnchorList::iterator it = taList->begin(); it != taList->end(); it++) {
        PermissionMgmtObj::TrustAnchor* ta = *it;
        if ((ta->use == PermissionMgmtObj::TRUST_ANCHOR_SG_AUTHORITY) ||
            (ta->use == PermissionMgmtObj::TRUST_ANCHOR_ADMIN_GROUP)) {
            if (cert.Verify(ta->keyInfo.GetPublicKey()) == ER_OK) {
                return ER_OK;  /* cert is verified */
            }
        }
    }
    return ER_UNKNOWN_CERTIFICATE;
}

static QStatus ValidateMembershipCertificateChain(bool checkSecurityGroupID, std::vector<MembershipCertificate*>& certs, PermissionMgmtObj::TrustAnchorList* taList)
{
    size_t idx = 0;
    bool validated = false;
    for (std::vector<MembershipCertificate*>::iterator it = certs.begin(); it != certs.end(); it++) {
        idx++;
        QStatus status;
        if (checkSecurityGroupID) {
            status = ValidateMembershipCertificate(*((MembershipCertificate*) *it), taList);
        } else {
            status = ValidateCertificate(*(*it), taList);
        }
        if (ER_OK == status) {
            validated = true;
            break;
        }
    }
    if (!validated) {
        return ER_UNKNOWN_CERTIFICATE;
    }

    if (idx == 1) {
        return ER_OK;  /* the leaf cert is trusted.  No need to validate the whole chain */
    }
    /* There are at least two nodes in the cert chain.
     * Now make sure the chain is a valid chain.
     */
    for (int cnt = (idx - 2); cnt >= 0; cnt--) {
        KeyInfoNISTP256 keyInfo;
        keyInfo.SetPublicKey(certs[cnt + 1]->GetSubjectPublicKey());
        if (certs[cnt]->Verify(keyInfo) != ER_OK) {
            return ER_INVALID_CERT_CHAIN;
        }
    }
    return ER_OK;
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

static QStatus LoadCertificate(CertificateX509::EncodingType encoding, const uint8_t* encoded, size_t encodedLen, CertificateX509& cert, PermissionMgmtObj::TrustAnchorList* taList)
{
    QStatus status = LoadCertificate(encoding, encoded, encodedLen, cert);
    if (ER_OK != status) {
        return status;
    }
    return ValidateCertificate(cert, taList);
}

QStatus PermissionMgmtObj::SameSubjectPublicKey(CertificateX509& cert, bool& outcome)
{
    ECCPublicKey pubKey;
    QStatus status = ca->GetDSAPublicKey(pubKey);
    if (status != ER_OK) {
        return status;
    }
    outcome = memcmp(&pubKey, cert.GetSubjectPublicKey(), sizeof(ECCPublicKey)) == 0;
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
        IdentityCertificate cert;

        status = LoadCertificate((CertificateX509::EncodingType) encoding, encoded, encodedLen, cert, &trustAnchors);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertChain failed to validate certificate status 0x%x", status));
            goto ExitStoreIdentity;
        }
        if (cnt == 0) {
            /* handle the leaf cert */
            bool sameKey = false;
            status = SameSubjectPublicKey(cert, sameKey);
            if (ER_OK != status) {
                QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertChain failed to validate certificate subject public key status 0x%x", status));
                goto ExitStoreIdentity;
            }
            if (!sameKey) {
                QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertChain failed since certificate subject public key is not the same as target public key"));
                status = ER_UNKNOWN_CERTIFICATE;
                goto ExitStoreIdentity;
            }
            KeyBlob kb(cert.GetEncoded(), cert.GetEncodedLen(), KeyBlob::GENERIC);
            status = ca->StoreKey(identityHead, kb);
            if (ER_OK != status) {
                goto ExitStoreIdentity;
            }
        } else {
            KeyBlob kb(cert.GetEncoded(), cert.GetEncodedLen(), KeyBlob::GENERIC);
            /* add the cert chain data as an associate of the identity entry */
            GUID128 guid;
            KeyStore::Key key(KeyStore::Key::LOCAL, guid);
            status = ca->AddAssociatedKey(identityHead, key, kb);
            if (ER_OK != status) {
                goto ExitStoreIdentity;
            }
        }
    }

ExitStoreIdentity:

    if (ER_OK != status) {
        /* clear out the identity entry and all of its associated key */
        ca->DeleteKey(identityHead);
    }
    return status;
}

QStatus PermissionMgmtObj::RetrieveIdentityCertChain(MsgArg** certArgs, size_t* count)
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
    KeyStore::Key* keys = NULL;
    size_t numOfKeys = 0;
    status = ca->GetKeys(identityHead, &keys, &numOfKeys);
    if (ER_OK != status) {
        return status;
    }
    *count = 1 + numOfKeys;
    *certArgs = new MsgArg[*count];
    /* leaf cert */
    MsgArg localArg;
    status = localArg.Set("(yay)", CertificateX509::ENCODING_X509_DER, kb.GetSize(), kb.GetData());
    if (ER_OK != status) {
        goto Exit;
    }
    /* copy the message arg for a deep copy of the array arguments */
    (*certArgs)[0] = localArg;

    /* go through the issuing certs in the chain */
    for (size_t cnt = 0; cnt < numOfKeys; cnt++) {
        status = ca->GetKey(keys[cnt], kb);
        if (ER_OK != status) {
            goto Exit;
        }
        MsgArg localArg;
        status = localArg.Set("(yay)", CertificateX509::ENCODING_X509_DER, kb.GetSize(), kb.GetData());
        if (ER_OK != status) {
            goto Exit;
        }
        /* copy the message arg for a deep copy of the array arguments */
        (*certArgs)[cnt + 1] = localArg;
    }
Exit:
    if (ER_OK != status) {
        delete [] *certArgs;
        *certArgs = NULL;
        *count = 0;
    }
    delete [] keys;
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
    serial = leafCert.GetSerial();
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
    MethodReply(msg, status);
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
    PermissionPolicy policy;
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[1];
    PermissionPolicy::Rule* localRules = NULL;
    if (count > 0) {
        localRules = new PermissionPolicy::Rule[count];
        for (size_t cnt = 0; cnt < count; cnt++) {
            localRules[cnt] = rules[cnt];
        }
    }
    acls[0].SetRules(count, localRules);
    policy.SetAcls(1, acls);
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    return policy.Digest(marshaller, digest, Crypto_SHA256::DIGEST_SIZE);
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
        QCC_DbgPrintf(("PermissionMgmtObj::StoreManifest failed to retrieve identify cert status 0x%x\n"));
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
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[1];
    acls[0].SetRules(count, rules);
    policy.SetAcls(1, acls);
    rules = NULL;  /* its memory is now controlled by policy object */

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

/**
 * Get the keystore key of the membeship cert in the chain of membership certificate
 */
static QStatus GetMembershipChainKey(CredentialAccessor* ca, KeyStore::Key& leafMembershipKey, const String& serialNum, const String& issuerAki, KeyStore::Key& membershipChainKey)
{
    KeyStore::Key* keys = NULL;
    size_t numOfKeys;
    QStatus status = ca->GetKeys(leafMembershipKey, &keys, &numOfKeys);
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
        /* check both serial number and issuer */
        MembershipCertificate cert;
        LoadCertificate(CertificateX509::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
        if ((cert.GetSerial() == serialNum) && (cert.GetAuthorityKeyId() == issuerAki)) {
            membershipChainKey = keys[cnt];
            found = true;
            break;
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

static QStatus GetMembershipKey(CredentialAccessor* ca, KeyStore::Key& membershipHead, const String& serialNum, const String& issuerAki, bool searchLeafCertOnly, KeyStore::Key& membershipKey)
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
            if ((cert.GetSerial() == serialNum) && (cert.GetAuthorityKeyId() == issuerAki)) {
                membershipKey = keys[cnt];
                found = true;
                break;
            }
        }
        if (searchLeafCertOnly) {
            continue;
        }
        /* may be its cert chain is a match */
        KeyStore::Key tmpKey;
        status = GetMembershipChainKey(ca, keys[cnt], serialNum, issuerAki, tmpKey);
        if (ER_OK == status) {
            membershipKey = tmpKey;
            found = true;
            break;
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

QStatus PermissionMgmtObj::StoreMembership(const MsgArg& msgArg)
{
    size_t certChainCount;
    MsgArg* certChain;
    QStatus status = msgArg.Get("a(yay)", &certChainCount, &certChain);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallMembership failed to retrieve certificate chain status 0x%x", status));
        return status;
    }

    GUID128 membershipGuid;
    KeyStore::Key membershipKey(KeyStore::Key::LOCAL, membershipGuid);
    for (size_t cnt = 0; cnt < certChainCount; cnt++) {
        MembershipCertificate cert;
        QStatus status = LoadX509CertFromMsgArg(certChain[cnt], cert);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionMgmtObj::InstallMembership failed to retrieve certificate [%d] status 0x%x", (int) cnt, status));
            return status;
        }
        KeyBlob kb(cert.GetEncoded(), cert.GetEncodedLen(), KeyBlob::GENERIC);
        if (cnt == 0) {
            /* handle the leaf cert */
            kb.SetTag(cert.GetSerial());

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
                checkDup = false;
            }
            /* check for duplicate */
            if (checkDup) {
                KeyStore::Key tmpKey;
                status = GetMembershipKey(ca, membershipHead, cert.GetSerial(), cert.GetAuthorityKeyId(), true, tmpKey);
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
        }
    }
    return status;
}

QStatus PermissionMgmtObj::LocateMembershipEntry(const String& serialNum, const String& issuerAki, KeyStore::Key& membershipKey, bool searchLeafCertOnly)
{
    /* look for memberships head in the key store */
    KeyStore::Key membershipHead;
    GetACLKey(ENTRY_MEMBERSHIPS, membershipHead);

    KeyBlob headerBlob;
    QStatus status = ca->GetKey(membershipHead, headerBlob);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return status;
    }
    return GetMembershipKey(ca, membershipHead, serialNum, issuerAki, searchLeafCertOnly, membershipKey);
}

QStatus PermissionMgmtObj::LocateMembershipEntry(const String& serialNum, const String& issuerAki, KeyStore::Key& membershipKey)
{
    return LocateMembershipEntry(serialNum, issuerAki, membershipKey, true);
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
    KeyInfoNISTP256 issuerKeyInfo;
    issuerKeyInfo.SetXCoord(xCoord);
    issuerKeyInfo.SetYCoord(yCoord);
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
        status = membershipArgs[cnt].Set("(ayay(yyayay))",
                                         leafCert->GetSerial().size(), (uint8_t*)  leafCert->GetSerial().data(),
                                         issuerKeyInfo.GetKeyIdLen(), issuerKeyInfo.GetKeyId(),
                                         issuerKeyInfo.GetAlgorithm(), issuerKeyInfo.GetCurve(),
                                         qcc::ECC_COORDINATE_SZ, issuerKeyInfo.GetXCoord(),
                                         qcc::ECC_COORDINATE_SZ, issuerKeyInfo.GetYCoord());
        membershipArgs[cnt].Stabilize();
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

QStatus PermissionMgmtObj::GenerateSendMemberships(std::vector<std::vector<MsgArg*> >& args)
{
    MembershipCertMap certMap;
    QStatus status = GetAllMembershipCerts(certMap);
    if (ER_OK != status) {
        return status;
    }
    if (certMap.size() == 0) {
        return ER_OK;
    }

    for (MembershipCertMap::iterator it = certMap.begin(); it != certMap.end(); it++) {
        std::vector<MsgArg*> argList;
        MembershipCertificate* cert = it->second;
        String der;
        status = cert->EncodeCertificateDER(der);
        if (ER_OK != status) {
            goto Exit;
            return status;
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
        args.push_back(argList);
    }

Exit:
    ClearMembershipCertMap(certMap);
    if (ER_OK != status) {
        _PeerState::ClearGuildArgs(args);
    }
    return ER_OK;
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
        MembershipCertificate* cert = new MembershipCertificate();
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
            peerState->SetGuildMetadata(cert->GetSerial(), cert->GetAuthorityKeyId(), meta);
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
        TrustAnchorList guildAuthorities;
        const PermissionPolicy* policy = bus.GetInternal().GetPermissionManager().GetPolicy();
        if (policy) {
            LoadSecurityGroupAuthorities(*policy, guildAuthorities);
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
                if (memcmp(&peerPublicKey, metadata->certChain[0]->GetSubjectPublicKey(), sizeof(ECCPublicKey)) != 0) {
                    /* remove this membership cert since it is not valid */
                    peerState->guildMap.erase(it);
                    delete metadata;
                    verified = false;
                    break;
                }
                /* build the vector of certs to verify.  The membership cert is the leaf node -- first item on the vector */
                /* check membership cert chain against the trust anchors */
                status = ValidateMembershipCertificateChain(false, metadata->certChain, &trustAnchors);
                if (ER_OK != status) {
                    /* check against the guild authorities */
                    status = ValidateMembershipCertificateChain(true, metadata->certChain, &guildAuthorities);
                }

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
        ClearTrustAnchorList(guildAuthorities);
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
    config.applicationState = (uint8_t) applicationState;
    return StoreConfiguration(config);
}

QStatus PermissionMgmtObj::SetApplicationState(PermissionConfigurator::ApplicationState newState)
{
    if ((newState == PermissionConfigurator::CLAIMABLE) && (applicationState == PermissionConfigurator::CLAIMED)) {
        return ER_INVALID_APPLICATION_STATE;
    }

    /* save the configuration */
    Configuration config;
    config.applicationState = (uint8_t) newState;
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
    GetACLKey(ENTRY_TRUST_ANCHOR, key);
    QStatus status = ca->DeleteKey(key);
    if (ER_OK != status) {
        return status;
    }
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

    GetACLKey(ENTRY_MEMBERSHIPS, key);
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
    bus.GetInternal().GetKeyStore().Clear(String(ECDHE_NAME_PREFIX_PATTERN));

    applicationState = PermissionConfigurator::CLAIMABLE;
    policyVersion = 0;
    PolicyChanged(NULL);
    return status;
}

QStatus PermissionMgmtObj::Reset()
{
    /* do full reset */
    return PerformReset(true);
}

void PermissionMgmtObj::Reset(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    MethodReply(msg, Reset());
}

QStatus PermissionMgmtObj::GetConnectedPeerPublicKey(const GUID128& guid, qcc::ECCPublicKey* publicKey, uint8_t* manifestDigest, std::vector<ECCPublicKey>& issuerPublicKeys)
{
    CredentialAccessor ca(bus);
    KeyBlob kb;
    KeyStore::Key key(KeyStore::Key::REMOTE, guid);
    QStatus status = ca.GetKey(key, kb);
    if (ER_OK != status) {
        return status;
    }
    KeyBlob msBlob;
    bool keyAvail = false;
    status = KeyExchanger::ParsePeerSecretRecord(kb, msBlob, publicKey, manifestDigest, issuerPublicKeys, keyAvail);
    if (ER_OK != status) {
        return status;
    }
    if (!keyAvail) {
        return ER_BUS_KEY_UNAVAILABLE;
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
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[1];
    PermissionPolicy::Rule* localRules = NULL;
    if (count > 0) {
        localRules = new PermissionPolicy::Rule[count];
        for (size_t cnt = 0; cnt < count; cnt++) {
            localRules[cnt] = rules[cnt];
        }
    }
    acls[0].SetRules(count, localRules);
    policy.SetAcls(1, acls);


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

    return status = ca->StoreKey(key, kb);
}

QStatus PermissionMgmtObj::RetrieveManifest(PermissionPolicy::Rule** manifest, size_t* count)
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
    PermissionPolicy::Acl* acls = (PermissionPolicy::Acl*) policy.GetAcls();
    *count = acls[0].GetRulesSize();
    *manifest = NULL;
    if (*count > 0) {
        *manifest = new PermissionPolicy::Rule[*count];
        for (size_t cnt = 0; cnt < *count; cnt++) {
            (*manifest)[cnt] = acls[0].GetRules()[cnt];
        }
    }
    return ER_OK;
}

void PermissionMgmtObj::GetManifestTemplate(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    KeyBlob kb;
    KeyStore::Key key;
    GetACLKey(ENTRY_MANIFEST_TEMPLATE, key);
    QStatus status = ca->GetKey(key, kb);
    if (ER_OK != status) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            status = ER_MANIFEST_NOT_FOUND;
        }
        MethodReply(msg, status);
        return;
    }
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    PermissionPolicy policy;
    status = policy.Import(marshaller, kb.GetData(), kb.GetSize());
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }
    if (policy.GetAclsSize() == 0) {
        MethodReply(msg, ER_MANIFEST_NOT_FOUND);
        return;
    }
    PermissionPolicy::Acl* acls = (PermissionPolicy::Acl*) policy.GetAcls();
    MsgArg rulesArg;
    status = PermissionPolicy::GenerateRules(acls[0].GetRules(), acls[0].GetRulesSize(), rulesArg);
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }

    MsgArg replyArgs[1];
    replyArgs[0].Set("(yv)", MANIFEST_TYPE_ALLJOYN, &rulesArg);
    BusObject::MethodReply(msg, replyArgs, ArraySize(replyArgs));
}

bool PermissionMgmtObj::ValidateCertChain(const qcc::String& certChainPEM, bool& authorized)
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
    /* go through the cert chain to see whether any of the issuer is the trust anchor */
    if (count == 1) {
        /* single cert is exchanged */
        /* locate the issuer */
        if (certChain[0].GetAuthorityKeyId().empty()) {
            if (IsTrustAnchor(certChain[0].GetSubjectPublicKey())) {
                authorized = (ER_OK == certChain[0].Verify(certChain[0].GetSubjectPublicKey()));
            }
        } else {
            TrustAnchorList anchors = LocateTrustAnchor(trustAnchors, certChain[0].GetAuthorityKeyId());
            if (anchors.empty()) {
                /* no trust anchor with the given authority key id */
                const ECCPublicKey* pubKey = NULL;
                if (IsTrustAnchor(certChain[0].GetSubjectPublicKey())) {
                    /* the peer is my trust anchor */
                    pubKey = certChain[0].GetSubjectPublicKey();
                } else {
                    /* check to see whether this app signed this cert */
                    KeyInfoNISTP256 keyInfo;
                    String aki;
                    if ((ER_OK == RetrieveAndGenDSAPublicKey(ca, keyInfo)) &&
                        (ER_OK == CertificateX509::GenerateAuthorityKeyId(keyInfo.GetPublicKey(), aki))) {
                        if (aki == certChain[0].GetAuthorityKeyId()) {
                            pubKey = keyInfo.GetPublicKey();
                        }
                    }
                }
                if (pubKey) {
                    authorized = (ER_OK == certChain[0].Verify(pubKey));
                }
            } else {
                for (TrustAnchorList::const_iterator it = anchors.begin(); it != anchors.end(); it++) {
                    authorized = (ER_OK == certChain[0].Verify((*it)->keyInfo.GetPublicKey()));
                    if (authorized) {
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
                authorized = true;
                break;
            }
        }
        if (!authorized) {
            /* may be this app signed one of the cert in the peer's cert chain */
            KeyInfoNISTP256 keyInfo;
            if (ER_OK == RetrieveAndGenDSAPublicKey(ca, keyInfo)) {
                for (size_t cnt = 1; cnt < count; cnt++) {
                    if (*certChain[cnt].GetSubjectPublicKey() == *keyInfo.GetPublicKey()) {
                        authorized = true;
                        break;
                    }
                }
            }
        }
    }
    delete [] certChain;
    return handled;
}

bool PermissionMgmtObj::KeyExchangeListener::RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials)
{
    if (strcmp("ALLJOYN_ECDHE_ECDSA", authMechanism) == 0) {
        /* Use the installed identity certificate instead of asking the application */
        KeyBlob kb;
        QStatus status = pmo->GetIdentityBlob(kb);
        if ((ER_OK == status) && (kb.GetSize() > 0)) {
            bool handled = true;
            /* handle private key */
            if ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                qcc::ECCPrivateKey pk;
                if (ER_OK != pmo->GetDSAPrivateKey(pk)) {
                    handled = false;
                }
                String pem;
                CertificateX509::EncodePrivateKeyPEM((const uint8_t*) &pk, sizeof(qcc::ECCPrivateKey), pem);
                credentials.SetPrivateKey(pem);
            }
            /* build the cert chain based on the identity cert */
            if ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                String der((const char*) kb.GetData(), kb.GetSize());
                String pem;
                CertificateX509::EncodeCertificatePEM(der, pem);
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
        qcc::String certChain = credentials.GetCertChain();
        if (certChain.empty()) {
            return false;
        }
        bool authorized = false;
        bool handled = pmo->ValidateCertChain(certChain, authorized);
        if (handled) {
            return authorized;
        }
    }
    return ProtectedAuthListener::VerifyCredentials(authMechanism, peerName, credentials);
}

void PermissionMgmtObj::ObjectRegistered(void)
{
    /* Bind to the reserved port for PermissionMgmt */
    BindPort();
}

QStatus PermissionMgmtObj::BindPort()
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort sessionPort = ALLJOYN_SESSIONPORT_PERMISSION_MGMT;
    if (portListener) {
        bus.UnbindSessionPort(sessionPort);
        delete portListener;
    }

    portListener = new PortListener();
    QStatus status = bus.BindSessionPort(sessionPort, opts, *portListener);
    if (ER_OK != status) {
        delete portListener;
        portListener = NULL;
    }
    return status;
}

QStatus PermissionMgmtObj::ManageMembershipTrustAnchors(PermissionPolicy* policy)
{
    if (policy == NULL) {
        return ER_OK;
    }
    TrustAnchorList addOns;
    LoadSGAuthoritiesAndRestrictedCAs(false, *policy, addOns);
    if (addOns.size() > 0) {
        /* remove all the membership trust anchors and re-add the new ones */
        ClearTrustAnchorListByUse(TRUST_ANCHOR_SG_AUTHORITY, trustAnchors);
        ClearTrustAnchorListByUse(TRUST_ANCHOR_RESTRICTED_CA, trustAnchors);
        trustAnchors.reserve(trustAnchors.size() + addOns.size());
        trustAnchors.insert(trustAnchors.end(), addOns.begin(), addOns.end());
        addOns.clear();
        return StoreTrustAnchors();
    }
    return ER_OK;
}

} /* namespace ajn */

