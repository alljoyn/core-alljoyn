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

using namespace ajn::securitymgr;

/** @file ManifestTests.cc */

namespace secmgr_tests {
class ManifestTests :
    public BasicTest {
  private:

  protected:

  public:
    ManifestTests()
    {
    }
};

/**
 * @test Verify that the manifest received and used for
 *       claiming is consistent with the persisted one.
 *       -# Create an application and make sure its claimable as well as
 *          some test IdentityInfo and store it.
 *       -# Set a generated manifest on the application.
 *       -# Make sure the application is claimed successfully using the test
 *          identity.
 *       -# Make sure the application has declared itself as claimed and that
 *          the remote identity and the manifest match the persisted ones.
 *       -# Make sure rule-by-rule that the persisted manifest is identical to
 *          the one received originally.
 **/
TEST_F(ManifestTests, SuccessfulGetManifest) {
    /* Start the test application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Set manifest */
    PermissionPolicy::Rule* rules;
    size_t count;
    testApp.GetManifest(&rules, count);

    /* Claim! */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    /* Retrieve manifest */
    PermissionPolicy::Rule* retrievedRules = nullptr;
    size_t retrievedCount = 0;
    Manifest mf;
    ASSERT_EQ(ER_OK, storage->GetManifest(lastAppInfo, mf));
    ASSERT_EQ(ER_OK, mf.GetRules(&retrievedRules, &retrievedCount));

    /* Compare set and retrieved manifest */
    ASSERT_EQ(count, retrievedCount);
    for (int i = 0; i < static_cast<int>(count); i++) {
        ASSERT_EQ(rules[i], retrievedRules[i]);
    }

    delete[] rules;
    rules = nullptr;
}

/**
 * @test Verify that if a new manifest is presented by a claimed application
 *       then the security agent is able to update CAStorage and
 *       accept/reject the new manifest.
 *       -# Start an application and make sure it's in a CLAMAIBLE state.
 *       -# Assign a generated manifest1.
 *       -# Create an identitInfo, claim the application and
 *          make sure that it's in CLAIMED state after accepting manifest1.
 *       -# Create a newly generated manifest2 (different than manifest1) and
 *          assign it to the application and make sure that it's
 *          now in the NEED_UPDATE application state.
 *       -# Internally the security agent should have handled the new
 *          application state accordingly and the application's new state
 *          must be now back (and verified to be) CLAIMED after the
 *          new manifest2 is accepted.
 *       -# Verify that the new manifest2 is identical to the one in storage.
 *       -# Repeat the scenario but make sure to reject manifest2 and make
 *          sure that the application's state remain at NEED_UPDATE and that
 *          manifest1 is identical to the one in storage.
 */
TEST_F(ManifestTests, DISABLED_UpdateManifest) {
}
} // namespace
