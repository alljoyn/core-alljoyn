/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/BusAttachment.h>

#include "ajTestCommon.h"

using namespace ajn;

class BusStartupTest : public testing::Test {
  public:
    BusAttachment* g_msgBus;

    virtual void SetUp() {
        g_msgBus = new BusAttachment("testservices", true);
        QStatus status = g_msgBus->Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        if (g_msgBus) {
            BusAttachment* deleteMe = g_msgBus;
            g_msgBus = NULL;
            delete deleteMe;
        }
    }
    //Common setup function for all service tests
    QStatus ServiceBusSetup() {
        QStatus status = ER_OK;

        if (!g_msgBus->IsConnected()) {
            /* Connect to the daemon and wait for the bus to exit */
            status = g_msgBus->Connect(ajn::getConnectArg().c_str());
        }

        return status;
    }

};

TEST_F(BusStartupTest, SUCCESS_Start) {
    /*
     * The bus should be started by the setup this is just verifying that it
     * was started.
     */
    EXPECT_TRUE(g_msgBus->IsStarted());
}

TEST_F(BusStartupTest, Fail_Already_Started) {
    QStatus status = ER_OK;
    EXPECT_TRUE(g_msgBus->IsStarted());
    /* Restart the msg bus */
    status = g_msgBus->Start();
    ASSERT_EQ(ER_BUS_BUS_ALREADY_STARTED, status);
}

TEST_F(BusStartupTest, SUCCESS_Connect) {
    QStatus status = g_msgBus->Connect(ajn::getConnectArg().c_str());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(BusStartupTest, Fail_Already_Connected) {
    QStatus status = ER_OK;
    /* Get env vars */
    status = g_msgBus->Connect(ajn::getConnectArg().c_str());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(g_msgBus->IsConnected());
    /* Connect to the daemon and wait for the bus to exit */
    status = g_msgBus->Connect(ajn::getConnectArg().c_str());
    ASSERT_EQ(ER_BUS_ALREADY_CONNECTED, status) << "  Actual Status: " << QCC_StatusText(status);
}
