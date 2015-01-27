/**
 * @file
 * This file implements the PermissionMgmt interface.
 */

/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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
#include <qcc/StringUtil.h>
#include "PermissionMgmtObj.h"
#include "PeerState.h"
#include "BusInternal.h"
#include "KeyInfoHelper.h"
#include "KeyExchanger.h"

#define QCC_MODULE "PERMISSION_MGMT"

/**
 * Tag name used in the certificate nodes in the membership list in the key store.
 */
#define AUTH_DATA_TAG_NAME "AUTH_DATA"
#define CERT_CHAIN_TAG_NAME "CERT_CHAIN"

/**
 * The field id used in the SendMembership call
 */
#define SEND_CERT           1
#define SEND_CERT_CHAIN     2
#define SEND_AUTH_DATA      3

/**
 * The entry status id used in the SendMembership call
 */
#define SEND_MBRSHIP_NONE   0
#define SEND_MBRSHIP_MORE   1
#define SEND_MBRSHIP_LAST   2

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

static QStatus RetrieveAndGenDSAPublicKey(CredentialAccessor* ca, KeyInfoNISTP256& keyInfo);

PermissionMgmtObj::PermissionMgmtObj(BusAttachment& bus) :
    BusObject(org::allseen::Security::PermissionMgmt::ObjectPath, false),
    bus(bus), notifySignalName(NULL), portListener(NULL)
{
    /* Add org.allseen.Security.PermissionMgmt interface */
    const InterfaceDescription* ifc = bus.GetInterface(org::allseen::Security::PermissionMgmt::InterfaceName);
    if (ifc) {
        AddInterface(*ifc);
        AddMethodHandler(ifc->GetMember("Claim"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::Claim));
        AddMethodHandler(ifc->GetMember("InstallPolicy"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::InstallPolicy));
        AddMethodHandler(ifc->GetMember("GetPolicy"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::GetPolicy));
        AddMethodHandler(ifc->GetMember("RemovePolicy"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::RemovePolicy));
        AddMethodHandler(ifc->GetMember("InstallIdentity"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::InstallIdentity));
        AddMethodHandler(ifc->GetMember("GetIdentity"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::GetIdentity));
        AddMethodHandler(ifc->GetMember("InstallMembership"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::InstallMembership));
        AddMethodHandler(ifc->GetMember("InstallMembershipAuthData"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::InstallMembershipAuthData));
        AddMethodHandler(ifc->GetMember("RemoveMembership"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::RemoveMembership));
        AddMethodHandler(ifc->GetMember("InstallGuildEquivalence"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::InstallGuildEquivalence));
        AddMethodHandler(ifc->GetMember("GetManifest"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::GetManifest));
        AddMethodHandler(ifc->GetMember("Reset"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::Reset));
        AddMethodHandler(ifc->GetMember("GetPublicKey"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::GetPublicKey));
    }
    /* Add org.allseen.Security.PermissionMgmt.Notification interface */
    const InterfaceDescription* notificationIfc = bus.GetInterface(org::allseen::Security::PermissionMgmt::Notification::InterfaceName);
    if (notificationIfc) {
        AddInterface(*notificationIfc);
        notifySignalName = notificationIfc->GetMember("NotifyConfig");
    }

    ca = new CredentialAccessor(bus);
    KeyInfoNISTP256 keyInfo;
    RetrieveAndGenDSAPublicKey(ca, keyInfo);
    bus.GetInternal().GetPermissionManager().SetPermissionMgmtObj(this);

    QStatus status = LoadTrustAnchors();
    if (ER_OK == status) {
        claimableState = PermissionConfigurator::STATE_CLAIMED;
    } else {
        claimableState = PermissionConfigurator::STATE_CLAIMABLE;
        Configuration config;
        status = GetConfiguration(config);
        if (ER_OK == status) {
            if (config.claimableState == PermissionConfigurator::STATE_UNCLAIMABLE) {
                claimableState = PermissionConfigurator::STATE_UNCLAIMABLE;
            }
        }
    }

    bus.RegisterBusObject(*this, true);
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
    NotifyConfig();
}

static QStatus RetrieveAndGenDSAPublicKey(CredentialAccessor* ca, KeyInfoNISTP256& keyInfo)
{
    bool genKeys = false;
    ECCPublicKey pubKey;
    QStatus status = PermissionMgmtObj::RetrieveDSAPublicKey(ca, &pubKey);
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
        status = PermissionMgmtObj::RetrieveDSAPublicKey(ca, &pubKey);
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

QStatus PermissionMgmtObj::GetACLGUID(ACLEntryType aclEntryType, qcc::GUID128& guid)
{
    /* each local key will be indexed by an hardcode randomly generated GUID. */
    if (aclEntryType == ENTRY_TRUST_ANCHOR) {
        guid = GUID128(qcc::String("E866F6C2CB5C005256F2944A042C0758"));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_POLICY) {
        guid = GUID128(qcc::String("F5CB9E723D7D4F1CFF985F4DD0D5E388"));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_MEMBERSHIPS) {
        guid = GUID128(qcc::String("42B0C7F35695A3220A46B3938771E965"));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_IDENTITY) {
        guid = GUID128(qcc::String("4D8B9E901D7BE0024A331609BBAA4B02"));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_EQUIVALENCES) {
        guid = GUID128(qcc::String("7EA4E59508DA5F3938EFF5F3CC5325CF"));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_MANIFEST) {
        guid = GUID128(qcc::String("2962ADEAE004074C8C0D598C5387FEB3"));
        return ER_OK;
    }
    if (aclEntryType == ENTRY_CONFIGURATION) {
        guid = GUID128(qcc::String("4EDDBBCE46E0542C24BEC5BF9717C971"));
        return ER_OK;
    }
    return ER_CRYPTO_KEY_UNAVAILABLE;      /* not available */
}

bool PermissionMgmtObj::IsTrustAnchor(const qcc::GUID128& peerGuid)
{
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        /* check against trust anchor */
        GUID128 taGuid(0);
        taGuid.SetBytes((*it)->GetKeyId());
        if (taGuid == peerGuid) {
            return true;
        }
    }
    return false;
}

bool PermissionMgmtObj::IsTrustAnchor(const ECCPublicKey* publicKey)
{
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        /* check against trust anchor */
        if (memcmp((*it)->GetPublicKey(), publicKey, sizeof(ECCPublicKey)) == 0) {
            return true;
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

static void LoadGuildAuthorities(const PermissionPolicy& policy, PermissionMgmtObj::TrustAnchorList& guildAuthoritiesList)
{
    const PermissionPolicy::Term* terms = policy.GetTerms();
    for (size_t cnt = 0; cnt < policy.GetTermsSize(); cnt++) {
        const PermissionPolicy::Peer* peers = terms[cnt].GetPeers();
        for (size_t idx = 0; idx < terms[cnt].GetPeersSize(); idx++) {
            if ((peers[idx].GetType() == PermissionPolicy::Peer::PEER_GUILD) && peers[idx].GetKeyInfo()) {
                if (KeyInfoHelper::InstanceOfKeyInfoNISTP256(*peers[idx].GetKeyInfo())) {
                    guildAuthoritiesList.push_back((KeyInfoNISTP256*) peers[idx].GetKeyInfo());
                }
            }
        }
    }
}

void PermissionMgmtObj::ClearTrustAnchors()
{
    ClearTrustAnchorList(trustAnchors);
}

QStatus PermissionMgmtObj::InstallTrustAnchor(KeyInfoNISTP256* keyInfo)
{
    LoadTrustAnchors();
    /* check for duplicate trust anchor */
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        if ((*it)->GetKeyIdLen() != keyInfo->GetKeyIdLen()) {
            continue;
        }
        if ((*it)->GetKeyIdLen() == 0) {
            continue;
        }
        if (memcmp((*it)->GetKeyId(), keyInfo->GetKeyId(), (*it)->GetKeyIdLen()) == 0) {
            return ER_DUPLICATE_KEY;  /* duplicate */
        }
    }
    trustAnchors.push_back(keyInfo);
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
        bufferSize += (*it)->GetExportSize();
    }
    uint8_t* buffer = new uint8_t[bufferSize];
    uint8_t* pBuf = buffer;
    /* the count field */
    *pBuf = (uint8_t) trustAnchors.size();
    pBuf += sizeof(uint8_t);
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        uint32_t* itemSize = (uint32_t*) pBuf;
        *itemSize = (uint32_t) (*it)->GetExportSize();
        pBuf += sizeof(uint32_t);
        (*it)->Export(pBuf);
        pBuf += *itemSize;
    }
    GUID128 trustAnchorGuid;
    GetACLGUID(ENTRY_TRUST_ANCHOR, trustAnchorGuid);
    KeyBlob kb(buffer, bufferSize, KeyBlob::GENERIC);
    kb.SetExpiration(0xFFFFFFFF);  /* never expired */

    QStatus status = ca->StoreKey(trustAnchorGuid, kb);
    delete [] buffer;
    return status;
}

QStatus PermissionMgmtObj::LoadTrustAnchors()
{
    QCC_DbgPrintf(("PermissionMgmtObj::LoadTrustAnchors from keystore (guid %s)\n",
                   bus.GetInternal().GetKeyStore().GetGuid().c_str()));
    GUID128 trustAnchorGuid;
    GetACLGUID(ENTRY_TRUST_ANCHOR, trustAnchorGuid);
    KeyBlob kb;
    QStatus status = ca->GetKey(trustAnchorGuid, kb);
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
        uint32_t* itemSize = (uint32_t*) pBuf;
        bytesRead += sizeof(uint32_t);
        if (bytesRead > kb.GetSize()) {
            ClearTrustAnchors();
            return ER_NO_TRUST_ANCHOR; /* no trust anchor */
        }
        pBuf += sizeof(uint32_t);
        bytesRead += *itemSize;
        if (bytesRead > kb.GetSize()) {
            ClearTrustAnchors();
            return ER_NO_TRUST_ANCHOR; /* no trust anchor */
        }
        KeyInfoNISTP256* ta = new KeyInfoNISTP256();
        ta->Import(pBuf, *itemSize);
        trustAnchors.push_back(ta);
        pBuf += *itemSize;
    }
    return ER_OK;
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
    GUID128 guid;
    ca->GetLocalGUID(KeyBlob::DSA_PRIVATE, guid);
    QStatus status = ca->StoreKey(guid, dsaPrivKb);
    if (status != ER_OK) {
        return status;
    }

    KeyBlob dsaPubKb((const uint8_t*) publicKey, sizeof(ECCPublicKey), KeyBlob::DSA_PUBLIC);
    ca->GetLocalGUID(KeyBlob::DSA_PUBLIC, guid);
    return ca->StoreKey(guid, dsaPubKb);
}

QStatus PermissionMgmtObj::RetrieveDSAPublicKey(CredentialAccessor* ca, ECCPublicKey* publicKey)
{
    GUID128 guid;
    KeyBlob kb;
    ca->GetLocalGUID(KeyBlob::DSA_PUBLIC, guid);
    QStatus status = ca->GetKey(guid, kb);
    if (status != ER_OK) {
        return status;
    }
    memcpy(publicKey, kb.GetData(), kb.GetSize());
    return ER_OK;
}

QStatus PermissionMgmtObj::RetrieveDSAPrivateKey(CredentialAccessor* ca, ECCPrivateKey* privateKey)
{
    GUID128 guid;
    KeyBlob kb;
    ca->GetLocalGUID(KeyBlob::DSA_PRIVATE, guid);
    QStatus status = ca->GetKey(guid, kb);
    if (status != ER_OK) {
        return status;
    }
    memcpy(privateKey, kb.GetData(), kb.GetSize());
    return ER_OK;
}

void PermissionMgmtObj::GetPublicKey(const InterfaceDescription::Member* member, Message& msg)
{
    KeyInfoNISTP256 replyKeyInfo;
    QStatus status = RetrieveAndGenDSAPublicKey(ca, replyKeyInfo);
    if (status != ER_OK) {
        MethodReply(msg, status);
        return;
    }

    MsgArg replyArgs[1];
    KeyInfoHelper::KeyInfoNISTP256ToMsgArg(replyKeyInfo, replyArgs[0]);
    MethodReply(msg, replyArgs, ArraySize(replyArgs));
}

void PermissionMgmtObj::Claim(const InterfaceDescription::Member* member, Message& msg)
{
    if (claimableState == PermissionConfigurator::STATE_UNCLAIMABLE) {
        MethodReply(msg, ER_PERMISSION_DENIED);
        return;
    }
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    QStatus status = KeyInfoHelper::MsgArgToKeyInfoNISTP256((MsgArg &) * msg->GetArg(0), *keyInfo);
    if (ER_OK != status) {
        delete keyInfo;
        MethodReply(msg, status);
        return;
    }

    /* clear most of the key entries with the exception of the DSA keys and manifest */
    PerformReset(true);

    /* install trust anchor */
    if (keyInfo->GetKeyId() == NULL) {
        /* the trust anchor guid is the calling peer's guid */
        qcc::GUID128 peerGuid;
        status = GetPeerGUID(msg, peerGuid);
        if (ER_OK != status) {
            delete keyInfo;
            MethodReply(msg, status);
            return;
        }
        keyInfo->SetKeyId(peerGuid.GetBytes(), qcc::GUID128::SIZE);
    }
    status = InstallTrustAnchor(keyInfo);
    if (ER_OK != status) {
        delete keyInfo;
        QCC_DbgPrintf(("PermissionMgmtObj::Claim failed to store trust anchor"));
        MethodReply(msg, ER_PERMISSION_DENIED);
        return;
    }

    claimableState = PermissionConfigurator::STATE_CLAIMED;
    status = StoreIdentityCertificate((MsgArg &) * msg->GetArg(1));
    if (ER_OK != status) {
        MethodReply(msg, status);
    } else {
        /* reply with the public key */
        GetPublicKey(member, msg);
    }

    NotifyConfig();
}

void PermissionMgmtObj::InstallPolicy(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    uint8_t version;
    MsgArg* variant;
    msg->GetArg(0)->Get("(yv)", &version, &variant);

    PermissionPolicy* policy = new PermissionPolicy();
    status = policy->Import(version, *variant);
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
        if (policy->GetSerialNum() <= existingPolicy.GetSerialNum()) {
            MethodReply(msg, ER_INSTALLING_OLDER_POLICY);
            delete policy;
            return;
        }
    }

    status = StorePolicy(*policy);
    if (ER_OK == status) {
        serialNum = policy->GetSerialNum();
    }
    MethodReply(msg, status);
    if (ER_OK == status) {
        PolicyChanged(policy);
    } else {
        delete policy;
    }
}

void PermissionMgmtObj::RemovePolicy(const InterfaceDescription::Member* member, Message& msg)
{
    GUID128 guid;
    GetACLGUID(ENTRY_POLICY, guid);
    QStatus status = ca->DeleteKey(guid);
    MethodReply(msg, status);
    if (ER_OK == status) {
        serialNum = 0;
        PolicyChanged(NULL);
    }
}


void PermissionMgmtObj::GetPolicy(const InterfaceDescription::Member* member, Message& msg)
{
    PermissionPolicy policy;
    QStatus status = RetrievePolicy(policy);
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }
    MsgArg msgArg;
    status = policy.Export(msgArg);
    if (ER_OK != status) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    MethodReply(msg, &msgArg, 1);
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
    GUID128 policyGuid;
    GetACLGUID(ENTRY_POLICY, policyGuid);
    KeyBlob kb((uint8_t*) buf, size, KeyBlob::GENERIC);
    delete [] buf;

    return ca->StoreKey(policyGuid, kb);
}

QStatus PermissionMgmtObj::RetrievePolicy(PermissionPolicy& policy)
{
    /* retrieve data from keystore */
    GUID128 policyGuid;
    GetACLGUID(ENTRY_POLICY, policyGuid);
    KeyBlob kb;
    QStatus status = ca->GetKey(policyGuid, kb);
    if (ER_OK != status) {
        return status;
    }
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    return policy.Import(marshaller, kb.GetData(), kb.GetSize());
}

QStatus PermissionMgmtObj::NotifyConfig()
{
    uint8_t flags = ALLJOYN_FLAG_SESSIONLESS;

    MsgArg args[4];
    MsgArg keyInfoArgs[1];
    ECCPublicKey pubKey;
    QStatus status = PermissionMgmtObj::RetrieveDSAPublicKey(ca, &pubKey);
    if (status == ER_OK) {
        KeyInfoNISTP256 keyInfo;
        qcc::GUID128 localGUID;
        ca->GetGuid(localGUID);
        keyInfo.SetKeyId(localGUID.GetBytes(), GUID128::SIZE);
        keyInfo.SetPublicKey(&pubKey);
        KeyInfoHelper::KeyInfoNISTP256ToMsgArg(keyInfo, keyInfoArgs[0]);
        args[0].Set("a(yv)", 1, keyInfoArgs);
    } else {
        args[0].Set("a(yv)", 0, NULL);
    }
    args[1].Set("y", claimableState);
    args[2].Set("u", serialNum);
    args[3].Set("a(ayay)", 0, NULL);
    return Signal(NULL, 0, *notifySignalName, args, 4, 0, flags);
}

static QStatus ValidateCertificate(CertificateX509& cert, PermissionMgmtObj::TrustAnchorList* taList)
{
    for (PermissionMgmtObj::TrustAnchorList::iterator it = taList->begin(); it != taList->end(); it++) {
        KeyInfoNISTP256* keyInfo = *it;
        if (cert.Verify(keyInfo->GetPublicKey()) == ER_OK) {
            return ER_OK;  /* cert is verified */
        }
    }
    return ER_UNKNOWN_CERTIFICATE;
}

static QStatus ValidateMembershipCertificate(MembershipCertificate& cert, PermissionMgmtObj::TrustAnchorList* taList)
{
    for (PermissionMgmtObj::TrustAnchorList::iterator it = taList->begin(); it != taList->end(); it++) {
        KeyInfoNISTP256* keyInfo = *it;
        if (keyInfo->GetKeyIdLen() != GUID128::SIZE) {
            continue;
        }
        GUID128 aGuid(0);
        aGuid.SetBytes(keyInfo->GetKeyId());
        if (aGuid != cert.GetGuild()) {
            continue;
        }

        if (cert.Verify(keyInfo->GetPublicKey()) == ER_OK) {
            return ER_OK;  /* cert is verified */
        }
    }
    return ER_UNKNOWN_CERTIFICATE;
}

static QStatus ValidateCertificateChain(bool checkGuildID, std::vector<CertificateX509*>& certs, PermissionMgmtObj::TrustAnchorList* taList)
{
    size_t idx = 0;
    bool validated = false;
    for (std::vector<CertificateX509*>::iterator it = certs.begin(); it != certs.end(); it++) {
        idx++;
        QStatus status;
        if (checkGuildID) {
            if ((*it)->GetType() != CertificateX509::MEMBERSHIP_CERTIFICATE) {
                continue;
            }
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

static QStatus LoadCertificate(Certificate::EncodingType encoding, const uint8_t* encoded, size_t encodedLen, CertificateX509& cert)
{
    if (encoding == Certificate::ENCODING_X509_DER) {
        return cert.DecodeCertificateDER(String((const char*) encoded, encodedLen));
    } else if (encoding == Certificate::ENCODING_X509_DER_PEM) {
        return cert.DecodeCertificatePEM(String((const char*) encoded, encodedLen));
    }
    return ER_NOT_IMPLEMENTED;
}

static QStatus LoadCertificate(Certificate::EncodingType encoding, const uint8_t* encoded, size_t encodedLen, CertificateX509& cert, PermissionMgmtObj::TrustAnchorList* taList)
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
    QStatus status = PermissionMgmtObj::RetrieveDSAPublicKey(ca, &pubKey);
    if (status != ER_OK) {
        return status;
    }
    outcome = memcmp(&pubKey, cert.GetSubjectPublicKey(), sizeof(ECCPublicKey)) == 0;
    return ER_OK;
}

QStatus PermissionMgmtObj::StoreIdentityCertificate(MsgArg& certArg)
{
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    QStatus status = certArg.Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        return status;
    }
    if ((encoding != Certificate::ENCODING_X509_DER) && (encoding != Certificate::ENCODING_X509_DER_PEM)) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertificate does not support encoding %d", encoding));
        return ER_NOT_IMPLEMENTED;
    }
    IdentityCertificate cert;
    status = LoadCertificate((Certificate::EncodingType) encoding, encoded, encodedLen, cert, &trustAnchors);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertificate failed to validate certificate status 0x%x", status));
        return status;
    }
    bool sameKey = false;
    status = SameSubjectPublicKey(cert, sameKey);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertificate failed to validate certificate subject public key status 0x%x", status));
        return status;
    }
    if (!sameKey) {
        QCC_DbgPrintf(("PermissionMgmtObj::StoreIdentityCertificate failed since certificate subject public key is not the same as target public key"));
        return ER_UNKNOWN_CERTIFICATE;
    }
    GUID128 guid(cert.GetSubject());
    /* store the Identity PEM  into the key store */
    GetACLGUID(ENTRY_IDENTITY, guid);
    KeyBlob kb(encoded, encodedLen, KeyBlob::GENERIC);

    return ca->StoreKey(guid, kb);
}

void PermissionMgmtObj::InstallIdentity(const InterfaceDescription::Member* member, Message& msg)
{
    MethodReply(msg, StoreIdentityCertificate((MsgArg &) * msg->GetArg(0)));
}

QStatus PermissionMgmtObj::GetIdentityBlob(KeyBlob& kb)
{
    /* Get the Identity blob from the key store */
    GUID128 guid;
    GetACLGUID(ENTRY_IDENTITY, guid);
    QStatus status = ca->GetKey(guid, kb);
    if (ER_OK != status) {
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            return ER_CERTIFICATE_NOT_FOUND;
        } else {
            return status;
        }
    }
    return ER_OK;
}

void PermissionMgmtObj::GetIdentity(const InterfaceDescription::Member* member, Message& msg)
{
    KeyBlob kb;
    QStatus status = GetIdentityBlob(kb);
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }
    MsgArg replyArgs[1];
    replyArgs[0].Set("(yay)", Certificate::ENCODING_X509_DER, kb.GetSize(), kb.GetData());
    MethodReply(msg, replyArgs, ArraySize(replyArgs));
}

static QStatus GetMembershipCert(CredentialAccessor* ca, GUID128& guid, MembershipCertificate& cert)
{
    KeyBlob kb;
    QStatus status = ca->GetKey(guid, kb);
    if (ER_OK != status) {
        return status;
    }
    return LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
}

/**
 * Get the guid of the membeship cert in the chain of membership certificate
 */
static QStatus GetMembershipChainGuid(CredentialAccessor* ca, GUID128& leafMembershipGuid, const String& serialNum, const GUID128& issuer, GUID128& membershipChainGuid)
{
    GUID128* guids = NULL;
    size_t numOfGuids;
    QStatus status = ca->GetKeys(leafMembershipGuid, &guids, &numOfGuids);
    if (ER_OK != status) {
        return status;
    }
    String tag = serialNum.substr(0, KeyBlob::MAX_TAG_LEN);
    bool found = false;
    status = ER_OK;
    for (size_t cnt = 0; cnt < numOfGuids; cnt++) {
        KeyBlob kb;
        status = ca->GetKey(guids[cnt], kb);
        if (ER_OK != status) {
            break;
        }
        /* check the tag */
        if (kb.GetTag() == CERT_CHAIN_TAG_NAME) {
            /* check both serial number and issuer */
            MembershipCertificate cert;
            LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
            if ((cert.GetSerial() == serialNum) && (cert.GetIssuer() == issuer)) {
                membershipChainGuid = guids[cnt];
                found = true;
                break;
            }
        }
    }
    delete [] guids;
    if (ER_OK != status) {
        return status;
    }
    if (found) {
        return ER_OK;
    }
    return ER_BUS_KEY_UNAVAILABLE;  /* not found */
}

static QStatus GetMembershipGuid(CredentialAccessor* ca, GUID128& membershipHead, const String& serialNum, const GUID128& issuer, bool searchLeafCertOnly, GUID128& membershipGuid)
{
    GUID128* guids = NULL;
    size_t numOfGuids;
    QStatus status = ca->GetKeys(membershipHead, &guids, &numOfGuids);
    if (ER_OK != status) {
        return status;
    }
    String tag = serialNum.substr(0, KeyBlob::MAX_TAG_LEN);
    bool found = false;
    status = ER_OK;
    for (size_t cnt = 0; cnt < numOfGuids; cnt++) {
        KeyBlob kb;
        status = ca->GetKey(guids[cnt], kb);
        if (ER_OK != status) {
            break;
        }
        /* check the tag */
        if (kb.GetTag() == tag) {
            /* maybe a match, check both serial number and issuer */
            MembershipCertificate cert;
            LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
            if ((cert.GetSerial() == serialNum) && (cert.GetIssuer() == issuer)) {
                membershipGuid = guids[cnt];
                found = true;
                break;
            }
        }
        if (searchLeafCertOnly) {
            continue;
        }
        /* may be its cert chain is a match */
        GUID128 tmpGuid(0);
        status = GetMembershipChainGuid(ca, guids[cnt], serialNum, issuer, tmpGuid);
        if (ER_OK == status) {
            membershipGuid = tmpGuid;
            found = true;
            break;
        }
    }
    delete [] guids;
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
    if ((encoding != Certificate::ENCODING_X509_DER) && (encoding != Certificate::ENCODING_X509_DER_PEM)) {
        return ER_NOT_IMPLEMENTED;
    }
    status = LoadCertificate((Certificate::EncodingType) encoding, encoded, encodedLen, cert);
    if (ER_OK != status) {
        return ER_INVALID_CERTIFICATE;
    }
    return ER_OK;
}

void PermissionMgmtObj::InstallMembership(const InterfaceDescription::Member* member, Message& msg)
{
    size_t certChainCount;
    MsgArg* certChain;
    QStatus status = msg->GetArg(0)->Get("a(yay)", &certChainCount, &certChain);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallMembership failed to retrieve certificate chain status 0x%x", status));
        MethodReply(msg, status);
        return;
    }

    GUID128 membershipGuid;
    for (size_t cnt = 0; cnt < certChainCount; cnt++) {
        MembershipCertificate cert;
        QStatus status = LoadX509CertFromMsgArg(certChain[cnt], cert);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionMgmtObj::InstallMembership failed to retrieve certificate [%d] status 0x%x", (int) cnt, status));
            MethodReply(msg, status);
            return;
        }
        KeyBlob kb(cert.GetEncoded(), cert.GetEncodedLen(), KeyBlob::GENERIC);
        if (cnt == 0) {
            /* handle the leaf cert */
            kb.SetTag(cert.GetSerial());

            /* store the Membership DER  into the key store */
            GUID128 membershipHead;
            GetACLGUID(ENTRY_MEMBERSHIPS, membershipHead);

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
                GUID128 tmpGuid(0);
                status = GetMembershipGuid(ca, membershipHead, cert.GetSerial(), cert.GetIssuer(), true, tmpGuid);
                if (ER_OK == status) {
                    /* found a duplicate */
                    MethodReply(msg, ER_DUPLICATE_CERTIFICATE);
                    return;
                }
            }

            /* add the membership cert as an associate entry to the membership list header node */
            status = ca->AddAssociatedKey(membershipHead, membershipGuid, kb);
        } else {
            /* handle the non-leaf cert */
            kb.SetTag(CERT_CHAIN_TAG_NAME);
            /* add the cert chain data as an associate of the membership entry */
            GUID128 guid;
            status = ca->AddAssociatedKey(membershipGuid, guid, kb);
        }
    }
    if (ER_OK == status) {
        LocalMembershipsChanged();
    }
    MethodReply(msg, status);
}

QStatus PermissionMgmtObj::LocateMembershipEntry(const String& serialNum, const GUID128& issuer, GUID128& membershipGuid, bool searchLeafCertOnly)
{
    /* look for memberships head in the key store */
    GUID128 membershipHead(0);
    GetACLGUID(ENTRY_MEMBERSHIPS, membershipHead);

    KeyBlob headerBlob;
    QStatus status = ca->GetKey(membershipHead, headerBlob);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return status;
    }
    return GetMembershipGuid(ca, membershipHead, serialNum, issuer, searchLeafCertOnly, membershipGuid);
}

QStatus PermissionMgmtObj::LocateMembershipEntry(const String& serialNum, const GUID128& issuer, GUID128& membershipGuid)
{
    return LocateMembershipEntry(serialNum, issuer, membershipGuid, true);
}

static QStatus LoadPolicyFromMsgArg(MsgArg& arg, PermissionPolicy& policy)
{
    uint8_t version;
    MsgArg* variant;
    QStatus status = arg.Get("(yv)", &version, &variant);
    if (ER_OK != status) {
        return status;
    }

    return policy.Import(version, *variant);
}

static QStatus LoadAndValidateAuthDataUsingCert(BusAttachment& bus, MsgArg& authDataArg, PermissionPolicy& authorization, MembershipCertificate& cert)
{
    QStatus status;
    /* retrieve the authorization data */
    status = LoadPolicyFromMsgArg(authDataArg, authorization);
    if (ER_OK != status) {
        return status;
    }
    if (!cert.IsDigestPresent()) {
        return ER_MISSING_DIGEST_IN_CERTIFICATE;
    }
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    status = authorization.Digest(marshaller, digest, Crypto_SHA256::DIGEST_SIZE);
    if (ER_OK != status) {
        return status;
    }
    if (memcmp(digest, cert.GetDigest(), Crypto_SHA256::DIGEST_SIZE)) {
        return ER_DIGEST_MISMATCH;
    }
    return ER_OK;
}

QStatus PermissionMgmtObj::LoadAndValidateAuthData(const String& serial, const GUID128& issuer, MsgArg& authDataArg, PermissionPolicy& authorization, GUID128& membershipGuid)
{
    QStatus status;
    status = LocateMembershipEntry(serial, issuer, membershipGuid, false);
    if (ER_OK != status) {
        return status;
    }

    MembershipCertificate cert;
    status = GetMembershipCert(ca, membershipGuid, cert);
    if (ER_OK != status) {
        return status;
    }
    //return LoadAndValidateAuthDataUsingCert(bus, authDataArg, authorization, cert);
    status = LoadAndValidateAuthDataUsingCert(bus, authDataArg, authorization, cert);
    return status;
}

void PermissionMgmtObj::InstallMembershipAuthData(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    const char* serial;
    status = msg->GetArg(0)->Get("s", &serial);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallMembershipAuthData failed to retrieve serial status 0x%x", status));
        MethodReply(msg, status);
        return;
    }
    uint8_t* issuer;
    size_t issuerLen;
    status = msg->GetArg(1)->Get("ay", &issuerLen, &issuer);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallMembershipAuthData failed to retrieve issuer status 0x%x", status));
        MethodReply(msg, status);
        return;
    }
    if (issuerLen != GUID128::SIZE) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    String serialNum(serial);
    GUID128 issuerGUID(0);
    issuerGUID.SetBytes(issuer);
    GUID128 membershipGuid(0);
    PermissionPolicy authorization;
    status = LoadAndValidateAuthData(serialNum, issuerGUID, (MsgArg &) * (msg->GetArg(2)), authorization, membershipGuid);
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }

    uint8_t* buf = NULL;
    size_t size;
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    status = authorization.Export(marshaller, &buf, &size);
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }
    KeyBlob kb((uint8_t*) buf, size, KeyBlob::GENERIC);
    kb.SetTag(AUTH_DATA_TAG_NAME);
    delete [] buf;
    /* add the authorization data as an associate of the membership entry */
    GUID128 guid;
    status = ca->AddAssociatedKey(membershipGuid, guid, kb);
    if (ER_OK == status) {
        LocalMembershipsChanged();
    }
    MethodReply(msg, status);
}

void PermissionMgmtObj::RemoveMembership(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    const char* serial;
    status = msg->GetArg(0)->Get("s", &serial);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::RemoveMembership failed to retrieve serial status 0x%x", status));
        MethodReply(msg, status);
        return;
    }
    uint8_t* issuer;
    size_t issuerLen;
    status = msg->GetArg(1)->Get("ay", &issuerLen, &issuer);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::RemoveMembership failed to retrieve issuer status 0x%x", status));
        MethodReply(msg, status);
        return;
    }
    if (issuerLen != GUID128::SIZE) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    String serialNum(serial);
    GUID128 issuerGUID(0);
    issuerGUID.SetBytes(issuer);
    GUID128 membershipGuid(0);
    status = LocateMembershipEntry(serialNum, issuerGUID, membershipGuid);
    if (ER_OK == status) {
        /* found it so delete it */
        status = ca->DeleteKey(membershipGuid);
    } else if (ER_BUS_KEY_UNAVAILABLE == status) {
        /* could not find it.  */
        status = ER_CERTIFICATE_NOT_FOUND;
    }
    if (ER_OK == status) {
        LocalMembershipsChanged();
    }
    MethodReply(msg, status);
}

static QStatus ReadMembershipCert(CredentialAccessor* ca, const GUID128& guid, MembershipCertificate& cert)
{
    KeyBlob kb;
    QStatus status = ca->GetKey(guid, kb);
    if (ER_OK != status) {
        return status;
    }
    return LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert);
}

QStatus PermissionMgmtObj::GetAllMembershipCerts(MembershipCertMap& certMap, bool loadCert)
{
    /* look for memberships head in the key store */
    GUID128 membershipHead(0);
    GetACLGUID(ENTRY_MEMBERSHIPS, membershipHead);

    KeyBlob headerBlob;
    QStatus status = ca->GetKey(membershipHead, headerBlob);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return ER_OK;  /* nothing to do */
    }
    GUID128* guids = NULL;
    size_t numOfGuids;
    status = ca->GetKeys(membershipHead, &guids, &numOfGuids);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::GetAllMembershipCerts failed to retrieve the list of membership certificates.  Status 0x%x", status));
        return status;
    }
    if (numOfGuids == 0) {
        return ER_OK;
    }
    for (size_t cnt = 0; cnt < numOfGuids; cnt++) {
        KeyBlob kb;
        status = ca->GetKey(guids[cnt], kb);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionMgmtObj::GetAllMembershipCerts error looking for membership certificate"));
            delete [] guids;
            return status;
        }
        MembershipCertificate* cert = NULL;
        if (loadCert) {
            cert = new MembershipCertificate();
            status = LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), *cert);
            if (ER_OK != status) {
                QCC_DbgPrintf(("PermissionMgmtObj::GetAllMembershipCerts error loading membership certificate"));
                delete [] guids;
                return status;
            }
        }
        certMap[guids[cnt]] = cert;
    }
    delete [] guids;
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
 *                    |          |
 *      +-------------+          |
 *      |                        |
 *   +-----------+           +-----------+
 *   | leaf cert |           | leaf cert |
 *   +-----------+           +-----------+
 *      |                      |        |
 * +-----------+       +-----------+   +-----------------+
 * | auth data |       | auth data |   | cert 1 in chain |
 * +-----------+       +-----------+   +-----------------+
 *                                       |            |
 *                                 +-----------+   +-----------------+
 *                                 | auth data |   | cert 2 in chain |
 *                                 +-----------+   +-----------------+
 *                                                    |
 *                                               +-----------+
 *                                               | auth data |
 *                                               +-----------+
 *
 */

static QStatus LoadMembershipMetaData(BusAttachment& bus, CredentialAccessor* ca, const GUID128& entryGuid, MembershipCertificate& cert, PermissionPolicy& authData, std::vector<_PeerState::MembershipMetaPair*>& certChain)
{
    QStatus status = ReadMembershipCert(ca, entryGuid, cert);
    if (ER_OK != status) {
        return status;
    }

    /* look into its associated nodes */
    GUID128* guids = NULL;
    size_t numOfGuids;
    status = ca->GetKeys((GUID128 &)entryGuid, &guids, &numOfGuids);
    if (ER_OK != status) {
        delete [] guids;
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            return ER_OK; /* nothing to generate */
        }
        return status;
    }

    /* go through the associated entries of this guid */
    status = ER_OK;
    for (size_t cnt = 0; cnt < numOfGuids; cnt++) {
        KeyBlob kb;
        status = ca->GetKey(guids[cnt], kb);
        if (ER_OK != status) {
            break;
        }

        /* check the tag */
        if (kb.GetTag() == AUTH_DATA_TAG_NAME) {
            Message tmpMsg(bus);
            DefaultPolicyMarshaller marshaller(tmpMsg);
            status = authData.Import(marshaller, kb.GetData(), kb.GetSize());
            if (ER_OK != status) {
                delete [] guids;
                return status;
            }
        } else if (kb.GetTag() == CERT_CHAIN_TAG_NAME) {
            _PeerState::MembershipMetaPair* metaPair = new _PeerState::MembershipMetaPair();
            certChain.push_back(metaPair);
            status = LoadMembershipMetaData(bus, ca, guids[cnt], metaPair->cert, metaPair->authData, certChain);
            if (ER_OK != status) {
                delete [] guids;
                certChain.pop_back();
                delete metaPair;
                return status;
            }
        }
    }
    delete [] guids;
    return status;
}

QStatus PermissionMgmtObj::LocalMembershipsChanged()
{
    MembershipCertMap certMap;
    QStatus status = GetAllMembershipCerts(certMap, false);
    if (ER_OK != status) {
        return status;
    }
    /* clear the local guild map */
    _PeerState::ClearGuildMap(guildMap);
    if (certMap.size() == 0) {
        return ER_OK;
    }

    for (MembershipCertMap::iterator it = certMap.begin(); it != certMap.end(); it++) {

        _PeerState::GuildMetadata* meta = new _PeerState::GuildMetadata();
        status = LoadMembershipMetaData(bus, ca, it->first, meta->cert, meta->authData, meta->certChain);
        if (ER_OK != status) {
            delete meta;
            ClearMembershipCertMap(certMap);
            return status;
        }
        guildMap[it->first.ToString()] = meta;
    }
    ClearMembershipCertMap(certMap);
    return ER_OK;
}

static QStatus GenerateSendMembershipForCertEntry(BusAttachment& bus, CredentialAccessor* ca, MembershipCertificate& cert, GUID128& entryGuid, std::vector<MsgArg*>& argList)
{
    GUID128* guids = NULL;
    size_t numOfGuids;
    QStatus status = ca->GetKeys(entryGuid, &guids, &numOfGuids);
    if (ER_OK != status) {
        delete [] guids;
        if (ER_BUS_KEY_UNAVAILABLE == status) {
            return ER_OK; /* nothing to generate */
        }
        return status;
    }

    /* go through the associated entries of this guid */
    for (size_t cnt = 0; cnt < numOfGuids; cnt++) {
        KeyBlob kb;
        status = ca->GetKey(guids[cnt], kb);
        if (ER_OK != status) {
            break;
        }
        /* check the tag */
        if (kb.GetTag() == AUTH_DATA_TAG_NAME) {
            Message tmpMsg(bus);
            DefaultPolicyMarshaller marshaller(tmpMsg);
            PermissionPolicy authData;
            status = authData.Import(marshaller, kb.GetData(), kb.GetSize());
            if (ER_OK != status) {
                delete [] guids;
                return status;
            }
            MsgArg authDataArg;
            status = authData.Export(authDataArg);
            if (ER_OK != status) {
                delete [] guids;
                return status;
            }

            MsgArg toBeCopied("(ayay(v))", cert.GetSerial().size(), cert.GetSerial().data(), GUID128::SIZE, cert.GetIssuer().GetBytes(), &authDataArg);
            MsgArg* msgArg = new MsgArg("(yv)", SEND_AUTH_DATA, new MsgArg(toBeCopied));
            msgArg->SetOwnershipFlags(MsgArg::OwnsArgs, true);
            argList.push_back(msgArg);
        } else if (kb.GetTag() == CERT_CHAIN_TAG_NAME) {
            MsgArg chainCertArg("(yay)", Certificate::ENCODING_X509_DER,  kb.GetSize(), kb.GetData());
            MsgArg toBeCopied("(ayay(v))", cert.GetSerial().size(), cert.GetSerial().data(), GUID128::SIZE, cert.GetIssuer().GetBytes(), &chainCertArg);
            MsgArg* msgArg = new MsgArg("(yv)", SEND_CERT_CHAIN, new MsgArg(toBeCopied));
            msgArg->SetOwnershipFlags(MsgArg::OwnsArgs, true);
            argList.push_back(msgArg);

            /* this cert chain node may have a parent cert */
            MembershipCertificate chainCert;
            status = LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), chainCert);
            if (ER_OK != status) {
                delete [] guids;
                return status;
            }
            status = GenerateSendMembershipForCertEntry(bus, ca, chainCert, guids[cnt], argList);
            if (ER_OK != status) {
                delete [] guids;
                return status;
            }
        }
    }
    delete [] guids;
    return ER_OK;
}

QStatus PermissionMgmtObj::GenerateSendMemberships(MsgArg** args, size_t* count)
{
    *args = NULL;
    *count = 0;
    MembershipCertMap certMap;
    QStatus status = GetAllMembershipCerts(certMap);
    if (ER_OK != status) {
        return status;
    }
    if (certMap.size() == 0) {
        return ER_OK;
    }

    std::vector<MsgArg*> argList;
    for (MembershipCertMap::iterator it = certMap.begin(); it != certMap.end(); it++) {
        MembershipCertificate* cert = it->second;
        String der;
        status = cert->EncodeCertificateDER(der);
        if (ER_OK != status) {
            ClearMembershipCertMap(certMap);
            return status;
        }
        MsgArg toBeCopied("(yay)", Certificate::ENCODING_X509_DER, der.length(), der.c_str());
        /* use a copy constructor of MsgArg to get a deep copy of all the array data */
        MsgArg* msgArg = new MsgArg("(yv)", SEND_CERT, new MsgArg(toBeCopied));
        msgArg->SetOwnershipFlags(MsgArg::OwnsArgs, true);
        argList.push_back(msgArg);

        status = GenerateSendMembershipForCertEntry(bus, ca, *cert, (GUID128 &)it->first, argList);
        if (ER_OK != status) {
            ClearArgVector(argList);
            ClearMembershipCertMap(certMap);
            return status;
        }
    }
    *count = argList.size();
    MsgArg* retArgs = new MsgArg[*count];
    size_t idx = 0;
    for (std::vector<MsgArg*>::iterator it = argList.begin(); it != argList.end(); idx++, it++) {
        uint8_t sendCode = ((idx + 1) == *count) ? SEND_MBRSHIP_LAST : SEND_MBRSHIP_MORE;
        retArgs[idx].Set("(yv)", sendCode, *it);
        retArgs[idx].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    argList.clear();  /* the members of the vector are assigned to the retArgs array already */
    ClearMembershipCertMap(certMap);
    *args = retArgs;
    return ER_OK;
}

QStatus PermissionMgmtObj::ParseSendMemberships(Message& msg, bool& done)
{
    done = false;
    MsgArg* varArray;
    size_t count;
    QStatus status = msg->GetArg(0)->Get("a(yv)", &count, &varArray);
    if (ER_OK != status) {
        return status;
    }
    if (count == 0) {
        return ER_OK;
    }

    PeerState peerState =  bus.GetInternal().GetPeerStateTable()->GetPeerState(msg->GetSender());
    bool needValidation = false;
    for (size_t idx = 0; idx < count; idx++) {
        uint8_t sendCode;
        MsgArg* entryArg;
        status = varArray[idx].Get("(yv)", &sendCode, &entryArg);
        if (ER_OK != status) {
            return status;
        }
        if (sendCode == SEND_MBRSHIP_NONE) {
            /* the send informs that it does not have any membership cert */
            done = true;
            return ER_OK;
        }
        uint8_t type;
        MsgArg* arg;
        status = entryArg->Get("(yv)", &type, &arg);
        if (ER_OK != status) {
            return status;
        }
        _PeerState::GuildMetadata* meta;
        switch (type) {
        case SEND_CERT:
            {
                meta = new _PeerState::GuildMetadata();
                status = LoadX509CertFromMsgArg(*arg, meta->cert);
                if (ER_OK != status) {
                    delete meta;
                    return status;
                }
                peerState->SetGuildMetadata(meta->cert.GetSerial(), meta->cert.GetIssuer(), meta);
            }
            break;

        case SEND_CERT_CHAIN:
        case SEND_AUTH_DATA:
            {
                uint8_t*serial;
                size_t serialLen;
                uint8_t* issuer;
                size_t issuerLen;
                MsgArg* variantArg;
                status = arg->Get("(ayay(v))", &serialLen, &serial, &issuerLen, &issuer, &variantArg);
                if (ER_OK != status) {
                    return status;
                }
                if (issuerLen != GUID128::SIZE) {
                    return ER_INVALID_DATA;
                }

                String serialNum((const char*) serial, serialLen);
                GUID128 issuerGUID(0);
                issuerGUID.SetBytes(issuer);
                /* look for the membership cert in peer state */
                meta = peerState->GetGuildMetadata(serialNum, issuerGUID);
                if (!meta) {
                    return ER_CERTIFICATE_NOT_FOUND;
                }

                if (type == SEND_CERT_CHAIN) {
                    _PeerState::MembershipMetaPair* metaPair = new _PeerState::MembershipMetaPair();
                    status = LoadX509CertFromMsgArg(*variantArg, metaPair->cert);
                    if (ER_OK != status) {
                        delete metaPair;
                        return status;
                    }

                    meta->certChain.push_back(metaPair);
                } else if (type == SEND_AUTH_DATA) {
                    MembershipCertificate* targetCert = NULL;
                    PermissionPolicy* targetAuthData = NULL;
                    if ((meta->cert.GetSerial() == serialNum) && (meta->cert.GetIssuer() == issuerGUID)) {
                        targetCert = &meta->cert;
                        targetAuthData = &meta->authData;
                    } else {
                        for (std::vector<_PeerState::MembershipMetaPair*>::iterator ccit = meta->certChain.begin(); ccit != meta->certChain.end(); ccit++) {
                            if (((*ccit)->cert.GetSerial() == serialNum) && ((*ccit)->cert.GetIssuer() == issuerGUID)) {
                                targetCert = &(*ccit)->cert;
                                targetAuthData = &(*ccit)->authData;
                                break;
                            }
                        }
                    }
                    if (targetCert) {
                        status = LoadAndValidateAuthDataUsingCert(bus, *variantArg, *targetAuthData,  *targetCert);
                        if (ER_OK != status) {
                            return status;
                        }
                    } else {
                        return ER_CERTIFICATE_NOT_FOUND;
                    }
                }
            }
            break;
        }
        if (sendCode == SEND_MBRSHIP_LAST) {
            needValidation = true;
        }
    }
    if (needValidation) {
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
            LoadGuildAuthorities(*policy, guildAuthorities);
        }

        while (!peerState->guildMap.empty()) {
            bool verified = true;
            for (_PeerState::GuildMap::iterator it = peerState->guildMap.begin(); it != peerState->guildMap.end(); it++) {
                _PeerState::GuildMetadata* metadata = it->second;
                /* validate the membership cert subject public key to match the
                    peer's public key to make sure it sends its own certs. */
                if (memcmp(&peerPublicKey, metadata->cert.GetSubjectPublicKey(), sizeof(ECCPublicKey)) != 0) {
                    /* remove this membership cert since it is not valid */
                    peerState->guildMap.erase(it);
                    delete metadata;
                    verified = false;
                    break;
                }
                /* build the vector of certs to verify.  The membership cert is the leaf node -- first item on the vector */
                std::vector<CertificateX509*> certsToVerify;
                certsToVerify.reserve(metadata->certChain.size() + 1);
                certsToVerify.assign(1, &metadata->cert);
                int cnt = 1;
                for (std::vector<_PeerState::MembershipMetaPair*>::iterator ccit = metadata->certChain.begin(); ccit != metadata->certChain.end(); ccit++) {
                    certsToVerify.insert(certsToVerify.begin() + cnt, &((*ccit)->cert));
                }
                /* check against the trust anchors */
                status = ValidateCertificateChain(false, certsToVerify, &trustAnchors);
                if (ER_OK != status) {
                    /* check against the guild authorities */
                    status = ValidateCertificateChain(true, certsToVerify, &guildAuthorities);
                }
                if (ER_OK != status) {
                    /* remove this membership cert since it is not valid */
                    peerState->guildMap.erase(it);
                    verified = false;
                    break;
                }
            }
            if (verified) {
                break;  /* done */
            }
        }
        guildAuthorities.clear();
        done = true;
    }
    return ER_OK;
}

void PermissionMgmtObj::InstallGuildEquivalence(const InterfaceDescription::Member* member, Message& msg)
{
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    QStatus status = msg->GetArg(0)->Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallGuildEquivalence failed to retrieve PEM status 0x%x", status));
        MethodReply(msg, status);
        return;
    }
    if ((encoding != Certificate::ENCODING_X509_DER) && (encoding != Certificate::ENCODING_X509_DER_PEM)) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallGuildEquivalence does not support encoding %d", encoding));
        MethodReply(msg, ER_NOT_IMPLEMENTED);
        return;
    }
    /* store the InstallGuildEquivalence PEM  into the key store */
    GUID128 headerGuid;
    GetACLGUID(ENTRY_EQUIVALENCES, headerGuid);
    KeyBlob kb(encoded, encodedLen, KeyBlob::GENERIC);

    KeyBlob headerBlob;
    status = ca->GetKey(headerGuid, headerBlob);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        /* make the header guid */
        status = ca->AddAssociatedKey(headerGuid, headerGuid, kb);
    } else {
        /* add the new cert as an associate node of the Guild Equivalence header node */
        GUID128 associateGuid;
        status = ca->AddAssociatedKey(headerGuid, associateGuid, kb);
    }
    MethodReply(msg, status);
}

QStatus PermissionMgmtObj::StoreConfiguration(const Configuration& config)
{
    /* store the message into the key store */
    GUID128 configGuid(0);
    GetACLGUID(ENTRY_CONFIGURATION, configGuid);
    KeyBlob kb((uint8_t*) &config, sizeof(Configuration), KeyBlob::GENERIC);
    return ca->StoreKey(configGuid, kb);
}

QStatus PermissionMgmtObj::GetConfiguration(Configuration& config)
{
    KeyBlob kb;
    GUID128 guid(0);
    GetACLGUID(ENTRY_CONFIGURATION, guid);
    QStatus status = ca->GetKey(guid, kb);
    if (ER_OK != status) {
        return status;
    }
    if (kb.GetSize() != sizeof(Configuration)) {
        return ER_INVALID_DATA;
    }
    memcpy(&config, kb.GetData(), kb.GetSize());
    return ER_OK;
}

PermissionConfigurator::ClaimableState PermissionMgmtObj::GetClaimableState()
{
    return claimableState;
}

QStatus PermissionMgmtObj::SetClaimable(bool claimable)
{
    if (claimableState == PermissionConfigurator::STATE_CLAIMED) {
        return ER_INVALID_CLAIMABLE_STATE;
    }

    PermissionConfigurator::ClaimableState newState;
    if (claimable) {
        newState = PermissionConfigurator::STATE_CLAIMABLE;
    } else {
        newState = PermissionConfigurator::STATE_UNCLAIMABLE;
    }
    /* save the configuration */
    Configuration config;
    config.claimableState = (uint8_t) newState;
    QStatus status = StoreConfiguration(config);
    if (ER_OK == status) {
        claimableState = newState;
        NotifyConfig();
    }
    return status;
}

QStatus PermissionMgmtObj::PerformReset(bool keepForClaim)
{
    bus.GetInternal().GetKeyStore().Reload();
    GUID128 guid;
    GetACLGUID(ENTRY_TRUST_ANCHOR, guid);
    QStatus status = ca->DeleteKey(guid);
    if (ER_OK != status) {
        return status;
    }
    ClearTrustAnchors();
    if (!keepForClaim) {
        ca->GetLocalGUID(KeyBlob::DSA_PRIVATE, guid);
        status = ca->DeleteKey(guid);
        if (status != ER_OK) {
            return status;
        }
        ca->GetLocalGUID(KeyBlob::DSA_PUBLIC, guid);
        status = ca->DeleteKey(guid);
        if (status != ER_OK) {
            return status;
        }
    }
    ca->GetLocalGUID(KeyBlob::PEM, guid);
    status = ca->DeleteKey(guid);
    if (status != ER_OK) {
        return status;
    }

    GetACLGUID(ENTRY_IDENTITY, guid);
    status = ca->DeleteKey(guid);
    if (ER_OK != status) {
        return status;
    }
    GetACLGUID(ENTRY_POLICY, guid);
    status = ca->DeleteKey(guid);
    if (ER_OK != status) {
        return status;
    }

    GetACLGUID(ENTRY_MEMBERSHIPS, guid);
    status = ca->DeleteKey(guid);
    if (ER_OK != status) {
        return status;
    }
    if (ER_OK == status) {
        serialNum = 0;
        PolicyChanged(NULL);
    }
    GetACLGUID(ENTRY_EQUIVALENCES, guid);
    status = ca->DeleteKey(guid);
    if (ER_OK != status) {
        return status;
    }
    GetACLGUID(ENTRY_CONFIGURATION, guid);
    status = ca->DeleteKey(guid);
    if (ER_OK != status) {
        return status;
    }

    /* clear out all the peer entries for all ECDHE key exchanges */
    bus.GetInternal().GetKeyStore().Clear(String(ECDHE_NAME_PREFIX_PATTERN));

    claimableState = PermissionConfigurator::STATE_CLAIMABLE;
    serialNum = 0;
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
    MethodReply(msg, Reset());
}

QStatus PermissionMgmtObj::GetConnectedPeerPublicKey(const GUID128& guid, qcc::ECCPublicKey* publicKey)
{
    CredentialAccessor ca(bus);
    KeyBlob kb;
    QStatus status = ca.GetKey(guid, kb);
    if (ER_OK != status) {
        return status;
    }
    KeyBlob msBlob;
    return KeyExchanger::ParsePeerSecretRecord(kb, msBlob, publicKey);
}

QStatus PermissionMgmtObj::SetManifest(PermissionPolicy::Rule* rules, size_t count)
{
    if (count == 0) {
        return ER_OK;
    }
    PermissionPolicy policy;
    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];
    terms[0].SetRules(count, rules);
    policy.SetTerms(1, terms);

    uint8_t* buf = NULL;
    size_t size;
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    QStatus status = policy.Export(marshaller, &buf, &size);
    terms[0].SetRules(0, NULL); /* does not manage the lifetime of the input rules */
    if (ER_OK != status) {
        return status;
    }
    /* store the message into the key store */
    GUID128 manifestGuid(0);
    GetACLGUID(ENTRY_MANIFEST, manifestGuid);
    KeyBlob kb((uint8_t*) buf, size, KeyBlob::GENERIC);
    delete [] buf;

    return status = ca->StoreKey(manifestGuid, kb);
}

void PermissionMgmtObj::GetManifest(const InterfaceDescription::Member* member, Message& msg)
{
    KeyBlob kb;
    GUID128 guid(0);
    GetACLGUID(ENTRY_MANIFEST, guid);
    QStatus status = ca->GetKey(guid, kb);
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
    if (policy.GetTermsSize() == 0) {
        MethodReply(msg, ER_MANIFEST_NOT_FOUND);
        return;
    }
    PermissionPolicy::Term* terms = (PermissionPolicy::Term*) policy.GetTerms();
    MsgArg rulesArg;
    status = PermissionPolicy::GenerateRules(terms[0].GetRules(), terms[0].GetRulesSize(), rulesArg);
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }

    MsgArg replyArgs[1];
    replyArgs[0].Set("(yv)", MANIFEST_TYPE_ALLJOYN, &rulesArg);
    MethodReply(msg, replyArgs, ArraySize(replyArgs));
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
    QStatus status = CertECCUtil_GetCertCount(certChainPEM, &count);
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
    for (size_t cnt = 0; cnt < count; cnt++) {
        if (IsTrustAnchor(certChain[cnt].GetIssuer())) {
            authorized = true;
            break;
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
            /* build the cert chain based on the identity cert */
            if ((credMask & AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                String der((const char*) kb.GetData(), kb.GetSize());
                String pem;
                CertificateX509::EncodeCertificatePEM(der, pem);
                credentials.SetCertChain(pem);
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
        if (handled && !authorized) {
            return false;
        }
    }
    return ProtectedAuthListener::VerifyCredentials(authMechanism, peerName, credentials);
}

void PermissionMgmtObj::ObjectRegistered(void)
{
    /* Bind to the reserved port for PermissionMgmt */
    BindPort();
    /* notify others */
    PermissionPolicy* policy = new PermissionPolicy();
    QStatus status = RetrievePolicy(*policy);
    if (ER_OK == status) {
        serialNum = policy->GetSerialNum();
    } else {
        serialNum = 0;
        delete policy;
        policy = NULL;
    }
    PolicyChanged(policy);
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

} /* namespace ajn */

