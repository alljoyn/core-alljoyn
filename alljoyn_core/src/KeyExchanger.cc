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
 * The auth version with support for trust anchors
 */
#define TRUST_ANCHORS_AUTH_VERSION      4

/**
 * The variant types for the org.alljoyn.Bus.Peer.Authentication KeyExchange method call.
 */
#define EXCHANGE_KEYINFO 1
#define EXCHANGE_TRUST_ANCHORS 2

/* the size of the master secret based on RFC5246 */
#define MASTER_SECRET_SIZE 48
/* the size of the master secret used in the PIN key exchange */
#define MASTER_SECRET_PINX_SIZE 24

struct PeerSecretRecord {
    uint8_t version;
    uint8_t secret[MASTER_SECRET_SIZE];
    ECCPublicKey publicKey;

    PeerSecretRecord() : version(1)
    {
    }
};

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
    void SetAlgorithm(uint8_t algorithm)
    {
        this->algorithm = algorithm;
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
     * Set the signature.  The signature is copied into the internal buffer.
     */
    void SetSignature(const ECCSignature* sig)
    {
        this->sig = *sig;
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
        Crypto_SHA256 sha;
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        sha.Init();
        sha.Update((const uint8_t*) &pms, sizeof (ECCSecret));
        sha.GetDigest(digest);
        KeyBlob pmsBlob(digest, sizeof (digest), KeyBlob::GENERIC);
        status = Crypto_PseudorandomFunction(pmsBlob, "master secret", "", keymatter, sizeof (keymatter));
    }
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

    status = GenerateMasterSecret(&peerPubKey);
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
    Crypto_ECC_OldEncoding::ReEncode(GetECDHEPublicKey(), &oldenc);
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
    Crypto_ECC_OldEncoding::ReEncode(&oldenc, &peerPubKey);
    /* hash the handshake data */
    hashUtil.Update(replyPubKey, replyPubKeyLen);

    return ER_OK;
}

void KeyExchangerECDHE::KeyExchangeGenKeyInfo(MsgArg& variant)
{
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(GetECDHEPublicKey());
    MsgArg* keyInfoVariant = new MsgArg();
    KeyInfoHelper::KeyInfoNISTP256ToMsgArg(keyInfo, *keyInfoVariant);
    variant.Set("(yv)", EXCHANGE_KEYINFO, keyInfoVariant);
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    hashUtil.Update((uint8_t*) GetECDHEPublicKey(), sizeof(ECCPublicKey));
}

void KeyExchangerECDHE::KeyExchangeGenKey(MsgArg& variant)
{
    if (PeerSupportsTrustAnchors()) {
        MsgArg* entries = new MsgArg[1];
        KeyExchangeGenKeyInfo(entries[0]);
        variant.Set("a(yv)", 1, entries);
        variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    } else {
        uint8_t curveType = ecc.GetCurveType();
        variant.Set("(yay)", curveType, sizeof(ECCPublicKey), (const uint8_t*) GetECDHEPublicKey());
        variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
        hashUtil.Update((uint8_t*) &curveType, sizeof(uint8_t));
        hashUtil.Update((uint8_t*) GetECDHEPublicKey(), sizeof(ECCPublicKey));
    }
}

void KeyExchangerECDHE_ECDSA::KeyExchangeGenTrustAnchorKeyInfos(MsgArg& variant)
{
    MsgArg* entries = new MsgArg[trustAnchorList->size()];
    size_t cnt = 0;
    for (PermissionMgmtObj::TrustAnchorList::iterator it = trustAnchorList->begin(); it != trustAnchorList->end(); it++) {
        PermissionMgmtObj::TrustAnchor* ta = *it;
        KeyInfoHelper::KeyInfoNISTP256ToMsgArg(ta->keyInfo, entries[cnt]);
        hashUtil.Update((uint8_t*) ta->keyInfo.GetPublicKey(), sizeof(ECCPublicKey));
        cnt++;
    }
    variant.Set("(yv)", EXCHANGE_TRUST_ANCHORS, new MsgArg("a(yv)", trustAnchorList->size(), entries));
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
}

bool KeyExchangerECDHE_ECDSA::IsTrustAnchor(const ECCPublicKey* publicKey)
{
    for (PermissionMgmtObj::TrustAnchorList::iterator it = trustAnchorList->begin(); it != trustAnchorList->end(); it++) {

        if (memcmp((*it)->keyInfo.GetPublicKey(), publicKey, sizeof(ECCPublicKey)) == 0) {
            return true;
        }
    }
    return false;
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
    QStatus status = KeyInfoHelper::MsgArgToKeyInfoNISTP256(variant, keyInfo);
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
    if (!PeerSupportsTrustAnchors()) {
        uint8_t eccCurveID;
        uint8_t* replyPubKey;
        size_t replyPubKeyLen;
        variant.Get("(yay)", &eccCurveID, &replyPubKeyLen, &replyPubKey);
        if (replyPubKeyLen != sizeof(ECCPublicKey)) {
            return ER_INVALID_DATA;
        }
        if (eccCurveID != ecc.GetCurveType()) {
            return ER_INVALID_DATA;
        }
        peerPubKey.Import(replyPubKey, sizeof(ECCPublicKey));
        /* hash the handshake data */
        hashUtil.Update(&eccCurveID, sizeof(uint8_t));
        hashUtil.Update(replyPubKey, replyPubKeyLen);
        return ER_OK;
    }
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
        PermissionMgmtObj::TrustAnchor* ta = new PermissionMgmtObj::TrustAnchor();
        status = KeyInfoHelper::MsgArgToKeyInfoNISTP256(entries[cnt], ta->keyInfo);
        if (status != ER_OK) {
            QCC_DbgHLPrintf(("KeyExchangerECDHE::KeyExchangeReadTrustAnchorKeyInfo parsing KeyInfo fails status 0x%x\n", status));
            delete ta;
            return status;
        }

        peerTrustAnchorList.push_back(ta);
        /* hash the handshake data */
        hashUtil.Update((uint8_t*) ta->keyInfo.GetPublicKey(), sizeof(ECCPublicKey));
    }
    return ER_OK;
}

/**
 * Check to whether the two lists have a common trust anchor
 */
static bool HasCommonTrustAnchors(PermissionMgmtObj::TrustAnchorList& list1, PermissionMgmtObj::TrustAnchorList& list2)
{
    for (PermissionMgmtObj::TrustAnchorList::iterator it1 = list1.begin(); it1 != list1.end(); it1++) {
        for (PermissionMgmtObj::TrustAnchorList::iterator it2 = list2.begin(); it2 != list2.end(); it2++) {
            /* compare the public key */
            if (memcmp((*it1)->keyInfo.GetPublicKey(), (*it2)->keyInfo.GetPublicKey(), sizeof(ECCPublicKey)) == 0) {
                return true;
            }
        }
    }
    return false;
}

QStatus KeyExchangerECDHE_ECDSA::KeyExchangeReadKey(MsgArg& variant)
{
    if (!PeerSupportsTrustAnchors()) {
        return KeyExchangerECDHE::KeyExchangeReadKey(variant);
    }
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
        return ER_INVALID_DATA;
    }
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(computedRemoteVerifier, sizeof(computedRemoteVerifier));
    if (status != ER_OK) {
        return status;
    }
    *authorized = (memcmp(remoteVerifier, computedRemoteVerifier, sizeof(computedRemoteVerifier)) == 0);
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

QStatus KeyExchanger::ParsePeerSecretRecord(const KeyBlob& rec, KeyBlob& masterSecret, ECCPublicKey* publicKey, bool& publicKeyAvailable)
{
    publicKeyAvailable = false;
    if ((rec.GetSize() == MASTER_SECRET_SIZE) || (rec.GetSize() == MASTER_SECRET_PINX_SIZE)) {
        /* support older format by the non ECDHE key exchanges */
        masterSecret = rec;
        if (publicKey) {
            memset(publicKey, 0, sizeof(ECCPublicKey));
        }
        return ER_OK;
    }
    if (rec.GetSize() == 0) {
        return ER_INVALID_DATA;
    }
    PeerSecretRecord* peerRec = (PeerSecretRecord*) rec.GetData();
    if (peerRec->version != 1) {
        return ER_NOT_IMPLEMENTED;
    }
    masterSecret.Set(peerRec->secret, MASTER_SECRET_SIZE, rec.GetType());
    /* recopy other fields from rec */
    masterSecret.SetTag(rec.GetTag(), rec.GetRole());
    Timespec expiration;
    if (rec.GetExpiration(expiration)) {
        masterSecret.SetExpiration(expiration);
    }

    if (publicKey) {
        memcpy(publicKey, &peerRec->publicKey, sizeof(ECCPublicKey));
    }
    publicKeyAvailable = true;
    return ER_OK;
}

QStatus KeyExchanger::ParsePeerSecretRecord(const KeyBlob& rec, KeyBlob& masterSecret)
{
    bool publicKeyAvail;
    return ParsePeerSecretRecord(rec, masterSecret, NULL, publicKeyAvail);
}

QStatus KeyExchangerECDHE_ECDSA::StoreMasterSecret(const qcc::GUID128& guid, const uint8_t accessRights[4])
{
    if (peerDSAPubKey) {
        /* build a new keyblob with master secret and peer DSA public key */
        PeerSecretRecord secretRecord;
        memcpy(secretRecord.secret, masterSecret.GetData(), sizeof(secretRecord.secret));
        memcpy(&secretRecord.publicKey, peerDSAPubKey, sizeof(ECCPublicKey));
        KeyBlob kb((const uint8_t*) &secretRecord, sizeof(secretRecord), KeyBlob::GENERIC);

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
    QStatus status = GenerateLocalVerifier(verifier, sizeof(verifier));
    if (status != ER_OK) {
        return status;
    }
    MsgArg variant;
    variant.Set("(ayay)", pskName.length(), pskName.data(), sizeof(verifier), verifier);
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
    return GenerateVerifier(label.c_str(), digest, sizeof(digest), masterSecret, verifier, verifierLen);
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
        hashUtil.Update(peerPskName, peerPskNameLen);
        hashUtil.Update((const uint8_t*) pskValue.data(), pskValue.length());
    }
    if (remoteVerifierLen != AUTH_VERIFIER_LEN) {
        return ER_INVALID_DATA;
    }
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(computedRemoteVerifier, sizeof(computedRemoteVerifier));
    if (status != ER_OK) {
        return status;
    }
    *authorized = (memcmp(remoteVerifier, computedRemoteVerifier, sizeof(computedRemoteVerifier)) == 0);
    if (!IsInitiator()) {
        hashUtil.Update(remoteVerifier, remoteVerifierLen);
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
    PermissionMgmtObj::ClearTrustAnchorList(peerTrustAnchorList);
    delete peerDSAPubKey;
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
        QCC_DbgHLPrintf(("listener::RequestCredentials does not provide private key"));
        return ER_AUTH_FAIL;
    }
    /* cert chain is required */
    if (!creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
        QCC_DbgHLPrintf(("listener::RequestCredentials does not provide certificate chain"));
        return ER_AUTH_FAIL;
    }
    if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
        SetSecretExpiration(creds.GetExpiration());
    } else {
        SetSecretExpiration(0xFFFFFFFF);      /* never expired */
    }
    qcc::String pemCertChain = creds.GetCertChain();
    QStatus status = CertificateX509::DecodePrivateKeyPEM(creds.GetPrivateKey(), (uint8_t*) &issuerPrivateKey, sizeof(ECCPrivateKey));
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::RequestCredentialsCB failed to parse the private key PEM"));
        return status;
    }
    status = ParseCertChainPEM((qcc::String&)creds.GetCertChain());
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::RequestCredentialsCB failed to parse the certificate chain"));
        return status;
    }
    if (certChainLen == 0) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::RequestCredentialsCB receives empty the certificate chain"));
        return ER_AUTH_FAIL; /* need both private key and public key */
    }
    issuerPublicKey = *certChain[0].GetSubjectPublicKey();
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
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::VerifyCredentialsCB failed because of no certificate"));
        return ER_AUTH_FAIL;
    }
    AuthListener::Credentials creds;
    creds.SetCertChain(EncodePEMCertChain(certs, numCerts));
    /* check with listener to validate the cert chain */
    bool ok = listener.VerifyCredentials(GetSuiteName(), peerName, creds);
    if (!ok) {
        QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::VerifyCredentialsCB listener::VerifyCredentials failed"));
        return ER_AUTH_FAIL;
    }
    return ER_OK;
}

/**
 * validate whether the certificate chain structure is a valid
 */
static bool IsCertChainStructureValid(CertificateX509* certs, size_t numCerts)
{
    if ((numCerts == 0) || !certs) {
        return false;
    }
    if (numCerts == 1) {
        return true;
    }
    for (size_t cnt = 0; cnt < (numCerts - 1); cnt++) {
        if (!certs[cnt + 1].IsCA()) {
            return false;
        }
        if (certs[cnt].Verify(certs[cnt + 1].GetSubjectPublicKey()) != ER_OK) {
            return false;
        }
    }
    return true;
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

    MsgArg* sigInfoVariant;
    uint8_t certChainEncoding;
    MsgArg* certChainVariant;
    status = variant->Get("(vyv)", &sigInfoVariant, &certChainEncoding, &certChainVariant);
    if (status != ER_OK) {
        return status;
    }
    if ((certChainEncoding != CertificateX509::ENCODING_X509_DER) &&
        (certChainEncoding != CertificateX509::ENCODING_X509_DER_PEM)) {
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
    /* compute the remote verifier */
    uint8_t computedRemoteVerifier[AUTH_VERIFIER_LEN];
    status = GenerateRemoteVerifier(computedRemoteVerifier, sizeof(computedRemoteVerifier));
    if (status != ER_OK) {
        return status;
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
        /* no cert chain to validate.  So it's not authorized */
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
        if (certChainEncoding == CertificateX509::ENCODING_X509_DER) {
            status = certs[cnt].LoadEncoded(encoded, encodedLen);
        } else if (certChainEncoding == CertificateX509::ENCODING_X509_DER_PEM) {
            status = certs[cnt].LoadPEM(String((const char*) encoded, encodedLen));
        } else {
            delete [] certs;
            return ER_INVALID_DATA;
        }
        if (status != ER_OK) {
            QCC_DbgHLPrintf(("KeyExchangerECDHE_ECDSA::ValidateRemoteVerifierVariant error loading peer cert encoded data"));
            delete [] certs;
            return status;
        }
        hashUtil.Update(encoded, encodedLen);
    }
    /* verify signature */
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(certs[0].GetSubjectPublicKey());
    SigInfoECC sigInfo;
    sigInfo.SetRCoord(rCoord);
    sigInfo.SetSCoord(sCoord);
    status = ecc.DSAVerifyDigest(computedRemoteVerifier, sizeof(computedRemoteVerifier), sigInfo.GetSignature());
    *authorized = (ER_OK == status);

    if (!*authorized) {
        delete [] certs;
        return ER_OK;  /* not authorized */
    }
    if (!IsCertChainStructureValid(certs, numCerts)) {
        *authorized = false;
        delete [] certs;
        return ER_OK;
    }
    status = VerifyCredentialsCB(peerName, certs, numCerts);
    if (ER_OK != status) {
        if (!IsTrustAnchor(certs[0].GetSubjectPublicKey())) {
            *authorized = false;
        }
    }

    if (*authorized) {
        peerDSAPubKey = (ECCPublicKey*) new uint8_t[sizeof(ECCPublicKey)];
        memcpy(peerDSAPubKey, certs[0].GetSubjectPublicKey(), sizeof(ECCPublicKey));
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
    MsgArg replyMsg("v", &variant);
    return peerObj->HandleMethodReply(msg, &replyMsg, 1);
}

QStatus KeyExchangerECDHE_ECDSA::GenVerifierSigInfoArg(MsgArg& msgArg, bool updateHash)
{
    /* build the SigInfo object */
    SigInfoECC sigInfo;
    uint8_t verifier[AUTH_VERIFIER_LEN];
    GenerateLocalVerifier(verifier, sizeof(verifier));

    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(&issuerPrivateKey);
    ECCSignature sig;
    QCC_DbgHLPrintf(("Verifier: %s\n", BytesToHexString(verifier, sizeof(verifier)).c_str()));
    QStatus status = ecc.DSASignDigest(verifier, sizeof(verifier), &sig);
    if (status != ER_OK) {
        QCC_LogError(status, ("KeyExchangerECDHE_ECDSA::GenVerifierSigInfoArg failed to generate local verifier sig info"));
        return status;
    }
    sigInfo.SetSignature(&sig);
    if (updateHash) {
        hashUtil.Update((const uint8_t*) sigInfo.GetSignature(), sizeof(ECCSignature));
    }

    MsgArg* certArgs = NULL;
    size_t certArgsCount = 0;
    uint8_t encoding = CertificateX509::ENCODING_X509_DER;
    if (updateHash) {
        hashUtil.Update(&encoding, 1);
    }
    if (certChainLen > 0) {
        certArgsCount = certChainLen;
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
        return status;
    }
    /* check the Auth listener */
    status = RequestCredentialsCB(peerName);
    if (status != ER_OK) {
        return status;
    }

    /* compute the local verifier to send back */
    MsgArg variant;
    status = GenVerifierSigInfoArg(variant, true);
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

bool KeyExchanger::PeerSupportsTrustAnchors()
{
    return GetPeerAuthVersion() >= TRUST_ANCHORS_AUTH_VERSION;
}

} /* namespace ajn */
