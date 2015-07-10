/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include "TestUtil.h"

using namespace ajn;
using namespace ajn::securitymgr;

/** @file ResetTests.cc */

namespace secmgr_tests {
class ResetTests :
    public BasicTest {
  private:

  protected:

  public:
    ResetTests()
    {
    }
};

/**
 * @test Reset an application and make sure it becomes CLAIMABLE again.
 *       -# Start the application.
 *       -# Make sure the application is in a CLAIMABLE state.
 *       -# Create and store an IdentityInfo.
 *       -# Claim the application using the IdentityInfo.
 *       -# Accept the manifest of the application.
 *       -# Check whether the application becomes CLAIMED.
 *       -# Remove the application from storage.
 *       -# Check whether it becomes CLAIMABLE again.
 *       -# Claim the application again.
 *       -# Check whether it becomes CLAIMED again.
 **/
TEST_F(ResetTests, SuccessfulReset) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    ASSERT_EQ(ER_OK, storage->RemoveApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, false));

    OnlineApplication checkUpdatesPendingInfo;
    checkUpdatesPendingInfo.keyInfo = lastAppInfo.keyInfo;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_FALSE(checkUpdatesPendingInfo.updatesPending);

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
}

/**
 * @test Verify that resetting an application with no keystore after claiming
 *       will fail.
 *       -# Start the application.
 *       -# Claim the application successfully.
 *       -# Remove only the keystore of the application.
 *       -# Try to remove the application and make sure this fails.
 **/
TEST_F(ResetTests, DISABLED_FailedReset) {
}
} // namespace
