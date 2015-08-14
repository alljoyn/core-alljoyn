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

/** @file RestartAgentTests.cc */

namespace secmgr_tests {
class RestartAgentTests :
    public SecurityAgentTest {
  private:

  protected:

  public:
    RestartAgentTests()
    {
    }
};

/**
 * @test Verify that the agent can restart and maintain a
 *       consistent view on the online applications.
 *       // See AS-1634 for the number of applications issue
 *       -# Start an even X number of applications and make sure they are claimable.
 *       -# Using a dedicated IdtityInfo (per application) and a default manifest,
 *          claim an X/2 (claimedApps) of the applications.
 *       -# Verify that claimedApps are in a CLAIMED state and that the other
 *          X/2 (claimableApps) are still in a CLAIMABLE state.
 *       -# Delete the security agent instance that claimed the claimedApps.
 *       -# Create a new security agent which uses the same keystore and CAStorage.
 *       -# Verify that all online applications are in a consistent
 *          online application state from the security agent perspective.
 **/
TEST_F(RestartAgentTests, SuccessfulAgentRestart) {
    size_t numOfApps = 3;
    vector<shared_ptr<TestApplication> > apps;
    vector<IdentityInfo> identities;

    for (size_t i = 0; i < numOfApps; i++) {
        identities.push_back(IdentityInfo());
        apps.push_back(shared_ptr<TestApplication>(new TestApplication(std::to_string(i) + "-Testapp")));
        ASSERT_EQ(ER_OK, storage->StoreIdentity(identities[i]));
    }

    for (size_t i = numOfApps / 2; i < numOfApps; i++) {
        ASSERT_EQ(ER_OK, apps[i]->Start());
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    }

    for (size_t i = 0; i < numOfApps / 2; i++) {
        ASSERT_EQ(ER_OK, apps[i]->Start());
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
        ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, identities[i]));
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    }

    RemoveSecAgent();
    InitSecAgent();
    WaitForEvents(numOfApps);
    RemoveSecAgent();

    apps.clear();
}
}
