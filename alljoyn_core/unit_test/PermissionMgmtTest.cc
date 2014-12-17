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

#include <gtest/gtest.h>
#include "ajTestCommon.h"
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <qcc/Log.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/StringUtil.h>
#include <alljoyn/PermissionPolicy.h>
#include "KeyInfoHelper.h"
#include "CredentialAccessor.h"
#include "PermissionMgmtObj.h"

using namespace ajn;
using namespace qcc;

static const char* INTERFACE_NAME = "org.allseen.Security.PermissionMgmt";
static const char* NOTIFY_INTERFACE_NAME = "org.allseen.Security.PermissionMgmt.Notification";
static const char* PERMISSION_MGMT_PATH = "/org/allseen/Security/PermissionMgmt";
static const char* APP_PATH = "/app";
static const char* ONOFF_IFC_NAME = "org.allseenalliance.control.OnOff";
static const char* TV_IFC_NAME = "org.allseenalliance.control.TV";

static const char* KEYX_ECDHE_NULL = "ALLJOYN_ECDHE_NULL";
static const char* KEYX_ECDHE_PSK = "ALLJOYN_ECDHE_PSK";
static const char* KEYX_ECDHE_ECDSA = "ALLJOYN_ECDHE_ECDSA";

static GUID128 membershipGUID1;
static const char* membershipSerial1 = "10001";
static GUID128 membershipGUID2;
static GUID128 membershipGUID3;
static const char* membershipSerial3 = "30003";

static const char clientCertChainType1PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAf8thIwHzhCU8qsedyuEldP/TouX6w7rZI/cJYST/kexAAAAAMvbuy8JDCJI\n"
    "Ms8vwkglUrf/infSYMNRYP/gsFvl5FutAAAAAAAAAAD/LYSMB84QlPKrHncrhJXT\n"
    "/06Ll+sO62SP3CWEk/5HsQAAAADL27svCQwiSDLPL8JIJVK3/4p30mDDUWD/4LBb\n"
    "5eRbrQAAAAAAAAAAAAAAAAASgF0AAAAAABKBiQABMa7uTLSqjDggO0t6TAgsxKNt\n"
    "+Zhu/jc3s242BE0drFU12USXXIYQdqps/HrMtqw6q9hrZtaGJS+e9y7mJegAAAAA\n"
    "APpeLT1cHNm3/OupnEcUCmg+jqi4SUEi4WTWSR4OzvCSAAAAAA==\n"
    "-----END CERTIFICATE-----"
};

static QStatus CreateIdentityCert(const qcc::String& serial, const qcc::GUID128& issuer, const ECCPrivateKey* issuerPrivateKey, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, const qcc::String& alias, qcc::String& der)
{
    QStatus status = ER_CRYPTO_ERROR;
    CertificateX509 x509(CertificateX509::IDENTITY_CERTIFICATE);

    x509.SetSerial(serial);
    x509.SetIssuer(issuer);
    x509.SetSubject(subject);
    x509.SetSubjectPublicKey(subjectPubKey);
    x509.SetAlias(alias);

    status = x509.Sign(issuerPrivateKey);
    if (ER_OK != status) {
        return status;
    }
    return x509.EncodeCertificateDER(der);
}

static QStatus CreateMembershipCert(const String& serial, const uint8_t* authDataHash, const qcc::GUID128& issuer, BusAttachment& signingBus, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, qcc::String& der)
{
    QStatus status = ER_CRYPTO_ERROR;
    MembershipCertificate x509;

    x509.SetSerial(serial);
    x509.SetIssuer(issuer);
    x509.SetSubject(subject);
    x509.SetSubjectPublicKey(subjectPubKey);
    x509.SetGuild(guild);
    x509.SetDigest(authDataHash, Certificate::SHA256_DIGEST_SIZE);
    /* use the signing bus to sign the cert */
    PermissionConfigurator& pc = signingBus.GetPermissionConfigurator();
    status = pc.SignCertificate(x509);
    if (ER_OK != status) {
        return status;
    }
    return x509.EncodeCertificateDER(der);
}

static QStatus InterestInSignal(BusAttachment* bus)
{
    const char* notifyConfigMatchRule = "type='signal',interface='" "org.allseen.Security.PermissionMgmt.Notification" "',member='NotifyConfig'";
    return bus->AddMatch(notifyConfigMatchRule);
}

/*
 * This is the local implementation of the an AuthListener.  ECDHEKeyXListener is
 * designed to only handle ECDHE Key Exchange Authentication requests.
 *
 * If any other authMechanism is used other than ECDHE Key Exchange authentication
 * will fail.
 */
class ECDHEKeyXListener : public AuthListener {
  public:
    /* the type of agent */
    typedef enum {
        RUN_AS_ADMIN = 0,
        RUN_AS_SERVICE = 1,
        RUN_AS_CONSUMER = 2
    } AgentType;

    ECDHEKeyXListener(AgentType agentType, BusAttachment& bus) : agentType(agentType)
    {
    }

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
    {
        if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0) {
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_PSK) == 0) {
            /*
             * Solicit the Pre shared secret
             */
            /*
             * Based on the pre shared secret id, the application can retrieve
             * the pre shared secret from storage or from the end user.
             * In this example, the pre shared secret is a hard coded string
             */
            String psk("123456");
            creds.SetPassword(psk);
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_ECDSA) == 0) {
            return true;
        }
        return false;
    }

    bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
    {
        /* only the ECDHE_ECDSA calls for peer credential verification */
        if (strcmp(authMechanism, KEYX_ECDHE_ECDSA) == 0) {
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
};

class PermissionMgmtTest : public testing::Test, public BusObject {
  public:

    PermissionMgmtTest() : BusObject(APP_PATH),
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

    virtual void SetUp()
    {
        status = SetupBus(adminBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = SetupBus(adminProxyBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = SetupBus(serviceBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = SetupBus(consumerBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        const InterfaceDescription* itf = adminBus.GetInterface(NOTIFY_INTERFACE_NAME);
        status = adminBus.RegisterSignalHandler(this,
                                                static_cast<MessageReceiver::SignalHandler>(&PermissionMgmtTest::SignalHandler), itf->GetMember("NotifyConfig"), NULL);
        EXPECT_EQ(ER_OK, status) << "  Failed to register signal handler.  Actual Status: " << QCC_StatusText(status);
        status = InterestInSignal(&adminBus);
        EXPECT_EQ(ER_OK, status) << "  Failed to show interest in session-less signal.  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown()
    {
        status = TeardownBus(adminBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = TeardownBus(adminProxyBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = TeardownBus(serviceBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = TeardownBus(consumerBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        delete serviceKeyListener;
        delete adminKeyListener;
        delete consumerKeyListener;
    }

    void EnableSecurity(const char* keyExchange, bool clearPrevious)
    {
        if (clearPrevious) {
            adminBus.EnablePeerSecurity(NULL, NULL, NULL, false);
            serviceBus.EnablePeerSecurity(NULL, NULL, NULL, false);
            consumerBus.EnablePeerSecurity(NULL, NULL, NULL, false);
        }
        delete adminKeyListener;
        adminKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_ADMIN, adminBus);
        adminBus.EnablePeerSecurity(keyExchange, adminKeyListener, NULL, true);
        delete serviceKeyListener;
        serviceKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_SERVICE, serviceBus);
        serviceBus.EnablePeerSecurity(keyExchange, serviceKeyListener, NULL, false);
        delete consumerKeyListener;
        consumerKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_CONSUMER, consumerBus);
        consumerBus.EnablePeerSecurity(keyExchange, consumerKeyListener, NULL, false);
    }

    void EnableSecurityAdminProxy(const char* keyExchange)
    {
        adminProxyBus.EnablePeerSecurity(keyExchange, adminKeyListener, NULL, true);
    }

    void CreateOnOffAppInterface(BusAttachment& bus, bool addService)
    {
        /* create/activate alljoyn_interface */
        InterfaceDescription* ifc = NULL;
        QStatus status = bus.CreateInterface(ONOFF_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(ifc != NULL);
        if (ifc != NULL) {
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "On", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Off", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            ifc->Activate();
        }
        if (!addService) {
            return;  /* done */
        }
        status = AddInterface(*ifc);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        AddMethodHandler(ifc->GetMember("On"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::OnOffOn));
        AddMethodHandler(ifc->GetMember("Off"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::OnOffOff));
    }

    void CreateTVAppInterface(BusAttachment& bus, bool addService)
    {
        /* create/activate alljoyn_interface */
        InterfaceDescription* ifc = NULL;
        QStatus status = bus.CreateInterface(TV_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(ifc != NULL);
        if (ifc != NULL) {
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Up", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Down", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Channel", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Mute", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            ifc->Activate();
        }
        if (!addService) {
            return;  /* done */
        }
        status = AddInterface(*ifc);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        AddMethodHandler(ifc->GetMember("Up"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::TVUp));
        AddMethodHandler(ifc->GetMember("Down"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::TVDown));
        AddMethodHandler(ifc->GetMember("Channel"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::TVChannel));
        AddMethodHandler(ifc->GetMember("Mute"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::TVMute));
    }

    void CreateAppInterfaces(BusAttachment& bus, bool addService)
    {
        CreateOnOffAppInterface(bus, addService);
        CreateTVAppInterface(bus, addService);
        if (addService) {
            QStatus status = bus.RegisterBusObject(*this);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    BusAttachment adminBus;
    BusAttachment adminProxyBus;
    BusAttachment serviceBus;
    BusAttachment consumerBus;
    GUID128 serviceGUID;
    GUID128 consumerGUID;
    QStatus status;
    bool authComplete;

    void SignalHandler(const InterfaceDescription::Member* member,
                       const char* sourcePath, Message& msg)
    {
        signalNotifyConfigReceived = true;
        MsgArg* keyArrayArg;
        size_t keyArrayLen = 0;
        uint32_t serialNum;
        uint8_t claimableState;
        QStatus status = msg->GetArg(0)->Get("a(yv)", &keyArrayLen, &keyArrayArg);
        EXPECT_EQ(ER_OK, status) << "  Retrieve the key info  failed.  Actual Status: " << QCC_StatusText(status);
        if (keyArrayLen > 0) {
            KeyInfoNISTP256 keyInfo;
            status = KeyInfoHelper::MsgArgToKeyInfoNISTP256(keyArrayArg[0], keyInfo);
            EXPECT_EQ(ER_OK, status) << "  Parse the key info  failed.  Actual Status: " << QCC_StatusText(status);
        }
        status = msg->GetArg(1)->Get("y", &claimableState);
        EXPECT_EQ(ER_OK, status) << "  Retrieve the claimableState failed.  Actual Status: " << QCC_StatusText(status);
        status = msg->GetArg(2)->Get("u", &serialNum);
        EXPECT_EQ(ER_OK, status) << "  Retrieve the serial number failed.  Actual Status: " << QCC_StatusText(status);
    }

    void SetNotifyConfigSignalReceived(bool flag)
    {
        signalNotifyConfigReceived = flag;
    }

    const bool GetNotifyConfigSignalReceived()
    {
        return signalNotifyConfigReceived;
    }

    void OnOffOn(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void OnOffOff(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void TVUp(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void TVDown(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void TVChannel(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void TVMute(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

  private:
    QStatus SetupBus(BusAttachment& bus)
    {
        QStatus status = bus.Start();
        if (ER_OK != status) {
            return status;
        }
        return bus.Connect(ajn::getConnectArg().c_str());
    }

    QStatus TeardownBus(BusAttachment& bus)
    {
        bus.UnregisterBusObject(*this);
        status = bus.Disconnect(ajn::getConnectArg().c_str());
        if (ER_OK != status) {
            return status;
        }
        status = bus.Stop();
        if (ER_OK != status) {
            return status;
        }
        return bus.Join();
    }

    ECDHEKeyXListener* serviceKeyListener;
    ECDHEKeyXListener* adminKeyListener;
    ECDHEKeyXListener* consumerKeyListener;
    bool signalNotifyConfigReceived;
};

static bool IsPermissionDeniedError(QStatus status, Message& msg)
{
    if (ER_PERMISSION_DENIED == status) {
        return true;
    }
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        qcc::String errorMsg;
        const char* errorName = msg->GetErrorName(&errorMsg);
        if (errorName == NULL) {
            return false;
        }
        if (strcmp(errorName, "org.alljoyn.Bus.ER_PERMISSION_DENIED") == 0) {
            return true;
        }
        if (strcmp(errorName, "org.alljoyn.Bus.ErStatus") != 0) {
            return false;
        }
        if (errorMsg == "ER_PERMISSION_DENIED") {
            return true;
        }
    }
    return false;
}

static QStatus RetrievePublicKeyFromMsgArg(MsgArg& arg, ECCPublicKey* pubKey)
{
    uint8_t keyFormat;
    MsgArg* variantArg;
    QStatus status = arg.Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        return status;
    }
    if (keyFormat != KeyInfo::FORMAT_ALLJOYN) {
        return status;
    }
    uint8_t* kid;
    size_t kidLen;
    uint8_t keyUsageType;
    uint8_t keyType;
    MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &kidLen, &kid, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        return status;
    }
    if ((keyUsageType != KeyInfo::USAGE_SIGNING) && (keyUsageType != KeyInfo::USAGE_ENCRYPTION)) {
        return status;
    }
    if (keyType != KeyInfoECC::KEY_TYPE) {
        return status;
    }
    uint8_t algorithm;
    uint8_t curve;
    MsgArg* curveVariant;
    status = keyVariantArg->Get("(yyv)", &algorithm, &curve, &curveVariant);
    if (ER_OK != status) {
        return status;
    }
    if (curve != Crypto_ECC::ECC_NIST_P256) {
        return status;
    }

    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    status = curveVariant->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        return status;
    }
    if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
        return status;
    }
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);
    memcpy(pubKey, keyInfo.GetPublicKey(), sizeof(ECCPublicKey));
    return ER_OK;
}

static QStatus ReadClaimResponse(Message& msg, ECCPublicKey* pubKey)
{
    return RetrievePublicKeyFromMsgArg((MsgArg &) * msg->GetArg(0), pubKey);
}

static QStatus Claim(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::GUID128& issuerGUID, const ECCPublicKey* pubKey, ECCPublicKey* claimedPubKey, const GUID128& claimedGUID, qcc::String& identityCertDER)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg inputs[3];

    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(pubKey);
    inputs[0].Set("(yv)", KeyInfo::FORMAT_ALLJOYN,
                  new MsgArg("(ayyyv)", GUID128::SIZE, issuerGUID.GetBytes(), KeyInfo::USAGE_SIGNING, KeyInfoECC::KEY_TYPE,
                             new MsgArg("(yyv)", keyInfo.GetAlgorithm(), keyInfo.GetCurve(),
                                        new MsgArg("(ayay)", ECC_COORDINATE_SZ, keyInfo.GetXCoord(), ECC_COORDINATE_SZ, keyInfo.GetYCoord()))));
    inputs[0].SetOwnershipFlags(MsgArg::OwnsArgs, true);

    inputs[1].Set("ay", GUID128::SIZE, claimedGUID.GetBytes());
    inputs[2].Set("(yay)", Certificate::ENCODING_X509_DER, identityCertDER.size(), identityCertDER.data());
    uint32_t timeout = 10000; /* Claim is a bit show */

    status = remoteObj.MethodCall(INTERFACE_NAME, "Claim", inputs, 3, reply, timeout);

    if (ER_OK == status) {
        status = ReadClaimResponse(reply, claimedPubKey);
        if (ER_OK != status) {
            return status;
        }
    } else if (IsPermissionDeniedError(status, reply)) {
        status = ER_PERMISSION_DENIED;
    }
    return status;
}

static PermissionPolicy* GeneratePolicy(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey, qcc::ECCPublicKey& guildAuthority)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(74892317);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the provider section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[4];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    prms[1].SetMemberName("*");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);
    terms[0].SetRules(1, rules);

    /* terms record 1 GUILD membershipGUID1 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(membershipGUID1.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("Channel");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(3, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    terms[1].SetRules(2, rules);

    /* terms record 2 GUILD membershipGUID2 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(membershipGUID2.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[2].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetObjPath("/control/settings");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    rules[0].SetMembers(1, prms);
    rules[1].SetObjPath("*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    terms[2].SetRules(2, rules);

    /* terms record 3 peer specific rule  */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[3].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("Mute");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    terms[3].SetRules(1, rules);

    policy->SetTerms(4, terms);

    return policy;
}

static PermissionPolicy* GenerateMembeshipAuthData()
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(88473);

    /* add the provider section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0 */
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    terms[0].SetRules(1, rules);
    policy->SetTerms(1, terms);

    return policy;
}

static QStatus GenerateManifest(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName(TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
    return ER_OK;
}

static QStatus GetManifest(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy::Rule** retRules, size_t* count)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "GetManifest", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    uint8_t type;
    MsgArg* variant;
    status = reply->GetArg(0)->Get("(yv)", &type, &variant);
    if (ER_OK != status) {
        return status;
    }
    return PermissionPolicy::ParseRules(*variant, retRules, count);
}

static QStatus InstallPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg policyArg;

    status = policy.Export(policyArg);
    if (ER_OK != status) {
        return status;
    }
    status = remoteObj.MethodCall(INTERFACE_NAME, "InstallPolicy", &policyArg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus GetPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "GetPolicy", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    uint8_t version;
    MsgArg* variant;
    reply->GetArg(0)->Get("(yv)", &version, &variant);
    return policy.Import(version, *variant);
}

static QStatus RemovePolicy(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "RemovePolicy", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus RetrieveDSAPrivateKeyFromKeyStore(BusAttachment& bus, ECCPrivateKey* privateKey)
{
    CredentialAccessor ca(bus);
    return PermissionMgmtObj::RetrieveDSAPrivateKey(&ca, privateKey);
}

static QStatus RetrieveDSAPublicKeyFromKeyStore(BusAttachment& bus, ECCPublicKey* publicKey)
{
    CredentialAccessor ca(bus);
    return PermissionMgmtObj::RetrieveDSAPublicKey(&ca, publicKey);
}

static QStatus RetrieveDSAKeys(BusAttachment& bus, ECCPrivateKey& privateKey, ECCPublicKey& publicKey)
{
    /* Retrieve the DSA keys */
    QStatus status = RetrieveDSAPrivateKeyFromKeyStore(bus, &privateKey);
    if (ER_OK != status) {
        return status;
    }
    return RetrieveDSAPublicKeyFromKeyStore(bus, &publicKey);
}

static QStatus LoadCertificateBytes(Message& msg, CertificateX509& cert)
{
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    QStatus status = msg->GetArg(0)->Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        return status;
    }
    status = ER_NOT_IMPLEMENTED;
    if (encoding == Certificate::ENCODING_X509_DER) {
        status = cert.DecodeCertificateDER(String((const char*) encoded, encodedLen));
    } else if (encoding == Certificate::ENCODING_X509_DER_PEM) {
        status = cert.DecodeCertificatePEM(String((const char*) encoded, encodedLen));
    }
    return status;
}

static QStatus InstallMembership(const String& serial, BusAttachment& bus, ProxyBusObject& remoteObj, BusAttachment& signingBus, const qcc::GUID128& subjectGUID, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    CredentialAccessor ca(bus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    if (status != ER_OK) {
        return status;
    }
    PermissionPolicy* membershipAuthData = GenerateMembeshipAuthData();
    uint8_t digest[Certificate::SHA256_DIGEST_SIZE];
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    membershipAuthData->Digest(marshaller, digest, Certificate::SHA256_DIGEST_SIZE);

    qcc::String der;
    status = CreateMembershipCert(serial, digest, localGUID, signingBus, subjectGUID, subjectPubKey, guild, der);
    if (status != ER_OK) {
        return status;
    }
    MsgArg certArgs[1];
    certArgs[0].Set("(yay)", Certificate::ENCODING_X509_DER, der.length(), der.c_str());
    MsgArg arg("a(yay)", 1, certArgs);

    status = remoteObj.MethodCall(INTERFACE_NAME, "InstallMembership", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }

    if (ER_OK == status) {
        /* installing the auth data */
        MsgArg args[3];
        args[0].Set("s", serial.c_str());
        args[1].Set("ay", GUID128::SIZE, localGUID.GetBytes());
        membershipAuthData->Export(args[2]);
        status = remoteObj.MethodCall(INTERFACE_NAME, "InstallMembershipAuthData", args, ArraySize(args), reply, 5000);
        if (ER_OK != status) {
            if (IsPermissionDeniedError(status, reply)) {
                status = ER_PERMISSION_DENIED;
            }
        }
    }
    delete membershipAuthData;
    return status;
}

static QStatus RemoveMembership(BusAttachment& bus, ProxyBusObject& remoteObj, const qcc::String serialNum, const qcc::GUID128& issuer)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    MsgArg inputs[2];
    inputs[0].Set("s", serialNum.c_str());
    inputs[1].Set("ay", GUID128::SIZE, issuer.GetBytes());

    status = remoteObj.MethodCall(INTERFACE_NAME, "RemoveMembership", inputs, 2, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus InstallIdentity(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::String& der)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg arg("(yay)", Certificate::ENCODING_X509_DER, der.size(), der.data());

    status = remoteObj.MethodCall(INTERFACE_NAME, "InstallIdentity", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus GetPeerPublicKey(BusAttachment& bus, ProxyBusObject& remoteObj, ECCPublicKey* pubKey)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "GetPublicKey", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    return RetrievePublicKeyFromMsgArg((MsgArg &) * reply->GetArg(0), pubKey);
}

static QStatus GetIdentity(BusAttachment& bus, ProxyBusObject& remoteObj, IdentityCertificate& cert)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "GetIdentity", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    return LoadCertificateBytes(reply, cert);
}

static QStatus Reset(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "Reset", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus InstallGuildEquivalence(BusAttachment& bus, ProxyBusObject& remoteObj, const char* pem)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg arg("(yay)", Certificate::ENCODING_X509_DER_PEM, strlen(pem), pem);

    status = remoteObj.MethodCall(INTERFACE_NAME, "InstallGuildEquivalence", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus ExcerciseOn(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(ONOFF_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(ONOFF_IFC_NAME, "On", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus ExcerciseOff(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(ONOFF_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(ONOFF_IFC_NAME, "Off", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus ExcerciseTVUp(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(TV_IFC_NAME, "Up", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus ExcerciseTVDown(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(TV_IFC_NAME, "Down", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus ExcerciseTVChannel(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(TV_IFC_NAME, "Channel", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus ExcerciseTVMute(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(TV_IFC_NAME, "Mute", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus JoinPeerSession(BusAttachment& initiator, BusAttachment& responder, SessionId& sessionId)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = ER_FAIL;
    for (int cnt = 0; cnt < 30; cnt++) {
        status = initiator.JoinSession(responder.GetUniqueName().c_str(),
                                       ALLJOYN_SESSIONPORT_PERMISSION_MGMT, NULL, sessionId, opts);
        if (ER_OK == status) {
            return status;
        }
        /* sleep a few seconds since the responder may not yet setup the listener port */
        qcc::Sleep(100);
    }
    return status;
}

/*
 *  Test PermissionMgmt Claim method to the admin
 */
TEST_F(PermissionMgmtTest, ClaimAdmin)
{
    QStatus status = ER_OK;
    EnableSecurity("ALLJOYN_ECDHE_NULL", false);

    /* factory reset */
    PermissionConfigurator& pc = adminBus.GetPermissionConfigurator();
    status = pc.Reset();
    EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
    /* Gen DSA keys */
    status = pc.GenerateSigningKeyPair();
    EXPECT_EQ(ER_OK, status) << "  GenerateSigningKeyPair failed.  Actual Status: " << QCC_StatusText(status);

    /* Retrieve the DSA keys */
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = JoinPeerSession(adminProxyBus, adminBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
    ProxyBusObject clientProxyObject(adminProxyBus, adminBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
    ECCPublicKey claimedPubKey;
    CredentialAccessor ca(adminBus);
    qcc::GUID128 issuerGUID;
    ca.GetGuid(issuerGUID);
    EnableSecurityAdminProxy("ALLJOYN_ECDHE_NULL");

    qcc::String der;
    status = CreateIdentityCert("1010101", issuerGUID, &issuerPrivateKey, issuerGUID, &issuerPubKey, "Admin user", der);
    EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);
    status = Claim(adminProxyBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, issuerGUID, der);
    EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);
    /* retrieve the current identity cert */
    IdentityCertificate cert;
    status = GetIdentity(adminBus, clientProxyObject, cert);
    EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);
}

/*
 *  Test PermissionMgmt Claim method to the service
 */
TEST_F(PermissionMgmtTest, ClaimService)
{
    QStatus status = ER_OK;
    EnableSecurity("ALLJOYN_ECDHE_NULL", false);
    /* factory reset */
    PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
    status = pc.Reset();
    EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = JoinPeerSession(adminBus, serviceBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);

    SetNotifyConfigSignalReceived(false);

    /* setup state unclaimable */
    PermissionConfigurator::ClaimableState claimableState = pc.GetClaimableState();
    EXPECT_EQ(PermissionConfigurator::STATE_CLAIMABLE, claimableState) << "  ClaimableState is not CLAIMABLE";
    status = pc.SetClaimable(false);
    EXPECT_EQ(ER_OK, status) << "  SetClaimable failed.  Actual Status: " << QCC_StatusText(status);
    claimableState = pc.GetClaimableState();
    EXPECT_EQ(PermissionConfigurator::STATE_UNCLAIMABLE, claimableState) << "  ClaimableState is not UNCLAIMABLE";
    CredentialAccessor ca(adminBus);
    qcc::GUID128 issuerGUID;
    ca.GetGuid(issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    ECCPublicKey claimedPubKey;
    /* retrieve public key from to-be-claimed app to create identity cert */
    status = GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey);
    EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
    /* create identity cert for the claimed app */
    qcc::String der;
    status = CreateIdentityCert("2020202", issuerGUID, &issuerPrivateKey, serviceGUID, &claimedPubKey, "Service Provider", der);
    EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

    /* try claiming with state unclaimable.  Exptect to fail */
    status = Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, serviceGUID, der);
    EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

    /* now switch it back to claimable */
    status = pc.SetClaimable(true);
    EXPECT_EQ(ER_OK, status) << "  SetClaimable failed.  Actual Status: " << QCC_StatusText(status);
    claimableState = pc.GetClaimableState();
    EXPECT_EQ(PermissionConfigurator::STATE_CLAIMABLE, claimableState) << "  ClaimableState is not CLAIMABLE";

    /* try claiming with state laimable.  Exptect to succeed */
    status = Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, serviceGUID, der);
    EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

    /* try to claim one more time */
    status = Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, serviceGUID, der);
    EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

    /* sleep a second to see whether the NotifyConfig signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetNotifyConfigSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
    /* retrieve the current identity cert */
    IdentityCertificate cert;
    status = GetIdentity(adminBus, clientProxyObject, cert);
    EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);
    ECCPublicKey claimedPubKey2;
    /* retrieve public key from claimed app to validate that it is not changed */
    status = GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey2);
    EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(memcmp(&claimedPubKey2, &claimedPubKey, sizeof(ECCPublicKey)), 0) << "  The public key of the claimed app has changed.";
}

/*
 *  Test PermissionMgmt Claim method to the consumer
 */
TEST_F(PermissionMgmtTest, ClaimConsumer)
{
    QStatus status = ER_OK;
    EnableSecurity("ALLJOYN_ECDHE_NULL", false);
    /* factory reset */
    PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
    status = pc.Reset();
    EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = JoinPeerSession(adminBus, consumerBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
    ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
    ECCPublicKey claimedPubKey;

    CredentialAccessor ca(adminBus);
    qcc::GUID128 issuerGUID;
    ca.GetGuid(issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
    /* retrieve public key from to-be-claimed app to create identity cert */
    status = GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey);
    EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
    /* create identity cert for the claimed app */
    qcc::String der;
    status = CreateIdentityCert("3030303", issuerGUID, &issuerPrivateKey, consumerGUID, &claimedPubKey, "Consumer", der);
    EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);
    SetNotifyConfigSignalReceived(false);
    status = Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, consumerGUID, der);
    EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

    /* try to claim a second time */
    status = Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, consumerGUID, der);
    EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

    /* sleep a second to see whether the NotifyConfig signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetNotifyConfigSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
    /* retrieve the current identity cert */
    IdentityCertificate cert;
    status = GetIdentity(adminBus, clientProxyObject, cert);
    EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);
}

/*
 *  Test PermissionMgmt InstallPolicy method
 */
TEST_F(PermissionMgmtTest, InstallPolicy)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    ECCPublicKey guildAuthorityPubKey;
    status = RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);

    SetNotifyConfigSignalReceived(false);
    PermissionPolicy* policy = GeneratePolicy(localGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    status = InstallPolicy(adminBus, clientProxyObject, *policy);
    EXPECT_EQ(ER_OK, status) << "  InstallPolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* retrieve back the policy to compare */
    PermissionPolicy retPolicy;
    status = GetPolicy(adminBus, clientProxyObject, retPolicy);
    EXPECT_EQ(ER_OK, status) << "  GetPolicy failed.  Actual Status: " << QCC_StatusText(status);

    EXPECT_EQ(policy->GetSerialNum(), retPolicy.GetSerialNum()) << " GetPolicy failed. Different serial number.";
    EXPECT_EQ(policy->GetAdminsSize(), retPolicy.GetAdminsSize()) << " GetPolicy failed. Different admin size.";
    EXPECT_EQ(policy->GetTermsSize(), retPolicy.GetTermsSize()) << " GetPolicy failed. Different provider size.";
    delete policy;
    /* sleep a second to see whether the NotifyConfig signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetNotifyConfigSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
}

/*
 *  Test PermissionMgmt InstallIdentity method
 */
TEST_F(PermissionMgmtTest, ReplaceIdentity)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    /* retrieve the current identity cert */
    IdentityCertificate cert;
    status = GetIdentity(adminBus, clientProxyObject, cert);
    EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);

    /* create a new identity cert */
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    ca.GetGuid(localGUID);
    qcc::String der;
    status = CreateIdentityCert("4040404", localGUID, &issuerPrivateKey, cert.GetSubject(), cert.GetSubjectPublicKey(), "Service Provider", der);
    EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

    status = InstallIdentity(adminBus, clientProxyObject, der);
    EXPECT_EQ(ER_OK, status) << "  InstallIdentity failed.  Actual Status: " << QCC_StatusText(status);

    /* retrieve back the identity cert to compare */
    IdentityCertificate newCert;
    status = GetIdentity(adminBus, clientProxyObject, newCert);
    EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);
    qcc::String retIdentity;
    status = newCert.EncodeCertificateDER(retIdentity);
    EXPECT_EQ(ER_OK, status) << "  newCert.EncodeCertificateDER failed.  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(der.c_str(), retIdentity.c_str()) << "  GetIdentity failed.  Return value does not equal original";

}

/*
 *  Test PermissionMgmt InstallMembership method to a provider
 */
TEST_F(PermissionMgmtTest, InstallMembershipToServiceProvider)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    ECCPublicKey claimedPubKey;
    status = RetrieveDSAPublicKeyFromKeyStore(serviceBus, &claimedPubKey);
    EXPECT_EQ(ER_OK, status) << "  InstallMembership RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    status = InstallMembership(membershipSerial3, adminBus, clientProxyObject, adminBus, serviceGUID, &claimedPubKey, membershipGUID3);
    EXPECT_EQ(ER_OK, status) << "  InstallMembership cert1 failed.  Actual Status: " << QCC_StatusText(status);
    status = InstallMembership(membershipSerial3, adminBus, clientProxyObject, adminBus, serviceGUID, &claimedPubKey, membershipGUID3);
    EXPECT_NE(ER_OK, status) << "  InstallMembership cert1 again is supposed to fail.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test PermissionMgmt RemoveMembership method from service provider
 */
TEST_F(PermissionMgmtTest, RemoveMembershipFromServiceProvider)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);

    status = RemoveMembership(adminBus, clientProxyObject, membershipSerial3, localGUID);
    EXPECT_EQ(ER_OK, status) << "  RemoveMembershipFromServiceProvider failed.  Actual Status: " << QCC_StatusText(status);

    /* removing it again */
    status = RemoveMembership(adminBus, clientProxyObject, membershipSerial3, localGUID);
    EXPECT_NE(ER_OK, status) << "  RemoveMembershipFromServiceProvider succeeded.  Expect it to fail.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test PermissionMgmt InstallMembership method to a consumer
 */
TEST_F(PermissionMgmtTest, InstallMembershipToConsumer)
{
    ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    ECCPublicKey claimedPubKey;
    status = RetrieveDSAPublicKeyFromKeyStore(consumerBus, &claimedPubKey);
    EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    /* use the consumer as the guild authority for the membership cert */
    status = InstallMembership(membershipSerial1, adminBus, clientProxyObject, consumerBus, consumerGUID, &claimedPubKey, membershipGUID1);

    EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer cert1 failed.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test PermissionMgmt InstallGuildEquivalence method
 */
TEST_F(PermissionMgmtTest, InstallGuildEquivalence)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    const char* pem = clientCertChainType1PEM;
    status = InstallGuildEquivalence(adminBus, clientProxyObject, pem);
    EXPECT_EQ(ER_OK, status) << "  InstallGuildEquivalence failed.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test access by any user
 */
TEST_F(PermissionMgmtTest, AnyUserCanCallOnAndNotOff)
{

    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);
    ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), APP_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    QStatus status = ExcerciseOn(consumerBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  AccessOnOffByAnyUser ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
    status = ExcerciseOff(consumerBus, clientProxyObject);
    EXPECT_NE(ER_OK, status) << "  AccessOffByAnyUser ExcersizeOff did not fail.  Actual Status: " << QCC_StatusText(status);
    ECCPublicKey publicKey;
    PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
    GUID128 serviceGUID(0);
    CredentialAccessor ca(consumerBus);
    qcc::String peerName = serviceBus.GetUniqueName();
    status = ca.GetPeerGuid(peerName, serviceGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetPeerGuid failed.  Actual Status: " << QCC_StatusText(status);
    status = pc.GetConnectedPeerPublicKey(serviceGUID, &publicKey);
    EXPECT_EQ(ER_OK, status) << "  GetConnectedPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
}

/*
 *  Test access by guild user
 */
TEST_F(PermissionMgmtTest, GuildMemberCanTVUpAndDownAndNotChannel)
{

    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);
    ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), APP_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    QStatus status = ExcerciseTVUp(consumerBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
    status = ExcerciseTVDown(consumerBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVDown failed.  Actual Status: " << QCC_StatusText(status);
    status = ExcerciseTVChannel(consumerBus, clientProxyObject);
    EXPECT_NE(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVChannel did not fail.  Actual Status: " << QCC_StatusText(status);
}

/*
 *  Test access by consumer
 */
TEST_F(PermissionMgmtTest, ConsumerCanMuteTV)
{

    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);
    ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), APP_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    QStatus status = ExcerciseTVMute(consumerBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  ExcerciseTVMute failed.  Actual Status: " << QCC_StatusText(status);
}

/*
 *  Test manifest
 */
TEST_F(PermissionMgmtTest, SetPermissionManifest)
{
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    PermissionPolicy::Rule* rules = NULL;
    size_t count = 0;
    QStatus status = GenerateManifest(&rules, &count);
    EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GenerateManifest failed.  Actual Status: " << QCC_StatusText(status);
    PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
    status = pc.SetPermissionManifest(rules, count);
    EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest SetPermissionManifest failed.  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    PermissionPolicy::Rule* retrievedRules = NULL;
    size_t retrievedCount = 0;
    status = GetManifest(consumerBus, clientProxyObject, &retrievedRules, &retrievedCount);
    EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GetManifest failed.  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(count, retrievedCount) << "  SetPermissionManifest GetManifest failed to retrieve the same count.";
    delete [] rules;
    delete [] retrievedRules;
}

/*
 *  Test PermissionMgmt RemovePolicy method
 */
TEST_F(PermissionMgmtTest, RemovePolicy)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
    ECCPublicKey guildAuthorityPubKey;
    status = RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);

    PermissionPolicy* policy = GeneratePolicy(localGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    status = InstallPolicy(adminBus, clientProxyObject, *policy);
    EXPECT_EQ(ER_OK, status) << "  InstallPolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* retrieve back the policy to compare */
    PermissionPolicy retPolicy;
    status = GetPolicy(adminBus, clientProxyObject, retPolicy);
    EXPECT_EQ(ER_OK, status) << "  GetPolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* remove the policy */
    SetNotifyConfigSignalReceived(false);
    status = RemovePolicy(adminBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  RemovePolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* get policy again.  Expect it to fail */
    status = GetPolicy(adminBus, clientProxyObject, retPolicy);
    EXPECT_NE(ER_OK, status) << "  GetPolicy did not fail.  Actual Status: " << QCC_StatusText(status);
    delete policy;
    /* sleep a second to see whether the NotifyConfig signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetNotifyConfigSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
}

/*
 * Test PermissionMgmt RemoveMembership method from consumer.
 */
TEST_F(PermissionMgmtTest, RemoveMembershipFromConsumer)
{
    ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);

    status = RemoveMembership(adminBus, clientProxyObject, membershipSerial1, localGUID);
    EXPECT_EQ(ER_OK, status) << "  RemoveMembershipFromConsumer failed.  Actual Status: " << QCC_StatusText(status);

    /* removing it again */
    status = RemoveMembership(adminBus, clientProxyObject, membershipSerial1, localGUID);
    EXPECT_NE(ER_OK, status) << "  RemoveMembershipFromConsumer succeeded.  Expect it to fail.  Actual Status: " << QCC_StatusText(status);

}

/*
 * Test PermissionMgmt Reset method on service.  The consumer should not be
 * able to reset the service since the consumer is not an admin.
 */
TEST_F(PermissionMgmtTest, FailResetServiceByConsumer)
{
    ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    status = Reset(consumerBus, clientProxyObject);
    EXPECT_NE(ER_OK, status) << "  Reset is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

}

/*
 * Test PermissionMgmt Reset method on service.  The admin should be
 * able to reset the service.
 */
TEST_F(PermissionMgmtTest, SuccessfulResetServiceByAdmin)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    status = Reset(adminBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
    /* retrieve the current identity cert */
    IdentityCertificate cert;
    status = GetIdentity(adminBus, clientProxyObject, cert);
    EXPECT_NE(ER_OK, status) << "  GetIdentity is not supposed to succeed since it was removed by Reset.  Actual Status: " << QCC_StatusText(status);

}
