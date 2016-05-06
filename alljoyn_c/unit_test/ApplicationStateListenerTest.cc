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
#include <gtest/gtest.h>
#include <alljoyn/ApplicationStateListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn_c/ApplicationStateListener.h>
#include <qcc/Util.h>
#include "ajTestCommon.h"
#include "InMemoryKeyStore.h"

using namespace ajn;
using namespace qcc;

class ApplicationStateListenerTest : public testing::Test {

  public:

    AJ_PCSTR m_someValidBusName;
    AJ_PSTR m_contextBusName;
    KeyInfoNISTP256 m_someValidKey;
    PermissionConfigurator::ApplicationState m_someValidApplicationState;
    alljoyn_applicationstatelistener m_listener;
    alljoyn_applicationstatelistener_callbacks m_callbacksWithNullStateCallback;
    alljoyn_applicationstatelistener_callbacks m_nonNullCallbacks;
    alljoyn_applicationstatelistener_callbacks m_callbacksPassingPublicKeyToContext;
    alljoyn_applicationstatelistener_callbacks m_callbacksPassingApplicationStateToContext;
    alljoyn_applicationstatelistener_callbacks m_callbacksPassingBusNameToContext;

    ApplicationStateListenerTest() :
        m_someValidBusName("someBusName"),
        m_contextBusName(nullptr),
        m_someValidApplicationState(PermissionConfigurator::CLAIMED),
        m_listener(nullptr)
    {
        SetCallback(m_callbacksWithNullStateCallback, nullptr);
        SetCallback(m_nonNullCallbacks, SomeCallback);
        SetCallback(m_callbacksPassingPublicKeyToContext, PassKeyToContextCallback);
        SetCallback(m_callbacksPassingApplicationStateToContext, PassApplicationStateToContextCallback);
        SetCallback(m_callbacksPassingBusNameToContext, PassBusNameToContextCallback);
    }

    virtual void SetUp()
    {
        InMemoryKeyStoreListener tempKeyStore;
        BusAttachment tempBus("tempBus");

        ASSERT_EQ(ER_OK, tempBus.RegisterKeyStoreListener(tempKeyStore));
        ASSERT_EQ(ER_OK, tempBus.Start());
        ASSERT_EQ(ER_OK, tempBus.Connect(getConnectArg().c_str()));
        ASSERT_EQ(ER_OK, tempBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", nullptr));

        ASSERT_EQ(ER_OK, tempBus.GetPermissionConfigurator().GetSigningPublicKey(m_someValidKey));

        ASSERT_EQ(ER_OK, tempBus.Stop());
        ASSERT_EQ(ER_OK, tempBus.Join());
    }

    virtual void TearDown()
    {
        delete[] m_contextBusName;

        if (nullptr != m_listener) {
            alljoyn_applicationstatelistener_destroy(m_listener);
        }
    }

  private:

    static void AJ_CALL SomeCallback(AJ_PCSTR busName,
                                     AJ_PCSTR publicKey,
                                     alljoyn_applicationstate applicationState,
                                     void* context)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(publicKey);
        QCC_UNUSED(applicationState);
        QCC_UNUSED(context);
    }

    static void AJ_CALL PassKeyToContextCallback(AJ_PCSTR busName,
                                                 AJ_PCSTR publicKey,
                                                 alljoyn_applicationstate applicationState,
                                                 void* context)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(applicationState);

        ASSERT_NE(nullptr, publicKey);
        ASSERT_NE(nullptr, context);

        ECCPublicKey eccPublicKey;
        ASSERT_EQ(ER_OK, CertificateX509::DecodePublicKeyPEM(String(publicKey), &eccPublicKey));
        *((ECCPublicKey*)context) = eccPublicKey;
    }

    static void AJ_CALL PassApplicationStateToContextCallback(AJ_PCSTR busName,
                                                              AJ_PCSTR publicKey,
                                                              alljoyn_applicationstate applicationState,
                                                              void* context)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(publicKey);

        ASSERT_NE(nullptr, context);

        *((alljoyn_applicationstate*)context) = applicationState;
    }

    static void AJ_CALL PassBusNameToContextCallback(AJ_PCSTR busName,
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

    void SetCallback(alljoyn_applicationstatelistener_callbacks& callback, alljoyn_applicationstatelistener_state_ptr state)
    {
        memset(&callback, 0, sizeof(callback));
        callback.state = state;
    }
};

TEST_F(ApplicationStateListenerTest, shouldCreateListenerWithCallbacksAndNullContext)
{
    EXPECT_NE(nullptr, alljoyn_applicationstatelistener_create(&m_nonNullCallbacks, nullptr));
}

TEST_F(ApplicationStateListenerTest, shouldCreateListenerWithCallbacksAndNonNullContext)
{
    EXPECT_NE(nullptr, alljoyn_applicationstatelistener_create(&m_nonNullCallbacks, this));
}

TEST_F(ApplicationStateListenerTest, shouldDestroyNullListenerWithoutException)
{
    alljoyn_applicationstatelistener_destroy(nullptr);
}

TEST_F(ApplicationStateListenerTest, shouldDestroyNonNullListenerWithoutException)
{
    m_listener = alljoyn_applicationstatelistener_create(&m_nonNullCallbacks, nullptr);

    alljoyn_applicationstatelistener_destroy(m_listener);
    m_listener = nullptr;
}

TEST_F(ApplicationStateListenerTest, shouldPassBusNameToCallback)
{
    m_contextBusName = nullptr;
    AJ_PCSTR passedBusName = m_someValidBusName;
    m_listener = alljoyn_applicationstatelistener_create(&m_callbacksPassingBusNameToContext, &m_contextBusName);

    ((ApplicationStateListener*)m_listener)->State(passedBusName, m_someValidKey, m_someValidApplicationState);

    EXPECT_STRCASEEQ(passedBusName, m_contextBusName);
}

TEST_F(ApplicationStateListenerTest, shouldPassPublicKeyToCallback)
{
    ECCPublicKey contextPublicKey;
    KeyInfoNISTP256 passedKeyInfo = m_someValidKey;
    m_listener = alljoyn_applicationstatelistener_create(&m_callbacksPassingPublicKeyToContext, &contextPublicKey);

    ((ApplicationStateListener*)m_listener)->State(m_someValidBusName, passedKeyInfo, m_someValidApplicationState);

    EXPECT_EQ(*(passedKeyInfo.GetPublicKey()), contextPublicKey);
}

TEST_F(ApplicationStateListenerTest, shouldPassApplicationStateToCallback)
{
    alljoyn_applicationstate contextApplicationState;
    PermissionConfigurator::ApplicationState passedApplicationState = m_someValidApplicationState;
    m_listener = alljoyn_applicationstatelistener_create(&m_callbacksPassingApplicationStateToContext, &contextApplicationState);

    ((ApplicationStateListener*)m_listener)->State(m_someValidBusName, m_someValidKey, passedApplicationState);

    EXPECT_EQ((alljoyn_applicationstate)passedApplicationState, contextApplicationState);
}
