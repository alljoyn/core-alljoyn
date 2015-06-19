/**
 * @file
 * The AllJoyn Key Exchanger object implements interfaces for AllJoyn encrypted channel key exchange.
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
#ifndef _ALLJOYN_KEYEXCHANGER_H
#define _ALLJOYN_KEYEXCHANGER_H

#include <qcc/platform.h>
#include <qcc/GUID.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <alljoyn/Message.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/BusAttachment.h>

#include "ProtectedAuthListener.h"
#include "PeerState.h"

using namespace qcc;

namespace ajn {

/* the key exchange is in the 16 MSB.
   The PIN-based key exchange mechanism was removed in 15.04:
        AUTH_KEYX_PIN           0x00040000
   The RSA-based auth mechanism was removed in 15.04:
        AUTH_KEYX_RSA           0x00200000
 */
#define AUTH_KEYX_ANONYMOUS     0x00010000
#define AUTH_KEYX_EXTERNAL      0x00020000
#define AUTH_KEYX_SRP           0x00080000
#define AUTH_KEYX_SRP_LOGON     0x00100000
#define AUTH_KEYX_ECDHE         0x00400000
#define AUTH_KEYX_GSSAPI        0x00800000

/*the key authentication suite is in the 16 LSB */

#define AUTH_SUITE_ANONYMOUS    AUTH_KEYX_ANONYMOUS
#define AUTH_SUITE_EXTERNAL     AUTH_KEYX_EXTERNAL
#define AUTH_SUITE_SRP_KEYX     AUTH_KEYX_SRP
#define AUTH_SUITE_SRP_LOGON    AUTH_KEYX_SRP_LOGON

#define AUTH_SUITE_ECDHE_NULL   (AUTH_KEYX_ECDHE | 0x0001)
#define AUTH_SUITE_ECDHE_PSK    (AUTH_KEYX_ECDHE | 0x0002)
#define AUTH_SUITE_ECDHE_ECDSA  (AUTH_KEYX_ECDHE | 0x0004)

#define AUTH_SUITE_GSSAPI       AUTH_KEYX_GSSAPI

#define AUTH_VERIFIER_LEN  Crypto_SHA256::DIGEST_SIZE

/* Forward declaration */
class AllJoynPeerObj;


/**
 * the Key exchanger Callback
 */
class KeyExchangerCB {
  public:
    KeyExchangerCB(ProxyBusObject& remoteObj, const InterfaceDescription* ifc, uint32_t timeout) : remoteObj(remoteObj), ifc(ifc), timeout(timeout) { }
    QStatus SendKeyExchange(MsgArg* args, size_t numArgs, Message* replyMsg);
    QStatus SendKeyAuthentication(MsgArg* variant, Message* replyMsg);

    ~KeyExchangerCB() { }
  private:
    /* Private assigment operator - does nothing */
    KeyExchangerCB operator=(const KeyExchangerCB&);

    ProxyBusObject& remoteObj;
    const InterfaceDescription* ifc;
    uint32_t timeout;
};

class KeyExchanger {
  public:

    KeyExchanger(bool initiator, AllJoynPeerObj* peerObj, BusAttachment& bus, ProtectedAuthListener& listener, PeerState peerState) : peerObj(peerObj), bus(bus), authCount(1), listener(listener), secretExpiration(3600), peerState(peerState), initiator(initiator) {
        showDigestCounter = 0;
    }

    virtual ~KeyExchanger() {
    }

    bool IsInitiator() { return initiator; }

    virtual QStatus GenerateLocalVerifier(uint8_t* verifier, size_t verifierLen) {
        QCC_UNUSED(verifier);
        QCC_UNUSED(verifierLen);
        return ER_NOT_IMPLEMENTED;
    }
    virtual QStatus GenerateRemoteVerifier(uint8_t* verifier, size_t verifierLen) {
        QCC_UNUSED(verifier);
        QCC_UNUSED(verifierLen);
        return ER_NOT_IMPLEMENTED;
    }

    virtual QStatus StoreMasterSecret(const qcc::GUID128& guid, const uint8_t accessRights[4]) {
        QCC_UNUSED(guid);
        QCC_UNUSED(accessRights);
        return ER_NOT_IMPLEMENTED;
    }

    virtual QStatus ReplyWithVerifier(Message& msg);

    virtual QStatus RespondToKeyExchange(Message& msg, MsgArg* variant, uint32_t remoteAuthMask, uint32_t authMask) {
        QCC_UNUSED(msg);
        QCC_UNUSED(variant);
        QCC_UNUSED(remoteAuthMask);
        QCC_UNUSED(authMask);
        return ER_NOT_IMPLEMENTED;
    }
    virtual QStatus ExecKeyExchange(uint32_t authMask, KeyExchangerCB& callback, uint32_t* remoteAuthMask) {
        QCC_UNUSED(authMask);
        QCC_UNUSED(callback);
        QCC_UNUSED(remoteAuthMask);
        return ER_NOT_IMPLEMENTED;
    }
    virtual QStatus KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized) {
        QCC_UNUSED(callback);
        QCC_UNUSED(peerName);
        QCC_UNUSED(authorized);
        return ER_NOT_IMPLEMENTED;
    }

    virtual QStatus ValidateRemoteVerifierVariant(const char* peerName, MsgArg* variant, uint8_t* authorized);

    virtual const uint32_t GetSuite() {
        return 0;
    }
    virtual const char* GetSuiteName() {
        return "";
    }

    void SetSecretExpiration(uint32_t expiresInSeconds) { secretExpiration = expiresInSeconds; }
    virtual QStatus RequestCredentialsCB(const char* peerName) {
        QCC_UNUSED(peerName);
        return ER_NOT_IMPLEMENTED;
    }

    void ShowCurrentDigest(const char* ref);

    const uint16_t GetPeerAuthVersion()
    {
        return peerState->GetAuthVersion() >> 16;
    }

    /**
     * Is the peer a legacy peer that uses the old ECC encoding
     */
    bool IsLegacyPeer();

  protected:
    AllJoynPeerObj* peerObj;
    BusAttachment& bus;


    /**
     * The number of times this authentication has been attempted.
     */
    uint16_t authCount;
    ProtectedAuthListener& listener;
    uint32_t secretExpiration;

    PeerState peerState;

  private:
    /**
     * Assignment not allowed
     */
    KeyExchanger& operator=(const KeyExchanger& other);

    /**
     * Copy constructor not allowed
     */
    KeyExchanger(const KeyExchanger& other);

    bool initiator;
    int showDigestCounter;
};

class KeyExchangerECDHE : public KeyExchanger {
  public:

    /**
     * Constructor
     *
     */
    KeyExchangerECDHE(bool initiator, AllJoynPeerObj* peerObj, BusAttachment& bus, ProtectedAuthListener& listener, PeerState peerState) : KeyExchanger(initiator, peerObj, bus, listener, peerState) {
    }
    /**
     * Desstructor
     *
     */
    virtual ~KeyExchangerECDHE() {
    }

    virtual QStatus GenerateLocalVerifier(uint8_t* verifier, size_t verifierLen);
    virtual QStatus GenerateRemoteVerifier(uint8_t* verifier, size_t verifierLen);
    virtual QStatus StoreMasterSecret(const qcc::GUID128& guid, const uint8_t accessRights[4]);
    QStatus GenerateECDHEKeyPair();

    const ECCPublicKey* GetPeerECDHEPublicKey() { return &peerPubKey; }
    const ECCPublicKey* GetECDHEPublicKey();
    void SetECDHEPublicKey(const ECCPublicKey* publicKey);
    const ECCPrivateKey* GetECDHEPrivateKey();
    void SetECDHEPrivateKey(const ECCPrivateKey* privateKey);

    QStatus GenerateMasterSecret(const ECCPublicKey* remotePubKey);

    QStatus RespondToKeyExchange(Message& msg, MsgArg* variant, uint32_t remoteAuthMask, uint32_t authMask);

    virtual QStatus ExecKeyExchange(uint32_t authMask, KeyExchangerCB& callback, uint32_t* remoteAuthMask);
    virtual void KeyExchangeGenLegacyKey(MsgArg& variant);
    virtual void KeyExchangeGenKey(MsgArg& variant);
    virtual QStatus KeyExchangeReadLegacyKey(MsgArg& variant);
    virtual QStatus KeyExchangeReadKey(MsgArg& variant);

    virtual QStatus KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized) {
        QCC_UNUSED(callback);
        QCC_UNUSED(peerName);
        QCC_UNUSED(authorized);
        return ER_OK;
    }


  protected:
    ECCPublicKey peerPubKey;
    Crypto_ECC ecc;
    qcc::KeyBlob masterSecret;

  private:

    /**
     * Assignment not allowed
     */
    KeyExchangerECDHE& operator=(const KeyExchangerECDHE& other);

    /**
     * Copy constructor not allowed
     */
    KeyExchangerECDHE(const KeyExchangerECDHE& other);

};

class KeyExchangerECDHE_NULL : public KeyExchangerECDHE {
  public:
    KeyExchangerECDHE_NULL(bool initiator, AllJoynPeerObj* peerObj, BusAttachment& bus, ProtectedAuthListener& listener, PeerState peerState) : KeyExchangerECDHE(initiator, peerObj, bus, listener, peerState) {
    }

    virtual ~KeyExchangerECDHE_NULL() {
    }

    const uint32_t GetSuite() {
        return AUTH_SUITE_ECDHE_NULL;
    }
    const char* GetSuiteName() {
        return "ALLJOYN_ECDHE_NULL";
    }
    QStatus KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized);

    QStatus RequestCredentialsCB(const char* peerName);

  private:
    /**
     * Assignment not allowed
     */
    KeyExchangerECDHE_NULL& operator=(const KeyExchangerECDHE_NULL& other);

    /**
     * Copy constructor not allowed
     */
    KeyExchangerECDHE_NULL(const KeyExchangerECDHE_NULL& other);

};

class KeyExchangerECDHE_PSK : public KeyExchangerECDHE {
  public:
    KeyExchangerECDHE_PSK(bool initiator, AllJoynPeerObj* peerObj, BusAttachment& bus, ProtectedAuthListener& listener, PeerState peerState) : KeyExchangerECDHE(initiator, peerObj, bus, listener, peerState) {
        pskName = "<anonymous>";
        pskValue = " ";
    }


    virtual ~KeyExchangerECDHE_PSK() {
    }

    const uint32_t GetSuite() {
        return AUTH_SUITE_ECDHE_PSK;
    }
    const char* GetSuiteName() {
        return "ALLJOYN_ECDHE_PSK";
    }
    QStatus ReplyWithVerifier(Message& msg);
    QStatus GenerateLocalVerifier(uint8_t* verifier, size_t verifierLen);
    QStatus GenerateRemoteVerifier(uint8_t* verifier, size_t verifierLen);
    QStatus ValidateRemoteVerifierVariant(const char* peerName, MsgArg* variant, uint8_t* authorized);

    QStatus KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized);

    QStatus RequestCredentialsCB(const char* peerName);

  private:
    /**
     * Assignment not allowed
     */
    KeyExchangerECDHE_PSK& operator=(const KeyExchangerECDHE_PSK& other);

    /**
     * Copy constructor not allowed
     */
    KeyExchangerECDHE_PSK(const KeyExchangerECDHE_PSK& other);

    qcc::String pskName;
    qcc::String pskValue;
};

class KeyExchangerECDHE_ECDSA : public KeyExchangerECDHE {
  public:
    KeyExchangerECDHE_ECDSA(bool initiator, AllJoynPeerObj* peerObj, BusAttachment& bus, ProtectedAuthListener& listener, PeerState peerState) : KeyExchangerECDHE(initiator, peerObj, bus, listener, peerState), certChainLen(0), certChain(NULL) {
    }

    virtual ~KeyExchangerECDHE_ECDSA();

    const uint32_t GetSuite() {
        return AUTH_SUITE_ECDHE_ECDSA;
    }
    const char* GetSuiteName() {
        return "ALLJOYN_ECDHE_ECDSA";
    }

    QStatus ReplyWithVerifier(Message& msg);

    QStatus KeyAuthentication(KeyExchangerCB& callback, const char* peerName, uint8_t* authorized);

    QStatus RequestCredentialsCB(const char* peerName);
    QStatus ValidateRemoteVerifierVariant(const char* peerName, MsgArg* variant, uint8_t* authorized);

  private:

    /**
     * Assignment not allowed
     */
    KeyExchangerECDHE_ECDSA& operator=(const KeyExchangerECDHE_ECDSA& other);

    /**
     * Copy constructor not allowed
     */
    KeyExchangerECDHE_ECDSA(const KeyExchangerECDHE_ECDSA& other);

    QStatus VerifyCredentialsCB(const char* peerName, CertificateX509* certs, size_t numCerts);
    QStatus ParseCertChainPEM(String& encodedCertChain);
    QStatus GenVerifierSigInfoArg(MsgArg& msgArg, bool updateHash);

    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPublicKey;
    size_t certChainLen;
    CertificateX509* certChain;
};

} /* namespace ajn */

#endif
