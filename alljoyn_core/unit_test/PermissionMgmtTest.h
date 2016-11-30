/**
 * @file
 * Helper functions for the PermissionMgmt Test Cases
 */

/******************************************************************************
 * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

#ifndef _ALLJOYN_PERMISSION_MGMT_TEST_H
#define _ALLJOYN_PERMISSION_MGMT_TEST_H

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
#include <alljoyn/SecurityApplicationProxy.h>
#include <alljoyn/PermissionPolicy.h>
#include "KeyStore.h"
#include "InMemoryKeyStore.h"

namespace ajn {

class TestApplicationStateListener : public ApplicationStateListener {
  public:
    TestApplicationStateListener() : signalApplicationStateReceived(false) { }
    void State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state);
    bool signalApplicationStateReceived;
};

class TestPermissionConfigurationListener : public PermissionConfigurationListener {
  public:
    TestPermissionConfigurationListener() : factoryResetReceived(false), policyChangedReceived(false) { }
    QStatus FactoryReset();
    void PolicyChanged();
    bool factoryResetReceived;
    bool policyChangedReceived;
};

class TestPortListener : public SessionPortListener {
  public:
    qcc::String lastJoiner;
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(id);
        lastJoiner = joiner;
    }
};

class BasePermissionMgmtTest : public testing::Test, public BusObject,
    public ProxyBusObject::PropertiesChangedListener {
  public:


    static const char* INTERFACE_NAME;
    static const char* NOTIFY_INTERFACE_NAME;
    static const char* ONOFF_IFC_NAME;
    static const char* TV_IFC_NAME;
    static const char* ADMIN_BUS_NAME;
    static const char* SERVICE_BUS_NAME;
    static const char* CONSUMER_BUS_NAME;
    static const char* RC_BUS_NAME;

    BasePermissionMgmtTest(const char* path) : BusObject(path),
        adminBus(ADMIN_BUS_NAME, false),
        serviceBus(SERVICE_BUS_NAME, false),
        servicePort(0),
        consumerBus(CONSUMER_BUS_NAME, false),
        remoteControlBus(RC_BUS_NAME, false),
        adminAdminGroupGUID("00112233445566778899AABBCCDDEEFF"),
        consumerAdminGroupGUID("AABBCCDDEEFF00112233445566778899"),
        serviceGUID(),
        consumerGUID(),
        remoteControlGUID(),
        status(ER_OK),
        serviceKeyListener(nullptr),
        adminKeyListener(nullptr),
        consumerKeyListener(nullptr),
        remoteControlKeyListener(nullptr),
        canTestStateSignalReception(false),
        currentTVChannel(1),
        volume(1),
        channelChangedSignalReceived(false),
        propertiesChangedSignalReceived(false),
        testASL(),
        testPCL()
    {
        adminBus.DeleteDefaultKeyStore(ADMIN_BUS_NAME);
        serviceBus.DeleteDefaultKeyStore(SERVICE_BUS_NAME);
        consumerBus.DeleteDefaultKeyStore(CONSUMER_BUS_NAME);
        remoteControlBus.DeleteDefaultKeyStore(RC_BUS_NAME);
    }

    virtual void SetUp();

    virtual void TearDown();

    virtual void PropertiesChanged(ProxyBusObject& obj, const char* ifaceName, const MsgArg& changed, const MsgArg& invalidated, void* context);

    void EnableSecurity(const char* keyExchange);
    bool GetFactoryResetReceived() const;
    void SetFactoryResetReceived(bool flag);
    bool GetPolicyChangedReceived() const;
    void SetPolicyChangedReceived(bool flag);

    void CreateOnOffAppInterface(BusAttachment& bus, bool addService);
    void CreateTVAppInterface(BusAttachment& bus, bool addService);
    void CreateAppInterfaces(BusAttachment& bus, bool addService);
    void GenerateCAKeys();

    BusAttachment adminBus;
    BusAttachment serviceBus;
    SessionPort servicePort;
    TestPortListener servicePortListener;
    BusAttachment consumerBus;
    BusAttachment remoteControlBus;
    qcc::GUID128 adminAdminGroupGUID;
    qcc::GUID128 consumerAdminGroupGUID;
    qcc::GUID128 serviceGUID;
    qcc::GUID128 consumerGUID;
    qcc::GUID128 remoteControlGUID;
    qcc::KeyInfoNISTP256 adminAdminGroupAuthority;
    qcc::KeyInfoNISTP256 consumerAdminGroupAuthority;
    QStatus status;

    QStatus InterestInChannelChangedSignal(BusAttachment* bus);
    void ChannelChangedSignalHandler(const InterfaceDescription::Member* member,
                                     const char* sourcePath, Message& msg);
    void SetApplicationStateSignalReceived(bool flag);
    bool GetApplicationStateSignalReceived() const;
    void SetChannelChangedSignalReceived(bool flag);
    bool GetChannelChangedSignalReceived() const;
    void SetPropertiesChangedSignalReceived(bool flag);
    bool GetPropertiesChangedSignalReceived() const;

    void OnOffOn(const InterfaceDescription::Member* member, Message& msg);
    void OnOffOff(const InterfaceDescription::Member* member, Message& msg);
    void TVUp(const InterfaceDescription::Member* member, Message& msg);

    void TVDown(const InterfaceDescription::Member* member, Message& msg);
    void TVChannel(const InterfaceDescription::Member* member, Message& msg);
    typedef enum {
        SEND_SIGNAL_UNICAST = 0,
        SEND_SIGNAL_SESSIONCAST = 1,
        SEND_SIGNAL_BROADCAST = 2
    } SignalSendMethod;
    void TVChannelChanged(const InterfaceDescription::Member* member, Message& msg, SignalSendMethod sendMethod);
    void TVMute(const InterfaceDescription::Member* member, Message& msg);
    void TVInputSource(const InterfaceDescription::Member* member, Message& msg);
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    QStatus Set(const char* ifcName, const char* propName, MsgArg& val);
    const qcc::String& GetAuthMechanisms() const;

    AuthListener* serviceKeyListener;
    AuthListener* adminKeyListener;
    AuthListener* consumerKeyListener;
    AuthListener* remoteControlKeyListener;
    InMemoryKeyStoreListener adminKeyStoreListener;
    InMemoryKeyStoreListener serviceKeyStoreListener;
    InMemoryKeyStoreListener consumerKeyStoreListener;
    InMemoryKeyStoreListener remoteControlKeyStoreListener;
    void DetermineStateSignalReachable();
    bool canTestStateSignalReception;
    QStatus SetupBus(BusAttachment& bus);
    QStatus TeardownBus(BusAttachment& bus);
    QStatus JoinSessionWithService(BusAttachment& initiator, SessionId& sessionId);

  private:
    void RegisterKeyStoreListeners();

    uint32_t currentTVChannel;
    uint32_t volume;
    bool channelChangedSignalReceived;
    bool propertiesChangedSignalReceived;
    qcc::String authMechanisms;
    TestApplicationStateListener testASL;
    TestPermissionConfigurationListener testPCL;
};

class PermissionMgmtTestHelper {
  public:
    static QStatus ExerciseOn(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExerciseOff(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExerciseTVUp(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExerciseTVDown(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExerciseTVChannel(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExerciseTVMute(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus ExerciseTVInputSource(BusAttachment& bus, ProxyBusObject& remoteObj);
    static QStatus GetTVVolume(BusAttachment& bus, ProxyBusObject& remoteObj, uint32_t& volume);
    static QStatus SetTVVolume(BusAttachment& bus, ProxyBusObject& remoteObj, uint32_t volume);
    static QStatus GetTVCaption(BusAttachment& bus, ProxyBusObject& remoteObj, size_t& propertyCount);
};

}
#endif