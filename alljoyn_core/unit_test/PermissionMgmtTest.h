/**
 * @file
 * Helper functions for the PermissionMgmt Test Cases
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

#include <qcc/platform.h>
#include <gtest/gtest.h>
#include <qcc/GUID.h>
#include <qcc/StringSink.h>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <qcc/Log.h>
#include <alljoyn/PermissionPolicy.h>
#include "KeyStore.h"
#include "InMemoryKeyStore.h"

namespace ajn {

class BasePermissionMgmtTest : public testing::Test, public BusObject {
  public:


    static const char* INTERFACE_NAME;
    static const char* NOTIFY_INTERFACE_NAME;
    static const char* ONOFF_IFC_NAME;
    static const char* TV_IFC_NAME;

    class ECDHEKeyXListener : public AuthListener {
      public:
        /* the type of agent */
        typedef enum {
            RUN_AS_ADMIN = 0,
            RUN_AS_SERVICE = 1,
            RUN_AS_CONSUMER = 2
        } AgentType;

        ECDHEKeyXListener(AgentType agentType, BusAttachment& bus) : agentType(agentType), bus(bus)
        {
        }

        bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
        {
            if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
                creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
                return true;
            } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
                /*
                 * Solicit the Pre shared secret
                 * Based on the pre shared secret id, the application can retrieve
                 * the pre shared secret from storage or from the end user.
                 * In this example, the pre shared secret is a hard coded string
                 */
                qcc::String psk("123456");
                creds.SetPassword(psk);
                creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
                return true;
            }
            return false;
        }

        bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
        {
            /* only the ECDHE_ECDSA calls for peer credential verification */
            if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
                if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                    /*
                     * AllJoyn sends back the certificate chain for the application to verify.
                     * The application has to option to verify the certificate
                     * chain.  If the cert chain is validated and trusted then return true; otherwise, return false.
                     */
                    return true;
                }
                return true;
            }
            return false;
        }

        void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
            qcc::String msg;
            if (agentType == RUN_AS_ADMIN) {
                msg += "Admin: ";
            } else if (agentType == RUN_AS_SERVICE) {
                msg += "Service: ";
            } else {
                msg += "Consumer: ";
            }
            msg += "AuthenticationComplete auth mechanism ";
            msg += authMechanism;
            msg += " failed";
            ASSERT_TRUE(success) << msg.c_str();
        }

      private:
        AgentType agentType;
        BusAttachment& bus;
    };

    BasePermissionMgmtTest(const char* path) : BusObject(path),
        adminBus("PermissionMgmtTestAdmin", false),
        adminProxyBus("PermissionMgmtTestAdmin", false),
        serviceBus("PermissionMgmtTestService", false),
        consumerBus("PermissionMgmtTestConsumer", false),
        status(ER_OK),
        authComplete(false),
        serviceKeyListener(NULL),
        adminKeyListener(NULL),
        consumerKeyListener(NULL),
        signalNotifyConfigReceived(false)
    {
    }

    virtual void SetUp();

    virtual void TearDown();

    void EnableSecurity(const char* keyExchange);
    void CreateOnOffAppInterface(BusAttachment& bus, bool addService);
    void CreateTVAppInterface(BusAttachment& bus, bool addService);
    void CreateAppInterfaces(BusAttachment& bus, bool addService);

    BusAttachment adminBus;
    BusAttachment adminProxyBus;
    BusAttachment serviceBus;
    BusAttachment consumerBus;
    qcc::GUID128 serviceGUID;
    qcc::GUID128 consumerGUID;
    QStatus status;
    bool authComplete;

    QStatus InterestInSignal(BusAttachment* bus);
    void SignalHandler(const InterfaceDescription::Member* member,
                       const char* sourcePath, Message& msg);
    void SetNotifyConfigSignalReceived(bool flag);
    const bool GetNotifyConfigSignalReceived();

    void OnOffOn(const InterfaceDescription::Member* member, Message& msg);
    void OnOffOff(const InterfaceDescription::Member* member, Message& msg);
    void TVUp(const InterfaceDescription::Member* member, Message& msg);

    void TVDown(const InterfaceDescription::Member* member, Message& msg);
    void TVChannel(const InterfaceDescription::Member* member, Message& msg);
    void TVMute(const InterfaceDescription::Member* member, Message& msg);

  private:
    void RegisterKeyStoreListeners();
    QStatus SetupBus(BusAttachment& bus);
    QStatus TeardownBus(BusAttachment& bus);

    ECDHEKeyXListener* serviceKeyListener;
    ECDHEKeyXListener* adminKeyListener;
    ECDHEKeyXListener* consumerKeyListener;
    bool signalNotifyConfigReceived;
    InMemoryKeyStoreListener adminKeyStoreListener;
    InMemoryKeyStoreListener serviceKeyStoreListener;
    InMemoryKeyStoreListener consumerKeyStoreListener;

};

class PermissionMgmtTestHelper {
  public:
    static QStatus CreateIdentityCert(const qcc::String& serial, const qcc::GUID128& issuer, const qcc::ECCPrivateKey* issuerPrivateKey, const qcc::GUID128& subject, const qcc::ECCPublicKey* subjectPubKey, const qcc::String& alias, qcc::String& der);

    static QStatus CreateMembershipCert(const qcc::String& serial, const uint8_t* authDataHash, const qcc::GUID128& issuer, BusAttachment& signingBus, const qcc::GUID128& subject, const qcc::ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, qcc::String& der);
    static QStatus InterestInSignal(BusAttachment* bus);
    static bool IsPermissionDeniedError(QStatus status, Message& msg);
    static QStatus ReadClaimResponse(Message& msg, qcc::ECCPublicKey* pubKey);
    static QStatus Claim(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::GUID128& issuerGUID, const qcc::ECCPublicKey* pubKey, qcc::ECCPublicKey* claimedPubKey, const qcc::GUID128& claimedGUID, qcc::String& identityCertDER);
    static QStatus GenerateManifest(PermissionPolicy::Rule** retRules, size_t* count);
    static QStatus GetManifest(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy::Rule** retRules, size_t* count);
    static QStatus InstallPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy);
    static QStatus GetPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy);
    static QStatus RemovePolicy(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus RetrieveDSAPrivateKeyFromKeyStore(BusAttachment& bus, qcc::ECCPrivateKey* privateKey);

    static QStatus RetrieveDSAPublicKeyFromKeyStore(BusAttachment& bus, qcc::ECCPublicKey* publicKey);

    static QStatus RetrieveDSAKeys(BusAttachment& bus, qcc::ECCPrivateKey& privateKey, qcc::ECCPublicKey& publicKey);
    static QStatus LoadCertificateBytes(Message& msg, qcc::CertificateX509& cert);
    static QStatus InstallMembership(const qcc::String& serial, BusAttachment& bus, ProxyBusObject& remoteObj, BusAttachment& signingBus, const qcc::GUID128& subjectGUID, const qcc::ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, PermissionPolicy* membershipAuthData);
    static QStatus RemoveMembership(BusAttachment& bus, ProxyBusObject& remoteObj, const qcc::String serialNum, const qcc::GUID128& issuer);
    static QStatus InstallIdentity(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::String& der);
    static QStatus RetrievePublicKeyFromMsgArg(MsgArg& arg, qcc::ECCPublicKey* pubKey);
    static QStatus GetPeerPublicKey(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::ECCPublicKey* pubKey);
    static QStatus GetIdentity(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::IdentityCertificate& cert);
    static QStatus Reset(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus InstallGuildEquivalence(BusAttachment& bus, ProxyBusObject& remoteObj, const char* pem);
    static QStatus ExcerciseOn(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseOff(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVUp(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVDown(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVChannel(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVMute(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus JoinPeerSession(BusAttachment& initiator, BusAttachment& responder, SessionId& sessionId);
    static QStatus GetGUID(BusAttachment& bus, qcc::GUID128& guid);
    static QStatus GetPeerGUID(BusAttachment& bus, qcc::String& peerName, qcc::GUID128& peerGuid);
};

}
