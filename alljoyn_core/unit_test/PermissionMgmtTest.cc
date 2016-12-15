/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
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
#include <alljoyn/PermissionConfigurationListener.h>
#include "ajTestCommon.h"
#include "KeyInfoHelper.h"
#include "CredentialAccessor.h"
#include "PermissionMgmtTest.h"
#include "BusInternal.h"
#include "SecurityTestHelper.h"
#include <vector>
#include <string>

using namespace ajn;
using namespace qcc;
using namespace std;

const char* BasePermissionMgmtTest::INTERFACE_NAME = "org.allseen.Security.PermissionMgmt";
const char* BasePermissionMgmtTest::ONOFF_IFC_NAME = "org.allseenalliance.control.OnOff";
const char* BasePermissionMgmtTest::TV_IFC_NAME = "org.allseenalliance.control.TV";
const char* BasePermissionMgmtTest::ADMIN_BUS_NAME = "PermissionMgmtTestAdmin";
const char* BasePermissionMgmtTest::SERVICE_BUS_NAME = "PermissionMgmtTestService";
const char* BasePermissionMgmtTest::CONSUMER_BUS_NAME = "PermissionMgmtTestConsumer";
const char* BasePermissionMgmtTest::RC_BUS_NAME = "PermissionMgmtTestRemoteControl";

void TestApplicationStateListener::State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state)
{
    QCC_UNUSED(busName);
    QCC_UNUSED(publicKeyInfo);
    QCC_UNUSED(state);
    signalApplicationStateReceived = true;
}

QStatus TestPermissionConfigurationListener::FactoryReset()
{
    factoryResetReceived = true;
    return ER_OK;
}

void TestPermissionConfigurationListener::PolicyChanged()
{
    policyChangedReceived = true;
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
    EXPECT_EQ(ER_OK, pc.GetSigningPublicKey(keyInfo));
}

void BasePermissionMgmtTest::SetUp()
{
    status = SetupBus(adminBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    SessionOpts opts;
    status = serviceBus.BindSessionPort(servicePort, opts, servicePortListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(consumerBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = SetupBus(remoteControlBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    RegisterKeyStoreListeners();

    EXPECT_EQ(ER_OK, adminBus.RegisterApplicationStateListener(testASL));
}

void BasePermissionMgmtTest::TearDown()
{
    status = TeardownBus(adminBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.UnbindSessionPort(servicePort);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = TeardownBus(serviceBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = TeardownBus(consumerBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = TeardownBus(remoteControlBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    delete serviceKeyListener;
    serviceKeyListener = nullptr;
    delete adminKeyListener;
    adminKeyListener = nullptr;
    delete consumerKeyListener;
    consumerKeyListener = nullptr;
    delete remoteControlKeyListener;
    remoteControlKeyListener = nullptr;
}

void BasePermissionMgmtTest::PropertiesChanged(ProxyBusObject& obj, const char* ifaceName, const MsgArg& changed, const MsgArg& invalidated, void* context)
{
    QCC_UNUSED(obj);
    QCC_UNUSED(ifaceName);
    QCC_UNUSED(changed);
    QCC_UNUSED(changed);
    QCC_UNUSED(invalidated);
    QCC_UNUSED(context);
    propertiesChangedSignalReceived = true;
}

void BasePermissionMgmtTest::GenerateCAKeys()
{
    KeyInfoNISTP256 keyInfo;
    EXPECT_EQ(ER_OK, adminBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo));
    EXPECT_EQ(ER_OK, consumerBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo));
    EXPECT_EQ(ER_OK, serviceBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo));
    EXPECT_EQ(ER_OK, remoteControlBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo));
    GenerateSecurityGroupKey(adminBus, adminAdminGroupAuthority);
    GenerateSecurityGroupKey(consumerBus, consumerAdminGroupAuthority);
}

static AuthListener* GenAuthListener(const char* keyExchange) {
    if (strstr(keyExchange, "ECDHE_PSK")) {
        qcc::String psk("38347892FFBEF5B2442AEDE9E53C4B32");
        DefaultECDHEAuthListener* authListener = new DefaultECDHEAuthListener();
        SecurityTestHelper::CallDeprecatedSetPSK(authListener, reinterpret_cast<const uint8_t*>(psk.data()), psk.size());
        return authListener;
    }
    return new DefaultECDHEAuthListener();
}

void BasePermissionMgmtTest::EnableSecurity(const char* keyExchange)
{
    if (strstr(keyExchange, "ECDHE_PSK")) {
    }
    delete adminKeyListener;
    adminKeyListener = GenAuthListener(keyExchange);
    adminBus.EnablePeerSecurity(keyExchange, adminKeyListener, nullptr, true);
    delete serviceKeyListener;
    serviceKeyListener = GenAuthListener(keyExchange);
    serviceBus.EnablePeerSecurity(keyExchange, serviceKeyListener, nullptr, false, &testPCL);
    delete consumerKeyListener;
    consumerKeyListener = GenAuthListener(keyExchange);
    consumerBus.EnablePeerSecurity(keyExchange, consumerKeyListener, nullptr, false, &testPCL);
    delete remoteControlKeyListener;
    remoteControlKeyListener = GenAuthListener(keyExchange);
    remoteControlBus.EnablePeerSecurity(keyExchange, remoteControlKeyListener, nullptr, false, &testPCL);
    authMechanisms = keyExchange;
}

const qcc::String& BasePermissionMgmtTest::GetAuthMechanisms() const
{
    return authMechanisms;
}

void BasePermissionMgmtTest::CreateOnOffAppInterface(BusAttachment& bus, bool addService)
{
    /* create/activate alljoyn_interface */
    InterfaceDescription* ifc = nullptr;
    QStatus status = bus.CreateInterface(BasePermissionMgmtTest::ONOFF_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(ifc != nullptr);
    if (ifc != nullptr) {
        status = ifc->AddMember(MESSAGE_METHOD_CALL, "On", nullptr, nullptr, nullptr);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = ifc->AddMember(MESSAGE_METHOD_CALL, "Off", nullptr, nullptr, nullptr);
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
    InterfaceDescription* ifc = nullptr;
    QStatus status = bus.CreateInterface(BasePermissionMgmtTest::TV_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(ifc != nullptr);
    if (ifc != nullptr) {
        status = ifc->AddMember(MESSAGE_METHOD_CALL, "Up", nullptr, nullptr, nullptr);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = ifc->AddMember(MESSAGE_METHOD_CALL, "Down", nullptr, nullptr, nullptr);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = ifc->AddMember(MESSAGE_METHOD_CALL, "Channel", nullptr, nullptr, nullptr);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = ifc->AddMember(MESSAGE_METHOD_CALL, "Mute", nullptr, nullptr, nullptr);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = ifc->AddMember(MESSAGE_METHOD_CALL, "InputSource", nullptr, nullptr, nullptr);
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
        EXPECT_EQ(ER_OK, ifc->AddPropertyAnnotation("Volume", ajn::org::freedesktop::DBus::AnnotateEmitsChanged, "true"));
        status = ifc->AddProperty("Caption", "y", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(ER_OK, ifc->AddPropertyAnnotation("Caption", ajn::org::freedesktop::DBus::AnnotateEmitsChanged, "true"));

        ifc->Activate();
        status = bus.RegisterSignalHandler(this,
                                           static_cast<MessageReceiver::SignalHandler>(&BasePermissionMgmtTest::ChannelChangedSignalHandler), ifc->GetMember("ChannelChanged"), nullptr);
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

bool BasePermissionMgmtTest::GetApplicationStateSignalReceived() const
{
    return testASL.signalApplicationStateReceived;
}

void BasePermissionMgmtTest::SetFactoryResetReceived(bool flag)
{
    testPCL.factoryResetReceived = flag;
}

bool BasePermissionMgmtTest::GetFactoryResetReceived() const
{
    return testPCL.factoryResetReceived;
}

void BasePermissionMgmtTest::SetPolicyChangedReceived(bool flag)
{
    testPCL.policyChangedReceived = flag;
}

bool BasePermissionMgmtTest::GetPolicyChangedReceived() const
{
    return testPCL.policyChangedReceived;
}

void BasePermissionMgmtTest::SetChannelChangedSignalReceived(bool flag)
{
    channelChangedSignalReceived = flag;
}

bool BasePermissionMgmtTest::GetChannelChangedSignalReceived() const
{
    return channelChangedSignalReceived;
}

void BasePermissionMgmtTest::SetPropertiesChangedSignalReceived(bool flag)
{
    propertiesChangedSignalReceived = flag;
}

bool BasePermissionMgmtTest::GetPropertiesChangedSignalReceived() const
{
    return propertiesChangedSignalReceived;
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
    TVChannelChanged(member, msg, SEND_SIGNAL_SESSIONCAST);
}

void BasePermissionMgmtTest::TVDown(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    if (currentTVChannel > 1) {
        currentTVChannel--;
    }
    MethodReply(msg, ER_OK);
    TVChannelChanged(member, msg, SEND_SIGNAL_BROADCAST);
}

void BasePermissionMgmtTest::TVChannel(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    MethodReply(msg, ER_OK);
    /* emit a signal */
    TVChannelChanged(member, msg, SEND_SIGNAL_UNICAST);
}

void BasePermissionMgmtTest::TVChannelChanged(const InterfaceDescription::Member* member, Message& msg, SignalSendMethod sendMethod)
{
    QCC_UNUSED(msg);
    /* emit a signal */
    MsgArg args[1];
    args[0].Set("u", currentTVChannel);
    if (sendMethod == SEND_SIGNAL_SESSIONCAST) {
        Signal(nullptr, SESSION_ID_ALL_HOSTED, *member->iface->GetMember("ChannelChanged"), args, 1, 0, 0);
    } else if (sendMethod == SEND_SIGNAL_BROADCAST) {
        /* sending a broadcast signal */
        Signal(nullptr, 0, *member->iface->GetMember("ChannelChanged"), args, 1, 0, 0);
    } else {
        Signal(consumerBus.GetUniqueName().c_str(), 0, *member->iface->GetMember("ChannelChanged"), args, 1, 0, 0);
        Signal(remoteControlBus.GetUniqueName().c_str(), 0, *member->iface->GetMember("ChannelChanged"), args, 1, 0, 0);
    }
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
    /* sleep to see whether the ApplicationState signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetApplicationStateSignalReceived()) {
            break;
        }
        qcc::Sleep(WAIT_TIME_10);
    }
    canTestStateSignalReception = GetApplicationStateSignalReceived();
    SetApplicationStateSignalReceived(false);
}

QStatus PermissionMgmtTestHelper::ExerciseOn(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::ONOFF_IFC_NAME, "On", nullptr, 0, reply, METHOD_CALL_TIMEOUT);
    if (ER_OK != status) {
        if (SecurityTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExerciseOff(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::ONOFF_IFC_NAME, "Off", nullptr, 0, reply, METHOD_CALL_TIMEOUT);
    if (ER_OK != status) {
        if (SecurityTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExerciseTVUp(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "Up", nullptr, 0, reply, METHOD_CALL_TIMEOUT);
    if (ER_OK != status) {
        if (SecurityTestHelper::IsPermissionDeniedError(status, reply)) {
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

QStatus PermissionMgmtTestHelper::GetTVCaption(BusAttachment& bus, ProxyBusObject& remoteObj, size_t& propertyCount)
{
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);

    /* test the GetAllProperites */
    MsgArg mapVal;
    QStatus status = remoteObj.GetAllProperties(BasePermissionMgmtTest::TV_IFC_NAME, mapVal);
    if (ER_OK != status) {
        return status;
    }
    MsgArg args;
    status = mapVal.Get("a{sv}", &propertyCount, &args);
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

QStatus PermissionMgmtTestHelper::ExerciseTVDown(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "Down", nullptr, 0, reply, METHOD_CALL_TIMEOUT);
    if (ER_OK != status) {
        if (SecurityTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExerciseTVChannel(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "Channel", nullptr, 0, reply, METHOD_CALL_TIMEOUT);
    if (ER_OK != status) {
        if (SecurityTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExerciseTVMute(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "Mute", nullptr, 0, reply, METHOD_CALL_TIMEOUT);
    if (ER_OK != status) {
        if (SecurityTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus PermissionMgmtTestHelper::ExerciseTVInputSource(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(BasePermissionMgmtTest::TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(BasePermissionMgmtTest::TV_IFC_NAME, "InputSource", nullptr, 0, reply, METHOD_CALL_TIMEOUT);
    if (ER_OK != status) {
        if (SecurityTestHelper::IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus BasePermissionMgmtTest::JoinSessionWithService(BusAttachment& initiator, SessionId& sessionId)
{
    servicePortListener.lastJoiner = String::Empty;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = initiator.JoinSession(serviceBus.GetUniqueName().c_str(), servicePort, nullptr, sessionId, opts);
    if (ER_OK != status) {
        return status;
    }
    for (uint32_t msecs = 0; msecs < LOOP_END_3000; msecs += WAIT_TIME_100) {
        if (servicePortListener.lastJoiner == initiator.GetUniqueName()) {
            return ER_OK;
        }
        qcc::Sleep(WAIT_TIME_100);
    }
    return ER_TIMEOUT;
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
        if (volume <= 20) {
            EmitPropChanged(BasePermissionMgmtTest::TV_IFC_NAME, "Volume", val, SESSION_ID_ALL_HOSTED, 0);
        } else {
            const char* propNames[1];
            propNames[0] = "Volume";
            EmitPropChanged(BasePermissionMgmtTest::TV_IFC_NAME, propNames, ArraySize(propNames), SESSION_ID_ALL_HOSTED, 0);
        }
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}