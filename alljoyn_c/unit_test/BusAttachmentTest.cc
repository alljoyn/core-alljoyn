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
#include <gtest/gtest.h>
#include <time.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn_c/ApplicationStateListener.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/InterfaceDescription.h>
#include <alljoyn_c/ProxyBusObject.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#if defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/NamedPipeWrapper.h>
#endif
#include "ajTestCommon.h"
#include "InMemoryKeyStore.h"

/*
 * The unit test uses a busy wait loop. The busy wait loops were chosen
 * over thread sleeps because of the ease of understanding the busy wait loops.
 * Also busy wait loops do not require any platform specific threading code.
 */
#define WAIT_MSECS 5
#define STATE_CHANGE_TIMEOUT_MS 2000

static const char s_busAttachmentTestName[] = "BusAttachmentTest";
static const char s_otherBusAttachmentTestName[] = "BusAttachment OtherBus";

class BusAttachmentSecurity20Test : public testing::Test {
  public:

    BusAttachmentSecurity20Test() :
        securityAgent((alljoyn_busattachment) & privateSecurityAgentBus),
        privateSecurityAgentBus("SecurityAgentBus"),
        managedApp("SampleManagedApp")
    {
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.state = stateCallback;
    };

    virtual void SetUp()
    {
        basicBusSetup(privateSecurityAgentBus, securityAgentKeyStoreListener);
        basicBusSetup(managedApp, managedAppKeyStoreListener);
        setupAgent();
    }

    virtual void TearDown()
    {
        appTearDown(privateSecurityAgentBus);
        appTearDown(managedApp);
    }

  protected:

    alljoyn_busattachment securityAgent;

    void createApplicationStateListener(alljoyn_applicationstatelistener* listener, bool* listenerCalled = nullptr)
    {
        *listener = alljoyn_applicationstatelistener_create(&callbacks, listenerCalled);
        ASSERT_NE(nullptr, listener);
    }

    void changeApplicationState()
    {
        managedApp.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", nullptr);
        SetManifestTemplate(managedApp);
    }

    bool waitForTrueOrTimeout(bool* isTrue, int timeoutMs)
    {
        time_t startTime = time(nullptr);
        int timeDiffMs = difftime(time(nullptr), startTime) * 1000;
        while (!(*isTrue)
               && (timeDiffMs < timeoutMs)) {
            qcc::Sleep(WAIT_MSECS);
            timeDiffMs = difftime(time(nullptr), startTime) * 1000;
        }

        return *isTrue;
    }

  private:

    ajn::BusAttachment privateSecurityAgentBus;
    ajn::BusAttachment managedApp;
    alljoyn_applicationstatelistener_callbacks callbacks;
    ajn::InMemoryKeyStoreListener securityAgentKeyStoreListener;
    ajn::InMemoryKeyStoreListener managedAppKeyStoreListener;

    static void AJ_CALL stateCallback(AJ_PCSTR busName,
                                      AJ_PCSTR publicKey,
                                      alljoyn_applicationstate applicationState,
                                      void* listenerCalled)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(publicKey);
        QCC_UNUSED(applicationState);

        if (listenerCalled) {
            *((bool*)listenerCalled) = true;
        }
    }

    void setupAgent()
    {
        ASSERT_EQ(ER_OK, privateSecurityAgentBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", nullptr));
    }

    void basicBusSetup(ajn::BusAttachment& bus, ajn::InMemoryKeyStoreListener& keyStoreListener)
    {
        ASSERT_EQ(ER_OK, bus.RegisterKeyStoreListener(keyStoreListener));
        ASSERT_EQ(ER_OK, bus.Start());
        ASSERT_EQ(ER_OK, bus.Connect(ajn::getConnectArg().c_str()));
    }

    void appTearDown(ajn::BusAttachment& bus)
    {
        bus.UnregisterKeyStoreListener();
        bus.Stop();
        bus.Join();
    }

    void SetManifestTemplate(ajn::BusAttachment& bus)
    {
        ajn::PermissionPolicy::Rule manifestTemplate[1];
        ajn::PermissionPolicy::Rule::Member member[1];
        member[0].Set("*",
                      ajn::PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                      ajn::PermissionPolicy::Rule::Member::ACTION_PROVIDE
                      | ajn::PermissionPolicy::Rule::Member::ACTION_MODIFY
                      | ajn::PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        manifestTemplate[0].SetObjPath("*");
        manifestTemplate[0].SetInterfaceName("*");
        manifestTemplate[0].SetMembers(1, member);
        EXPECT_EQ(ER_OK, bus.GetPermissionConfigurator().SetPermissionManifest(manifestTemplate, 1));
    }
};

TEST_F(BusAttachmentSecurity20Test, shouldReturnNonNullPermissionConfigurator)
{
    EXPECT_NE(alljoyn_busattachment_getpermissionconfigurator(securityAgent), nullptr);
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenRegisteringWithNullListener)
{
    EXPECT_EQ(ER_INVALID_ADDRESS, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, nullptr));
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenUnregisteringWithNullListener)
{
    EXPECT_EQ(ER_INVALID_ADDRESS, alljoyn_busattachment_unregisterapplicationstatelistener(securityAgent, nullptr));
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenUnregisteringUnknownListener)
{
    alljoyn_applicationstatelistener listener = nullptr;
    createApplicationStateListener(&listener);

    EXPECT_EQ(ER_APPLICATION_STATE_LISTENER_NO_SUCH_LISTENER, alljoyn_busattachment_unregisterapplicationstatelistener(securityAgent, listener));
}

TEST_F(BusAttachmentSecurity20Test, shouldRegisterSuccessfullyForNewListener)
{
    alljoyn_applicationstatelistener listener = nullptr;
    createApplicationStateListener(&listener);

    EXPECT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));
}

TEST_F(BusAttachmentSecurity20Test, shouldUnregisterSuccessfullyForSameListener)
{
    alljoyn_applicationstatelistener listener = nullptr;
    createApplicationStateListener(&listener);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));

    EXPECT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(securityAgent, listener));
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenRegisteringSameListenerTwice)
{
    alljoyn_applicationstatelistener listener = nullptr;
    createApplicationStateListener(&listener);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));

    EXPECT_EQ(ER_APPLICATION_STATE_LISTENER_ALREADY_EXISTS, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenUnregisteringSameListenerTwice)
{
    alljoyn_applicationstatelistener listener = nullptr;
    createApplicationStateListener(&listener);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(securityAgent, listener));

    EXPECT_EQ(ER_APPLICATION_STATE_LISTENER_NO_SUCH_LISTENER, alljoyn_busattachment_unregisterapplicationstatelistener(securityAgent, listener));
}

TEST_F(BusAttachmentSecurity20Test, shouldRegisterSameListenerSuccessfullyAfterUnregister)
{
    alljoyn_applicationstatelistener listener = nullptr;
    createApplicationStateListener(&listener);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(securityAgent, listener));

    EXPECT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));
}

TEST_F(BusAttachmentSecurity20Test, shouldCallStateListenerAfterRegister)
{
    bool listenerCalled = false;
    alljoyn_applicationstatelistener listener = nullptr;
    createApplicationStateListener(&listener, &listenerCalled);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));
    changeApplicationState();

    EXPECT_TRUE(waitForTrueOrTimeout(&listenerCalled, STATE_CHANGE_TIMEOUT_MS));
}

TEST_F(BusAttachmentSecurity20Test, shouldNotCallStateListenerAfterUnregister)
{
    bool listenerCalled = false;
    alljoyn_applicationstatelistener listener = nullptr;
    createApplicationStateListener(&listener, &listenerCalled);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, listener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(securityAgent, listener));
    changeApplicationState();

    EXPECT_FALSE(waitForTrueOrTimeout(&listenerCalled, STATE_CHANGE_TIMEOUT_MS));
}

TEST_F(BusAttachmentSecurity20Test, shouldCallAllStateListeners)
{
    bool firstListenerCalled = false;
    bool secondListenerCalled = false;
    alljoyn_applicationstatelistener firstListener = nullptr;
    alljoyn_applicationstatelistener secondListener = nullptr;
    createApplicationStateListener(&firstListener, &firstListenerCalled);
    createApplicationStateListener(&secondListener, &secondListenerCalled);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, firstListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, secondListener));
    changeApplicationState();

    EXPECT_TRUE(waitForTrueOrTimeout(&firstListenerCalled, STATE_CHANGE_TIMEOUT_MS));
    EXPECT_TRUE(waitForTrueOrTimeout(&secondListenerCalled, STATE_CHANGE_TIMEOUT_MS));
}

TEST_F(BusAttachmentSecurity20Test, shouldCallOnlyOneStateListenerWhenOtherUnregistered)
{
    bool firstListenerCalled = false;
    bool secondListenerCalled = false;
    alljoyn_applicationstatelistener firstListener = nullptr;
    alljoyn_applicationstatelistener secondListener = nullptr;
    createApplicationStateListener(&firstListener, &firstListenerCalled);
    createApplicationStateListener(&secondListener, &secondListenerCalled);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, firstListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(securityAgent, secondListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(securityAgent, firstListener));
    changeApplicationState();

    EXPECT_FALSE(waitForTrueOrTimeout(&firstListenerCalled, STATE_CHANGE_TIMEOUT_MS));
    EXPECT_TRUE(waitForTrueOrTimeout(&secondListenerCalled, STATE_CHANGE_TIMEOUT_MS));
}

TEST(BusAttachmentTest, createinterface) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.BusAttachment", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, deleteinterface) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.BusAttachment", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_deleteinterface(bus, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, start_stop_join) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, isstarted_isstopping) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));
    EXPECT_EQ(QCC_FALSE, alljoyn_busattachment_isstarted(bus));
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(QCC_TRUE, alljoyn_busattachment_isstarted(bus));
    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /*
     * Assumption made that the isstopping function will be called before all of
     * the BusAttachement threads have completed so it will return QCC_TRUE it is
     * possible, but unlikely, that this could return QCC_FALSE.
     */

    EXPECT_EQ(QCC_TRUE, alljoyn_busattachment_isstopping(bus));
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(QCC_FALSE, alljoyn_busattachment_isstarted(bus));
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getconcurrency) {
    alljoyn_busattachment bus = NULL;
    unsigned int concurrency = (unsigned int)-1;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

    concurrency = alljoyn_busattachment_getconcurrency(bus);
    //The default value for getconcurrency is 4
    EXPECT_EQ(4u, concurrency) << "  Expected a concurrency of 4 got " << concurrency;

    alljoyn_busattachment_destroy(bus);

    bus = NULL;
    concurrency = (unsigned int)-1;

    bus = alljoyn_busattachment_create_concurrency(s_busAttachmentTestName, QCC_TRUE, 8);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

    concurrency = alljoyn_busattachment_getconcurrency(bus);
    //The default value for getconcurrency is 8
    EXPECT_EQ(8u, concurrency) << "  Expected a concurrency of 8 got " << concurrency;

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, isconnected)
{
    QStatus status;
    alljoyn_busattachment bus;
    size_t i;

    QCC_BOOL allow_remote[2] = { QCC_FALSE, QCC_TRUE };

    for (i = 0; i < ArraySize(allow_remote); i++) {
        status = ER_FAIL;
        bus = NULL;

        bus = alljoyn_busattachment_create(s_busAttachmentTestName, allow_remote[i]);
        EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));

        status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_disconnect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_stop(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_join(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_busattachment_destroy(bus);
    }
}

TEST(BusAttachmentTest, disconnect)
{
    QStatus status;
    alljoyn_busattachment bus;
    size_t i;

    QCC_BOOL allow_remote[2] = { QCC_FALSE, QCC_TRUE };

    for (i = 0; i < ArraySize(allow_remote); i++) {
        status = ER_FAIL;
        bus = NULL;

        bus = alljoyn_busattachment_create(s_busAttachmentTestName, allow_remote[i]);
        EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

        status = alljoyn_busattachment_disconnect(bus, NULL);
        EXPECT_EQ(ER_BUS_BUS_NOT_STARTED, status);

        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));

        status = alljoyn_busattachment_disconnect(bus, NULL);
        EXPECT_EQ(ER_BUS_NOT_CONNECTED, status);

        status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_disconnect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_stop(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_join(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_busattachment_destroy(bus);
    }
}

TEST(BusAttachmentTest, connect_null)
{
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));

    AJ_PCSTR connectspec = alljoyn_busattachment_getconnectspec(bus);

    /**
     * Note: the default connect spec here must match the one in alljoyn_core BusAttachment.
     */
    AJ_PCSTR preferredConnectSpec;

#if defined(QCC_OS_GROUP_WINDOWS)
    if (qcc::NamedPipeWrapper::AreApisAvailable()) {
        preferredConnectSpec = "npipe:";
    } else {
        preferredConnectSpec = "tcp:addr=127.0.0.1,port=9955";
    }
#else
    preferredConnectSpec = "unix:abstract=alljoyn";
#endif

    /*
     * The BusAttachment has joined either a separate daemon (preferredConnectSpec) or
     * it is using the null transport (in bundled router).  If the null transport is used, the
     * connect spec will be 'null:' otherwise it will match the preferred default connect spec.
     */
    EXPECT_TRUE(strcmp(preferredConnectSpec, connectspec) == 0 ||
                strcmp("null:", connectspec) == 0);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getconnectspec)
{
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AJ_PCSTR connectspec = alljoyn_busattachment_getconnectspec(bus);

    /*
     * The BusAttachment has joined either a separate daemon or it is using
     * the in process name service.  If the internal name service is used
     * the connect spec will be 'null:' otherwise it will match the ConnectArg.
     */
    EXPECT_TRUE(strcmp(ajn::getConnectArg().c_str(), connectspec) == 0 ||
                strcmp("null:", connectspec) == 0);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getdbusobject) {
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject dBusProxyObject = alljoyn_busattachment_getdbusproxyobj(bus);

    alljoyn_msgarg msgArgs = alljoyn_msgarg_array_create(2);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(msgArgs, 0), "s", "org.alljoyn.test.BusAttachment");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(msgArgs, 1), "u", 7u);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_message replyMsg = alljoyn_message_create(bus);

    status = alljoyn_proxybusobject_methodcall(dBusProxyObject, "org.freedesktop.DBus", "RequestName", msgArgs, 2, replyMsg, 25000, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    unsigned int requestNameReply;
    alljoyn_msgarg reply = alljoyn_message_getarg(replyMsg, 0);
    alljoyn_msgarg_get(reply, "u", &requestNameReply);

    EXPECT_EQ(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER, requestNameReply);

    alljoyn_msgarg_destroy(msgArgs);
    alljoyn_message_destroy(replyMsg);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, ping_self) {
    QStatus status;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_ping(bus, alljoyn_busattachment_getuniquename(bus), 1000));

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, ping_other_on_same_bus) {
    QStatus status;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment otherbus = NULL;
    otherbus = alljoyn_busattachment_create(s_otherBusAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_otherBusAttachmentTestName));

    status = alljoyn_busattachment_start(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(otherbus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_ping(bus, alljoyn_busattachment_getuniquename(otherbus), 1000));

    status = alljoyn_busattachment_stop(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(otherbus);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

static QCC_BOOL test_alljoyn_authlistener_requestcredentials(const void* context, AJ_PCSTR authMechanism, AJ_PCSTR peerName, uint16_t authCount,
                                                             AJ_PCSTR userName, uint16_t credMask, alljoyn_credentials credentials)
{
    QCC_UNUSED(context);
    QCC_UNUSED(authMechanism);
    QCC_UNUSED(peerName);
    QCC_UNUSED(authCount);
    QCC_UNUSED(userName);
    QCC_UNUSED(credMask);
    QCC_UNUSED(credentials);
    return true;
}

static void test_alljoyn_authlistener_authenticationcomplete(const void* context, AJ_PCSTR authMechanism, AJ_PCSTR peerName, QCC_BOOL success)
{

    QCC_UNUSED(authMechanism);
    QCC_UNUSED(peerName);
    QCC_UNUSED(success);
    if (context) {
        int* count = (int*) context;
        *count += 1;
    }
}


TEST(BusAttachmentTest, BasicSecureConnection)
{
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, alljoyn_busattachment_secureconnection(bus, "busname", false));

    QStatus status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, alljoyn_busattachment_secureconnection(bus, "busname", false));
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(ER_BUS_SECURITY_NOT_ENABLED, alljoyn_busattachment_secureconnection(bus, "busname", false));

    alljoyn_busattachment otherbus = NULL;
    otherbus = alljoyn_busattachment_create(s_otherBusAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_otherBusAttachmentTestName));

    status = alljoyn_busattachment_start(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(otherbus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_authlistener al = NULL;
    alljoyn_authlistener_callbacks cbs;
    cbs.authentication_complete = &test_alljoyn_authlistener_authenticationcomplete;
    cbs.request_credentials = &test_alljoyn_authlistener_requestcredentials;
    cbs.security_violation = NULL;
    cbs.verify_credentials = NULL;
    al = alljoyn_authlistener_create(&cbs, NULL);

    EXPECT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(bus, "ALLJOYN_ECDHE_NULL", al, "myKeyStore", false));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(otherbus, "ALLJOYN_ECDHE_NULL", al, "myOtherKeyStore", false));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_secureconnection(bus, alljoyn_busattachment_getuniquename(otherbus), false));

    status = alljoyn_busattachment_stop(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_clearkeystore(otherbus);
    status = alljoyn_busattachment_join(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(otherbus);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_clearkeystore(bus);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
    alljoyn_authlistener_destroy(al);
}

TEST(BusAttachmentTest, BasicSecureConnectionAsync)
{
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_busAttachmentTestName));
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, alljoyn_busattachment_secureconnectionasync(bus, "busname", false));

    QStatus status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, alljoyn_busattachment_secureconnectionasync(bus, "busname", false));
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(ER_BUS_SECURITY_NOT_ENABLED, alljoyn_busattachment_secureconnectionasync(bus, "busname", false));

    alljoyn_busattachment otherbus = NULL;
    otherbus = alljoyn_busattachment_create(s_otherBusAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(s_otherBusAttachmentTestName));

    status = alljoyn_busattachment_start(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(otherbus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_authlistener al = NULL;
    alljoyn_authlistener_callbacks cbs;
    cbs.authentication_complete = &test_alljoyn_authlistener_authenticationcomplete;
    cbs.request_credentials = &test_alljoyn_authlistener_requestcredentials;
    cbs.security_violation = NULL;
    cbs.verify_credentials = NULL;
    int authCompleteCount = 0;
    al = alljoyn_authlistener_create(&cbs, &authCompleteCount);

    EXPECT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(bus, "ALLJOYN_ECDHE_NULL", al, "myKeyStore", false));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(otherbus, "ALLJOYN_ECDHE_NULL", al, "myOtherKeyStore", false));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_secureconnectionasync(bus, alljoyn_busattachment_getuniquename(otherbus), false));

    int ticks = 0;
    while (authCompleteCount == 0 && (ticks++ < 50)) {
        qcc::Sleep(100);
    }
    EXPECT_NE(0, authCompleteCount);
    status = alljoyn_busattachment_stop(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_clearkeystore(otherbus);
    status = alljoyn_busattachment_join(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(otherbus);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_clearkeystore(bus);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
    alljoyn_authlistener_destroy(al);
}