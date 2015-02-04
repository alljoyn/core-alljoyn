/******************************************************************************
 * Copyright (c) 2014-2015, AllSeen Alliance. All rights reserved.
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
#include "ajTestCommon.h"
#include "KeyInfoHelper.h"
#include "CredentialAccessor.h"
#include "PermissionMgmtObj.h"
#include "PermissionMgmtTest.h"

using namespace ajn;
using namespace qcc;

const char* BasePermissionMgmtTest::INTERFACE_NAME = "org.allseen.Security.PermissionMgmt";
const char* BasePermissionMgmtTest::NOTIFY_INTERFACE_NAME = "org.allseen.Security.PermissionMgmt.Notification";
const char* BasePermissionMgmtTest::ONOFF_IFC_NAME = "org.allseenalliance.control.OnOff";
const char* BasePermissionMgmtTest::TV_IFC_NAME = "org.allseenalliance.control.TV";

QStatus PermissionMgmtTestHelper::CreateIdentityCert(const qcc::String& serial, const qcc::GUID128& issuer, const ECCPrivateKey* issuerPrivateKey, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, const qcc::String& alias, qcc::String& der)
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

QStatus PermissionMgmtTestHelper::CreateMembershipCert(const String& serial, const uint8_t* authDataHash, const qcc::GUID128& issuer, BusAttachment& signingBus, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, bool delegate, qcc::String& der)
{
    QStatus status = ER_CRYPTO_ERROR;
    MembershipCertificate x509;

    x509.SetSerial(serial);
    x509.SetIssuer(issuer);
    x509.SetSubject(subject);
    x509.SetSubjectPublicKey(subjectPubKey);
    x509.SetGuild(guild);
    x509.SetCA(delegate);
    x509.SetDigest(authDataHash, Certificate::SHA256_DIGEST_SIZE);
    /* use the signing bus to sign the cert */
    PermissionConfigurator& pc = signingBus.GetPermissionConfigurator();
    status = pc.SignCertificate(x509);
    if (ER_OK != status) {
        return status;
    }
    return x509.EncodeCertificateDER(der);
}

QStatus PermissionMgmtTestHelper::CreateMembershipCert(const String& serial, const uint8_t* authDataHash, const qcc::GUID128& issuer, BusAttachment& signingBus, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, qcc::String& der)
{
    return CreateMembershipCert(serial, authDataHash, issuer, signingBus, subject, subjectPubKey, guild, false, der);
}

QStatus BasePermissionMgmtTest::InterestInSignal(BusAttachment* bus)
{
    const char* notifyConfigMatchRule = "type='signal',interface='" "org.allseen.Security.PermissionMgmt.Notification" "',member='NotifyConfig'";
    return bus->AddMatch(notifyConfigMatchRule);
}

QStatus BasePermissionMgmtTest::InterestInChannelChangedSignal(BusAttachment* bus)
{
    const char* tvChannelChangedMatchRule = "type='signal',interface='" "org.allseenalliance.control.TV" "',member='ChannelChanged'";
    return bus->AddMatch(tvChannelChangedMatchRule);
}

void BasePermissionMgmtTest::RegisterKeyStoreListeners()
{
    status = adminBus.RegisterKeyStoreListener(adminKeyStoreListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = adminProxyBus.RegisterKeyStoreListener(adminKeyStoreListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.RegisterKeyStoreListener(serviceKeyStoreListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = consumerBus.RegisterKeyStoreListener(consumerKeyStoreListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = remoteControlBus.RegisterKeyStoreListener(remoteControlKeyStoreListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

void BasePermissionMgmtTest::SetUp()
{
    status = SetupBus(adminBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(adminProxyBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(consumerBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(remoteControlBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    RegisterKeyStoreListeners();
    const InterfaceDescription* itf = adminBus.GetInterface(BasePermissionMgmtTest::NOTIFY_INTERFACE_NAME);
    status = adminBus.RegisterSignalHandler(this,
                                            static_cast<MessageReceiver::SignalHandler>(&BasePermissionMgmtTest::SignalHandler), itf->GetMember("NotifyConfig"), NULL);
    EXPECT_EQ(ER_OK, status) << "  Failed to register signal handler.  Actual Status: " << QCC_StatusText(status);
    status = InterestInSignal(&adminBus);
    EXPECT_EQ(ER_OK, status) << "  Failed to show interest in session-less signal.  Actual Status: " << QCC_StatusText(status);
}

void BasePermissionMgmtTest::TearDown()
{
    status = TeardownBus(adminBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = TeardownBus(adminProxyBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = TeardownBus(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = TeardownBus(consumerBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = TeardownBus(remoteControlBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    delete serviceKeyListener;
    serviceKeyListener = NULL;
    delete adminKeyListener;
    adminKeyListener = NULL;
    delete consumerKeyListener;
    consumerKeyListener = NULL;
    delete remoteControlKeyListener;
    remoteControlKeyListener = NULL;
}

void BasePermissionMgmtTest::EnableSecurity(const char* keyExchange)
{
    delete adminKeyListener;
    adminKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_ADMIN);
    adminBus.EnablePeerSecurity(keyExchange, adminKeyListener, NULL, true);
    adminProxyBus.EnablePeerSecurity(keyExchange, adminKeyListener, NULL, true);
    delete serviceKeyListener;
    serviceKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_SERVICE);
    serviceBus.EnablePeerSecurity(keyExchange, serviceKeyListener, NULL, false);
    delete consumerKeyListener;
    consumerKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_CONSUMER);
    consumerBus.EnablePeerSecurity(keyExchange, consumerKeyListener, NULL, false);
    delete remoteControlKeyListener;
    remoteControlKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_CONSUMER);
    remoteControlBus.EnablePeerSecurity(keyExchange, remoteControlKeyListener, NULL, false);
}

void BasePermissionMgmtTest::CreateOnOffAppInterface(BusAttachment& bus, bool addService)
{
    /* create/activate alljoyn_interface */
    InterfaceDescription* ifc = NULL;
    QStatus status = bus.CreateInterface(BasePermissionMgmtTest::ONOFF_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
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
    AddMethodHandler(ifc->GetMember("On"), static_cast<MessageReceiver::MethodHandler>(&BasePermissionMgmtTest::OnOffOn));
    AddMethodHandler(ifc->GetMember("Off"), static_cast<MessageReceiver::MethodHandler>(&BasePermissionMgmtTest::OnOffOff));
}

void BasePermissionMgmtTest::CreateTVAppInterface(BusAttachment& bus, bool addService)
{
    /* create/activate alljoyn_interface */
    InterfaceDescription* ifc = NULL;
    QStatus status = bus.CreateInterface(BasePermissionMgmtTest::TV_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
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
        status = ifc->AddMember(MESSAGE_METHOD_CALL, "InputSource", NULL, NULL, NULL);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = ifc->AddSignal("ChannelChanged", "u", "newChannel");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = ifc->AddProperty("Volume", "u", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = ifc->AddProperty("Caption", "y", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        ifc->Activate();
        status = bus.RegisterSignalHandler(this,
                                           static_cast<MessageReceiver::SignalHandler>(&BasePermissionMgmtTest::ChannelChangedSignalHandler), ifc->GetMember("ChannelChanged"), NULL);
        EXPECT_EQ(ER_OK, status) << "  Failed to register channel changed signal handler.  Actual Status: " << QCC_StatusText(status);
        status = InterestInChannelChangedSignal(&bus);
        EXPECT_EQ(ER_OK, status) << "  Failed to show interest in channel changed signal.  Actual Status: " << QCC_StatusText(status);
    }
    if (!addService) {
        return;  /* done */
    }
    status = AddInterface(*ifc);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AddMethodHandler(ifc->GetMember("Up"), static_cast<MessageReceiver::MethodHandler>(&BasePermissionMgmtTest::TVUp));
    AddMethodHandler(ifc->GetMember("Down"), static_cast<MessageReceiver::MethodHandler>(&BasePermissionMgmtTest::TVDown));
    AddMethodHandler(ifc->GetMember("Channel"), static_cast<MessageReceiver::MethodHandler>(&BasePermissionMgmtTest::TVChannel));
    AddMethodHandler(ifc->GetMember("Mute"), static_cast<MessageReceiver::MethodHandler>(&BasePermissionMgmtTest::TVMute));
    AddMethodHandler(ifc->GetMember("InputSource"), static_cast<MessageReceiver::MethodHandler>(&BasePermissionMgmtTest::TVInputSource));
}

void BasePermissionMgmtTest::CreateAppInterfaces(BusAttachment& bus, bool addService)
{
    CreateOnOffAppInterface(bus, addService);
    CreateTVAppInterface(bus, addService);
    if (addService) {
        QStatus status = bus.RegisterBusObject(*this);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }
}

void BasePermissionMgmtTest::SignalHandler(const InterfaceDescription::Member* member,
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

void BasePermissionMgmtTest::ChannelChangedSignalHandler(const InterfaceDescription::Member* member,
                                                         const char* sourcePath, Message& msg)
{
    uint32_t channel;
    QStatus status = msg->GetArg(0)->Get("u", &channel);
    EXPECT_EQ(ER_OK, status) << "  Retrieve the TV channel failed.  Actual Status: " << QCC_StatusText(status);
    SetChannelChangedSignalReceived(true);
}

void BasePermissionMgmtTest::SetNotifyConfigSignalReceived(bool flag)
{
    signalNotifyConfigReceived = flag;
}

const bool BasePermissionMgmtTest::GetNotifyConfigSignalReceived()
{
    return signalNotifyConfigReceived;
}

void BasePermissionMgmtTest::SetChannelChangedSignalReceived(bool flag)
{
    channelChangedSignalReceived = flag;
}

const bool BasePermissionMgmtTest::GetChannelChangedSignalReceived()
{
    return channelChangedSignalReceived;
}

void BasePermissionMgmtTest::OnOffOn(const InterfaceDescription::Member* member, Message& msg)
{
    MethodReply(msg, ER_OK);
}

void BasePermissionMgmtTest::OnOffOff(const InterfaceDescription::Member* member, Message& msg)
{
    MethodReply(msg, ER_OK);
}

void BasePermissionMgmtTest::TVUp(const InterfaceDescription::Member* member, Message& msg)
{
    currentTVChannel++;
    MethodReply(msg, ER_OK);
    TVChannelChanged(member, msg);
}

void BasePermissionMgmtTest::TVDown(const InterfaceDescription::Member* member, Message& msg)
{
    if (currentTVChannel > 1) {
        currentTVChannel--;
    }
    MethodReply(msg, ER_OK);
    TVChannelChanged(member, msg);
}

void BasePermissionMgmtTest::TVChannel(const InterfaceDescription::Member* member, Message& msg)
{
    MethodReply(msg, ER_OK);
    /* emit a signal */
    TVChannelChanged(member, msg);
}

void BasePermissionMgmtTest::TVChannelChanged(const InterfaceDescription::Member* member, Message& msg)
{
    /* emit a signal */
    MsgArg args[1];
    args[0].Set("u", currentTVChannel);
    Signal(consumerBus.GetUniqueName().c_str(), 0, *member->iface->GetMember("ChannelChanged"), args, 1, 0, 0);
    Signal(remoteControlBus.GetUniqueName().c_str(), 0, *member->iface->GetMember("ChannelChanged"), args, 1, 0, 0);
}

void BasePermissionMgmtTest::TVMute(const InterfaceDescription::Member* member, Message& msg)
{
    MethodReply(msg, ER_OK);
}

void BasePermissionMgmtTest::TVInputSource(const InterfaceDescription::Member* member, Message& msg)
{
    MethodReply(msg, ER_OK);
}

QStatus BasePermissionMgmtTest::SetupBus(BusAttachment& bus)
{
    QStatus status = bus.Start();
    if (ER_OK != status) {
        return status;
    }
    return bus.Connect(getConnectArg().c_str());
}

QStatus BasePermissionMgmtTest::TeardownBus(BusAttachment& bus)
{
    bus.UnregisterKeyStoreListener();
    bus.UnregisterBusObject(*this);
    status = bus.Disconnect(getConnectArg().c_str());
    if (ER_OK != status) {
        return status;
    }
    status = bus.Stop();
    if (ER_OK != status) {
        return status;
    }
    return bus.Join();
}

bool PermissionMgmtTestHelper::IsPermissionDeniedError(QStatus status, Message& msg)
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

QStatus PermissionMgmtTestHelper::RetrievePublicKeyFromMsgArg(MsgArg& arg, ECCPublicKey* pubKey)
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

QStatus PermissionMgmtTestHelper::ReadClaimResponse(Message& msg, ECCPublicKey* pubKey)
{
    return RetrievePublicKeyFromMsgArg((MsgArg &) * msg->GetArg(0), pubKey);
}

QStatus PermissionMgmtTestHelper::Claim(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::GUID128& issuerGUID, const ECCPublicKey* pubKey, ECCPublicKey* claimedPubKey, qcc::String& identityCertDER, bool setKeyId)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg inputs[2];

    KeyInfoNISTP256 keyInfo;
    if (setKeyId) {
        keyInfo.SetKeyId(issuerGUID.GetBytes(), GUID128::SIZE);
    }
    keyInfo.SetPublicKey(pubKey);
    KeyInfoHelper::KeyInfoNISTP256ToMsgArg(keyInfo, inputs[0]);
    inputs[1].Set("(yay)", Certificate::ENCODING_X509_DER, identityCertDER.size(), identityCertDER.data());
    uint32_t timeout = 10000; /* Claim is a bit show */

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "Claim", inputs, 2, reply, timeout);

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

QStatus PermissionMgmtTestHelper::Claim(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::GUID128& issuerGUID, const ECCPublicKey* pubKey, ECCPublicKey* claimedPubKey, qcc::String& identityCertDER)
{
    return Claim(bus, remoteObj, issuerGUID, pubKey, claimedPubKey, identityCertDER, true);
}

QStatus PermissionMgmtTestHelper::GetManifest(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy::Rule** retRules, size_t* count)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "GetManifest", NULL, 0, reply, 5000);

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

QStatus PermissionMgmtTestHelper::InstallPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg policyArg;

    status = policy.Export(policyArg);
    if (ER_OK != status) {
        return status;
    }
    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "InstallPolicy", &policyArg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::GetPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "GetPolicy", NULL, 0, reply, 5000);

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

QStatus PermissionMgmtTestHelper::RemovePolicy(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "RemovePolicy", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::RetrieveDSAPrivateKeyFromKeyStore(BusAttachment& bus, ECCPrivateKey* privateKey)
{
    CredentialAccessor ca(bus);
    return PermissionMgmtObj::RetrieveDSAPrivateKey(&ca, privateKey);
}

QStatus PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(BusAttachment& bus, ECCPublicKey* publicKey)
{
    CredentialAccessor ca(bus);
    return PermissionMgmtObj::RetrieveDSAPublicKey(&ca, publicKey);
}

QStatus PermissionMgmtTestHelper::RetrieveDSAKeys(BusAttachment& bus, ECCPrivateKey& privateKey, ECCPublicKey& publicKey)
{
    /* Retrieve the DSA keys */
    QStatus status = RetrieveDSAPrivateKeyFromKeyStore(bus, &privateKey);
    if (ER_OK != status) {
        return status;
    }
    return RetrieveDSAPublicKeyFromKeyStore(bus, &publicKey);
}

QStatus PermissionMgmtTestHelper::LoadCertificateBytes(Message& msg, CertificateX509& cert)
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

static QStatus InvokeInstallMembership(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::String& der)
{
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg certArgs[1];
    certArgs[0].Set("(yay)", Certificate::ENCODING_X509_DER, der.length(), der.c_str());
    MsgArg arg("a(yay)", 1, certArgs);

    QStatus status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "InstallMembership", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (PermissionMgmtTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus InvokeInstallMembership(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::String* derArray, size_t derCount)
{
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg* certArgs = new MsgArg[derCount];
    for (size_t cnt = 0; cnt < derCount; cnt++) {
        certArgs[cnt].Set("(yay)", Certificate::ENCODING_X509_DER, derArray[cnt].length(), derArray[cnt].c_str());
    }
    MsgArg arg("a(yay)", derCount, certArgs);

    QStatus status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "InstallMembership", &arg, 1, reply, 5000);
    delete [] certArgs;
    if (ER_OK != status) {
        if (PermissionMgmtTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

static QStatus InvokeInstallMembershipAuthData(BusAttachment& bus, ProxyBusObject& remoteObj, const String& serial, const GUID128& issuer, PermissionPolicy& authData)
{
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg args[3];
    args[0].Set("s", serial.c_str());
    args[1].Set("ay", GUID128::SIZE, issuer.GetBytes());
    authData.Export(args[2]);
    QStatus status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "InstallMembershipAuthData", args, ArraySize(args), reply, 5000);
    if (ER_OK != status) {
        if (PermissionMgmtTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::InstallMembership(const String& serial, BusAttachment& bus, ProxyBusObject& remoteObj, BusAttachment& signingBus, const qcc::GUID128& subjectGUID, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, PermissionPolicy* membershipAuthData)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    CredentialAccessor ca(bus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    if (status != ER_OK) {
        return status;
    }
    uint8_t digest[Certificate::SHA256_DIGEST_SIZE];
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    membershipAuthData->Digest(marshaller, digest, Certificate::SHA256_DIGEST_SIZE);

    qcc::String der;
    status = CreateMembershipCert(serial, digest, localGUID, signingBus, subjectGUID, subjectPubKey, guild, der);
    if (status != ER_OK) {
        return status;
    }
    status = InvokeInstallMembership(bus, remoteObj, der);
    if (ER_OK != status) {
        return status;
    }

    /* installing the auth data */
    return InvokeInstallMembershipAuthData(bus, remoteObj, serial, localGUID, *membershipAuthData);
}

QStatus PermissionMgmtTestHelper::InstallMembershipChain(const String& serial, BusAttachment& bus, ProxyBusObject& remoteObj, BusAttachment& signingBus, BusAttachment& subjectBus, const qcc::GUID128& subjectGUID, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, PermissionPolicy** authDataArray)
{
    CredentialAccessor ca(bus);
    qcc::GUID128 localGUID;
    QStatus status = ca.GetGuid(localGUID);
    if (status != ER_OK) {
        return status;
    }
    /* create the second cert first -- with delegate on  */
    uint8_t digest[Certificate::SHA256_DIGEST_SIZE];
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    authDataArray[1]->Digest(marshaller, digest, Certificate::SHA256_DIGEST_SIZE);
    qcc::String derArray[2];
    status = CreateMembershipCert(serial, digest, localGUID, signingBus, subjectGUID, subjectPubKey, guild, true, derArray[1]);
    if (status != ER_OK) {
        return status;
    }

    /* create the leaf cert signed by the subject */
    authDataArray[0]->Digest(marshaller, digest, Certificate::SHA256_DIGEST_SIZE);
    status = CreateMembershipCert(serial, digest, subjectGUID, subjectBus, subjectGUID, subjectPubKey, guild, false, derArray[0]);
    if (status != ER_OK) {
        return status;
    }

    /* install cert chain */
    status = InvokeInstallMembership(bus, remoteObj, derArray, 2);
    if (ER_OK != status) {
        return status;
    }

    /* installing the auth data for leaf cert*/
    status = InvokeInstallMembershipAuthData(bus, remoteObj, serial, subjectGUID, *authDataArray[0]);
    if (ER_OK != status) {
        return status;
    }

    /* installing the auth data for second cert */
    status = InvokeInstallMembershipAuthData(bus, remoteObj, serial, localGUID, *authDataArray[1]);
    if (ER_OK != status) {
        return status;
    }

    return status;
}

QStatus PermissionMgmtTestHelper::RemoveMembership(BusAttachment& bus, ProxyBusObject& remoteObj, const qcc::String serialNum, const qcc::GUID128& issuer)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    MsgArg inputs[2];
    inputs[0].Set("s", serialNum.c_str());
    inputs[1].Set("ay", GUID128::SIZE, issuer.GetBytes());

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "RemoveMembership", inputs, 2, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::InstallIdentity(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::String& der)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg arg("(yay)", Certificate::ENCODING_X509_DER, der.size(), der.data());

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "InstallIdentity", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::GetPeerPublicKey(BusAttachment& bus, ProxyBusObject& remoteObj, ECCPublicKey* pubKey)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "GetPublicKey", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    return RetrievePublicKeyFromMsgArg((MsgArg &) * reply->GetArg(0), pubKey);
}

QStatus PermissionMgmtTestHelper::GetIdentity(BusAttachment& bus, ProxyBusObject& remoteObj, IdentityCertificate& cert)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "GetIdentity", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    return LoadCertificateBytes(reply, cert);
}

QStatus PermissionMgmtTestHelper::Reset(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "Reset", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::InstallGuildEquivalence(BusAttachment& bus, ProxyBusObject& remoteObj, const char* pem)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg arg("(yay)", Certificate::ENCODING_X509_DER_PEM, strlen(pem), pem);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "InstallGuildEquivalence", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExcerciseOn(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::ONOFF_IFC_NAME, "On", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExcerciseOff(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::ONOFF_IFC_NAME, "Off", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExcerciseTVUp(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "Up", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::GetTVVolume(BusAttachment& bus, ProxyBusObject& remoteObj, uint32_t& volume)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    MsgArg val;
    status = remoteObj.GetProperty(BasePermissionMgmtTest::TV_IFC_NAME, "Volume", val);
    if (ER_OK == status) {
        val.Get("u", &volume);
    }
    return status;
}

QStatus PermissionMgmtTestHelper::GetTVCaption(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);

    /* test the GetAllProperites */
    MsgArg mapVal;
    return remoteObj.GetAllProperties(BasePermissionMgmtTest::TV_IFC_NAME, mapVal);
}

QStatus PermissionMgmtTestHelper::SetTVVolume(BusAttachment& bus, ProxyBusObject& remoteObj, uint32_t volume)
{
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    MsgArg val("u", volume);
    return remoteObj.SetProperty(BasePermissionMgmtTest::TV_IFC_NAME, "Volume", val);
}

QStatus PermissionMgmtTestHelper::ExcerciseTVDown(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "Down", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExcerciseTVChannel(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "Channel", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExcerciseTVMute(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "Mute", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExcerciseTVInputSource(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "InputSource", NULL, 0, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::JoinPeerSession(BusAttachment& initiator, BusAttachment& responder, SessionId& sessionId)
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

QStatus PermissionMgmtTestHelper::GetGUID(BusAttachment& bus, qcc::GUID128& guid)
{
    CredentialAccessor ca(bus);
    return ca.GetGuid(guid);
}

QStatus PermissionMgmtTestHelper::GetPeerGUID(BusAttachment& bus, qcc::String& peerName, qcc::GUID128& peerGuid)
{
    CredentialAccessor ca(bus);
    return ca.GetPeerGuid(peerName, peerGuid);
}

QStatus BasePermissionMgmtTest::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    if (0 == strcmp("Volume", propName)) {
        val.typeId = ALLJOYN_UINT32;
        val.v_uint32 = volume;
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}

void BasePermissionMgmtTest::GetAllProps(const InterfaceDescription::Member* member, Message& msg)
{
    MsgArg vals;
    vals.Set("a{sv}", 0, NULL);
    MethodReply(msg, &vals, 1);
}

QStatus BasePermissionMgmtTest::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    if ((0 == strcmp("Volume", propName)) && (val.typeId == ALLJOYN_UINT32)) {
        volume = val.v_uint32;
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}

QStatus PermissionMgmtTestHelper::InstallCredential(const PermissionMgmtObj::TrustAnchorType taType, BusAttachment& bus, ProxyBusObject& remoteObj, const qcc::GUID128& guid, const ECCPublicKey* pubKey)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg keyInfoArg;
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetKeyId(guid.GetBytes(), GUID128::SIZE);
    keyInfo.SetPublicKey(pubKey);
    KeyInfoHelper::KeyInfoNISTP256ToMsgArg(keyInfo, keyInfoArg);
    MsgArg credentialArg("(yv)", taType, &keyInfoArg);

    MsgArg args[2];
    args[0].Set("y", 0);
    args[1].Set("v", &credentialArg);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "InstallCredential", args, 2, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::RemoveCredential(const PermissionMgmtObj::TrustAnchorType taType, BusAttachment& bus, ProxyBusObject& remoteObj, const qcc::GUID128& guid, const ECCPublicKey* pubKey)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg keyInfoArg;
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetKeyId(guid.GetBytes(), GUID128::SIZE);
    keyInfo.SetPublicKey(pubKey);
    KeyInfoHelper::KeyInfoNISTP256ToMsgArg(keyInfo, keyInfoArg);
    MsgArg credentialArg("(yv)", taType, &keyInfoArg);

    MsgArg args[2];
    args[0].Set("y", 0);
    args[1].Set("v", &credentialArg);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::INTERFACE_NAME, "RemoveCredential", args, 2, reply, 5000);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}
