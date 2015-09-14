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

#include "TestUtil.h"

using namespace ajn;
using namespace ajn::securitymgr;

/** @file MultiAgentAppTests.cc */

namespace secmgr_tests {
class MultiAgentAppTests :
    public BasicTest {
  private:

  protected:

  public:
    MultiAgentAppTests()
    {
    }
};

/**
 * @test Verify that where multi-agents are active, the applications
 *       needing to be claimed and be managed are dealt with consistently.
 *       -# Start an number of security agents which share the same storage.
 *       -# Start a number of applications and make sure that all agents
 *          see those applications in the CLAIMABLE state.
 *       -# Randomly, let the security agents start claiming random claimable
 *          applications.
 *       -# Verify that all applications have been claimed successfully.
 *       -# Randomly, let the security agents start unclaiming random claimed
 *          applications.
 *       -# Verify that all applications have changed state to CLAIMABLE
 **/
TEST_F(MultiAgentAppTests, DISABLED_SuccessfulClaimAndUnclaimManyApps) {
}

/**
 * @test Verify that where multi-agents are active, claimed applications
 *       can be updated correctly after a restart.
 *       -# Start an number of security agents which share the same storage.
 *       -# Start a number of applications and make sure that all agents
 *          see those applications in the CLAIMABLE state.
 *       -# Claim successfully all applications...could be using
 *          one security agent.
 *       -# Verify that all applications have been claimed successfully.
 *       -# Stop all claimed applications.
 *       -# Make sure all security agents see all applications as off-line
 *          (i.e., no busName).
 *       -# Update all applications with predefined membership and policy
 *          using the available security agents randomly.
 *       -# Verify that storage has the predefined membership and policy
 *          for all applications.
 *       -# Restart all the applications.
 *       -# Verify that all applications have been updated correctly and
 *          that no sync errors have been encountered.
 *       -# Verify all applications are in the CLAIMED state.
 **/
TEST_F(MultiAgentAppTests, DISABLED_SuccessfulUpdateManyApps) {
}

/**
 * @test Verify that a restarted Security Agent (SA) after claiming many
 *       applications will maintain a consistent view.
 *       -# Start a number of applications and make sure they are claimable.
 *       -# Start an SA and verify that it could see all
 *          the online applications in a CLAIMABLE state.
 *       -# Ask the SA to start claiming (at no specific order) all
 *          claimable applications.
 *       -# Verify that all applications have been claimed; state change to
 *          CLAIMED.
 *       -# Ask the SA to install/update all claimed applications with a
 *          predefined membership certificate and policy.
 *       -# Kill the SA.
 *       -# Start the SA again and make sure that all previously
 *          claimed applications are in a consistent online state and
 *          no sync errors will have occurred.
 *       -# Kill all applications.
 *       -# Verify that the SA considers all applications as offline.
 **/
TEST_F(MultiAgentAppTests, DISABLED_SuccessfulRestartSecAgentWithManyApps) {
}
}
