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
        return ER_INVALID_DATA;
    }

    status = GenerateECDHEKeyPair();
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange failed to generate ECDHE key pair"));
        return status;
    }

    status = GenerateECDHESecret(&peerPubKey);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange failed to generate ECDHE secret"));
        return status;
    }
    status = GenerateMasterSecret();
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange failed to generate master secret"));
        return status;
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

static QStatus ParseKeyInfo(KeyInfoNISTP256& keyInfo, MsgArg& variant)
{
    uint8_t keyFormat;
    MsgArg* variantArg;
    QStatus status = variant.Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if (keyFormat != KeyInfo::FORMAT_ALLJOYN) {
        return ER_INVALID_DATA;
    }
    uint8_t* keyId;
    size_t keyIdLen;
    uint8_t keyUsageType;
    uint8_t keyType;
    MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &keyIdLen, &keyId, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
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

void KeyExchangerECDHE::KeyExchangeGenKey(MsgArg& variant)
{
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(GetECDHEPublicKey());
    MsgArg coordArg("(ayay)", ECC_COORDINATE_SZ, keyInfo.GetXCoord(), ECC_COORDINATE_SZ, keyInfo.GetYCoord());

    /* no need to send the key ID */
    variant.Set("(yv)", KeyInfo::FORMAT_ALLJOYN,
                new MsgArg("(ayyyv)", 0, NULL, KeyInfo::USAGE_SIGNING, KeyInfoECC::KEY_TYPE,
                           new MsgArg("(yyv)", keyInfo.GetAlgorithm(), keyInfo.GetCurve(), new MsgArg(coordArg))));
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    hashUtil.Update((uint8_t*) GetECDHEPublicKey(), sizeof(ECCPublicKey));
}

QStatus KeyExchangerECDHE::KeyExchangeReadKey(MsgArg& variant)
{
    KeyInfoNISTP256 keyInfo;
    QStatus status = ParseKeyInfo(keyInfo, variant);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::KeyExchangeReadKey parsing KeyInfo fails status 0x%x\n", status));
        return status;
    }
    memcpy(&peerPubKey, keyInfo.GetPublicKey(), sizeof(ECCPublicKey));
    /* hash the handshake data */
    hashUtil.Update((uint8_t*) &peerPubKey, sizeof(ECCPublicKey));
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
    QStatus status;
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
    MsgArg* chainArg;
    size_t numCerts;
    status = variant->Get("a(ay)", &numCerts, &chainArg);
    if (status != ER_OK) {
        return status;
    }
    if (numCerts <= 0) {
        return ER_OK;
    }
    CertificateType0 cert0;
    uint8_t* encoded;
    size_t encodedLen;
    status = chainArg[0].Get("(ay)", &encodedLen, &encoded);
    if (status != ER_OK) {
        return status;
    }
    status = cert0.LoadEncoded(encoded, encodedLen);
    if (status != ER_OK) {
        return status;
    }
    /* take the leaf cert to validate the verifier */

    bool certVerified = cert0.VerifySignature();
    if (!certVerified) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ValidateRemoteVerifierVariant leaf cert is not verified"));
        return ER_OK;
    }
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];

    status = GenerateRemoteVerifier(computedRemoteVerifier, AUTH_VERIFIER_LEN);
    if (status != ER_OK) {
        return status;
    }
    // the verifier is the digest
    *authorized = (memcmp(cert0.GetExternalDataDigest(), computedRemoteVerifier, AUTH_VERIFIER_LEN) == 0);

    if (!*authorized) {
        return ER_OK;  /* not authorized */
    }
    hashUtil.Update(encoded, encodedLen);
    if (numCerts == 1) {
        return ER_OK; /* done */
    }
    /* scan the array of certificates */
    numCerts--;
    CertificateX509* certs = new CertificateX509[numCerts];
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        status = chainArg[cnt + 1].Get("(ay)", &encodedLen, &encoded);
        if (status != ER_OK) {
            delete [] certs;
            return status;
        }
        status = certs[cnt].LoadEncoded(encoded, encodedLen);
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
    CertificateType0 leafCert;
    QStatus status = GenerateLocalVerifierCert(leafCert);
    if (status != ER_OK) {
        QCC_LogError(status, ("KeyExchangerECDHE_ECDSA::ReplyWithVerifier failed to generate local verifier cert"));
        return status;
    }
    MsgArg variant;
    MsgArg* certArgs;
    int numCerts = 1;
    if (!IsLegacyPeer()) {
        /* do not X509 cert chain to legacy peers */
        if (certChainLen > 0) {
            numCerts += certChainLen;
        }
    }
    certArgs = new MsgArg[numCerts];
    certArgs[0].Set("(ay)", leafCert.GetEncodedLen(), leafCert.GetEncoded());

    if (!IsLegacyPeer()) {
        /* add the local cert chain to the list of certs to send */
        for (int cnt = 1; cnt < numCerts; cnt++) {
            certArgs[cnt].Set("(ay)", certChain[cnt - 1].GetEncodedLen(), certChain[cnt - 1].GetEncoded());
        }
    }
    status = variant.Set("a(ay)", numCerts, certArgs);
    if (status != ER_OK) {
        return status;
    }
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    MsgArg replyMsg("v", &variant);
    status = peerObj->HandleMethodReply(msg, &replyMsg, 1);
    return status;
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

QStatus KeyExchangerECDHE_ECDSA::PrepareKeyAuthenticationLegacy(MsgArg& msgArg)
{
    /* compute the local verifier to send back */
    CertificateType0 leafCert;
    QStatus status = GenerateLocalVerifierCert(leafCert);
    if (status != ER_OK) {
        QCC_LogError(status, ("KeyExchangerECDHE_ECDSA::KeyAuthentication failed to generate local verifier cert"));
        return status;
    }

    /* make an array of certs */
    int numCerts = 1;
    MsgArg* certArgs = new MsgArg[numCerts];

    certArgs[0].Set("(ay)", leafCert.GetEncodedLen(), leafCert.GetEncoded());
    hashUtil.Update(leafCert.GetEncoded(), leafCert.GetEncodedLen());

    status = msgArg.Set("a(ay)", numCerts, certArgs);
    if (status != ER_OK) {
        delete [] certArgs;
        return status;
    }
    msgArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
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
        status = PrepareKeyAuthenticationLegacy(variant);
        if (status != ER_OK) {
            return status;
        }
    } else {
        /* compute the local verifier to send back */
        CertificateType0 leafCert;
        status = GenerateLocalVerifierCert(leafCert);
        if (status != ER_OK) {
            QCC_LogError(status, ("KeyExchangerECDHE_ECDSA::KeyAuthentication failed to generate local verifier cert"));
            return status;
        }

        /* make an array of certs */
        int numCerts = 1;

        if (certChainLen > 0) {
            numCerts += certChainLen;
        }
        MsgArg* certArgs = new MsgArg[numCerts];

        certArgs[0].Set("(ay)", leafCert.GetEncodedLen(), leafCert.GetEncoded());
        hashUtil.Update(leafCert.GetEncoded(), leafCert.GetEncodedLen());

        /* add the local cert chain to the list of certs to send */
        for (int cnt = 1; cnt < numCerts; cnt++) {
            int idx = cnt - 1;
            certArgs[cnt].Set("(ay)", certChain[idx].GetEncodedLen(), certChain[idx].GetEncoded());
            hashUtil.Update(certChain[idx].GetEncoded(), certChain[idx].GetEncodedLen());
        }

        status = variant.Set("a(ay)", numCerts, certArgs);
        if (status != ER_OK) {
            delete [] certArgs;
            return status;
        }
        variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }

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
