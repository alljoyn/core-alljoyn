/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/
#include <qcc/Thread.h>
#include "TestUtil.h"
#include "AgentStorageWrapper.h"

using namespace ajn;
using namespace ajn::securitymgr;

/** @file PingApplicationTests.cc */

namespace secmgr_tests {
class PingApplicationTests : public SecurityAgentTest {

  public:
    void SetUp() {
        SecurityAgentTest::SetUp();
        ASSERT_EQ(ER_OK, m_testApp.Start());
        ASSERT_EQ(ER_OK, GetPublicKey(m_testApp, m_app));
        m_appStarted = true;
    }

    void TearDown() {
        SecurityAgentTest::TearDown();
        if (m_appStarted) {
            EXPECT_EQ(ER_OK, m_testApp.Stop());
        }
    }

  protected:
    TestApplication m_testApp;
    OnlineApplication m_app;
    bool m_appStarted;
};

/**
 * @test Start an application and make sure it is pingable by secmgr.
 *       -# Make sure the application is pingable.
 **/

TEST_F(PingApplicationTests, SuccessPingApplication) {
    EXPECT_EQ(ER_OK, secMgr->PingApplication(m_app));
}

/**
 * @test Start then stop an application and make sure it is not pingable by secmgr.
 *       -# Stop the application.
 *       -# Sleep for 1 second.
 *       -# Make sure the application is not pingable.
 **/

TEST_F(PingApplicationTests, FailPingApplication) {
    ASSERT_EQ(ER_OK, m_testApp.Stop());
    qcc::Sleep(1);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNREACHABLE, secMgr->PingApplication(m_app));
    m_appStarted = false;
}

}
