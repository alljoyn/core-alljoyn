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
#include "AgentStorageWrapper.h"

using namespace ajn;
using namespace ajn::securitymgr;

/** @file PingApplicationTests.cc */

namespace secmgr_tests {
class PingApplicationTests :
    public SecurityAgentTest {
};

/**
 * @test Start an application and make sure it is pingable by secmgr.
 *       -# Start the application.
 *       -# Make sure the application is pingable.
 **/

TEST_F(PingApplicationTests, SuccessPingApplication) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;

    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    ASSERT_EQ(ER_OK, secMgr->PingApplication(app));
    ASSERT_EQ(ER_OK, testApp.Stop());
}

/**
 * @test Start then stop an application and make sure it is not pingable by secmgr.
 *       -# Start the application.
 *       -# Stop the application.
 *       -# Sleep for 1 second.
 *       -# Make sure the application is not pingable.
 **/

TEST_F(PingApplicationTests, FailPingApplication) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;

    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    ASSERT_EQ(ER_OK, testApp.Stop());
    sleep(1);
    ASSERT_EQ(ER_ALLJOYN_PING_REPLY_UNREACHABLE, secMgr->PingApplication(app));
}

}
