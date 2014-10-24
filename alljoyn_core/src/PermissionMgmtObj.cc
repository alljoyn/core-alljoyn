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

#include <alljoyn/AllJoynStd.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/CertificateECC.h>
#include "PermissionMgmtObj.h"
#include "PeerState.h"
#include "BusInternal.h"

#define QCC_MODULE "PERMISSION_MGMT"

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
    }
    /* Add org.allseen.Security.PermissionMgmt.Notification interface */
    const InterfaceDescription* notificationIfc = bus.GetInterface(org::allseen::Security::PermissionMgmt::Notification::InterfaceName);
    if (notificationIfc) {
        AddInterface(*notificationIfc);
        notifySignalName = notificationIfc->GetMember("NotifyConfig");
    }

    bus.RegisterBusObject(*this, true);

    ca = new CredentialAccessor(bus);
    bus.GetInternal().GetPermissionManager().SetPermissionMgmtObj(this);

    TrustAnchor ta;
    QStatus status = GetTrustAnchor(ta);
    if (ER_OK == status) {
        claimableState = STATE_CLAIMED;
    } else {
        claimableState = STATE_CLAIMABLE;
    }
    PermissionPolicy policy;
    status = RetrievePolicy(policy);
    if (ER_OK == status) {
        serialNum = policy.GetSerialNum();
    } else {
        serialNum = 0;
    }

    NotifyConfig();
}

PermissionMgmtObj::~PermissionMgmtObj()
{
    delete ca;
    if (portListener) {
        bus.UnbindSessionPort(ALLJOYN_SESSIONPORT_PERMISSION_MGMT);
        delete portListener;
    }
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
    return ER_CRYPTO_KEY_UNAVAILABLE;      /* not available */
}

QStatus PermissionMgmtObj::InstallTrustAnchor(const qcc::GUID128& guid, const uint8_t* pubKey, size_t pubKeyLen)
{
    GUID128 trustAnchorGuid;
    GetACLGUID(ENTRY_TRUST_ANCHOR, trustAnchorGuid);

    if (pubKeyLen != sizeof(ECCPublicKey)) {
        return ER_INVALID_DATA;
    }
    TrustAnchor ta(guid.GetBytes(), (ECCPublicKey*) pubKey);
    KeyBlob kb((uint8_t*) &ta, sizeof(TrustAnchor), KeyBlob::GENERIC);
    kb.SetExpiration(0xFFFFFFFF);  /* never expired */

    QCC_DbgPrintf(("PermissionMgmtObj::InstallTrustAnchor with guid %s and ECCPublicKey %s", trustAnchorGuid.ToString().c_str(),
                   BytesToHexString(pubKey, pubKeyLen).c_str()));
    return ca->StoreKey(trustAnchorGuid, kb);
}

QStatus PermissionMgmtObj::GetTrustAnchor(TrustAnchor& trustAnchor)
{
    QCC_DbgPrintf(("PermissionMgmtObj::GetTrustAnchor from keystore (guid %s)\n",
                   bus.GetInternal().GetKeyStore().GetGuid().c_str()));
    GUID128 trustAnchorGuid;
    GetACLGUID(ENTRY_TRUST_ANCHOR, trustAnchorGuid);
    KeyBlob kb;
    QStatus status = ca->GetKey(trustAnchorGuid, kb);
    if (ER_OK != status) {
        return status;
    }
    memcpy(&trustAnchor, kb.GetData(), kb.GetSize());
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


void PermissionMgmtObj::Claim(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    uint8_t keyFormat;
    MsgArg* variantArg;
    status = msg->GetArg(0)->Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if (keyFormat != KeyInfo::FORMAT_ALLJOYN) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    uint8_t* kid;
    size_t kidLen;
    uint8_t keyUsageType;
    uint8_t keyType;
    MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &kidLen, &kid, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if ((keyUsageType != KeyInfo::USAGE_SIGNING) && (keyUsageType != KeyInfo::USAGE_ENCRYPTION)) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if (keyType != KeyInfoECC::KEY_TYPE) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    uint8_t algorithm;
    uint8_t curve;
    MsgArg* curveVariant;
    status = keyVariantArg->Get("(yyv)", &algorithm, &curve, &curveVariant);
    if (ER_OK != status) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if (curve != Crypto_ECC::ECC_NIST_P256) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }

    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    status = curveVariant->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);

    uint8_t* guid;
    size_t guidLen;
    status = msg->GetArg(1)->Get("ay", &guidLen, &guid);
    if (ER_OK != status) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }
    if (guidLen != GUID128::SIZE) {
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
        MethodReply(msg, status);
        return;
    }
    const ECCPublicKey* adminPubKey = keyInfo.GetPublicKey();
    QCC_DbgPrintf(("PermissionMgmtObj::Claim kid %s peer guid %s",
                   BytesToHexString(kid, kidLen).c_str(),
                   peerGuid.ToString().c_str()));

    status = InstallTrustAnchor(peerGuid, (const uint8_t*) adminPubKey, sizeof(ECCPublicKey));
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionMgmtObj::Claim failed to store trust anchor"));
        MethodReply(msg, ER_PERMISSION_DENIED);
        return;
    }

    TrustAnchor ta;
    status = GetTrustAnchor(ta);
    if (ER_OK != status) {
        MethodReply(msg, status);
        return;
    }

    QCC_DbgPrintf(("Claim: trust anchor guid: %s public key: %s",
                   BytesToHexString(ta.guid, qcc::GUID128::SIZE).c_str(),
                   BytesToHexString((uint8_t*) &ta.publicKey, sizeof(ta.publicKey)).c_str()));

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

    keyInfo.SetPublicKey(&pubKey);
    replyArgs[0].Set("(yv)", keyFormat,
                     new MsgArg("(ayyyv)", GUID128::SIZE, newGUID.GetBytes(), KeyInfo::USAGE_SIGNING, KeyInfoECC::KEY_TYPE,
                                new MsgArg("(yyv)", keyInfo.GetAlgorithm(), keyInfo.GetCurve(),
                                           new MsgArg("(ayay)", ECC_COORDINATE_SZ, keyInfo.GetXCoord(), ECC_COORDINATE_SZ, keyInfo.GetYCoord()))));
    replyArgs[0].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    MethodReply(msg, replyArgs, ArraySize(replyArgs));

    NotifyConfig();
}

void PermissionMgmtObj::InstallPolicy(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    uint8_t version;
    MsgArg* variant;
    msg->GetArg(0)->Get("(yv)", &version, &variant);

    PermissionPolicy policy;
    status = policy.Import(version, *variant);

    status = StorePolicy(policy);
    if (ER_OK == status) {
        serialNum = policy.GetSerialNum();
    }
    MethodReply(msg, status);
    if (ER_OK == status) {
        NotifyConfig();
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
    QStatus status = policy.Export(bus, &buf, &size);
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
    return policy.Import(bus, kb.GetData(), kb.GetSize());
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

void PermissionMgmtObj::ObjectRegistered(void)
{
    /* Bind to the reserved port for PermissionMgmt */
    BindPort();
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

