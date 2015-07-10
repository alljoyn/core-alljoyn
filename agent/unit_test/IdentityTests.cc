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

/** @file IdentityTests.cc */

namespace secmgr_tests {
class IdentityTests :
    public BasicTest {
  private:

  protected:

  public:
    IdentityTests()
    {
    }
};
/**
 * @test Update the identity certificate of an application and check that it
 *        gets installed correctly.
 *       -# Start the application.
 *       -# Make sure the application is in a CLAIMABLE state.
 *       -# Create and store an IdentityInfo.
 *       -# Claim the application using the IdentityInfo.
 *       -# Accept the manifest of the application.
 *       -# Check whether the application becomes CLAIMED.
 *       -# Create and store another IdentityInfo.
 *       -# Update the identity certificate of the application.
 *       -# Wait for the updates to be completed.
 *       -# Check whether the identity certificate was installed successfully.
 **/
TEST_F(IdentityTests, SuccessfulInstallIdentity) {
    /* Start the application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    IdentityInfo info;
    info.name = "MyName";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(info));

    /* Claim! */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, info));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckIdentity(info, aa.lastManifest));

    /* Try to install another identity */
    IdentityInfo info2;
    info2.name = "AnotherName";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(info2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, info2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckIdentity(info2, aa.lastManifest));
}

/**
 * @test Verify that claiming with a different manifest digest in the
 *       generated identity certificate would be handled correctly.
 *       -# Start an application and make sure it's claimable.
 *       -# Try to claim the application after generating an identity
 *          certificate based on an ALTERED version of the received
 *          manifest.
 *       -# Verify that the claiming would fail and that the application
 *          is still claimable.
 *       -# Make sure that agent does not manage the application.
 **/
TEST_F(IdentityTests, DISABLED_IdentityDigestFail) {
}

/**
 * @test Update the identity certificate chain.
 *       -# Pending AS-1573 (and implementation in core?)
 **/
TEST_F(IdentityTests, DISABLED_SuccessfulInstallIdentityChain) {
}
} // namespace
