/**
 * @file
 *
 * This file implements the KeyExchanger features
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

#include <qcc/platform.h>
#include <qcc/KeyBlob.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECCOldEncoding.h>
#include <qcc/StringUtil.h>
#include <qcc/CertificateECC.h>
#include <qcc/CertificateHelper.h>
#include <qcc/Debug.h>
#include <qcc/time.h>

#include <stdio.h>

#include "KeyExchanger.h"
#include "AllJoynPeerObj.h"
#include "CredentialAccessor.h"
#include "KeyInfoHelper.h"
#include "ConversationHash.h"

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
 * The auth version with support for KeyInfo
 */
#define KEY_INFO_AUTH_VERSION      4

/* the size of the master secret based on RFC5246 */
#define MASTER_SECRET_SIZE 48
/* the size of the master secret used in the PIN key exchange */
#define MASTER_SECRET_PINX_SIZE 24

/* the current PeerSecretRecord version */
#define PEER_SECRET_RECORD_VERSION 1

/**
 * Calculate the size of the Peer secret record.  This record has the
 * following fields:
 *  uint8_t version;
 *  uint8_t secret[MASTER_SECRET_SIZE];
 *  ECCPublicKey publicKey;
 *  uint8_t manifestDigest[Crypto_SHA256::DIGEST_SIZE];
 *  uint8_t numIssuerPublicKeys;
 *  uint8_t* issuerPublicKeys;
 */

static size_t CalcPeerSecretRecordSize(uint8_t numIssuerKeys)
{
    ECCPublicKey emptyKey;
    size_t publicKeySize = emptyKey.GetSize();
    size_t ret = sizeof(uint8_t) + /* version */
                 MASTER_SECRET_SIZE + /* secret */
                 publicKeySize + /* publicKey */
                 Crypto_SHA256::DIGEST_SIZE + /* manifestDigest */
                 sizeof(uint8_t); /* numIssuerPublicKeys */
    if (numIssuerKeys == 0) {
        ret += sizeof(uint8_t*);
    } else {
        /* issuerPublicKeys array */
        ret += numIssuerKeys * publicKeySize;
    }
    return ret;
}

class SigInfo {

  public:

    static const size_t ALGORITHM_ECDSA_SHA_256 = 0;

    /**
     * Default constructor.
     */
    SigInfo() : algorithm(0xFF)
    {
    }

    /**
     * destructor.
     */
    virtual ~SigInfo()
    {
    }

    /**
     * Retrieve the signature algorithm
     * @return the signature ECC algorithm
     */
    const uint8_t GetAlgorithm() const
    {
        return algorithm;
    }

    /**
     * Virtual initializer to be implemented by derived classes.  The derired
     * class should call the protected SigInfo::SetAlgorithm() method to set
     * the signature algorithm.
     */

    virtual void Init() = 0;

  protected:

    /**
     * Set the signature algorithm
     */
    void SetAlgorithm(uint8_t newAlgorithm)
    {
        this->algorithm = newAlgorithm;
    }


  private:
    /**
     * Assignment operator is private
     */
    SigInfo& operator=(const SigInfo& other);

    /**
     * Copy constructor is private
     */
    SigInfo(const SigInfo& other);

    uint8_t algorithm;
};

class SigInfoECC : public SigInfo {

  public:

    /**
     * Default constructor.
     */
    SigInfoECC()
    {
        Init();
    }

    virtual void Init() {
        SetAlgorithm(ALGORITHM_ECDSA_SHA_256);
        memset(&sig, 0, sizeof(ECCSignature));
    }

    /**
     * desstructor.
     */
    virtual ~SigInfoECC()
    {
    }

    /**
     * Assign the R coordinate
     * @param rCoord the R coordinate value to copy
     */
    void SetRCoord(const uint8_t* rCoord)
    {
        memcpy(sig.r, rCoord, ECC_COORDINATE_SZ);
    }
    /**
     * Retrieve the R coordinate value
     * @return the R coordinate value.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     */
    const uint8_t* GetRCoord() const
    {
        return sig.r;
    }

    /**
     * Retrieve the size of the R value
     * @return the size of the R value
     */
    size_t GetRSize() const
    {
        /* This should eventually call a suitable method on ECCSignature, when at some point
         * ECCSignature is expanded to return this information. For now, since we only have NIST P-256,
         * it's hardcoded.
         */
        return ECC_COORDINATE_SZ;
    }

    /**
     * Assign the S coordinate
     * @param sCoord the S coordinate value to copy
     */
    void SetSCoord(const uint8_t* sCoord)
    {
        memcpy(sig.s, sCoord, ECC_COORDINATE_SZ);
    }

    /**
     * Retrieve the S coordinate value
     * @return the S coordinate value.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     */
    const uint8_t* GetSCoord() const
    {
        return sig.s;
    }

    /**
     * Retrieve the size of the S value
     * @return the size of the S value
     */
    size_t GetSSize() const
    {
        /* This should eventually call a suitable method on ECCSignature, when at some point
         * ECCSignature is expanded to return this information. For now, since we only have NIST P-256,
         * it's hardcoded.
         */
        return ECC_COORDINATE_SZ;
    }

    /**
     * Set the signature.  The signature is copied into the internal buffer.
     */
    void SetSignature(const ECCSignature* signature)
    {
        this->sig = *signature;
    }

    /**
     * Get the signature.
     * @return the signature.
     */
    const ECCSignature* GetSignature() const
    {
        return &sig;
    }

  private:
    /**
     * Assignment operator is private
     */
    SigInfoECC& operator=(const SigInfoECC& other);

    /**
     * Copy constructor is private
     */
    SigInfoECC(const SigInfoECC& other);

    ECCSignature sig;
};

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

QStatus KeyExchangerECDHE::GenerateMasterSecret(const ECCPublicKey* remotePubKey)
{
    QStatus status;
    uint8_t keymatter[MASTER_SECRET_SIZE];      /* RFC5246 */
    if (IsLegacyPeer()) {
        ECCSecretOldEncoding pms;
        status = Crypto_ECC_OldEncoding::GenerateSharedSecret(ecc, remotePubKey, &pms);
        if (ER_OK != status) {
            return status;
        }
        KeyBlob pmsBlob((const uint8_t*) &pms, sizeof (ECCSecretOldEncoding), KeyBlob::GENERIC);
        status = Crypto_PseudorandomFunction(pmsBlob, "master secret", "", keymatter, sizeof (keymatter));
    } else {
        ECCSecret pms;
        status = ecc.GenerateSharedSecret(remotePubKey, &pms);
        if (ER_OK != status) {
            return status;
        }
        uint8_t pmsDigest[Crypto_SHA256::DIGEST_SIZE];
        status = pms.DerivePreMasterSecret(pmsDigest, sizeof(pmsDigest));
        KeyBlob pmsBlob(pmsDigest, sizeof (pmsDigest), KeyBlob::GENERIC);
        status = Crypto_PseudorandomFunction(pmsBlob, "master secret", "", keymatter, sizeof (keymatter));
    }
    masterSecret.Set(keymatter, sizeof(keymatter), KeyBlob::GENERIC);
    return status;
}

void KeyExchanger::ShowCurrentDigest(const char* ref)
{
    QCC_UNUSED(ref);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->GetDigest(IsInitiator(), digest, true);
    peerState->ReleaseConversationHashLock(IsInitiator());
    QCC_DbgHLPrintf(("Current digest %s ref[%s]: %s\n", IsInitiator() ? "I" : "R", ref, BytesToHexString(digest, sizeof(digest)).c_str()));
}

QStatus KeyExchangerECDHE::RespondToKeyExchange(Message& msg, MsgArg* variant, uint32_t remoteAuthMask, uint32_t authMask)
{
    Message replyMsg(bus);

    QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange"));
    /* hash the handshake data */
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, HexStringToByteString(U32ToString(remoteAuthMask, 16, 2 * sizeof(remoteAuthMask), '0')));

    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, msg);
    peerState->ReleaseConversationHashLock(IsInitiator());

    QStatus status;
    MsgArg outVariant;
    MsgArg args[2];
    if (IsLegacyPeer()) {
        status = KeyExchangeReadLegacyKey(*variant);
    } else {
        status = KeyExchangeReadKey(*variant);
    }
    if (ER_OK != status) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange received invalid data from peer"));
        status = ER_INVALID_DATA;
        goto Exit;
    }

    status = GenerateECDHEKeyPair();
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange failed to generate ECDHE key pair"));
        goto Exit;
    }

    status = GenerateMasterSecret(&peerPubKey);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::RespondToKeyExchange failed to generate master secret"));
        goto Exit;
    }
    /* hash the handshake data */
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, HexStringToByteString(U32ToString(authMask, 16, 2 * sizeof(authMask), '0')));
    peerState->ReleaseConversationHashLock(IsInitiator());

    if (IsLegacyPeer()) {
        KeyExchangeGenLegacyKey(outVariant);
    } else {
        KeyExchangeGenKey(outVariant);
    }
    args[0].Set("u", authMask);
    args[1].Set("v", &outVariant);

Exit:
    peerState->AcquireConversationHashLock(IsInitiator());
    if (ER_OK == status) {
        status = peerObj->HandleMethodReply(msg, replyMsg, args, ArraySize(args));
    } else {
        status = peerObj->HandleMethodReply(msg, replyMsg, status);
    }
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
    peerState->ReleaseConversationHashLock(IsInitiator());

    return status;
}

void KeyExchangerECDHE::KeyExchangeGenLegacyKey(MsgArg& variant)
{
    uint8_t buf[1 + sizeof(ECCPublicKeyOldEncoding)];
    buf[0] = ecc.GetCurveType();
    ECCPublicKeyOldEncoding oldenc;
    Crypto_ECC_OldEncoding::ReEncode(GetECDHEPublicKey(), &oldenc);
    memcpy(&buf[1], oldenc.data, sizeof(oldenc.data));
    MsgArg localArg("ay", sizeof(buf), buf);
    variant = localArg;
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, buf, sizeof(buf));
    peerState->ReleaseConversationHashLock(IsInitiator());

    /* In CONVERSATION_V4, this content is hashed one level up in ExecKeyExchange or
     * RespondToKeyExchange. So no hashing is done here for that version.
     */
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
    memcpy(oldenc.data, &replyPubKey[1], sizeof(oldenc.data));
    Crypto_ECC_OldEncoding::ReEncode(&oldenc, &peerPubKey);
    /* hash the handshake data */
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, replyPubKey, replyPubKeyLen);
    peerState->ReleaseConversationHashLock(IsInitiator());

    /* In CONVERSATION_V4, this content is hashed one level up in ExecKeyExchange or
     * RespondToKeyExchange. So no hashing is done here for that version.
     */

    return ER_OK;
}

void KeyExchangerECDHE::KeyExchangeGenKeyInfo(MsgArg& variant)
{
    KeyInfoNISTP256 keyInfo;
    const ECCPublicKey* publicKey = GetECDHEPublicKey();
    keyInfo.SetPublicKey(publicKey);
    KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg(keyInfo, variant);

    size_t exportedPublicKeySize = publicKey->GetSize();
    uint8_t* exportedPublicKey = new uint8_t[exportedPublicKeySize];
    QCC_VERIFY(ER_OK == publicKey->Export(exportedPublicKey, &exportedPublicKeySize));

    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, exportedPublicKey, exportedPublicKeySize);
    peerState->ReleaseConversationHashLock(IsInitiator());

    delete[] exportedPublicKey;
}

void KeyExchangerECDHE::KeyExchangeGenKey(MsgArg& variant)
{
    if (PeerSupportsKeyInfo()) {
        KeyExchangeGenKeyInfo(variant);
    } else {
        uint8_t curveType = ecc.GetCurveType();
        const ECCPublicKey* publicKey = GetECDHEPublicKey();
        size_t exportedPublicKeySize = publicKey->GetSize();
        uint8_t* exportedPublicKey = new uint8_t[exportedPublicKeySize];
        QCC_VERIFY(ER_OK == publicKey->Export(exportedPublicKey, &exportedPublicKeySize));
        variant.Set("(yay)", curveType, exportedPublicKeySize, exportedPublicKey);

        /* The MsgArg takes ownership of exportedPublicKey and will delete it on destruction. */
        variant.SetOwnershipFlags(MsgArg::OwnsArgs | MsgArg::OwnsData, true);
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, &curveType, sizeof(curveType));
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, exportedPublicKey, exportedPublicKeySize);
        peerState->ReleaseConversationHashLock(IsInitiator());

        /* In CONVERSATION_V4, this content is hashed one level up in ExecKeyExchange or
         * RespondToKeyExchange. So no hashing is done here for that version.
         */
    }
}


bool KeyExchangerECDHE_ECDSA::IsTrustAnchor(const ECCPublicKey* publicKey)
{
    bool isTrustAnchor = false;
    trustAnchorList->Lock(MUTEX_CONTEXT);

    for (std::vector<std::shared_ptr<PermissionMgmtObj::TrustAnchor> >::iterator it = trustAnchorList->begin(); it != trustAnchorList->end(); it++) {
        if (*(*it)->keyInfo.GetPublicKey() == *publicKey) {
            isTrustAnchor = true;
            break;
        }
    }

    trustAnchorList->Unlock(MUTEX_CONTEXT);
    return isTrustAnchor;
}

QStatus KeyExchangerECDHE::KeyExchangeReadKeyInfo(MsgArg& variant)
{
    KeyInfoNISTP256 keyInfo;
    QStatus status = KeyInfoHelper::MsgArgToKeyInfoNISTP256PubKey(variant, keyInfo);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::KeyExchangeReadKeyInfo parsing KeyInfo fails status 0x%x\n", status));
        return status;
    }
    peerPubKey = *keyInfo.GetPublicKey();
    /* hash the handshake data */
    size_t exportedPublicKeySize = peerPubKey.GetSize();
    uint8_t* exportedPublicKey = new uint8_t [exportedPublicKeySize];
    QCC_VERIFY(ER_OK == peerPubKey.Export(exportedPublicKey, &exportedPublicKeySize));
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, exportedPublicKey, exportedPublicKeySize);
    peerState->ReleaseConversationHashLock(IsInitiator());
    delete[] exportedPublicKey;
    return ER_OK;
}

QStatus KeyExchangerECDHE::KeyExchangeReadKey(MsgArg& variant)
{
    if (!PeerSupportsKeyInfo()) {
        uint8_t eccCurveID;
        uint8_t* replyPubKey;
        size_t replyPubKeyLen;
        variant.Get("(yay)", &eccCurveID, &replyPubKeyLen, &replyPubKey);
        if (replyPubKeyLen != peerPubKey.GetSize()) {
            return ER_INVALID_DATA;
        }
        if (eccCurveID != ecc.GetCurveType()) {
            return ER_INVALID_DATA;
        }
        QCC_VERIFY(ER_OK == peerPubKey.Import(replyPubKey, replyPubKeyLen));
        /* hash the handshake data */
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, &eccCurveID, sizeof(eccCurveID));
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, replyPubKey, replyPubKeyLen);
        peerState->ReleaseConversationHashLock(IsInitiator());

        /* In CONVERSATION_V4, this content is hashed one level up in ExecKeyExchange or
         * RespondToKeyExchange. So no hashing is done here for that version.
         */

        return ER_OK;
    }
    return KeyExchangeReadKeyInfo(variant);
}

QStatus KeyExchangerECDHE::ExecKeyExchange(uint32_t authMask, KeyExchangerCB& callback, uint32_t* remoteAuthMask)
{
    QStatus status = GenerateECDHEKeyPair();
    if (status != ER_OK) {
        return status;
    }

    /* Hash the handshake data for version 1. This has to happen here instead of with the
     * hashing for version 4 because in version 1, KeyExchangeGen(Legacy)Key also hash data.
     */
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, HexStringToByteString(U32ToString(authMask, 16, 2 * sizeof(authMask), '0')));
    peerState->ReleaseConversationHashLock(IsInitiator());

    MsgArg variant;
    if (IsLegacyPeer()) {
        KeyExchangeGenLegacyKey(variant);
    } else {
        KeyExchangeGenKey(variant);
    }
    Message sentMsg(bus);
    Message replyMsg(bus);
    MsgArg args[2];
    args[0].Set("u", authMask);
    status = args[1].Set("v", &variant);
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE::ExecKeyExchange set variant fails status 0x%x\n", status));
        return status;
    }

    peerState->AcquireConversationHashLock(IsInitiator());
    status = callback.SendKeyExchange(args, ArraySize(args), &sentMsg, &replyMsg);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, sentMsg);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
    peerState->ReleaseConversationHashLock(IsInitiator());
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
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, HexStringToByteString(U32ToString(*remoteAuthMask, 16, 2 * sizeof(*remoteAuthMask), '0')));
    peerState->ReleaseConversationHashLock(IsInitiator());

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
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->GetDigest(IsInitiator(), digest, true);
    peerState->ReleaseConversationHashLock(IsInitiator());
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
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->GetDigest(IsInitiator(), digest, true);
    peerState->ReleaseConversationHashLock(IsInitiator());
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
        return ER_INVALID_DATA;
    }
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(computedRemoteVerifier, sizeof(computedRemoteVerifier));
    if (status != ER_OK) {
        return status;
    }
    *authorized = (Crypto_Compare(remoteVerifier, computedRemoteVerifier, sizeof(computedRemoteVerifier)) == 0);
    if (!IsInitiator()) {
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, remoteVerifier, remoteVerifierLen);
        peerState->ReleaseConversationHashLock(IsInitiator());
        /* In CONVERSATION_V4, the whole reply message including the variant is hashed one level up
         * in either this->KeyAuthentication or AllJoynPeerObj::DoKeyAuthentication.
         */
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
        KeyStore::Key key(KeyStore::Key::REMOTE, guid);
        status = keyStore.AddKey(key, secretBlob, accessRights);
        if (ER_OK == status) {
            status = keyStore.Store();
        }
    }
    return status;
}

QStatus KeyExchangerECDHE::StoreMasterSecret(const qcc::GUID128& guid, const uint8_t accessRights[4])
{
    return DoStoreMasterSecret(bus, guid, masterSecret, (const uint8_t*) GetSuiteName(), strlen(GetSuiteName()), secretExpiration, IsInitiator(), accessRights);
}

QStatus KeyExchanger::ParsePeerSecretRecord(const KeyBlob& rec, KeyBlob& masterSecret, ECCPublicKey* publicKey, uint8_t* manifestDigest, std::vector<ECCPublicKey>& issuerPublicKeys, bool& publicKeyAvailable)
{
    publicKeyAvailable = false;
    if ((rec.GetSize() == MASTER_SECRET_SIZE) || (rec.GetSize() == MASTER_SECRET_PINX_SIZE)) {
        /* support older format by the non ECDHE key exchanges */
        masterSecret = rec;
        if (publicKey) {
            publicKey->Clear();
        }
        if (manifestDigest) {
            memset(manifestDigest, 0, Crypto_SHA256::DIGEST_SIZE);
        }
        return ER_OK;
    }
    if (rec.GetSize() == 0) {
        return ER_INVALID_DATA;
    }
    uint8_t* pBuf = (uint8_t*) rec.GetData();
    uint8_t version = *pBuf;
    size_t bytesRead = sizeof(version);
    if (bytesRead > rec.GetSize()) {
        return ER_INVALID_DATA;
    }
    if (version != PEER_SECRET_RECORD_VERSION) {
        return ER_NOT_IMPLEMENTED;
    }
    pBuf += sizeof(version);
    if ((bytesRead + MASTER_SECRET_SIZE) > rec.GetSize()) {
        return ER_INVALID_DATA;
    }
    masterSecret.Set(pBuf, MASTER_SECRET_SIZE, rec.GetType());
    bytesRead += MASTER_SECRET_SIZE;
    pBuf += MASTER_SECRET_SIZE;
    /* recopy other fields from rec */
    masterSecret.SetTag(rec.GetTag(), rec.GetRole());
    Timespec<EpochTime> expiration;
    if (rec.GetExpiration(expiration)) {
        masterSecret.SetExpiration(expiration);
    }

    /* the public key field */
    size_t publicKeySize;
    if (publicKey) {
        publicKeySize = publicKey->GetSize();
    } else {
        ECCPublicKey emptyKey;
        publicKeySize = emptyKey.GetSize();
    }
    if ((bytesRead + publicKeySize) > rec.GetSize()) {
        return ER_INVALID_DATA;
    }
    if (publicKey) {
        publicKey->Import(pBuf, publicKeySize);
    }
    bytesRead += publicKeySize;
    pBuf += publicKeySize;

    /* the manifest field */
    if ((bytesRead + Crypto_SHA256::DIGEST_SIZE) > rec.GetSize()) {
        return ER_INVALID_DATA;
    }
    if (manifestDigest) {
        memcpy(manifestDigest, pBuf, Crypto_SHA256::DIGEST_SIZE);
    }
    bytesRead += Crypto_SHA256::DIGEST_SIZE;
    pBuf += Crypto_SHA256::DIGEST_SIZE;

    /* the number of issuers field */
    if ((bytesRead + sizeof(uint8_t)) > rec.GetSize()) {
        return ER_INVALID_DATA;
    }
    uint8_t numIssuerPublicKeys = *pBuf;
    bytesRead += sizeof(numIssuerPublicKeys);
    pBuf += sizeof(numIssuerPublicKeys);
    for (size_t cnt = 0; cnt < numIssuerPublicKeys; cnt++) {
        if ((bytesRead + publicKeySize) > rec.GetSize()) {
            return ER_INVALID_DATA;
        }
        ECCPublicKey issuerPubKey;
        QStatus status = issuerPubKey.Import(pBuf, publicKeySize);
        if (ER_OK != status) {
            return status;
        }
        issuerPublicKeys.push_back(issuerPubKey);
        bytesRead += publicKeySize;
        pBuf += publicKeySize;
    }
    publicKeyAvailable = true;
    return ER_OK;
}

QStatus KeyExchanger::ParsePeerSecretRecord(const KeyBlob& rec, KeyBlob& masterSecret)
{
    bool publicKeyAvail;
    std::vector<ECCPublicKey> issuerKeys;
    return ParsePeerSecretRecord(rec, masterSecret, NULL, NULL, issuerKeys, publicKeyAvail);
}

QStatus KeyExchangerECDHE_ECDSA::StoreMasterSecret(const qcc::GUID128& guid, const uint8_t accessRights[4])
{
    if (peerDSAPubKey) {
        /* build a new keyblob with master secret, peer DSA public key, manifest digest, and issuer public keys */
        size_t bufferSize = CalcPeerSecretRecordSize(peerIssuerPubKeys.size());
        uint8_t* buffer = new uint8_t[bufferSize];
        uint8_t* pBuf = buffer;
        /* the version field */
        *pBuf = PEER_SECRET_RECORD_VERSION;
        pBuf += sizeof(uint8_t);
        /* the master secret field */
        memcpy(pBuf, masterSecret.GetData(), MASTER_SECRET_SIZE);
        pBuf += MASTER_SECRET_SIZE;
        /* the public key field */
        size_t keySize = peerDSAPubKey->GetSize();
        peerDSAPubKey->Export(pBuf, &keySize);
        pBuf += keySize;
        /* the manifest digest field */
        memcpy(pBuf, peerManifestDigest, sizeof(peerManifestDigest));
        pBuf += sizeof(peerManifestDigest);
        /* the numIssuerPublicKeys field */
        *pBuf = (uint8_t) peerIssuerPubKeys.size();
        pBuf += sizeof(uint8_t);
        if (peerIssuerPubKeys.size() > 0) {
            for (size_t cnt = 0; cnt < peerIssuerPubKeys.size(); cnt++) {
                size_t bufSize = peerIssuerPubKeys[cnt].GetSize();
                QStatus status = peerIssuerPubKeys[cnt].Export(pBuf, &bufSize);
                if (ER_OK != status) {
                    delete[] buffer;
                    return status;
                }
                pBuf += bufSize;
            }
        }
        KeyBlob kb(buffer, bufferSize, KeyBlob::GENERIC);
        delete [] buffer;

        return DoStoreMasterSecret(bus, guid, kb, (const uint8_t*) GetSuiteName(), strlen(GetSuiteName()), secretExpiration, IsInitiator(), accessRights);
    } else {
        return DoStoreMasterSecret(bus, guid, masterSecret, (const uint8_t*) GetSuiteName(), strlen(GetSuiteName()), secretExpiration, IsInitiator(), accessRights);
    }
}

QStatus KeyExchanger::ReplyWithVerifier(Message& msg)
{
    /* compute the local verifier to send back */
    uint8_t verifier[AUTH_VERIFIER_LEN];
    QStatus status = GenerateLocalVerifier(verifier, sizeof(verifier));
    if (status != ER_OK) {
        return status;
    }
    MsgArg variant;
    variant.Set("ay", sizeof(verifier), verifier);
    MsgArg replyArg("v", &variant);
    Message replyMsg(bus);
    peerState->AcquireConversationHashLock(IsInitiator());
    status = peerObj->HandleMethodReply(msg, replyMsg, &replyArg, 1);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
    peerState->ReleaseConversationHashLock(IsInitiator());
    return status;
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
        SetSecretExpiration(1);      /* expires in one second */
    }
    return ER_OK;
}

QStatus KeyExchangerECDHE_NULL::KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized)
{
    *authorized = false;
    QStatus status = GenerateMasterSecret(&peerPubKey);
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
    Message sentMsg(bus);
    Message replyMsg(bus);
    MsgArg verifierArg("ay", sizeof(verifier), verifier);
    MsgArg verifierMsg("v", &verifierArg);

    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, verifier, sizeof(verifier));
    status = callback.SendKeyAuthentication(&verifierMsg, &sentMsg, &replyMsg);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, sentMsg);
    if (status != ER_OK) {
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
        peerState->ReleaseConversationHashLock(IsInitiator());
        return status;
    }
    peerState->ReleaseConversationHashLock(IsInitiator());

    MsgArg* variant;
    status = replyMsg->GetArg(0)->Get("v", &variant);
    if (status != ER_OK) {
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
        peerState->ReleaseConversationHashLock(IsInitiator());
        return status;
    }
    status = ValidateRemoteVerifierVariant(peerName, variant, authorized);
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
    peerState->ReleaseConversationHashLock(IsInitiator());
    return status;
}

QStatus KeyExchangerECDHE_PSK::ReplyWithVerifier(Message& msg)
{
    /* compute the local verifier to send back */
    uint8_t verifier[AUTH_VERIFIER_LEN];
    QStatus status = GenerateLocalVerifier(verifier, sizeof(verifier));
    if (status != ER_OK) {
        return status;
    }
    MsgArg variant;
    variant.Set("(ayay)", pskName.length(), pskName.data(), sizeof(verifier), verifier);
    MsgArg replyArg("v", &variant);
    Message replyMsg(bus);
    peerState->AcquireConversationHashLock(IsInitiator());
    status = peerObj->HandleMethodReply(msg, replyMsg, &replyArg, 1);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
    peerState->ReleaseConversationHashLock(IsInitiator());
    return status;

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
        QCC_DbgPrintf(("KeyExchangerECDHE_PSK::RequestCredentialsCB PSK value not provided"));
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
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->GetDigest(IsInitiator(), digest, true);
    peerState->ReleaseConversationHashLock(IsInitiator());
    if (GetPeerAuthVersion() >= CONVERSATION_V4) {
        qcc::String seed((const char*)digest, sizeof(digest));
        seed += pskName;
        seed += pskValue;
        QStatus status = Crypto_PseudorandomFunction(masterSecret, label.c_str(), seed, verifier, verifierLen);
        seed.secure_clear();
        return status;
    } else {
        return GenerateVerifier(label.c_str(), digest, sizeof(digest), masterSecret, verifier, verifierLen);
    }
}

QStatus KeyExchangerECDHE_PSK::GenerateRemoteVerifier(uint8_t* peerPskName, size_t peerPskNameLength, uint8_t* verifier, size_t verifierLen)
{
    qcc::String label;
    if (IsInitiator()) {
        label.assign("server finished");
    } else {
        label.assign("client finished");
    }
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->GetDigest(IsInitiator(), digest, true);
    peerState->ReleaseConversationHashLock(IsInitiator());

    /* In CONVERSATION_V4, the hash captures the entire conversation for its protection, and so
     * the PSK cannot be hashed into it, because if the PSK between the peers mismatches the digests will never
     * agree, and no other auth mechanism could possibly succeed. Instead, the PSK is added here as an input to
     * the seed that generates the verifier.
     */
    if (GetPeerAuthVersion() >= CONVERSATION_V4) {
        qcc::String seed((const char*)digest, sizeof(digest));
        seed.append((const char*)peerPskName, peerPskNameLength);
        seed += pskValue;
        QStatus status = Crypto_PseudorandomFunction(masterSecret, label.c_str(), seed, verifier, verifierLen);
        seed.secure_clear();
        return status;
    } else {
        return GenerateVerifier(label.c_str(), digest, sizeof(digest), masterSecret, verifier, verifierLen);
    }
}

QStatus KeyExchangerECDHE_PSK::ValidateRemoteVerifierVariant(const char* peerName, MsgArg* variant, uint8_t* authorized)
{
    QStatus status;
    *authorized = false;
    uint8_t* peerPskName;
    size_t peerPskNameLen;
    uint8_t* remoteVerifier;
    size_t remoteVerifierLen;
    status = variant->Get("(ayay)", &peerPskNameLen, &peerPskName, &remoteVerifierLen, &remoteVerifier);
    pskName.assign((const char*) peerPskName, peerPskNameLen);
    if (!IsInitiator()) {
        status = RequestCredentialsCB(peerName);
        if (status != ER_OK) {
            return status;
        }
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, peerPskName, peerPskNameLen);
        /* Calling SetConversationHashSensitiveMode ensures the PSK won't end up in the log if conversation
         * hash tracing is turned on.
         */
        peerState->SetConversationHashSensitiveMode(IsInitiator(), true);
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, (const uint8_t*) pskValue.data(), pskValue.length());
        peerState->SetConversationHashSensitiveMode(IsInitiator(), false);
        peerState->ReleaseConversationHashLock(IsInitiator());
    }
    if (remoteVerifierLen != AUTH_VERIFIER_LEN) {
        return ER_INVALID_DATA;
    }
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(peerPskName, peerPskNameLen, computedRemoteVerifier, sizeof(computedRemoteVerifier));
    if (status != ER_OK) {
        return status;
    }
    *authorized = (Crypto_Compare(remoteVerifier, computedRemoteVerifier, sizeof(computedRemoteVerifier)) == 0);
    if (!IsInitiator()) {
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, remoteVerifier, remoteVerifierLen);
        peerState->ReleaseConversationHashLock(IsInitiator());
    }
    return ER_OK;
}

QStatus KeyExchangerECDHE_PSK::KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized)
{
    *authorized = false;
    QStatus status = GenerateMasterSecret(&peerPubKey);
    if (status != ER_OK) {
        return status;
    }
    /* check the Auth listener */
    status = RequestCredentialsCB(peerName);
    if (status != ER_OK) {
        return status;
    }

    /* hash the handshake */
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, (const uint8_t*)pskName.data(), pskName.length());
    /* Calling SetConversationHashSensitiveMode ensures the PSK won't end up in the log if conversation
     * hash tracing is turned on.
     */
    peerState->SetConversationHashSensitiveMode(IsInitiator(), true);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, (const uint8_t*)pskValue.data(), pskValue.length());
    peerState->SetConversationHashSensitiveMode(IsInitiator(), false);
    peerState->ReleaseConversationHashLock(IsInitiator());

    uint8_t verifier[AUTH_VERIFIER_LEN];
    GenerateLocalVerifier(verifier, sizeof(verifier));
    Message sentMsg(bus);
    Message replyMsg(bus);
    MsgArg verifierArg;
    status = verifierArg.Set("(ayay)", pskName.length(), pskName.data(), sizeof(verifier), verifier);
    if (status != ER_OK) {
        return status;
    }
    MsgArg verifierMsg("v", &verifierArg);

    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, verifier, sizeof(verifier));

    status = callback.SendKeyAuthentication(&verifierMsg, &sentMsg, &replyMsg);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, sentMsg);
    if (status != ER_OK) {
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
        peerState->ReleaseConversationHashLock(IsInitiator());
        return status;
    }
    peerState->ReleaseConversationHashLock(IsInitiator());
    MsgArg* variant;
    status = replyMsg->GetArg(0)->Get("v", &variant);
    if (status != ER_OK) {
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
        peerState->ReleaseConversationHashLock(IsInitiator());
        return status;
    }
    status = ValidateRemoteVerifierVariant(peerName, variant, authorized);
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
    peerState->ReleaseConversationHashLock(IsInitiator());
    return status;
}

QStatus KeyExchangerECDHE_ECDSA::ParseCertChainPEM(String& encodedCertChain)
{
    size_t count = 0;
    QStatus status = CertificateHelper::GetCertCount(encodedCertChain, &count);
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

KeyExchangerECDHE_ECDSA::~KeyExchangerECDHE_ECDSA()
{
    delete [] certChain;
    delete peerDSAPubKey;
    peerIssuerPubKeys.clear();
}

QStatus KeyExchangerECDHE_ECDSA::RequestCredentialsCB(const char* peerName)
{
    /* check the Auth listener */
    AuthListener::Credentials creds;
    uint16_t credsMask = AuthListener::CRED_PRIVATE_KEY | AuthListener::CRED_CERT_CHAIN | AuthListener::CRED_EXPIRATION;

    bool ok = listener.RequestCredentials(GetSuiteName(), peerName, authCount, "", credsMask, creds);
    if (!ok) {
        QCC_DbgHLPrintf(("listener::RequestCredentials returns false"));
        return ER_AUTH_FAIL;
    }
    /* private key is required */
    if (!creds.IsSet(AuthListener::CRED_PRIVATE_KEY)) {
        QCC_DbgPrintf(("listener::RequestCredentials does not provide private key"));
        return ER_AUTH_FAIL;
    }
    /* cert chain is required */
    if (!creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
        QCC_DbgPrintf(("listener::RequestCredentials does not provide certificate chain"));
        return ER_AUTH_FAIL;
    }
    if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
        SetSecretExpiration(creds.GetExpiration());
    } else {
        SetSecretExpiration(0xFFFFFFFF);      /* never expired */
    }
    qcc::String pemCertChain = creds.GetCertChain();
    QStatus status = CertificateX509::DecodePrivateKeyPEM(creds.GetPrivateKey(), &issuerPrivateKey);
    if (status != ER_OK) {
        QCC_DbgPrintf(("RequestCredentialsCB failed to parse the private key PEM"));
        return status;
    }
    status = ParseCertChainPEM((qcc::String&)creds.GetCertChain());
    if (status != ER_OK) {
        QCC_DbgPrintf(("RequestCredentialsCB failed to parse the certificate chain"));
        return status;
    }
    if (certChainLen == 0) {
        QCC_DbgPrintf(("RequestCredentialsCB receives empty the certificate chain"));
        return ER_AUTH_FAIL; /* need both private key and public key */
    }
    issuerPublicKey = *certChain[0].GetSubjectPublicKey();
    return ER_OK;
}

static qcc::String EncodePEMCertChain(CertificateX509* certs, size_t numCerts)
{
    qcc::String chain;
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        if (cnt > 0) {
            chain += "\n";
        }
        chain += certs[cnt].GetPEM();
    }
    return chain;
}

QStatus KeyExchangerECDHE_ECDSA::VerifyCredentialsCB(const char* peerName, CertificateX509* certs, size_t numCerts)
{
    if (numCerts == 0) {
        QCC_DbgPrintf(("VerifyCredentialsCB failed because of no certificate"));
        return ER_AUTH_FAIL;
    }
    AuthListener::Credentials creds;
    creds.SetCertChain(EncodePEMCertChain(certs, numCerts));
    /* check with listener to validate the cert chain */
    bool ok = listener.VerifyCredentials(GetSuiteName(), peerName, creds);
    if (!ok) {
        QCC_DbgPrintf(("KeyExchangerECDHE_ECDSA::VerifyCredentialsCB listener::VerifyCredentials failed"));
        return ER_AUTH_FAIL;
    }
    return ER_OK;
}

/**
 * validate whether the certificate chain structure is a valid
 */
bool KeyExchangerECDHE_ECDSA::IsCertChainStructureValid(const CertificateX509* certs, size_t numCerts)
{
    if ((numCerts == 0) || !certs) {
        QCC_DbgPrintf(("Empty certificate chain"));
        return false;
    }
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        if (ER_OK != certs[cnt].VerifyValidity()) {
            QCC_DbgPrintf(("Invalid certificate date validity period"));
            return false;
        }
    }
    /* Leaf certificate must have an AllJoyn EKU or be unrestricted.
     * The reason to allow unrestricted here is for backwards compatibility
     * with pre 1509 peers that use certificates without EKUs in Security 1.0
     * contexts. Certificates created by AllJoyn in 1509 and above will always
     * have an EKU indicating the type.
     */
    if ((certs[0].GetType() != CertificateX509::IDENTITY_CERTIFICATE) &&
        (certs[0].GetType() != CertificateX509::MEMBERSHIP_CERTIFICATE) &&
        (certs[0].GetType() != CertificateX509::UNRESTRICTED_CERTIFICATE)) {
        QCC_DbgPrintf(("Invalid EKU"));
        return false;
    }
    if (numCerts == 1) {
        return true;
    }
    for (size_t cnt = 0; cnt < (numCerts - 1); cnt++) {
        if (!certs[cnt + 1].IsCA()) {
            QCC_DbgPrintf(("Certificate basic extension CA is false"));
            return false;
        }
        if (!certs[cnt + 1].IsIssuerOf(certs[cnt])) {
            QCC_DbgPrintf(("Certificate chain issuer DN verification failed"));
            return false;
        }
    }
    return true;
}

/**
 * Extract the list of issuer keys from the cert chain
 */
static void ExtractIssuerPublicKeys(const CertificateX509* certs, size_t numCerts, PermissionMgmtObj::TrustAnchorList* trustAnchorList, std::vector<ECCPublicKey>& issuerKeys)
{
    if ((numCerts == 0) || !certs) {
        return;
    }
    if (numCerts == 1) {
        /* use issuer authority key id to locate the issuer's public key
         * from the list of trust anchors
         */
        qcc::String aki = certs[0].GetAuthorityKeyId();
        if (aki.size() == 0) {
            return;
        }

        trustAnchorList->Lock(MUTEX_CONTEXT);

        for (std::vector<std::shared_ptr<PermissionMgmtObj::TrustAnchor> >::const_iterator it = trustAnchorList->begin(); it != trustAnchorList->end(); it++) {
            if ((aki.size() == (*it)->keyInfo.GetKeyIdLen()) &&
                (memcmp(aki.data(), (*it)->keyInfo.GetKeyId(), aki.size()) == 0) &&
                (ER_OK == certs[0].Verify((*it)->keyInfo.GetPublicKey()))) {
                issuerKeys.push_back(*(*it)->keyInfo.GetPublicKey());
            }
        }

        trustAnchorList->Unlock(MUTEX_CONTEXT);
    }
    /* Skip the end-entity cert[0], go through the issuer certs to collect
     * the issuers' public key.
     */
    for (size_t cnt = 1; cnt < numCerts; cnt++) {
        issuerKeys.push_back(*certs[cnt].GetSubjectPublicKey());
    }
}

/**
 * Calculate the number of seconds before the secret expires based on the input
 * expiry and the certificate validity period.  The secret can't outlast the
 * certificate validity period.
 * @param cert the certificate
 * @param expiry[in,out] the number of seconds before the secret expires.
 */
static void CalculateSecretExpiration(const CertificateX509& cert, uint32_t& expiry)
{
    uint64_t currentTime = GetEpochTimestamp() / 1000;
    if (cert.GetValidity()->validTo < currentTime) {
        expiry = 0;
    } else if ((cert.GetValidity()->validTo - currentTime) < expiry) {
        expiry = cert.GetValidity()->validTo - currentTime;
    }
}

QStatus KeyExchangerECDHE_ECDSA::ValidateRemoteVerifierVariant(const char* peerName, MsgArg* variant, uint8_t* authorized)
{
    QStatus status = ER_OK;
    if (!IsInitiator()) {
        status = RequestCredentialsCB(peerName);
        if (status != ER_OK) {
            QCC_DbgPrintf(("Error requesting credentials from listener"));
            return status;
        }
    }

    MsgArg* sigInfoVariant;
    uint8_t certChainEncoding;
    MsgArg* certChainVariant;
    status = variant->Get("(vyv)", &sigInfoVariant, &certChainEncoding, &certChainVariant);
    if (status != ER_OK) {
        QCC_LogError(status, ("Invalid parameters for remote verifier"));
        return status;
    }
    if ((certChainEncoding != CertificateX509::ENCODING_X509_DER) &&
        (certChainEncoding != CertificateX509::ENCODING_X509_DER_PEM)) {
        QCC_LogError(ER_INVALID_DATA, ("Certificate data must be in DER or PEM format"));
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
        QCC_LogError(ER_INVALID_DATA, ("Verifier signature algorithm must be SHA256"));
        return ER_INVALID_DATA;
    }
    size_t rCoordLen;
    uint8_t* rCoord;
    size_t sCoordLen;
    uint8_t* sCoord;
    status = sigVariant->Get("(ayay)", &rCoordLen, &rCoord, &sCoordLen, &sCoord);
    if (status != ER_OK) {
        QCC_LogError(status, ("Invalid verifier signature data"));
        return status;
    }
    if (rCoordLen != ECC_COORDINATE_SZ) {
        QCC_LogError(ER_INVALID_DATA, ("Invalid verifier signature data size (r)"));
        return ER_INVALID_DATA;
    }
    if (sCoordLen != ECC_COORDINATE_SZ) {
        QCC_LogError(ER_INVALID_DATA, ("Invalid verifier signature data size (s)"));
        return ER_INVALID_DATA;
    }
    /* compute the remote verifier */
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(computedRemoteVerifier, sizeof(computedRemoteVerifier));
    if (status != ER_OK) {
        QCC_LogError(status, ("Error computing remote verifier"));
        return status;
    }

    /* Hashing for CONVERSATION_V4 is done one level up in KeyAuthentication. */
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, rCoord, rCoordLen);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, sCoord, sCoordLen);
    peerState->ReleaseConversationHashLock(IsInitiator());

    /* handle the certChain variant */
    MsgArg* chainArg;
    size_t numCerts;
    status = certChainVariant->Get("a(ay)", &numCerts, &chainArg);
    if (status != ER_OK) {
        QCC_LogError(status, ("Error retrieving peer's certificate chain"));
        return status;
    }
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, &certChainEncoding, sizeof(certChainEncoding));
    peerState->ReleaseConversationHashLock(IsInitiator());
    if (numCerts == 0) {
        /* no cert chain to validate.  So it's not authorized */
        QCC_DbgPrintf(("Peer's certificate chain is empty.  Not authorized"));
        return ER_OK;
    }

    CertificateX509* certs = new CertificateX509[numCerts];
    size_t encodedLen;
    uint8_t* encoded;
    for (size_t cnt = 0; cnt < numCerts; cnt++) {
        status = chainArg[cnt].Get("(ay)", &encodedLen, &encoded);
        if (status != ER_OK) {
            delete [] certs;
            QCC_LogError(status, ("Error retrieving peer's certificate chain"));
            return status;
        }
        if (certChainEncoding == CertificateX509::ENCODING_X509_DER) {
            status = certs[cnt].DecodeCertificateDER(String((const char*) encoded, encodedLen));
        } else if (certChainEncoding == CertificateX509::ENCODING_X509_DER_PEM) {
            status = certs[cnt].DecodeCertificatePEM(String((const char*) encoded, encodedLen));
        } else {
            delete [] certs;
            QCC_LogError(status, ("Peer's certificate chain data are not in DER or PEM format"));
            return ER_INVALID_DATA;
        }
        if (status != ER_OK) {
            QCC_LogError(status, ("Error parsing peer's certificate chain"));
            delete [] certs;
            return status;
        }
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, encoded, encodedLen);
        peerState->ReleaseConversationHashLock(IsInitiator());
    }
    /* verify signature */
    Crypto_ECC cryptoEcc;
    cryptoEcc.SetDSAPublicKey(certs[0].GetSubjectPublicKey());
    SigInfoECC sigInfo;
    sigInfo.SetRCoord(rCoord);
    sigInfo.SetSCoord(sCoord);
    status = cryptoEcc.DSAVerifyDigest(computedRemoteVerifier, sizeof(computedRemoteVerifier), sigInfo.GetSignature());
    *authorized = (ER_OK == status);

    if (!*authorized) {
        delete [] certs;
        QCC_LogError(ER_OK, ("Verifier mismatched"));
        return ER_OK;  /* not authorized */
    }
    if (!IsCertChainStructureValid(certs, numCerts)) {
        *authorized = false;
        delete [] certs;
        QCC_LogError(ER_OK, ("Certificate chain structure is invalid"));
        return ER_OK;
    }
    status = VerifyCredentialsCB(peerName, certs, numCerts);
    if (ER_OK != status) {
        if (!IsTrustAnchor(certs[0].GetSubjectPublicKey())) {
            *authorized = false;
        }
    }

    if (*authorized) {
        peerDSAPubKey = new ECCPublicKey();
        *peerDSAPubKey = *(certs[0].GetSubjectPublicKey());
        if (certs[0].GetDigestSize() == Crypto_SHA256::DIGEST_SIZE) {
            memcpy(peerManifestDigest, certs[0].GetDigest(), Crypto_SHA256::DIGEST_SIZE);
        }

        ExtractIssuerPublicKeys(certs, numCerts, trustAnchorList, peerIssuerPubKeys);
        CalculateSecretExpiration(certs[0], secretExpiration);
    } else {
        QCC_DbgPrintf(("Not authorized by VerifyCredential callback"));
    }
    delete [] certs;
    return ER_OK;
}


QStatus KeyExchangerECDHE_ECDSA::ReplyWithVerifier(Message& msg)
{
    QStatus status;
    MsgArg variant;
    status = GenVerifierSigInfoArg(variant, false);
    if (ER_OK != status) {
        return status;
    }
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    MsgArg replyArg("v", &variant);
    Message replyMsg(bus);
    peerState->AcquireConversationHashLock(IsInitiator());
    status = peerObj->HandleMethodReply(msg, replyMsg, &replyArg, 1);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
    peerState->ReleaseConversationHashLock(IsInitiator());
    return status;
}

QStatus KeyExchangerECDHE_ECDSA::GenVerifierSigInfoArg(MsgArg& msgArg, bool updateHash)
{
    /* build the SigInfo object */
    SigInfoECC sigInfo;
    uint8_t verifier[AUTH_VERIFIER_LEN];
    GenerateLocalVerifier(verifier, sizeof(verifier));

    Crypto_ECC cryptoEcc;
    cryptoEcc.SetDSAPrivateKey(&issuerPrivateKey);
    ECCSignature sig;
    QCC_DbgHLPrintf(("Verifier: %s\n", BytesToHexString(verifier, sizeof(verifier)).c_str()));
    QStatus status = cryptoEcc.DSASignDigest(verifier, sizeof(verifier), &sig);
    if (status != ER_OK) {
        QCC_LogError(status, ("KeyExchangerECDHE_ECDSA::GenVerifierSigInfoArg failed to generate local verifier sig info"));
        return status;
    }
    QCC_DEBUG_ONLY(
        /*
         * In debug builds, test that the signature we just created verifies properly with
         * our public key, before sending it.  If this check fails, the likely cause is
         * that the signer's private key is not consistent with their certificate (i.e.,
         * issuerPrivateKey*NistP256BasePoint != issuerPublicKey). Since this check adds
         * considerable cost to the authentication protocol and is not security critical,
         * it should not be done for release builds.
         */
        cryptoEcc.SetDSAPublicKey(&issuerPublicKey);
        status = cryptoEcc.DSAVerifyDigest(verifier, sizeof(verifier), &sig);
        if (status != ER_OK) {
            QCC_DbgPrintf(("KeyExchangerECDHE_ECDSA::GenVerifierSigInfoArg failed to verify the signature just created, the key exchange protocol will fail."));
            QCC_ASSERT(false);
        }
        );

    sigInfo.SetSignature(&sig);
    if (updateHash) {
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, sigInfo.GetRCoord(), sigInfo.GetRSize());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, sigInfo.GetSCoord(), sigInfo.GetSSize());
        peerState->ReleaseConversationHashLock(IsInitiator());
        /* Hashing for CONVERSATION_V4 is handled one level up in KeyAuthentication. */
    }

    MsgArg* certArgs = NULL;
    size_t certArgsCount = 0;
    uint8_t encoding = CertificateX509::ENCODING_X509_DER;
    if (updateHash) {
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, &encoding, sizeof(encoding));
        peerState->ReleaseConversationHashLock(IsInitiator());
    }
    if (certChainLen > 0) {
        certArgsCount = certChainLen;
        certArgs = new MsgArg[certChainLen];
        /* add the local cert chain to the list of certs to send */
        for (size_t cnt = 0; cnt < certChainLen; cnt++) {
            String der;
            status = certChain[cnt].EncodeCertificateDER(der);
            if (status != ER_OK) {
                QCC_LogError(status, ("KeyExchangerECDHE_ECDSA::GenVerifierSigInfoArg failed to generate DER encoding for certificate"));
                delete [] certArgs;
                return status;
            }
            certArgs[cnt].Set("(ay)", der.size(), (const uint8_t*) der.data());
            certArgs[cnt].Stabilize();
            if (updateHash) {
                peerState->AcquireConversationHashLock(IsInitiator());
                peerState->UpdateHash(IsInitiator(), CONVERSATION_V1, (const uint8_t*) der.data(), der.size());
                peerState->ReleaseConversationHashLock(IsInitiator());
            }
        }
    }
    /* copy the message args */
    MsgArg localArg;
    localArg.Set("(vyv)",
                 new MsgArg("(yv)", sigInfo.GetAlgorithm(),
                            new MsgArg("(ayay)", ECC_COORDINATE_SZ, sigInfo.GetRCoord(), ECC_COORDINATE_SZ, sigInfo.GetSCoord())),
                 encoding,
                 new MsgArg("a(ay)", certArgsCount, certArgs));
    localArg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    msgArg = localArg;

    return ER_OK;
}

QStatus KeyExchangerECDHE_ECDSA::KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized)
{
    *authorized = false;
    QStatus status = GenerateMasterSecret(&peerPubKey);
    if (status != ER_OK) {
        QCC_LogError(status, ("Error generating master secret"));
        return status;
    }
    /* check the Auth listener */
    status = RequestCredentialsCB(peerName);
    if (status != ER_OK) {
        QCC_DbgPrintf(("Error requesting credentials from listener"));
        return status;
    }

    /* compute the local verifier to send back */
    MsgArg variant;
    status = GenVerifierSigInfoArg(variant, true);
    if (status != ER_OK) {
        QCC_LogError(status, ("Error generating verifier"));
        return status;
    }
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);

    MsgArg verifierMsg("v", &variant);

    Message sentMsg(bus);
    Message replyMsg(bus);
    peerState->AcquireConversationHashLock(IsInitiator());
    status = callback.SendKeyAuthentication(&verifierMsg, &sentMsg, &replyMsg);
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, sentMsg);
    if (status != ER_OK) {
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
        peerState->ReleaseConversationHashLock(IsInitiator());
        return status;
    }
    peerState->ReleaseConversationHashLock(IsInitiator());
    MsgArg* remoteVariant;
    status = replyMsg->GetArg(0)->Get("v", &remoteVariant);
    if (status != ER_OK) {
        peerState->AcquireConversationHashLock(IsInitiator());
        peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
        peerState->ReleaseConversationHashLock(IsInitiator());
        return status;
    }
    status = ValidateRemoteVerifierVariant(peerName, remoteVariant, authorized);
    peerState->AcquireConversationHashLock(IsInitiator());
    peerState->UpdateHash(IsInitiator(), CONVERSATION_V4, replyMsg);
    peerState->ReleaseConversationHashLock(IsInitiator());
    return status;
}

bool KeyExchanger::IsLegacyPeer()
{
    return (GetPeerAuthVersion() == LEGACY_AUTH_VERSION);
}

bool KeyExchanger::PeerSupportsKeyInfo()
{
    return GetPeerAuthVersion() >= KEY_INFO_AUTH_VERSION;
}

} /* namespace ajn */
