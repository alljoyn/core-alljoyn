/**
 * @file
 *
 * This file implements the KeyExchanger features
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

#include <qcc/platform.h>
#include <qcc/KeyBlob.h>
#include <qcc/Crypto.h>
#include <qcc/StringUtil.h>
#include <qcc/CertificateECC.h>
#include <qcc/Debug.h>
#include <qcc/time.h>

#include <stdio.h>

#include "KeyExchanger.h"
#include "AllJoynPeerObj.h"
#include "CredentialAccessor.h"

#define QCC_MODULE "AUTH_KEY_EXCHANGER"

using namespace std;
using namespace qcc;

namespace ajn {


#define AUTH_TIMEOUT      120000

/**
 * The legacy auth version with old ECC encoding
 */
#define LEGACY_AUTH_VERSION      2

/**
 * The variant types for the org.alljoyn.Bus.Peer.Authentication KeyExchange method call.
 */
#define EXCHANGE_KEYINFO 1
#define EXCHANGE_TRUST_ANCHORS 2


QStatus KeyExchangerECDHE::GenerateECDHEKeyPair()
{
    return ecc.GenerateDHKeyPair();
}

const ECCPublicKey* KeyExchangerECDHE::GetECDHEPublicKey()
{
    return ecc.GetDHPublicKey();
}
void KeyExchangerECDHE::SetECDHEPublicKey(const ECCPublicKey* publicKey)
{
    ecc.SetDHPublicKey(publicKey);
}

const ECCPrivateKey* KeyExchangerECDHE::GetECDHEPrivateKey()
{
    return ecc.GetDHPrivateKey();
}
void KeyExchangerECDHE::SetECDHEPrivateKey(const ECCPrivateKey* privateKey)
{
    ecc.SetDHPrivateKey(privateKey);
}

const ECCSecret* KeyExchangerECDHE::GetECDHESecret()
{
    return &pms;
}
void KeyExchangerECDHE::SetECDHESecret(const ECCSecret* newSecret)
{
    memcpy(&pms, newSecret, sizeof(ECCSecret));
}

QStatus KeyExchangerECDHE::GenerateECDHESecret(const ECCPublicKey* remotePubKey)
{
    return ecc.GenerateSharedSecret(remotePubKey, &pms);
}

QStatus KeyExchangerECDHE::GenerateMasterSecret()
{
    QStatus status;
    uint8_t keymatter[48];      /* RFC5246 */
    ECCSecretOldEncoding oldenc;
    ecc.ReEncode(&pms, &oldenc);
    KeyBlob pmsBlob((const uint8_t*) &oldenc, sizeof (ECCSecretOldEncoding), KeyBlob::GENERIC);
    status = Crypto_PseudorandomFunction(pmsBlob, "master secret", "", keymatter, sizeof (keymatter));
    masterSecret.Set(keymatter, sizeof(keymatter), KeyBlob::GENERIC);
    return status;
}

void KeyExchanger::ShowCurrentDigest(const char* ref)
{
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    hashUtil.GetDigest(digest, true);
    QCC_DbgHLPrintf(("Current digest [%d] ref[%s]: %s\n", ++showDigestCounter, ref, BytesToHexString(digest, sizeof(digest)).c_str()));
}

QStatus KeyExchangerECDHE::RespondToKeyExchange(Message& msg, MsgArg* variant, uint32_t remoteAuthMask, uint32_t authMask)
{
    QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange"));
    /* hash the handshake data */
    hashUtil.Update(HexStringToByteString(U32ToString(remoteAuthMask, 16, 2 * sizeof (authMask), '0')));
    QStatus status;
    if (IsLegacyPeer()) {
        status = KeyExchangeReadLegacyKey(*variant);
    } else {
        status = KeyExchangeReadKey(*variant);
    }
    if (ER_OK != status) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange received invalid data from peer"));
        return peerObj->HandleMethodReply(msg, ER_INVALID_DATA);
    }

    status = GenerateECDHEKeyPair();
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange failed to generate ECDHE key pair"));
        return peerObj->HandleMethodReply(msg, status);
    }

    status = GenerateECDHESecret(&peerPubKey);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange failed to generate ECDHE secret"));
        return peerObj->HandleMethodReply(msg, status);
    }
    status = GenerateMasterSecret();
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange failed to generate master secret"));
        return peerObj->HandleMethodReply(msg, status);
    }
    /* hash the handshake data */
    hashUtil.Update(HexStringToByteString(U32ToString(authMask, 16, 2 * sizeof (authMask), '0')));

    MsgArg outVariant;
    if (IsLegacyPeer()) {
        KeyExchangeGenLegacyKey(outVariant);
    } else {
        KeyExchangeGenKey(outVariant);
    }
    MsgArg args[2];
    args[0].Set("u", authMask);
    args[1].Set("v", &outVariant);
    return peerObj->HandleMethodReply(msg, args, ArraySize(args));
}

void KeyExchangerECDHE::KeyExchangeGenLegacyKey(MsgArg& variant)
{
    uint8_t buf[1 + sizeof(ECCPublicKeyOldEncoding)];
    buf[0] = ecc.GetCurveType();
    ECCPublicKeyOldEncoding oldenc;
    ecc.ReEncode(GetECDHEPublicKey(), &oldenc);
    memcpy(&buf[1], &oldenc, sizeof(ECCPublicKeyOldEncoding));
    MsgArg localArg("ay", sizeof(buf), buf);
    variant = localArg;
    hashUtil.Update(buf, sizeof(buf));
}

QStatus KeyExchangerECDHE::KeyExchangeReadLegacyKey(MsgArg& variant)
{
    uint8_t* replyPubKey;
    size_t replyPubKeyLen;
    variant.Get("ay", &replyPubKeyLen, &replyPubKey);
    /* the first byte is the ECC curve type */
    if (replyPubKeyLen != (1 + sizeof(ECCPublicKeyOldEncoding))) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::KeyExchangeReadLegacyKey invalid public key size %d expecting 1 + %d\n", replyPubKeyLen, sizeof(ECCPublicKeyOldEncoding)));
        return ER_INVALID_DATA;
    }
    uint8_t eccCurveID = replyPubKey[0];
    if (eccCurveID != ecc.GetCurveType()) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::KeyExchangeReadLegacyKey invalid curve type %d expecting %d\n", eccCurveID, ecc.GetCurveType()));
        return ER_INVALID_DATA;
    }
    ECCPublicKeyOldEncoding oldenc;
    memcpy(&oldenc, &replyPubKey[1], sizeof(ECCPublicKeyOldEncoding));
    ecc.ReEncode(&oldenc, &peerPubKey);
    /* hash the handshake data */
    hashUtil.Update(replyPubKey, replyPubKeyLen);

    return ER_OK;
}

void KeyExchangerECDHE::KeyExchangeGenKeyInfo(MsgArg& variant)
{
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(GetECDHEPublicKey());
    MsgArg* keyInfoVariant = new MsgArg();
    PermissionMgmtObj::KeyInfoNISTP256ToMsgArg(keyInfo, *keyInfoVariant);
    variant.Set("(yv)", EXCHANGE_KEYINFO, keyInfoVariant);
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    hashUtil.Update((uint8_t*) GetECDHEPublicKey(), sizeof(ECCPublicKey));
}

void KeyExchangerECDHE::KeyExchangeGenKey(MsgArg& variant)
{
    MsgArg* entries = new MsgArg[1];
    KeyExchangeGenKeyInfo(entries[0]);
    variant.Set("a(yv)", 1, entries);
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
}

void KeyExchangerECDHE_ECDSA::KeyExchangeGenTrustAnchorKeyInfos(MsgArg& variant)
{
    MsgArg* entries = new MsgArg[trustAnchorList->size()];
    size_t cnt = 0;
    for (PermissionMgmtObj::TrustAnchorList::iterator it = trustAnchorList->begin(); it != trustAnchorList->end(); it++) {
        KeyInfoNISTP256* keyInfo = *it;
        PermissionMgmtObj::KeyInfoNISTP256ToMsgArg(*keyInfo, entries[cnt]);
        hashUtil.Update((uint8_t*) keyInfo->GetPublicKey(), sizeof(ECCPublicKey));
    }
    variant.Set("(yv)", EXCHANGE_TRUST_ANCHORS, new MsgArg("a(yv)", trustAnchorList->size(), entries));
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
}

void KeyExchangerECDHE_ECDSA::KeyExchangeGenKey(MsgArg& variant)
{
    size_t numEntries = 1;
    if (trustAnchorList->size() > 0) {
        numEntries++;
    }
    MsgArg* entries = new MsgArg[numEntries];
    KeyExchangeGenKeyInfo(entries[0]);
    if (numEntries > 1) {
        KeyExchangeGenTrustAnchorKeyInfos(entries[1]);
    }
    variant.Set("a(yv)", numEntries, entries);
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
}

QStatus KeyExchangerECDHE::KeyExchangeReadKeyInfo(MsgArg& variant)
{
    KeyInfoNISTP256 keyInfo;
    QStatus status = PermissionMgmtObj::MsgArgToKeyInfoNISTP256(variant, keyInfo);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::KeyExchangeReadKeyInfo parsing KeyInfo fails status 0x%x\n", status));
        return status;
    }
    memcpy(&peerPubKey, keyInfo.GetPublicKey(), sizeof(ECCPublicKey));
    /* hash the handshake data */
    hashUtil.Update((uint8_t*) &peerPubKey, sizeof(ECCPublicKey));
    return ER_OK;
}

QStatus KeyExchangerECDHE::KeyExchangeReadKey(MsgArg& variant)
{
    size_t entryCount;
    MsgArg* entries;
    QStatus status = variant.Get("a(yv)", &entryCount, &entries);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if (entryCount == 0) {
        return ER_INVALID_DATA;
    }

    for (size_t cnt = 0; cnt < entryCount; cnt++) {
        uint8_t entryType;
        MsgArg* entryVariant;
        status = entries[cnt].Get("(yv)", &entryType, &entryVariant);
        if (ER_OK != status) {
            return ER_INVALID_DATA;
        }
        if (entryType == EXCHANGE_KEYINFO) {
            status = KeyExchangeReadKeyInfo(*entryVariant);
            if (ER_OK != status) {
                return status;
            }
        }
    }
    return ER_OK;
}

QStatus KeyExchangerECDHE_ECDSA::KeyExchangeReadTrustAnchorKeyInfo(MsgArg& variant)
{
    PermissionMgmtObj::ClearTrustAnchorList(peerTrustAnchorList);
    size_t numEntries;
    MsgArg* entries;
    QStatus status = variant.Get("a(yv)", &numEntries, &entries);
    if (status != ER_OK) {
        return status;
    }
    if (numEntries == 0) {
        return ER_OK;  /* nothing to do */
    }

    for (size_t cnt = 0; cnt < numEntries; cnt++) {
        KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
        status = PermissionMgmtObj::MsgArgToKeyInfoNISTP256(entries[cnt], *keyInfo);
        if (status != ER_OK) {
            QCC_DbgHLPrintf(("KeyExchangerECDHE::KeyExchangeReadTrustAnchorKeyInfo parsing KeyInfo fails status 0x%x\n", status));
            return status;
        }

        peerTrustAnchorList.push_back(keyInfo);
        /* hash the handshake data */
        hashUtil.Update((uint8_t*) keyInfo->GetPublicKey(), sizeof(ECCPublicKey));
    }
    return ER_OK;
}

/**
 * Check to whether the two lists have a common trust anchor
 */
static bool HasCommonTrustAnchors(PermissionMgmtObj::TrustAnchorList& list1, PermissionMgmtObj::TrustAnchorList& list2)
{
    for (PermissionMgmtObj::TrustAnchorList::iterator it1 = list1.begin(); it1 != list1.end(); it1++) {
        GUID128 l1Guid(0);
        l1Guid.SetBytes((*it1)->GetKeyId());
        for (PermissionMgmtObj::TrustAnchorList::iterator it2 = list2.begin(); it2 != list2.end(); it2++) {
            GUID128 l2Guid(0);
            l2Guid.SetBytes((*it2)->GetKeyId());
            if (l1Guid != l2Guid) {
                continue;
            }
            /* compare the public key */
            if (memcmp((*it1)->GetPublicKey(), (*it2)->GetPublicKey(), sizeof(ECCPublicKey)) == 0) {
                return true;
            }
        }
    }
    return false;
}

QStatus KeyExchangerECDHE_ECDSA::KeyExchangeReadKey(MsgArg& variant)
{
    size_t entryCount;
    MsgArg* entries;
    QStatus status = variant.Get("a(yv)", &entryCount, &entries);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if (entryCount == 0) {
        return ER_INVALID_DATA;
    }

    for (size_t cnt = 0; cnt < entryCount; cnt++) {
        uint8_t entryType;
        MsgArg* entryVariant;
        status = entries[cnt].Get("(yv)", &entryType, &entryVariant);
        if (ER_OK != status) {
            return ER_INVALID_DATA;
        }
        switch (entryType) {
        case EXCHANGE_KEYINFO:
            status = KeyExchangeReadKeyInfo(*entryVariant);
            if (ER_OK != status) {
                return status;
            }
            break;

        case EXCHANGE_TRUST_ANCHORS:
            status = KeyExchangeReadTrustAnchorKeyInfo(*entryVariant);
            if (ER_OK != status) {
                return status;
            }
            /* check to see whether there are any common trust anchors */
            hasCommonTrustAnchors = HasCommonTrustAnchors(*trustAnchorList, peerTrustAnchorList);
            break;
        }
    }
    return ER_OK;
}

QStatus KeyExchangerECDHE::ExecKeyExchange(uint32_t authMask, KeyExchangerCB& callback, uint32_t* remoteAuthMask)
{

    QStatus status = GenerateECDHEKeyPair();
    if (status != ER_OK) {
        return status;
    }
    /* hash the handshake data */
    hashUtil.Update(HexStringToByteString(U32ToString(authMask, 16, 2 * sizeof (authMask), '0')));

    MsgArg variant;
    if (IsLegacyPeer()) {
        KeyExchangeGenLegacyKey(variant);
    } else {
        KeyExchangeGenKey(variant);
    }
    Message replyMsg(bus);
    MsgArg args[2];
    args[0].Set("u", authMask);
    status = args[1].Set("v", &variant);
    status = callback.SendKeyExchange(args, ArraySize(args), &replyMsg);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::ExecKeyExchange send KeyExchange fails status 0x%x\n", status));
        return status;
    }
    *remoteAuthMask = replyMsg->GetArg(0)->v_uint32;
    MsgArg* outVariant;
    status = replyMsg->GetArg(1)->Get("v", &outVariant);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::ExecKeyExchange send KeyExchange fails to retrieve variant from response status 0x%x\n", status));
        return status;
    }

    /* hash the handshake data */
    hashUtil.Update(HexStringToByteString(U32ToString(*remoteAuthMask, 16, 2 * sizeof (authMask), '0')));

    if (IsLegacyPeer()) {
        status = KeyExchangeReadLegacyKey(*outVariant);
    } else {
        status = KeyExchangeReadKey(*outVariant);
    }
    return status;
}

static QStatus GenerateVerifier(const char* label, const uint8_t* handshake, size_t handshakeLen, const KeyBlob& secretBlob, uint8_t* verifier, size_t verifierLen)
{
    qcc::String seed((const char*) handshake, handshakeLen);
    return Crypto_PseudorandomFunction(secretBlob, label, seed, verifier, verifierLen);
}

QStatus KeyExchangerECDHE::GenerateLocalVerifier(uint8_t* verifier, size_t verifierLen)
{
    qcc::String label;
    if (IsInitiator()) {
        label.assign("client finished");
    } else {
        label.assign("server finished");
    }
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    hashUtil.GetDigest(digest, true);
    QStatus status = GenerateVerifier(label.c_str(), digest, sizeof(digest), masterSecret, verifier, verifierLen);
    return status;
}

QStatus KeyExchangerECDHE::GenerateRemoteVerifier(uint8_t* verifier, size_t verifierLen)
{
    qcc::String label;
    if (IsInitiator()) {
        label.assign("server finished");
    } else {
        label.assign("client finished");
    }
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    hashUtil.GetDigest(digest, true);
    return GenerateVerifier(label.c_str(), digest, sizeof(digest), masterSecret, verifier, verifierLen);
}

QStatus KeyExchanger::ValidateRemoteVerifierVariant(const char* peerName, MsgArg* variant, uint8_t* authorized)
{
    QStatus status;
    if (!IsInitiator()) {
        status = RequestCredentialsCB(peerName);
        if (status != ER_OK) {
            return status;
        }
    }
    *authorized = false;
    uint8_t*remoteVerifier;
    size_t remoteVerifierLen;
    status = variant->Get("ay", &remoteVerifierLen, &remoteVerifier);
    if (remoteVerifierLen != AUTH_VERIFIER_LEN) {
        return ER_OK;
    }
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(computedRemoteVerifier, AUTH_VERIFIER_LEN);
    if (status != ER_OK) {
        return status;
    }
    *authorized = (memcmp(remoteVerifier, computedRemoteVerifier, AUTH_VERIFIER_LEN) == 0);
    if (!IsInitiator()) {
        hashUtil.Update(remoteVerifier, remoteVerifierLen);
    }
    return ER_OK;
}

static QStatus DoStoreMasterSecret(BusAttachment& bus, const qcc::GUID128& guid, KeyBlob& secretBlob, const uint8_t* tag, size_t tagLen, uint32_t expiresInSeconds, bool initiator, const uint8_t accessRights[4])
{
    QStatus status = ER_OK;
    secretBlob.SetExpiration(expiresInSeconds);
    KeyStore& keyStore = bus.GetInternal().GetKeyStore();
    if (status == ER_OK) {
        qcc::String tagStr((const char*) tag, tagLen);
        secretBlob.SetTag(tagStr, initiator ? KeyBlob::INITIATOR : KeyBlob::RESPONDER);
        status = keyStore.AddKey(guid, secretBlob, accessRights);
    }
    return status;
}

QStatus KeyExchangerECDHE::StoreMasterSecret(const qcc::GUID128& guid, const uint8_t accessRights[4])
{
    return DoStoreMasterSecret(bus, guid, masterSecret, (const uint8_t*) GetSuiteName(), strlen(GetSuiteName()), secretExpiration, IsInitiator(), accessRights);
}

QStatus KeyExchanger::ReplyWithVerifier(Message& msg)
{
    /* compute the local verifier to send back */
    uint8_t verifier[AUTH_VERIFIER_LEN];
    QStatus status = GenerateLocalVerifier(verifier, AUTH_VERIFIER_LEN);
    if (status != ER_OK) {
        return status;
    }
    MsgArg variant;
    variant.Set("ay", AUTH_VERIFIER_LEN, verifier);
    MsgArg replyMsg("v", &variant);
    return peerObj->HandleMethodReply(msg, &replyMsg, 1);
}

QStatus KeyExchangerECDHE_NULL::RequestCredentialsCB(const char* peerName)
{
    AuthListener::Credentials creds;
    bool ok = listener.RequestCredentials(GetSuiteName(),
                                          peerName, authCount, "", AuthListener::CRED_EXPIRATION, creds);
    if (!ok) {
        return ER_AUTH_FAIL;
    }
    if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
        SetSecretExpiration(creds.GetExpiration());
    } else {
        SetSecretExpiration(86400);      /* expires in one day */
    }
    return ER_OK;
}

QStatus KeyExchangerECDHE_NULL::KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized)
{

    *authorized = false;
    QStatus status = GenerateECDHESecret(&peerPubKey);
    if (status != ER_OK) {
        return status;
    }
    status = GenerateMasterSecret();
    if (status != ER_OK) {
        return status;
    }
    /* check the Auth listener */
    status = RequestCredentialsCB(peerName);
    if (status != ER_OK) {
        return status;
    }
    uint8_t verifier[AUTH_VERIFIER_LEN];
    GenerateLocalVerifier(verifier, sizeof(verifier));
    Message replyMsg(bus);
    MsgArg verifierArg("ay", sizeof(verifier), verifier);

    hashUtil.Update(verifier, sizeof(verifier));
    status = callback.SendKeyAuthentication(&verifierArg, &replyMsg);
    if (status != ER_OK) {
        return status;
    }
    MsgArg* variant;
    status = replyMsg->GetArg(0)->Get("v", &variant);
    if (status != ER_OK) {
        return status;
    }
    return ValidateRemoteVerifierVariant(peerName, variant, authorized);
}

QStatus KeyExchangerECDHE_PSK::ReplyWithVerifier(Message& msg)
{
    /* compute the local verifier to send back */
    uint8_t verifier[AUTH_VERIFIER_LEN];
    QStatus status = GenerateLocalVerifier(verifier, AUTH_VERIFIER_LEN);
    if (status != ER_OK) {
        return status;
    }
    MsgArg variant;
    variant.Set("(ayay)", pskName.length(), pskName.data(), AUTH_VERIFIER_LEN, verifier);
    MsgArg replyMsg("v", &variant);
    return peerObj->HandleMethodReply(msg, &replyMsg, 1);
}

QStatus KeyExchangerECDHE_PSK::RequestCredentialsCB(const char* peerName)
{
    AuthListener::Credentials creds;
    uint16_t credsMask = AuthListener::CRED_PASSWORD;
    if (pskName != "<anonymous>") {
        creds.SetUserName(pskName);
        credsMask |= AuthListener::CRED_USER_NAME;
    }

    bool ok = listener.RequestCredentials(GetSuiteName(), peerName, authCount, "", credsMask, creds);
    if (!ok) {
        return ER_AUTH_USER_REJECT;
    }
    if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
        SetSecretExpiration(creds.GetExpiration());
    } else {
        SetSecretExpiration(86400);      /* expires in one day */
    }
    if (creds.IsSet(AuthListener::CRED_USER_NAME)) {
        pskName = creds.GetUserName();
    }
    if (creds.IsSet(AuthListener::CRED_PASSWORD)) {
        pskValue = creds.GetPassword();
    } else {
        QCC_LogError(ER_AUTH_FAIL, ("KeyExchangerECDHE_PSK::RequestCredentialsCB PSK value not provided"));
        return ER_AUTH_FAIL;
    }
    return ER_OK;
}

QStatus KeyExchangerECDHE_PSK::GenerateLocalVerifier(uint8_t* verifier, size_t verifierLen)
{
    qcc::String label;
    if (IsInitiator()) {
        label.assign("client finished");
    } else {
        label.assign("server finished");
    }
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    hashUtil.GetDigest(digest, true);
    QStatus status = GenerateVerifier(label.c_str(), digest, sizeof(digest), masterSecret, verifier, verifierLen);
    return status;
}

QStatus KeyExchangerECDHE_PSK::GenerateRemoteVerifier(uint8_t* verifier, size_t verifierLen)
{
    qcc::String label;
    if (IsInitiator()) {
        label.assign("server finished");
    } else {
        label.assign("client finished");
    }
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    hashUtil.GetDigest(digest, true);
    return GenerateVerifier(label.c_str(), digest, sizeof(digest), masterSecret, verifier, verifierLen);
}

QStatus KeyExchangerECDHE_PSK::ValidateRemoteVerifierVariant(const char* peerName, MsgArg* variant, uint8_t* authorized)
{
    QStatus status;
    *authorized = false;
    uint8_t* pskName;
    size_t pskNameLen;
    uint8_t* remoteVerifier;
    size_t remoteVerifierLen;
    status = variant->Get("(ayay)", &pskNameLen, &pskName, &remoteVerifierLen, &remoteVerifier);
    if (!IsInitiator()) {
        String peerPskName((const char*) pskName, pskNameLen);
        status = RequestCredentialsCB(peerName);
        if (status != ER_OK) {
            return status;
        }
        hashUtil.Update(pskName, pskNameLen);
        hashUtil.Update((const uint8_t*) pskValue.data(), pskValue.length());
    }
    if (remoteVerifierLen != AUTH_VERIFIER_LEN) {
        return ER_OK;
    }
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(computedRemoteVerifier, AUTH_VERIFIER_LEN);
    if (status != ER_OK) {
        return status;
    }
    *authorized = (memcmp(remoteVerifier, computedRemoteVerifier, AUTH_VERIFIER_LEN) == 0);
    if (!IsInitiator()) {
        hashUtil.Update(remoteVerifier, remoteVerifierLen);
    }
    return ER_OK;
}

QStatus KeyExchangerECDHE_PSK::KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized)
{
    *authorized = false;
    QStatus status = GenerateECDHESecret(&peerPubKey);
    if (status != ER_OK) {
        return status;
    }
    status = GenerateMasterSecret();
    if (status != ER_OK) {
        return status;
    }
    /* check the Auth listener */
    status = RequestCredentialsCB(peerName);
    if (status != ER_OK) {
        return status;
    }

    /* hash the handshake */
    hashUtil.Update((const uint8_t*) pskName.data(), pskName.length());
    hashUtil.Update((const uint8_t*) pskValue.data(), pskValue.length());

    uint8_t verifier[AUTH_VERIFIER_LEN];
    GenerateLocalVerifier(verifier, sizeof(verifier));
    Message replyMsg(bus);
    MsgArg verifierArg;
    status = verifierArg.Set("(ayay)", pskName.length(), pskName.data(), sizeof(verifier), verifier);
    if (status != ER_OK) {
        return status;
    }

    hashUtil.Update(verifier, sizeof(verifier));
    status = callback.SendKeyAuthentication(&verifierArg, &replyMsg);
    if (status != ER_OK) {
        return status;
    }
    MsgArg* variant;
    status = replyMsg->GetArg(0)->Get("v", &variant);
    if (status != ER_OK) {
        return status;
    }
    return ValidateRemoteVerifierVariant(peerName, variant, authorized);
}

static QStatus StoreLocalKey(CredentialAccessor& ca, KeyBlob& kb, uint32_t expiration)
{
    kb.SetExpiration(expiration);
    GUID128 guid;
    ca.GetLocalGUID(kb.GetType(), guid);
    QStatus status = ca.StoreKey(guid, kb);
    if (status != ER_OK) {
        QCC_LogError(status, ("StoreLocalKey failed to save to key store"));
        return status;
    }
    return status;
}

static QStatus DoStoreDSAKeys(BusAttachment& bus, uint32_t expiration, ECCPrivateKey* privateKey, ECCPublicKey* publicKey, String& encodedCertChain)
{
    CredentialAccessor ca(bus);

    KeyBlob dsaPrivKb((const uint8_t*) privateKey, sizeof(ECCPrivateKey), KeyBlob::DSA_PRIVATE);
    QStatus status = StoreLocalKey(ca, dsaPrivKb, expiration);
    if (status != ER_OK) {
        return status;
    }

    KeyBlob dsaPubKb((const uint8_t*) publicKey, sizeof(ECCPublicKey), KeyBlob::DSA_PUBLIC);
    status = StoreLocalKey(ca, dsaPubKb, expiration);
    if (status != ER_OK) {
        return status;
    }
    if (!encodedCertChain.empty()) {
        KeyBlob dsaPEMKb((const uint8_t*) encodedCertChain.data(), encodedCertChain.length(), KeyBlob::PEM);
        status = StoreLocalKey(ca, dsaPEMKb, expiration);
        if (status != ER_OK) {
            return status;
        }
    }
    return status;
}

static QStatus DoRetrieveDSAKeys(BusAttachment& bus, ECCPrivateKey* privateKey, ECCPublicKey* publicKey, qcc::String& encodedCertChain, bool* found, uint32_t* keyExpiration)
{
    *found = false;
    *keyExpiration = 0;
    CredentialAccessor ca(bus);
    GUID128 guid;
    KeyBlob kb;
    ca.GetLocalGUID(KeyBlob::DSA_PRIVATE, guid);
    QStatus status = ca.GetKey(guid, kb);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        return ER_OK;       /* not found */
    }
    if ((status == ER_OK) && (kb.GetSize() == sizeof(ECCPrivateKey))) {
        memcpy(privateKey, kb.GetData(), kb.GetSize());
        Timespec expiry;
        kb.GetExpiration(expiry);
        Timespec now;
        GetTimeNow(&now);
        *keyExpiration = expiry.seconds - now.seconds;
        /* look up the DSA public key */
        ca.GetLocalGUID(KeyBlob::DSA_PUBLIC, guid);
        status = ca.GetKey(guid, kb);
        if ((status == ER_OK) && (kb.GetSize() == sizeof(ECCPublicKey))) {
            *found = true;
            memcpy(publicKey, kb.GetData(), kb.GetSize());
        }
        /* look up the public cert chain */
        ca.GetLocalGUID(KeyBlob::PEM, guid);
        status = ca.GetKey(guid, kb);
        if (status == ER_OK) {
            encodedCertChain.assign((const char*) kb.GetData(), kb.GetSize());
            *found = true;
        }
        status = ER_OK;
    }
    return status;
}

QStatus KeyExchangerECDHE_ECDSA::ParseCertChainPEM(String& encodedCertChain)
{
    size_t count = 0;
    QStatus status = CertECCUtil_GetCertCount(encodedCertChain, &count);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ParseCertChainPEM has error counting certs in the PEM"));
        return status;
    }
    certChainLen = count;
    delete [] certChain;
    certChain = NULL;
    if (count == 0) {
        return ER_OK;
    }
    certChain = new CertificateX509[count];
    status = CertificateX509::DecodeCertChainPEM(encodedCertChain, certChain, certChainLen);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ParseCertChainPEM has error loading certs in the PEM"));
        delete [] certChain;
        certChain = NULL;
        certChainLen = 0;
    }
    return status;
}

QStatus KeyExchangerECDHE_ECDSA::StoreDSAKeys(String& encodedPrivateKey, String& encodedCertChain)
{
    QStatus status = CertECCUtil_DecodePrivateKey(encodedPrivateKey, (uint32_t*) &issuerPrivateKey, sizeof(ECCPrivateKey));
    if (status != ER_OK) {
        return status;
    }

    status = ParseCertChainPEM(encodedCertChain);
    if (status != ER_OK) {
        return status;
    }
    if (certChainLen == 0) {
        return status;   /* need both private key and public key */
    }
    memcpy(&issuerPublicKey, certChain[0].GetSubjectPublicKey(), sizeof(ECCPublicKey));
    /* store the DSA keys in the key store */
    return DoStoreDSAKeys(bus, secretExpiration /* + 120 for testing */, &issuerPrivateKey, &issuerPublicKey, encodedCertChain);
}

static QStatus GenerateCertificateType0(uint8_t* verifier, size_t verifierLen, const ECCPrivateKey* privateKey, const ECCPublicKey* issuer, CertificateType0& cert)
{
    cert.SetIssuer(issuer);
    // the verifier is the digest
    cert.SetExternalDataDigest(verifier);

    return cert.Sign(privateKey);
}

static void FreeCertChain(CertificateECC* chain[], size_t chainLen)
{
    for (size_t cnt = 0; cnt < chainLen; cnt++) {
        delete chain[cnt];
    }
    delete [] chain;
}

KeyExchangerECDHE_ECDSA::~KeyExchangerECDHE_ECDSA()
{
    delete [] certChain;
    PermissionMgmtObj::ClearTrustAnchorList(peerTrustAnchorList);
}

QStatus KeyExchangerECDHE_ECDSA::RetrieveDSAKeys(bool generateIfNotFound)
{
    QStatus status = ER_OK;
    bool found = (certChainLen > 0);
    if (!found) {
        qcc::String encodedCertChain;
        uint32_t keyExpiration;
        status = DoRetrieveDSAKeys(bus, &issuerPrivateKey, &issuerPublicKey, encodedCertChain, &found, &keyExpiration);
        if (status != ER_OK) {
            return status;
        }
        if (found) {
            SetSecretExpiration(keyExpiration);
            status = ParseCertChainPEM(encodedCertChain);
            if (status != ER_OK) {
                return status;
            }
        }
    }
    if (found) {
        hasDSAKeys = true;
        return ER_OK;
    }
    if (!generateIfNotFound) {
        return ER_OK;
    }
    /* generate the key pair */
    Crypto_ECC ecc;
    ecc.GenerateDSAKeyPair();
    memcpy(&issuerPrivateKey, ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey));
    memcpy(&issuerPublicKey, ecc.GetDSAPublicKey(), sizeof(ECCPublicKey));
    hasDSAKeys = true;
    /* store them */
    String emptyStr;
    return DoStoreDSAKeys(bus, secretExpiration, &issuerPrivateKey, &issuerPublicKey, emptyStr);
}


QStatus KeyExchangerECDHE_ECDSA::RequestCredentialsCB(const char* peerName)
{
    RetrieveDSAKeys(false);      /* try to retrieve saved DSA keys */
    if (hasDSAKeys) {
        return ER_OK;      /* don't need to call the app */
    }
    /* check the Auth listener */
    AuthListener::Credentials creds;
    uint16_t credsMask = AuthListener::CRED_PRIVATE_KEY | AuthListener::CRED_CERT_CHAIN | AuthListener::CRED_EXPIRATION;

    bool ok = listener.RequestCredentials(GetSuiteName(), peerName, authCount, "", credsMask, creds);
    if (!ok) {
        return ER_AUTH_FAIL;
    }
    if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
        SetSecretExpiration(creds.GetExpiration());
    } else {
        SetSecretExpiration(0xFFFFFFFF);      /* never expired */
    }
    if (creds.IsSet(AuthListener::CRED_PRIVATE_KEY) && creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
        qcc::String pemPrivateKey = creds.GetPrivateKey();
        qcc::String pemCertChain = creds.GetCertChain();
        QStatus status = StoreDSAKeys(pemPrivateKey, pemCertChain);
        if (status != ER_OK) {
            return status;
        }
    }
    return ER_OK;
}

static qcc::String EncodePEMCertChain(CertificateX509* certs, size_t numCerts)
{
    qcc::String chain;
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        chain += certs[cnt].GetPEM();
        if (numCerts > 1) {
            chain += "\n";
        }
    }
    return chain;
}

QStatus KeyExchangerECDHE_ECDSA::VerifyCredentialsCB(const char* peerName, CertificateX509* certs, size_t numCerts)
{
    if (numCerts == 0) {
        return ER_OK;
    }
    AuthListener::Credentials creds;

    creds.SetCertChain(EncodePEMCertChain(certs, numCerts));

    /* check with listener to validate the cert chain */
    bool ok = listener.VerifyCredentials(GetSuiteName(), peerName, creds);
    if (!ok) {
        return ER_AUTH_FAIL;
    }
    return ER_OK;
}

QStatus KeyExchangerECDHE_ECDSA::ValidateLegacyRemoteVerifierVariant(MsgArg* variant, uint8_t* authorized)
{
    *authorized = false;
    MsgArg* chainArg;
    size_t numCerts;
    QStatus status = variant->Get("a(ay)", &numCerts, &chainArg);
    if (status != ER_OK) {
        return status;
    }
    if (numCerts <= 0) {
        return ER_OK;
    }
    /* scan the array of certificates */
    CertificateECC** certs = new CertificateECC * [numCerts];
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        certs[cnt] = NULL;
    }
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        uint32_t certVersion;
        uint8_t* encoded;
        size_t encodedLen;
        status = chainArg[cnt].Get("(ay)", &encodedLen, &encoded);
        if (status != ER_OK) {
            FreeCertChain(certs, numCerts);
            return status;
        }
        status = CertECCUtil_GetVersionFromEncoded(encoded, encodedLen, &certVersion);
        if (status != ER_OK) {
            QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ValidateLegacyRemoteVerifierVariant invalid peer cert data"));
            FreeCertChain(certs, numCerts);
            return ER_INVALID_DATA;
        }
        switch (certVersion) {
        case 0:
            certs[cnt] = new CertificateType0();
            break;

        case 1:
            certs[cnt] = new CertificateType1();
            break;

        case 2:
            certs[cnt] = new CertificateType2();
            break;

        default:
            QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ValidateLegacyRemoteVerifierVariant unknown cert"));
            FreeCertChain(certs, numCerts);
            return ER_INVALID_DATA;
        }

        /* load the cert using the encoded bytes */
        status = certs[cnt]->LoadEncoded(encoded, encodedLen);
        if (status != ER_OK) {
            QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ValidateRemoteVerifierVariant error loading peer cert encoded data"));
            FreeCertChain(certs, numCerts);
            return status;
        }
    }
    /* take the leaf cert to validate the verifier */
    CertificateECC* cert = certs[0];

    bool certVerified = cert->VerifySignature();
    if (!certVerified) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ValidateLegacyRemoteVerifierVariant leaf cert is not verified"));
        FreeCertChain(certs, numCerts);
        return ER_OK;
    }
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];

    status = GenerateRemoteVerifier(computedRemoteVerifier, AUTH_VERIFIER_LEN);
    if (status != ER_OK) {
        FreeCertChain(certs, numCerts);
        return status;
    }
    // the verifier is the digest
    *authorized = (memcmp(cert->GetExternalDataDigest(), computedRemoteVerifier, AUTH_VERIFIER_LEN) == 0);

    /* hash the certs */
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        hashUtil.Update(certs[cnt]->GetEncoded(), certs[cnt]->GetEncodedLen());
    }
    FreeCertChain(certs, numCerts);
    return ER_OK;
}

QStatus KeyExchangerECDHE_ECDSA::ValidateRemoteVerifierVariant(const char* peerName, MsgArg* variant, uint8_t* authorized)
{
    QStatus status = ER_OK;
    if (!IsInitiator()) {
        status = RequestCredentialsCB(peerName);
        if (status != ER_OK) {
            return status;
        }
    }
    *authorized = false;
    if (IsLegacyPeer()) {
        return ValidateLegacyRemoteVerifierVariant(variant, authorized);
    }

    uint8_t sigFormat;
    MsgArg* sigInfoVariant;
    uint8_t certChainEncoding;
    MsgArg* certChainVariant;
    status = variant->Get("(yvyv)", &sigFormat, &sigInfoVariant, &certChainEncoding, &certChainVariant);
    if (status != ER_OK) {
        return status;
    }
    if (sigFormat != KeyInfo::FORMAT_ALLJOYN) {
        return ER_INVALID_DATA;
    }
    if ((certChainEncoding != Certificate::ENCODING_X509_DER) &&
        (certChainEncoding != Certificate::ENCODING_X509_DER_PEM)) {
        return ER_INVALID_DATA;
    }

    /* handle the sigInfo variant */
    uint8_t sigAlgorithm;
    MsgArg* sigVariant;
    status = sigInfoVariant->Get("(yv)", &sigAlgorithm, &sigVariant);
    if (status != ER_OK) {
        return status;
    }
    if (sigAlgorithm != SigInfo::ALGORITHM_ECDSA_SHA_256) {
        return ER_INVALID_DATA;
    }
    size_t rCoordLen;
    uint8_t* rCoord;
    size_t sCoordLen;
    uint8_t* sCoord;
    status = sigVariant->Get("(ayay)", &rCoordLen, &rCoord, &sCoordLen, &sCoord);
    if (status != ER_OK) {
        return status;
    }
    if (rCoordLen != ECC_COORDINATE_SZ) {
        return ER_INVALID_DATA;
    }
    if (sCoordLen != ECC_COORDINATE_SZ) {
        return ER_INVALID_DATA;
    }
    /* verify */
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(computedRemoteVerifier, AUTH_VERIFIER_LEN);
    if (status != ER_OK) {
        return status;
    }
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(&peerPubKey);
    SigInfoECC sigInfo;
    sigInfo.SetRCoord(rCoord);
    sigInfo.SetSCoord(sCoord);
    status = ecc.DSAVerify(computedRemoteVerifier, AUTH_VERIFIER_LEN, sigInfo.GetSignature());
    *authorized = (ER_OK == status);

    if (!*authorized) {
        return ER_OK;  /* not authorized */
    }
    hashUtil.Update(rCoord, rCoordLen);
    hashUtil.Update(sCoord, sCoordLen);

    /* handle the certChain variant */
    MsgArg* chainArg;
    size_t numCerts;
    status = certChainVariant->Get("a(ay)", &numCerts, &chainArg);
    if (status != ER_OK) {
        return status;
    }
    hashUtil.Update(&certChainEncoding, 1);
    if (numCerts == 0) {
        /* no cert chain to validate */
        return ER_OK;
    }

    CertificateX509* certs = new CertificateX509[numCerts];
    size_t encodedLen;
    uint8_t* encoded;
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        status = chainArg[cnt].Get("(ay)", &encodedLen, &encoded);
        if (status != ER_OK) {
            delete [] certs;
            return status;
        }
        if (certChainEncoding == Certificate::ENCODING_X509_DER) {
            status = certs[cnt].LoadEncoded(encoded, encodedLen);
        } else if (certChainEncoding == Certificate::ENCODING_X509_DER_PEM) {
            status = certs[cnt].LoadPEM(String((const char*) encoded, encodedLen));
        } else {
            return ER_INVALID_DATA;
        }
        if (status != ER_OK) {
            QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ValidateRemoteVerifierVariant error loading peer cert encoded data"));
            delete [] certs;
            return status;
        }
        hashUtil.Update(encoded, encodedLen);
    }
    status = VerifyCredentialsCB(peerName, certs, numCerts);
    if (status != ER_OK) {
        *authorized = false;
    }
    delete [] certs;
    return ER_OK;
}


QStatus KeyExchangerECDHE_ECDSA::ReplyWithVerifier(Message& msg)
{
    QStatus status;
    MsgArg variant;
    if (IsLegacyPeer()) {
        status = GenVerifierCertArg(variant, false);
    } else {
        status = GenVerifierSigInfoArg(variant, false);
    }
    if (ER_OK != status) {
        return status;
    }
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    MsgArg replyMsg("v", &variant);
    return peerObj->HandleMethodReply(msg, &replyMsg, 1);
}

QStatus KeyExchangerECDHE_ECDSA::GenerateLocalVerifierCert(CertificateType0& cert)
{
    uint8_t verifier[AUTH_VERIFIER_LEN];
    GenerateLocalVerifier(verifier, sizeof(verifier));

    QStatus status = RetrieveDSAKeys(true);     /* make sure the keys are there or generate them */
    if (status != ER_OK) {
        return status;
    }
    return GenerateCertificateType0(verifier, AUTH_VERIFIER_LEN, &issuerPrivateKey, &issuerPublicKey, cert);
}

QStatus KeyExchangerECDHE_ECDSA::GenerateLocalVerifierSigInfo(SigInfoECC& sigInfo)
{
    uint8_t verifier[AUTH_VERIFIER_LEN];
    GenerateLocalVerifier(verifier, sizeof(verifier));

    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(GetECDHEPrivateKey());
    ECCSignature sig;
    QStatus status = ecc.DSASign(verifier, AUTH_VERIFIER_LEN, &sig);
    if (status != ER_OK) {
        return status;
    }
    sigInfo.SetSignature(&sig);
    return status;
}

QStatus KeyExchangerECDHE_ECDSA::GenVerifierCertArg(MsgArg& msgArg, bool updateHash)
{
    /* compute the local verifier to send back */
    CertificateType0 leafCert;
    QStatus status = GenerateLocalVerifierCert(leafCert);
    if (status != ER_OK) {
        QCC_LogError(status, ("KeyExchangerECDHE_ECDSA::GenVerifierCertArg failed to generate local verifier cert"));
        return status;
    }

    /* make an array of certs */
    int numCerts = 1;
    MsgArg* certArgs = new MsgArg[numCerts];

    certArgs[0].Set("(ay)", leafCert.GetEncodedLen(), leafCert.GetEncoded());
    if (updateHash) {
        hashUtil.Update(leafCert.GetEncoded(), leafCert.GetEncodedLen());
    }

    status = msgArg.Set("a(ay)", numCerts, certArgs);
    if (status != ER_OK) {
        delete [] certArgs;
        return status;
    }
    msgArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    return ER_OK;
}

QStatus KeyExchangerECDHE_ECDSA::GenVerifierSigInfoArg(MsgArg& msgArg, bool updateHash)
{
    /* build the SigInfo object */
    SigInfoECC sigInfo;
    QStatus status = GenerateLocalVerifierSigInfo(sigInfo);
    if (status != ER_OK) {
        QCC_LogError(status, ("KeyExchangerECDHE_ECDSA::GenVerifierSigInfoArg failed to generate local verifier sig info"));
        return status;
    }
    if (updateHash) {
        hashUtil.Update((const uint8_t*) sigInfo.GetSignature(), sizeof(ECCSignature));
    }

    MsgArg* certArgs = NULL;

    uint8_t encoding = Certificate::ENCODING_X509_DER;
    if (updateHash) {
        hashUtil.Update(&encoding, 1);
    }
    if (certChainLen > 0) {
        certArgs = new MsgArg[certChainLen];
        /* add the local cert chain to the list of certs to send */
        for (size_t cnt = 0; cnt < certChainLen; cnt++) {
            certArgs[cnt].Set("(ay)", certChain[cnt].GetEncodedLen(), certChain[cnt].GetEncoded());
            if (updateHash) {
                hashUtil.Update(certChain[cnt].GetEncoded(), certChain[cnt].GetEncodedLen());
            }
        }
    }
    /* copy the message args */
    MsgArg localArg;

    localArg.Set("(yvyv)",
                 sigInfo.GetFormat(),
                 new MsgArg("(yv)", sigInfo.GetAlgorithm(),
                            new MsgArg("(ayay)", ECC_COORDINATE_SZ, sigInfo.GetRCoord(), ECC_COORDINATE_SZ, sigInfo.GetSCoord())),
                 encoding,
                 new MsgArg("a(ay)", certChainLen, certArgs));
    localArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    msgArg = localArg;
    return ER_OK;
}

QStatus KeyExchangerECDHE_ECDSA::KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized)
{
    *authorized = false;
    QStatus status = GenerateECDHESecret(&peerPubKey);
    if (status != ER_OK) {
        return status;
    }
    status = GenerateMasterSecret();
    if (status != ER_OK) {
        return status;
    }
    /* check the Auth listener */
    status = RequestCredentialsCB(peerName);
    if (status != ER_OK) {
        return status;
    }

    MsgArg variant;
    if (IsLegacyPeer()) {
        status = GenVerifierCertArg(variant, true);
    } else {
        status = GenVerifierSigInfoArg(variant, true);
    }
    if (status != ER_OK) {
        return status;
    }
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);

    Message replyMsg(bus);
    status = callback.SendKeyAuthentication(&variant, &replyMsg);
    if (status != ER_OK) {
        return status;
    }
    MsgArg* remoteVariant;
    status = replyMsg->GetArg(0)->Get("v", &remoteVariant);
    if (status != ER_OK) {
        return status;
    }
    return ValidateRemoteVerifierVariant(peerName, remoteVariant, authorized);
}

bool KeyExchanger::IsLegacyPeer()
{
    return (GetPeerAuthVersion() == LEGACY_AUTH_VERSION);
}

} /* namespace ajn */
