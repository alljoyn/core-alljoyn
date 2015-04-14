/**
 * @file
 * Helper functions for the PermissionMgmt Test Cases
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
#include <alljoyn/Init.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <qcc/Log.h>
#include <alljoyn/PermissionMgmtProxy.h>
#include <alljoyn/PermissionPolicy.h>
#include "KeyStore.h"
#include "PermissionMgmtObj.h"
#include "InMemoryKeyStore.h"

namespace ajn {

class TestPermissionMgmtListener : public PermissionMgmtListener {
  public:
    TestPermissionMgmtListener() : signalNotifyConfigReceived(false) { }
    void NotifyConfig(const char* busName,
                      uint16_t version,
                      MsgArg publicKeyArg,
                      PermissionConfigurator::ClaimableState claimableState,
                      MsgArg trustAnchorsArg,
                      uint32_t serialNumber,
                      MsgArg membershipsArg);
    bool signalNotifyConfigReceived;
};

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

        ECDHEKeyXListener(AgentType agentType) : agentType(agentType), authComplete(false)
        {
        }

        bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
        {
            QCC_UNUSED(credMask);
            QCC_UNUSED(userId);
            QCC_UNUSED(authCount);
            QCC_UNUSED(authPeer);
            if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
                // creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
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
            } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
                return true;
            }
            return false;
        }

        bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
        {
            QCC_UNUSED(authPeer);
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
            QCC_UNUSED(authPeer);
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
            printf("%s ", msg.c_str());
            if (success) {
                printf("succeeded\n");
            } else {
                printf("failed\n");
            }
            authComplete = success;
        }

        bool IsAuthComplete()
        {
            return authComplete;
        }

      private:
        AgentType agentType;
        bool authComplete;
    };

    BasePermissionMgmtTest(const char* path) : BusObject(path),
        adminBus("PermissionMgmtTestAdmin", false),
        adminProxyBus("PermissionMgmtTestAdminProxy", false),
        serviceBus("PermissionMgmtTestService", false),
        consumerBus("PermissionMgmtTestConsumer", false),
        remoteControlBus("PermissionMgmtTestRemoteControl", false),
        serviceGUID(),
        consumerGUID(),
        remoteControlGUID(),
        status(ER_OK),
        serviceKeyListener(NULL),
        adminKeyListener(NULL),
        consumerKeyListener(NULL),
        remoteControlKeyListener(NULL),
        currentTVChannel(1),
        volume(1),
        channelChangedSignalReceived(false),
        testPML()
    {
        AllJoynInit();
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
    BusAttachment remoteControlBus;
    qcc::GUID128 serviceGUID;
    qcc::GUID128 consumerGUID;
    qcc::GUID128 remoteControlGUID;
    QStatus status;

    QStatus InterestInChannelChangedSignal(BusAttachment* bus);
    void ChannelChangedSignalHandler(const InterfaceDescription::Member* member,
                                     const char* sourcePath, Message& msg);
    void SetNotifyConfigSignalReceived(bool flag);
    const bool GetNotifyConfigSignalReceived();
    void SetChannelChangedSignalReceived(bool flag);
    const bool GetChannelChangedSignalReceived();

    void OnOffOn(const InterfaceDescription::Member* member, Message& msg);
    void OnOffOff(const InterfaceDescription::Member* member, Message& msg);
    void TVUp(const InterfaceDescription::Member* member, Message& msg);

    void TVDown(const InterfaceDescription::Member* member, Message& msg);
    void TVChannel(const InterfaceDescription::Member* member, Message& msg);
    void TVChannelChanged(const InterfaceDescription::Member* member, Message& msg);
    void TVMute(const InterfaceDescription::Member* member, Message& msg);
    void TVInputSource(const InterfaceDescription::Member* member, Message& msg);
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    QStatus Set(const char* ifcName, const char* propName, MsgArg& val);
    void GetAllProps(const InterfaceDescription::Member* member, Message& msg);
    const qcc::String& GetAuthMechanisms() const;

    ECDHEKeyXListener* serviceKeyListener;
    ECDHEKeyXListener* adminKeyListener;
    ECDHEKeyXListener* consumerKeyListener;
    ECDHEKeyXListener* remoteControlKeyListener;
  private:
    void RegisterKeyStoreListeners();
    QStatus SetupBus(BusAttachment& bus);
    QStatus TeardownBus(BusAttachment& bus);

    InMemoryKeyStoreListener adminKeyStoreListener;
    InMemoryKeyStoreListener serviceKeyStoreListener;
    InMemoryKeyStoreListener consumerKeyStoreListener;
    InMemoryKeyStoreListener remoteControlKeyStoreListener;
    uint32_t currentTVChannel;
    uint32_t volume;
    bool channelChangedSignalReceived;
    qcc::String authMechanisms;
    TestPermissionMgmtListener testPML;
};

class PermissionMgmtTestHelper {
  public:
    static QStatus CreateIdentityCert(BusAttachment& issuerBus, const qcc::String& serial, const qcc::GUID128& subject, const qcc::ECCPublicKey* subjectPubKey, const qcc::String& alias, uint32_t expiredInSecs, qcc::String& der);

    static QStatus CreateIdentityCert(BusAttachment& issuerBus, const qcc::String& serial, const qcc::GUID128& subject, const qcc::ECCPublicKey* subjectPubKey, const qcc::String& alias, qcc::String& der);

    static QStatus CreateMembershipCert(const qcc::String& serial, const uint8_t* authDataHash, BusAttachment& signingBus, const qcc::GUID128& subject, const qcc::ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, bool delegate, uint32_t expiredInSecs, qcc::String& der);
    static QStatus CreateMembershipCert(const qcc::String& serial, const uint8_t* authDataHash, BusAttachment& signingBus, const qcc::GUID128& subject, const qcc::ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, bool delegate, qcc::String& der);
    static QStatus CreateMembershipCert(const qcc::String& serial, const uint8_t* authDataHash, BusAttachment& signingBus, const qcc::GUID128& subject, const qcc::ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, qcc::String& der);
    static bool IsPermissionDeniedError(QStatus status, Message& msg);
    static QStatus ReadClaimResponse(Message& msg, qcc::ECCPublicKey* pubKey);
    static QStatus GenerateManifest(PermissionPolicy::Rule** retRules, size_t* count);

    static QStatus RetrieveDSAPublicKeyFromKeyStore(BusAttachment& bus, qcc::ECCPublicKey* publicKey);

    static QStatus LoadCertificateBytes(Message& msg, qcc::CertificateX509& cert);
    static QStatus InstallMembership(const qcc::String& serial, BusAttachment& bus, const qcc::String& remoteObjName, BusAttachment& signingBus, const qcc::GUID128& subjectGUID, const qcc::ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, PermissionPolicy* membershipAuthData);
    static QStatus InstallMembershipChain(BusAttachment& topBus, BusAttachment& secondBus, const qcc::String& serial0, const qcc::String& serial1, const qcc::String& remoteObjName, const qcc::GUID128& secondGUID, const qcc::ECCPublicKey* secondPubKey, const qcc::GUID128& targetGUID, const qcc::ECCPublicKey* targetPubKey, const qcc::GUID128& guild, PermissionPolicy** authDataArray);

    static QStatus RetrievePublicKeyFromMsgArg(MsgArg& arg, qcc::ECCPublicKey* pubKey);
    static QStatus GetPeerPublicKey(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::ECCPublicKey* pubKey);
    static QStatus ExcerciseOn(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseOff(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVUp(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVDown(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVChannel(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVMute(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExcerciseTVInputSource(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus JoinPeerSession(BusAttachment& initiator, BusAttachment& responder, SessionId& sessionId);
    static QStatus GetGUID(BusAttachment& bus, qcc::GUID128& guid);
    static QStatus GetPeerGUID(BusAttachment& bus, qcc::String& peerName, qcc::GUID128& peerGuid);
    static QStatus GetTVVolume(BusAttachment& bus, ProxyBusObject& remoteObj, uint32_t& volume);
    static QStatus SetTVVolume(BusAttachment& bus, ProxyBusObject& remoteObj, uint32_t volume);
    static QStatus GetTVCaption(BusAttachment& bus, ProxyBusObject& remoteObj);
};

}
