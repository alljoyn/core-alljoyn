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
#include <alljoyn/PermissionConfigurationListener.h>
#include <alljoyn_c/PermissionConfigurationListener.h>
#include <qcc/Util.h>

using namespace ajn;

class PermissionConfigurationListenerTest : public testing::Test {

  public:

    PermissionConfigurationListenerTest()
    { }

    virtual void SetUp()
    {
        memset(&m_callbacks, 0, sizeof(m_callbacks));
        m_callbacks.factory_reset = FactoryResetCallback;
        m_callbacks.policy_changed = PolicyChangedCallback;
        m_callbacks.start_management = StartManagementCallback;
        m_callbacks.end_management = EndManagementCallback;
    }

  protected:

    alljoyn_permissionconfigurationlistener_callbacks m_callbacks;

  private:

    static void AJ_CALL PolicyChangedCallback(const void* context)
    {
        ASSERT_NE(nullptr, context);

        bool* policyChangedHappened = (bool*)context;
        *policyChangedHappened = true;
    }

    static QStatus AJ_CALL FactoryResetCallback(const void* context)
    {
        /*
         * ASSERT_* returns from the method without any value so it cannot be used here.
         */
        EXPECT_NE(nullptr, context);

        if (nullptr != context) {
            bool* factoryResetHappened = (bool*)context;
            *factoryResetHappened = true;
        }

        return ER_OK;
    }

    static void AJ_CALL StartManagementCallback(const void* context)
    {
        ASSERT_NE(nullptr, context);

        bool* startManagementHappened = (bool*)context;
        *startManagementHappened = true;
    }

    static void AJ_CALL EndManagementCallback(const void* context)
    {
        ASSERT_NE(nullptr, context);

        bool* endManagementHappened = (bool*)context;
        *endManagementHappened = true;
    }
};

TEST_F(PermissionConfigurationListenerTest, shouldCreateListenerWithCallbacksAndNullContext)
{
    EXPECT_NE(nullptr, alljoyn_permissionconfigurationlistener_create(&m_callbacks, nullptr));
}

TEST_F(PermissionConfigurationListenerTest, shouldCreateListenerWithCallbacksAndNonNullContext)
{
    EXPECT_NE(nullptr, alljoyn_permissionconfigurationlistener_create(&m_callbacks, this));
}

TEST_F(PermissionConfigurationListenerTest, shouldDestroyNonNullListenerWithoutException)
{
    alljoyn_permissionconfigurationlistener listener = alljoyn_permissionconfigurationlistener_create(&m_callbacks, nullptr);

    alljoyn_permissionconfigurationlistener_destroy(listener);
}

TEST_F(PermissionConfigurationListenerTest, shouldCallFactoryResetCallback)
{
    bool factoryResetHappened = false;

    alljoyn_permissionconfigurationlistener listener = alljoyn_permissionconfigurationlistener_create(&m_callbacks, &factoryResetHappened);
    ((PermissionConfigurationListener*)listener)->FactoryReset();

    EXPECT_TRUE(factoryResetHappened);
}

TEST_F(PermissionConfigurationListenerTest, shouldCallPolicyChangedCallback)
{
    bool policyChangedHappened = false;

    alljoyn_permissionconfigurationlistener listener = alljoyn_permissionconfigurationlistener_create(&m_callbacks, &policyChangedHappened);
    ((PermissionConfigurationListener*)listener)->PolicyChanged();

    EXPECT_TRUE(policyChangedHappened);
}

TEST_F(PermissionConfigurationListenerTest, shouldCallStartManagementCallback)
{
    bool startManagementHappened = false;

    alljoyn_permissionconfigurationlistener listener = alljoyn_permissionconfigurationlistener_create(&m_callbacks, &startManagementHappened);
    ((PermissionConfigurationListener*)listener)->StartManagement();

    EXPECT_TRUE(startManagementHappened);
}

TEST_F(PermissionConfigurationListenerTest, shouldCallEndManagementCallback)
{
    bool endManagementHappened = false;

    alljoyn_permissionconfigurationlistener listener = alljoyn_permissionconfigurationlistener_create(&m_callbacks, &endManagementHappened);
    ((PermissionConfigurationListener*)listener)->EndManagement();

    EXPECT_TRUE(endManagementHappened);
}
