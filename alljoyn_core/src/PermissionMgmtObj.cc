/**
 * @file
 * This file implements the PermissionMgmt interface.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
 * The manifest type ID
 */

#define MANIFEST_TYPE_ALLJOYN 1

using namespace std;
using namespace qcc;

namespace ajn {

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
        AddMethodHandler(ifc->GetMember("RemoveIdentity"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::RemoveIdentity));
        AddMethodHandler(ifc->GetMember("InstallMembership"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::InstallMembership));
        AddMethodHandler(ifc->GetMember("InstallMembershipAuthData"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::InstallMembershipAuthData));
        AddMethodHandler(ifc->GetMember("RemoveMembership"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::RemoveMembership));
        AddMethodHandler(ifc->GetMember("InstallGuildEquivalence"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::InstallGuildEquivalence));
        AddMethodHandler(ifc->GetMember("GetManifest"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtObj::GetManifest));
    }
    /* Add org.allseen.Security.PermissionMgmt.Notification interface */
    const InterfaceDescription* notificationIfc = bus.GetInterface(org::allseen::Security::PermissionMgmt::Notification::InterfaceName);
    if (notificationIfc) {
        AddInterface(*notificationIfc);
        notifySignalName = notificationIfc->GetMember("NotifyConfig");
    }

    ca = new CredentialAccessor(bus);
    bus.GetInternal().GetPermissionManager().SetPermissionMgmtObj(this);

    QStatus status = LoadTrustAnchors();
    if (ER_OK == status) {
        claimableState = STATE_CLAIMED;
    } else {
        claimableState = STATE_CLAIMABLE;
    }

    bus.RegisterBusObject(*this, true);
}

PermissionMgmtObj::~PermissionMgmtObj()
{
    delete ca;
    ClearTrustAnchors();
    if (portListener) {
        bus.UnbindSessionPort(ALLJOYN_SESSIONPORT_PERMISSION_MGMT);
        delete portListener;
    }
}

void PermissionMgmtObj::PolicyChanged(PermissionPolicy* policy)
{
    qcc::GUID128 localGUID;
    ca->GetGuid(localGUID);
    bus.GetInternal().GetPermissionManager().SetPolicy(policy);
    NotifyConfig();
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
        if (memcmp((*it)->GetKeyId(), keyInfo->GetKeyId(), (*it)->GetKeyIdLen()) != 0) {
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
        bufferSize += sizeof(size_t);  /* account for the item size */
        bufferSize += (*it)->GetExportSize();
    }
    uint8_t* buffer = new uint8_t[bufferSize];
    uint8_t* pBuf = buffer;
    /* the count field */
    *pBuf = (uint8_t) trustAnchors.size();
    pBuf += sizeof(uint8_t);
    for (TrustAnchorList::iterator it = trustAnchors.begin(); it != trustAnchors.end(); it++) {
        size_t* itemSize = (size_t*) pBuf;
        *itemSize = (*it)->GetExportSize();
        pBuf += sizeof(size_t);
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
        return status;
    }
    /* the format of the persistent buffer:
     * count
     * size:trust anchor
     * size:trust anchor
     * ...
     */

    ClearTrustAnchors();

    uint8_t* pBuf = (uint8_t*) kb.GetData();
    uint8_t count = *pBuf;
    pBuf += sizeof(uint8_t);
    for (size_t cnt = 0; cnt < count; cnt++) {
        size_t* itemSize = (size_t*) pBuf;
        pBuf += sizeof(size_t);
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

QStatus PermissionMgmtObj::StoreDSAKeys(const ECCPrivateKey* privateKey, const ECCPublicKey* publicKey)
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

QStatus PermissionMgmtObj::RetrieveDSAPublicKey(ECCPublicKey* publicKey)
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

QStatus PermissionMgmtObj::RetrieveDSAPrivateKey(ECCPrivateKey* privateKey)
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

void PermissionMgmtObj::KeyInfoNISTP256ToMsgArg(KeyInfoNISTP256& keyInfo, MsgArg& variant)
{
    MsgArg coordArg("(ayay)", ECC_COORDINATE_SZ, keyInfo.GetXCoord(), ECC_COORDINATE_SZ, keyInfo.GetYCoord());

    variant.Set("(yv)", KeyInfo::FORMAT_ALLJOYN,
                new MsgArg("(ayyyv)", keyInfo.GetKeyIdLen(), keyInfo.GetKeyId(), KeyInfo::USAGE_SIGNING, KeyInfoECC::KEY_TYPE,
                           new MsgArg("(yyv)", keyInfo.GetAlgorithm(), keyInfo.GetCurve(), new MsgArg(coordArg))));
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
}

QStatus PermissionMgmtObj::MsgArgToKeyInfoNISTP256(MsgArg& variant, KeyInfoNISTP256& keyInfo)
{
    QStatus status;
    uint8_t keyFormat;
    MsgArg* variantArg;
    status = variant.Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if (keyFormat != KeyInfo::FORMAT_ALLJOYN) {
        return ER_INVALID_DATA;
    }
    uint8_t* kid;
    size_t kidLen;
    uint8_t keyUsageType;
    uint8_t keyType;
    MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &kidLen, &kid, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    keyInfo.SetKeyId(kid, kidLen);
    if ((keyUsageType != KeyInfo::USAGE_SIGNING) && (keyUsageType != KeyInfo::USAGE_ENCRYPTION)) {
        return ER_INVALID_DATA;
    }
    if (keyType != KeyInfoECC::KEY_TYPE) {
        return ER_INVALID_DATA;
    }
    uint8_t algorithm;
    uint8_t curve;
    MsgArg* curveVariant;
    status = keyVariantArg->Get("(yyv)", &algorithm, &curve, &curveVariant);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if (curve != Crypto_ECC::ECC_NIST_P256) {
        return ER_INVALID_DATA;
    }

    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    status = curveVariant->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
        return ER_INVALID_DATA;
    }
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);
    return ER_OK;
}

void PermissionMgmtObj::Claim(const InterfaceDescription::Member* member, Message& msg)
{
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    QStatus status = MsgArgToKeyInfoNISTP256((MsgArg &) * msg->GetArg(0), *keyInfo);
    if (ER_OK != status) {
        delete keyInfo;
        MethodReply(msg, status);
        return;
    }

    uint8_t* guid;
    size_t guidLen;
    status = msg->GetArg(1)->Get("ay", &guidLen, &guid);
    if (ER_OK != status) {
        delete keyInfo;
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if (guidLen != GUID128::SIZE) {
        delete keyInfo;
        MethodReply(msg, ER_INVALID_GUID);
        return;
    }
    MsgArg replyArgs[1];
    KeyStore& keyStore = bus.GetInternal().GetKeyStore();

    GUID128 newGUID;

    newGUID.SetBytes(guid);
    keyStore.ResetMasterGUID(newGUID);
    /* install trust anchor */
    qcc::GUID128 peerGuid;
    status = GetPeerGUID(msg, peerGuid);
    if (ER_OK != status) {
        delete keyInfo;
        MethodReply(msg, status);
        return;
    }

    status = InstallTrustAnchor(keyInfo);
    if (ER_OK != status) {
        delete keyInfo;
        QCC_DbgPrintf(("PermissionMgmtObj::Claim failed to store trust anchor"));
        MethodReply(msg, ER_PERMISSION_DENIED);
        return;
    }

    Crypto_ECC ecc;
    status = ecc.GenerateDSAKeyPair();
    if (status != ER_OK) {
        MethodReply(msg, ER_CRYPTO_KEY_UNAVAILABLE);
        return;
    }
    status = StoreDSAKeys(ecc.GetDSAPrivateKey(), ecc.GetDSAPublicKey());
    if (status != ER_OK) {
        MethodReply(msg, ER_CRYPTO_KEY_UNAVAILABLE);
        return;
    }

    ECCPublicKey pubKey;
    status = RetrieveDSAPublicKey(&pubKey);
    if (status != ER_OK) {
        MethodReply(msg, status);
        return;
    }
    claimableState = STATE_CLAIMED;

    KeyInfoNISTP256 replyKeyInfo;
    replyKeyInfo.SetKeyId(newGUID.GetBytes(), GUID128::SIZE);
    replyKeyInfo.SetPublicKey(&pubKey);
    KeyInfoNISTP256ToMsgArg(replyKeyInfo, replyArgs[0]);

    MethodReply(msg, replyArgs, ArraySize(replyArgs));

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

    status = ca->StoreKey(policyGuid, kb);
    QCC_DbgPrintf(("PermissionMgmtObj::StorePolicy save message size %d to key store return status 0x%x\n", size, status));
    if (ER_OK != status) {
        return status;
    }

    return ER_OK;
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

    qcc::GUID128 localGUID;
    QStatus status = ca->GetGuid(localGUID);
    if (ER_OK != status) {
        return status;
    }

    MsgArg args[4];
    args[0].Set("ay", localGUID.SIZE, localGUID.GetBytes());
    args[1].Set("y", claimableState);
    args[2].Set("u", serialNum);
    args[3].Set("a(ayay)", 0, NULL);
    return Signal(NULL, 0, *notifySignalName, args, 4, 0, flags);
}

static QStatus ValidateCertificate(CertificateX509& cert, PermissionMgmtObj::TrustAnchorList* taList)
{
    for (PermissionMgmtObj::TrustAnchorList::iterator it = taList->begin(); it != taList->end(); it++) {
        KeyInfoNISTP256* keyInfo = *it;
        if (cert.Verify(*keyInfo) == ER_OK) {
            return ER_OK;  /* cert is verified */
        }
    }
    return ER_UNKNOWN_CERTIFICATE;
}

static QStatus ValidateCertificateChain(std::vector<CertificateX509*>& certs, PermissionMgmtObj::TrustAnchorList* taList)
{
    size_t idx = 0;
    bool validated = false;
    for (std::vector<CertificateX509*>::iterator it = certs.begin(); it != certs.end(); it++) {
        idx++;
        QStatus status = ValidateCertificate(*(*it), taList);
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

static QStatus LoadCertificate(Certificate::EncodingType encoding, const uint8_t* encoded, size_t encodedLen, CertificateX509& cert, PermissionMgmtObj::TrustAnchorList* taList)
{
    QStatus status;
    if (encoding == Certificate::ENCODING_X509_DER) {
        status = cert.DecodeCertificateDER(String((const char*) encoded, encodedLen));
    } else if (encoding == Certificate::ENCODING_X509_DER_PEM) {
        status = cert.DecodeCertificatePEM(String((const char*) encoded, encodedLen));
    } else {
        return ER_NOT_IMPLEMENTED;
    }
    if (ER_OK != status) {
        return status;
    }
    /* verify its signature if requested */
    if (!taList) {
        return ER_OK;
    }
    return ValidateCertificate(cert, taList);
}

void PermissionMgmtObj::InstallIdentity(const InterfaceDescription::Member* member, Message& msg)
{
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    QStatus status = msg->GetArg(0)->Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallIdentity failed to retrieve PEM status 0x%x", status));
        MethodReply(msg, status);
        return;
    }
    if ((encoding != Certificate::ENCODING_X509_DER) && (encoding != Certificate::ENCODING_X509_DER_PEM)) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallIdentity does not support encoding %d", encoding));
        MethodReply(msg, ER_NOT_IMPLEMENTED);
        return;
    }
    IdentityCertificate cert;
    status = LoadCertificate((Certificate::EncodingType) encoding, encoded, encodedLen, cert, &trustAnchors);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::InstallIdentity failed to validate certificate status 0x%x", status));
        MethodReply(msg, status);
        return;
    }
    GUID128 guid(cert.GetSubject());
    /* store the Identity PEM  into the key store */
    GetACLGUID(ENTRY_IDENTITY, guid);
    KeyBlob kb(encoded, encodedLen, KeyBlob::GENERIC);

    status = ca->StoreKey(guid, kb);
    MethodReply(msg, status);
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

void PermissionMgmtObj::RemoveIdentity(const InterfaceDescription::Member* member, Message& msg)
{
    GUID128 guid;
    GetACLGUID(ENTRY_IDENTITY, guid);
    QStatus status = ca->DeleteKey(guid);
    if (ER_BUS_KEY_UNAVAILABLE == status) {
        status = ER_CERTIFICATE_NOT_FOUND;
    }
    MethodReply(msg, status);
}

static QStatus GetMembershipCert(CredentialAccessor* ca, GUID128& guid, MembershipCertificate& cert)
{
    KeyBlob kb;
    QStatus status = ca->GetKey(guid, kb);
    if (ER_OK != status) {
        return status;
    }
    return LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert, NULL);
}

static QStatus GetMembershipGuid(CredentialAccessor* ca, GUID128& membershipHead, const String& serialNum, const GUID128& issuer, GUID128& membershipGuid)
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
            LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), cert, NULL);
            if ((cert.GetSerial() == serialNum) && (cert.GetIssuer() == issuer)) {
                membershipGuid = guids[cnt];
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
    status = LoadCertificate((Certificate::EncodingType) encoding, encoded, encodedLen, cert, NULL);
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
                status = GetMembershipGuid(ca, membershipHead, cert.GetSerial(), cert.GetIssuer(), tmpGuid);
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
    MethodReply(msg, status);
}

QStatus PermissionMgmtObj::LocateMembershipEntry(const String& serialNum, const GUID128& issuer, GUID128& membershipGuid)
{
    /* look for memberships head in the key store */
    GUID128 membershipHead(0);
    GetACLGUID(ENTRY_MEMBERSHIPS, membershipHead);

    KeyBlob headerBlob;
    QStatus status = ca->GetKey(membershipHead, headerBlob);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return status;
    }
    return GetMembershipGuid(ca, membershipHead, serialNum, issuer, membershipGuid);
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
    status = LocateMembershipEntry(serial, issuer, membershipGuid);
    if (ER_OK != status) {
        return status;
    }

    MembershipCertificate cert;
    status = GetMembershipCert(ca, membershipGuid, cert);
    if (ER_OK != status) {
        return status;
    }
    return LoadAndValidateAuthDataUsingCert(bus, authDataArg, authorization, cert);
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
    MethodReply(msg, status);
}

QStatus PermissionMgmtObj::GetAllMembershipCerts(MembershipCertMap& certMap)
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
        MembershipCertificate* cert = new MembershipCertificate();
        status = LoadCertificate(Certificate::ENCODING_X509_DER, kb.GetData(), kb.GetSize(), *cert, NULL);
        if (ER_OK != status) {
            QCC_DbgPrintf(("PermissionMgmtObj::GetAllMembershipCerts error loading membership certificate"));
            delete [] guids;
            return status;
        }
        certMap[guids[cnt]] = cert;
    }
    delete [] guids;
    return ER_OK;
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
        MsgArg* msgArg = new MsgArg("(yv)", SEND_CERT, new MsgArg(toBeCopied));
        msgArg->SetOwnershipFlags(MsgArg::OwnsArgs, true);
        argList.push_back(msgArg);

        GUID128* guids = NULL;
        size_t numOfGuids;
        status = ca->GetKeys((GUID128 &)it->first, &guids, &numOfGuids);
        if (ER_OK != status) {
            delete [] guids;
            ClearArgVector(argList);
            ClearMembershipCertMap(certMap);
            return status;
        }

        /* go through the associated entries of this membership */
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
                    ClearArgVector(argList);
                    ClearMembershipCertMap(certMap);
                    return status;
                }
                MsgArg authDataArg;
                status = authData.Export(authDataArg);
                if (ER_OK != status) {
                    delete [] guids;
                    ClearArgVector(argList);
                    ClearMembershipCertMap(certMap);
                    return status;
                }

                MsgArg toBeCopied("(say(v))", cert->GetSerial().c_str(), GUID128::SIZE, cert->GetIssuer().GetBytes(), &authDataArg);
                msgArg = new MsgArg("(yv)", SEND_AUTH_DATA, new MsgArg(toBeCopied));
                msgArg->SetOwnershipFlags(MsgArg::OwnsArgs, true);
                argList.push_back(msgArg);
            } else if (kb.GetTag() == CERT_CHAIN_TAG_NAME) {
                MsgArg toBeCopied("(say(v))", cert->GetSerial().c_str(), GUID128::SIZE, cert->GetIssuer().GetBytes(), new MsgArg("(yay)", Certificate::ENCODING_X509_DER,  kb.GetSize(), kb.GetData()));
                msgArg = new MsgArg("(yv)", SEND_CERT_CHAIN, new MsgArg(toBeCopied));
                msgArg->SetOwnershipFlags(MsgArg::OwnsArgs, true);
                argList.push_back(msgArg);
            }
        }
        delete [] guids;
    }
    *count = argList.size();
    MsgArg* retArgs = new MsgArg[*count];
    size_t idx = 0;
    for (std::vector<MsgArg*>::iterator it = argList.begin(); it != argList.end(); idx++, it++) {
        retArgs[idx].Set("(yyv)", (idx + 1), *count, *it);
        retArgs[idx].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    argList.clear();  /* the members of the vector are assigned to the retArgs array already */
    ClearMembershipCertMap(certMap);
    *args = retArgs;
    return ER_OK;
}

QStatus PermissionMgmtObj::ParseSendMemberships(Message& msg)
{
    MsgArg* varArray;
    size_t count;
    QStatus status = msg->GetArg(0)->Get("a(yyv)", &count, &varArray);
    if (ER_OK != status) {
        return status;
    }
    if (count == 0) {
        return ER_OK;
    }

    PeerState peerState =  bus.GetInternal().GetPeerStateTable()->GetPeerState(msg->GetSender());
    bool needValidation = false;
    for (size_t idx = 0; idx < count; idx++) {
        uint8_t entry;
        uint8_t numOfEntries;
        MsgArg* entryArg;
        status = varArray[idx].Get("(yyv)", &entry, &numOfEntries, &entryArg);
        if (ER_OK != status) {
            return status;
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
                const char* serial;
                uint8_t* issuer;
                size_t issuerLen;
                MsgArg* variantArg;
                status = arg->Get("(say(v))", &serial, &issuerLen, &issuer, &variantArg);
                if (ER_OK != status) {
                    return status;
                }
                if (issuerLen != GUID128::SIZE) {
                    return ER_INVALID_DATA;
                }

                String serialNum(serial);
                GUID128 issuerGUID(0);
                issuerGUID.SetBytes(issuer);
                /* look for the membership cert in peer state */
                meta = peerState->GetGuildMetadata(serialNum, issuerGUID);
                if (!meta) {
                    return ER_CERTIFICATE_NOT_FOUND;
                }

                if (type == SEND_CERT_CHAIN) {
                    CertificateX509* cert = new CertificateX509();
                    status = LoadX509CertFromMsgArg(*variantArg, *cert);
                    if (ER_OK != status) {
                        return status;
                    }
                    meta->certChain.push_back(cert);
                } else if (type == SEND_AUTH_DATA) {
                    status = LoadAndValidateAuthDataUsingCert(bus, *variantArg, meta->authData, meta->cert);
                    if (ER_OK != status) {
                        return status;
                    }
                }
            }
            break;
        }
        if (entry == numOfEntries) {
            needValidation = true;
        }
    }
    if (needValidation) {
        /* do the membership cert validation for the peer */
        while (!peerState->guildMap.empty()) {
            bool verified = true;
            for (_PeerState::GuildMap::iterator it = peerState->guildMap.begin(); it != peerState->guildMap.end(); it++) {
                _PeerState::GuildMetadata* metadata = it->second;
                /* build the vector of certs to verify.  The membership cert is the leaf node -- first item on the vector */
                std::vector<CertificateX509*> certsToVerify;
                certsToVerify.reserve(metadata->certChain.size() + 1);
                certsToVerify.assign(1, &metadata->cert);
                certsToVerify.insert(certsToVerify.begin() + 1, metadata->certChain.begin(), metadata->certChain.end());
                status = ValidateCertificateChain(certsToVerify, &trustAnchors);
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

static void MakePEM(qcc::String& der, qcc::String& pem)
{
    qcc::String tag1 = "-----BEGIN CERTIFICATE-----\n";
    qcc::String tag2 = "-----END CERTIFICATE-----";
    Crypto_ASN1::EncodeBase64(der, pem);
    pem = tag1 + pem + tag2;
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
                MakePEM(der, pem);
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
    portListener = new PortListener();
    QStatus status = bus.BindSessionPort(sessionPort, opts, *portListener);
    if (ER_OK != status) {
        delete portListener;
        portListener = NULL;
    }
    return status;
}

} /* namespace ajn */

