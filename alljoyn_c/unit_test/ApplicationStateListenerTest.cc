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
#include <alljoyn/ApplicationStateListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn_c/ApplicationStateListener.h>
#include <qcc/Util.h>
#include "ajTestCommon.h"
#include "InMemoryKeyStore.h"

using namespace ajn;

class ApplicationStateListenerTest : public testing::Test {

  public:

    AJ_PCSTR someValidBusName;
    AJ_PSTR contextBusName;
    qcc::KeyInfoNISTP256 someValidKey;
    PermissionConfigurator::ApplicationState someValidApplicationState;
    alljoyn_applicationstatelistener listener;
    alljoyn_applicationstatelistener_callbacks callbacksWithNullStateCallback;
    alljoyn_applicationstatelistener_callbacks nonNullCallbacks;
    alljoyn_applicationstatelistener_callbacks callbacksPassingPublicKeyToContext;
    alljoyn_applicationstatelistener_callbacks callbacksPassingApplicationStateToContext;
    alljoyn_applicationstatelistener_callbacks callbacksPassingBusNameToContext;

    ApplicationStateListenerTest() :
        someValidBusName("someBusName"),
        contextBusName(nullptr),
        someValidApplicationState(PermissionConfigurator::CLAIMED),
        listener(nullptr)
    {
        setCallback(callbacksWithNullStateCallback, nullptr);
        setCallback(nonNullCallbacks, someCallback);
        setCallback(callbacksPassingPublicKeyToContext, passKeyToContextCallback);
        setCallback(callbacksPassingApplicationStateToContext, passApplicationStateToContextCallback);
        setCallback(callbacksPassingBusNameToContext, passBusNameToContextCallback);
    }

    virtual void SetUp()
    {
        InMemoryKeyStoreListener tempKeyStore;
        BusAttachment tempBus("tempBus");

        ASSERT_EQ(ER_OK, tempBus.RegisterKeyStoreListener(tempKeyStore));
        ASSERT_EQ(ER_OK, tempBus.Start());
        ASSERT_EQ(ER_OK, tempBus.Connect(getConnectArg().c_str()));
        ASSERT_EQ(ER_OK, tempBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", nullptr));

        ASSERT_EQ(ER_OK, tempBus.GetPermissionConfigurator().GetSigningPublicKey(someValidKey));

        ASSERT_EQ(ER_OK, tempBus.Stop());
        ASSERT_EQ(ER_OK, tempBus.Join());
    }

    virtual void TearDown()
    {
        delete[] contextBusName;

        if (nullptr != listener) {
            alljoyn_applicationstatelistener_destroy(listener);
        }
    }

  private:

    static void AJ_CALL someCallback(AJ_PCSTR busName,
                                     AJ_PCSTR publicKey,
                                     alljoyn_applicationstate applicationState,
                                     void* context)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(publicKey);
        QCC_UNUSED(applicationState);
        QCC_UNUSED(context);
    }

    static void AJ_CALL passKeyToContextCallback(AJ_PCSTR busName,
                                                 AJ_PCSTR publicKey,
                                                 alljoyn_applicationstate applicationState,
                                                 void* context)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(applicationState);

        ASSERT_NE(nullptr, publicKey);
        ASSERT_NE(nullptr, context);

        qcc::ECCPublicKey eccPublicKey;
        ASSERT_EQ(ER_OK, qcc::CertificateX509::DecodePublicKeyPEM(qcc::String(publicKey), &eccPublicKey));
        *((qcc::ECCPublicKey*)context) = eccPublicKey;
    }

    static void AJ_CALL passApplicationStateToContextCallback(AJ_PCSTR busName,
                                                              AJ_PCSTR publicKey,
                                                              alljoyn_applicationstate applicationState,
                                                              void* context)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(publicKey);

        ASSERT_NE(nullptr, context);

        *((alljoyn_applicationstate*)context) = applicationState;
    }

    static void AJ_CALL passBusNameToContextCallback(AJ_PCSTR busName,
                                                     AJ_PCSTR publicKey,
                                                     alljoyn_applicationstate applicationState,
                                                     void* context)
    {
        QCC_UNUSED(publicKey);
        QCC_UNUSED(applicationState);

        ASSERT_NE(nullptr, busName);
        ASSERT_NE(nullptr, context);

        size_t busNameSize = strlen(busName) + 1;
        AJ_PSTR* contextBusNamePointer = (AJ_PSTR*)context;

        *contextBusNamePointer = new(std::nothrow) char[busNameSize];
        ASSERT_NE(nullptr, *contextBusNamePointer);

        strcpy(*contextBusNamePointer, busName);
    }

    void setCallback(alljoyn_applicationstatelistener_callbacks& callback, alljoyn_applicationstatelistener_state_ptr state)
    {
        memset(&callback, 0, sizeof(callback));
        callback.state = state;
    }
};

TEST_F(ApplicationStateListenerTest, shouldCreateListenerWithCallbacksAndNullContext)
{
    EXPECT_NE(nullptr, alljoyn_applicationstatelistener_create(&nonNullCallbacks, nullptr));
}

TEST_F(ApplicationStateListenerTest, shouldCreateListenerWithCallbacksAndNonNullContext)
{
    EXPECT_NE(nullptr, alljoyn_applicationstatelistener_create(&nonNullCallbacks, this));
}

TEST_F(ApplicationStateListenerTest, shouldDestroyNullListenerWithoutException)
{
    alljoyn_applicationstatelistener_destroy(nullptr);
}

TEST_F(ApplicationStateListenerTest, shouldDestroyNonNullListenerWithoutException)
{
    listener = alljoyn_applicationstatelistener_create(&nonNullCallbacks, nullptr);

    alljoyn_applicationstatelistener_destroy(listener);
    listener = nullptr;
}

TEST_F(ApplicationStateListenerTest, shouldPassBusNameToCallback)
{
    contextBusName = nullptr;
    AJ_PCSTR passedBusName = someValidBusName;
    listener = alljoyn_applicationstatelistener_create(&callbacksPassingBusNameToContext, &contextBusName);

    ((ajn::ApplicationStateListener*)listener)->State(passedBusName, someValidKey, someValidApplicationState);

    EXPECT_STRCASEEQ(passedBusName, contextBusName);
}

TEST_F(ApplicationStateListenerTest, shouldPassPublicKeyToCallback)
{
    qcc::ECCPublicKey contextPublicKey;
    qcc::KeyInfoNISTP256 passedKeyInfo = someValidKey;
    listener = alljoyn_applicationstatelistener_create(&callbacksPassingPublicKeyToContext, &contextPublicKey);

    ((ajn::ApplicationStateListener*)listener)->State(someValidBusName, passedKeyInfo, someValidApplicationState);

    EXPECT_EQ(*(passedKeyInfo.GetPublicKey()), contextPublicKey);
}

TEST_F(ApplicationStateListenerTest, shouldPassApplicationStateToCallback)
{
    alljoyn_applicationstate contextApplicationState;
    PermissionConfigurator::ApplicationState passedApplicationState = someValidApplicationState;
    listener = alljoyn_applicationstatelistener_create(&callbacksPassingApplicationStateToContext, &contextApplicationState);

    ((ajn::ApplicationStateListener*)listener)->State(someValidBusName, someValidKey, passedApplicationState);

    EXPECT_EQ((alljoyn_applicationstate)passedApplicationState, contextApplicationState);
}