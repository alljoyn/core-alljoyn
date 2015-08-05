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

#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <qcc/Log.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/StringUtil.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/ApplicationStateListener.h>
#include <alljoyn/FactoryResetListener.h>
#include "ajTestCommon.h"
#include "KeyInfoHelper.h"
#include "CredentialAccessor.h"
#include "PermissionMgmtObj.h"
#include "PermissionMgmtTest.h"
#include "BusInternal.h"

using namespace ajn;
using namespace qcc;

const char* BasePermissionMgmtTest::INTERFACE_NAME = "org.allseen.Security.PermissionMgmt";
const char* BasePermissionMgmtTest::ONOFF_IFC_NAME = "org.allseenalliance.control.OnOff";
const char* BasePermissionMgmtTest::TV_IFC_NAME = "org.allseenalliance.control.TV";

static void BuildValidity(CertificateX509::ValidPeriod& validity, uint32_t expiredInSecs)
{
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + expiredInSecs;
}

void TestApplicationStateListener::State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state)
{
    QCC_UNUSED(busName);
    QCC_UNUSED(publicKeyInfo);
    QCC_UNUSED(state);
    signalApplicationStateReceived = true;
}

QStatus TestFactoryResetListener::FactoryReset()
{
    factoryResetReceived = true;
    return ER_OK;
}

QStatus PermissionMgmtTestHelper::CreateIdentityCertChain(BusAttachment& caBus, BusAttachment& issuerBus, const qcc::String& serial, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::String& alias, uint32_t expiredInSecs, qcc::IdentityCertificate* certChain, size_t chainCount, uint8_t* digest, size_t digestSize)
{
    if (chainCount > 3) {
        return ER_INVALID_DATA;
    }
    QStatus status = ER_CRYPTO_ERROR;

    qcc::GUID128 ca(0);
    GetGUID(caBus, ca);
    String caStr = ca.ToString();
    PermissionConfigurator& caPC = caBus.GetPermissionConfigurator();
    if (chainCount == 3) {
        /* generate the self signed CA cert */
        String caSerial = serial + "02";
        certChain[2].SetSerial(reinterpret_cast<const uint8_t*>(caSerial.data()), caSerial.size());
        certChain[2].SetIssuerCN((const uint8_t*) caStr.data(), caStr.size());
        certChain[2].SetSubjectCN((const uint8_t*) caStr.data(), caStr.size());
        CertificateX509::ValidPeriod validity;
        BuildValidity(validity, expiredInSecs);
        certChain[2].SetValidity(&validity);
        certChain[2].SetCA(true);
        KeyInfoNISTP256 keyInfo;
        caPC.GetSigningPublicKey(keyInfo);
        certChain[2].SetSubjectPublicKey(keyInfo.GetPublicKey());
        status = caPC.SignCertificate(certChain[2]);
        if (ER_OK != status) {
            return status;
        }
    }

    /* generate the issuer cert */
    qcc::GUID128 issuer(0);
    GetGUID(issuerBus, issuer);
    String issuerStr = issuer.ToString();

    String issuerSerial = serial + "01";
    certChain[1].SetSerial(reinterpret_cast<const uint8_t*>(issuerSerial.data()), issuerSerial.size());
    certChain[1].SetIssuerCN((const uint8_t*) caStr.data(), caStr.size());
    certChain[1].SetSubjectCN((const uint8_t*) issuerStr.data(), issuerStr.size());
    CertificateX509::ValidPeriod validity;
    BuildValidity(validity, expiredInSecs);
    certChain[1].SetValidity(&validity);
    certChain[1].SetCA(true);
    PermissionConfigurator& pc = issuerBus.GetPermissionConfigurator();
    KeyInfoNISTP256 keyInfo;
    pc.GetSigningPublicKey(keyInfo);
    certChain[1].SetSubjectPublicKey(keyInfo.GetPublicKey());

    status = caPC.SignCertificate(certChain[1]);
    if (ER_OK != status) {
        return status;
    }

    /* generate the leaf cert */
    certChain[0].SetSerial(reinterpret_cast<const uint8_t*>(serial.data()), serial.size());
    certChain[0].SetIssuerCN((const uint8_t*) issuerStr.data(), issuerStr.size());
    certChain[0].SetSubjectCN((const uint8_t*) subject.data(), subject.size());
    certChain[0].SetSubjectPublicKey(subjectPubKey);
    certChain[0].SetAlias(alias);
    if (digest) {
        certChain[0].SetDigest(digest, digestSize);
    }
    certChain[0].SetValidity(&validity);

    /* use the issuer bus to sign the cert */
    status = pc.SignCertificate(certChain[0]);
    if (ER_OK != status) {
        return status;
    }

    status = certChain[0].Verify(certChain[1].GetSubjectPublicKey());
    if (ER_OK != status) {
        return status;
    }

    return ER_OK;
}

QStatus PermissionMgmtTestHelper::CreateIdentityCert(BusAttachment& issuerBus, const qcc::String& serial, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::String& alias, uint32_t expiredInSecs, qcc::IdentityCertificate& cert, uint8_t* digest, size_t digestSize)
{
    qcc::GUID128 issuer(0);
    GetGUID(issuerBus, issuer);

    QStatus status = ER_CRYPTO_ERROR;

    cert.SetSerial(reinterpret_cast<const uint8_t*>(serial.data()), serial.size());
    String issuerStr = issuer.ToString();
    cert.SetIssuerCN((const uint8_t*) issuerStr.data(), issuerStr.size());
    cert.SetSubjectCN((const uint8_t*) subject.data(), subject.size());
    cert.SetSubjectPublicKey(subjectPubKey);
    cert.SetAlias(alias);
    if (digest) {
        cert.SetDigest(digest, digestSize);
    }
    CertificateX509::ValidPeriod validity;
    BuildValidity(validity, expiredInSecs);
    cert.SetValidity(&validity);

    /* use the issuer bus to sign the cert */
    PermissionConfigurator& pc = issuerBus.GetPermissionConfigurator();
    status = pc.SignCertificate(cert);
    if (ER_OK != status) {
        return status;
    }

    KeyInfoNISTP256 keyInfo;
    pc.GetSigningPublicKey(keyInfo);
    status = cert.Verify(keyInfo.GetPublicKey());
    if (ER_OK != status) {
        return status;
    }

    return ER_OK;
}

QStatus PermissionMgmtTestHelper::CreateIdentityCert(BusAttachment& issuerBus, const qcc::String& serial, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::String& alias, uint32_t expiredInSecs, qcc::String& der)
{
    IdentityCertificate cert;
    QStatus status = CreateIdentityCert(issuerBus, serial, subject, subjectPubKey, alias, expiredInSecs, cert, NULL, 0);
    if (ER_OK != status) {
        return status;
    }
    return cert.EncodeCertificateDER(der);
}

QStatus PermissionMgmtTestHelper::CreateIdentityCert(BusAttachment& issuerBus, const qcc::String& serial, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::String& alias, qcc::String& der)
{
    /* expire the cert in 1 hour */
    return CreateIdentityCert(issuerBus, serial, subject, subjectPubKey, alias, 24 * 3600, der);
}

QStatus PermissionMgmtTestHelper::CreateMembershipCert(const String& serial, BusAttachment& signingBus, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, bool delegate, uint32_t expiredInSecs, qcc::MembershipCertificate& cert)
{
    qcc::GUID128 issuer(0);
    GetGUID(signingBus, issuer);

    cert.SetSerial(reinterpret_cast<const uint8_t*>(serial.data()), serial.size());
    String issuerStr = issuer.ToString();
    cert.SetIssuerCN((const uint8_t*) issuerStr.data(), issuerStr.size());
    cert.SetSubjectCN((const uint8_t*) subject.data(), subject.size());
    cert.SetSubjectPublicKey(subjectPubKey);
    cert.SetGuild(guild);
    cert.SetCA(delegate);
    CertificateX509::ValidPeriod validity;
    BuildValidity(validity, expiredInSecs);
    cert.SetValidity(&validity);
    /* use the signing bus to sign the cert */
    PermissionConfigurator& pc = signingBus.GetPermissionConfigurator();
    return pc.SignCertificate(cert);
}

QStatus PermissionMgmtTestHelper::CreateMembershipCert(const String& serial, BusAttachment& signingBus, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, bool delegate, uint32_t expiredInSecs, qcc::String& der)
{
    MembershipCertificate cert;
    QStatus status = CreateMembershipCert(serial, signingBus, subject, subjectPubKey, guild, delegate, expiredInSecs, cert);
    if (ER_OK != status) {
        return status;
    }
    return cert.EncodeCertificateDER(der);
}

QStatus PermissionMgmtTestHelper::CreateMembershipCert(const String& serial, BusAttachment& signingBus, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, bool delegate, qcc::String& der)
{
    /* expire the cert in 1 hour */
    return CreateMembershipCert(serial, signingBus, subject, subjectPubKey, guild, delegate, 24 * 3600, der);
}

QStatus PermissionMgmtTestHelper::CreateMembershipCert(const String& serial, BusAttachment& signingBus, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, qcc::String& der)
{
    return CreateMembershipCert(serial, signingBus, subject, subjectPubKey, guild, false, der);
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
    status = serviceBus.RegisterKeyStoreListener(serviceKeyStoreListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = consumerBus.RegisterKeyStoreListener(consumerKeyStoreListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = remoteControlBus.RegisterKeyStoreListener(remoteControlKeyStoreListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

static void GenerateSecurityGroupKey(BusAttachment& bus, KeyInfoNISTP256& keyInfo)
{
    PermissionConfigurator& pc = bus.GetPermissionConfigurator();
    pc.GetSigningPublicKey(keyInfo);
}

void BasePermissionMgmtTest::SetUp()
{
    status = SetupBus(adminBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(consumerBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(remoteControlBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    RegisterKeyStoreListeners();

    adminBus.RegisterApplicationStateListener(testASL);
    status = adminBus.AddApplicationStateRule();
    EXPECT_EQ(ER_OK, status) << "  Failed to show interest in session-less signal.  Actual Status: " << QCC_StatusText(status);
}

void BasePermissionMgmtTest::TearDown()
{
    status = TeardownBus(adminBus);
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

void BasePermissionMgmtTest::GenerateCAKeys()
{
    KeyInfoNISTP256 keyInfo;
    adminBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    consumerBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    serviceBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    remoteControlBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    GenerateSecurityGroupKey(adminBus, adminAdminGroupAuthority);
    GenerateSecurityGroupKey(consumerBus, consumerAdminGroupAuthority);
}

static AuthListener* GenAuthListener(const char* keyExchange) {
    if (strstr(keyExchange, "ECDHE_PSK")) {
        qcc::String psk("38347892FFBEF5B2442AEDE9E53C4B32");
        return new DefaultECDHEAuthListener(reinterpret_cast<const uint8_t*>(psk.data()), psk.size());
    }
    return new DefaultECDHEAuthListener();
}

void BasePermissionMgmtTest::EnableSecurity(const char* keyExchange)
{
    if (strstr(keyExchange, "ECDHE_PSK")) {
    }
    delete adminKeyListener;
    adminKeyListener = GenAuthListener(keyExchange);
    adminBus.EnablePeerSecurity(keyExchange, adminKeyListener, NULL, true);
    delete serviceKeyListener;
    serviceKeyListener = GenAuthListener(keyExchange);
    serviceBus.EnablePeerSecurity(keyExchange, serviceKeyListener, NULL, false, &testFRL);
    delete consumerKeyListener;
    consumerKeyListener = GenAuthListener(keyExchange);
    consumerBus.EnablePeerSecurity(keyExchange, consumerKeyListener, NULL, false, &testFRL);
    delete remoteControlKeyListener;
    remoteControlKeyListener = GenAuthListener(keyExchange);
    remoteControlBus.EnablePeerSecurity(keyExchange, remoteControlKeyListener, NULL, false, &testFRL);
    authMechanisms = keyExchange;
}

const qcc::String& BasePermissionMgmtTest::GetAuthMechanisms() const
{
    return authMechanisms;
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
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
        status = ifc->AddSignal("ChannelChanged", "u", "newChannel");
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
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

void BasePermissionMgmtTest::ChannelChangedSignalHandler(const InterfaceDescription::Member* member,
                                                         const char* sourcePath, Message& msg)
{
    QCC_UNUSED(sourcePath);
    QCC_UNUSED(member);
    uint32_t channel;
    QStatus status = msg->GetArg(0)->Get("u", &channel);
    EXPECT_EQ(ER_OK, status) << "  Retrieve the TV channel failed.  Actual Status: " << QCC_StatusText(status);
    SetChannelChangedSignalReceived(true);
}

void BasePermissionMgmtTest::SetApplicationStateSignalReceived(bool flag)
{
    testASL.signalApplicationStateReceived = flag;
}

const bool BasePermissionMgmtTest::GetApplicationStateSignalReceived()
{
    return testASL.signalApplicationStateReceived;
}

void BasePermissionMgmtTest::SetFactoryResetReceived(bool flag)
{
    testFRL.factoryResetReceived = flag;
}

const bool BasePermissionMgmtTest::GetFactoryResetReceived()
{
    return testFRL.factoryResetReceived;
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
    QCC_UNUSED(member);
    MethodReply(msg, ER_OK);
}

void BasePermissionMgmtTest::OnOffOff(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    MethodReply(msg, ER_OK);
}

void BasePermissionMgmtTest::TVUp(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    currentTVChannel++;
    MethodReply(msg, ER_OK);
    TVChannelChanged(member, msg);
}

void BasePermissionMgmtTest::TVDown(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    if (currentTVChannel > 1) {
        currentTVChannel--;
    }
    MethodReply(msg, ER_OK);
    TVChannelChanged(member, msg);
}

void BasePermissionMgmtTest::TVChannel(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    MethodReply(msg, ER_OK);
    /* emit a signal */
    TVChannelChanged(member, msg);
}

void BasePermissionMgmtTest::TVChannelChanged(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QCC_UNUSED(msg);
    /* emit a signal */
    MsgArg args[1];
    args[0].Set("u", currentTVChannel);
    Signal(consumerBus.GetUniqueName().c_str(), 0, *member->iface->GetMember("ChannelChanged"), args, 1, 0, 0);
    Signal(remoteControlBus.GetUniqueName().c_str(), 0, *member->iface->GetMember("ChannelChanged"), args, 1, 0, 0);
}

void BasePermissionMgmtTest::TVMute(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    MethodReply(msg, ER_OK);
}

void BasePermissionMgmtTest::TVInputSource(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
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
    if (!bus.IsStarted()) {
        return ER_OK;
    }
    bus.UnregisterKeyStoreListener();
    bus.UnregisterBusObject(*this);
    status = bus.Disconnect();
    if (ER_OK != status) {
        return status;
    }
    status = bus.Stop();
    if (ER_OK != status) {
        return status;
    }
    return bus.Join();
}

void BasePermissionMgmtTest::DetermineStateSignalReachable()
{
    /* sleep a max of 1 second to see whether the ApplicationState signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetApplicationStateSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    canTestStateSignalReception = GetApplicationStateSignalReceived();
    SetApplicationStateSignalReceived(false);
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
        if (strcmp(errorName, "org.alljoyn.Bus.Security.Error.PermissionDenied") == 0) {
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
    return pubKey->Import(xCoord, xLen, yCoord, yLen);
}

QStatus PermissionMgmtTestHelper::ReadClaimResponse(Message& msg, ECCPublicKey* pubKey)
{
    return RetrievePublicKeyFromMsgArg((MsgArg&) *msg->GetArg(0), pubKey);
}

QStatus PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(BusAttachment& bus, ECCPublicKey* publicKey)
{
    CredentialAccessor ca(bus);
    return ca.GetDSAPublicKey(*publicKey);
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
    if (encoding == CertificateX509::ENCODING_X509_DER) {
        status = cert.DecodeCertificateDER(String((const char*) encoded, encodedLen));
    } else if (encoding == CertificateX509::ENCODING_X509_DER_PEM) {
        status = cert.DecodeCertificatePEM(String((const char*) encoded, encodedLen));
    }
    return status;
}

QStatus PermissionMgmtTestHelper::InstallMembership(const String& serial, BusAttachment& bus, const qcc::String& remoteObjName, BusAttachment& signingBus, const qcc::String& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild)
{
    SecurityApplicationProxy saProxy(bus, remoteObjName.c_str());
    QStatus status;

    qcc::MembershipCertificate certs[1];
    status = CreateMembershipCert(serial, signingBus, subject, subjectPubKey, guild, false, 24 * 3600, certs[0]);
    if (status != ER_OK) {
        return status;
    }

    return saProxy.InstallMembership(certs, 1);
}

QStatus PermissionMgmtTestHelper::InstallMembershipChain(BusAttachment& topBus, BusAttachment& secondBus, const String& serial0, const String& serial1, const qcc::String& remoteObjName, const qcc::String& secondSubject, const ECCPublicKey* secondPubKey, const qcc::String& targetSubject, const ECCPublicKey* targetPubKey, const qcc::GUID128& guild)
{
    SecurityApplicationProxy saSecondProxy(secondBus, remoteObjName.c_str());

    /* create the second cert first -- with delegate on  */
    qcc::MembershipCertificate certs[2];
    QStatus status = CreateMembershipCert(serial1, topBus, secondSubject, secondPubKey, guild, true, 24 * 3600, certs[1]);
    if (status != ER_OK) {
        return status;
    }

    /* create the leaf cert signed by the subject */
    status = CreateMembershipCert(serial0, secondBus, targetSubject, targetPubKey, guild, false, 24 * 3600, certs[0]);
    if (status != ER_OK) {
        return status;
    }

    /* install cert chain */
    return saSecondProxy.InstallMembership(certs, 2);
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
    QStatus status = remoteObj.GetAllProperties(BasePermissionMgmtTest::TV_IFC_NAME, mapVal);
    if (ER_OK != status) {
        return status;
    }
    MsgArg* propArg;
    return mapVal.GetElement("{sv}", "Caption", &propArg);
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
    QCC_UNUSED(ifcName);
    if (0 == strcmp("Volume", propName)) {
        val.typeId = ALLJOYN_UINT32;
        val.v_uint32 = volume;
        return ER_OK;
    } else if (0 == strcmp("Caption", propName)) {
        val.typeId = ALLJOYN_BYTE;
        val.v_byte = 45;
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}

QStatus BasePermissionMgmtTest::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    QCC_UNUSED(ifcName);
    if ((0 == strcmp("Volume", propName)) && (val.typeId == ALLJOYN_UINT32)) {
        volume = val.v_uint32;
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}
