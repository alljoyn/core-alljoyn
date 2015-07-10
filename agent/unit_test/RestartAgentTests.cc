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
    public BasicTest {
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
 *       -# Start an X number of applications and make sure they are claimable.
 *       -# Using a dedicated idtityInfo (per application) and a default manifest,
 *          claim an X/2 (claimedApps) of the applications.
 *       -# Verify that claimedApps are in a CLAIMED state and that the other
 *          X/2 (claimableApps) are still in a CLAIMABLE state.
 *       -# Delete the security agent instance that claimed the claimedApps.
 *       -# Create a new security agent which uses the same keystore and CAStorage.
 *       -# Verify that all online applications are in a consistent
 *          online application state from the security agent perspective.
 **/
TEST_F(RestartAgentTests, DISABLED_SuccessfulAgentRestart) {
}
}
