/**
 * @file
 * Service test
 */

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

#include "ServiceTestObject.h"
#include "ajTestCommon.h"
/* Header files included for Google Test Framework */
#include <gtest/gtest.h>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace ajn;
using namespace qcc;

class NameAcquiredTest : public testing::Test {
  public:
    BusAttachment* g_msgBus;
    ServiceTestObject*testObj;

    virtual void SetUp() {
        g_msgBus = new BusAttachment("bb_client_test", true);
        testObj = new ServiceTestObject(*g_msgBus, "/com/cool");

        QStatus status = g_msgBus->Start();
        ASSERT_EQ(ER_OK, status);
    }

    virtual void TearDown() {
        //g_msgBus->Stop();
        if (g_msgBus) {
            BusAttachment* deleteMe = g_msgBus;
            g_msgBus = NULL;
            delete deleteMe;
        }

        if (testObj) {
            ServiceTestObject*temp = testObj;
            testObj = NULL;
            delete temp;
        }
    }

};


TEST_F(NameAcquiredTest, NameAcquiredSignal_UniqueName) {
    /* When you do a Bus->Connect() you get a unique name..and NameAcquired signal with it.This test checks for that signal - output = 1 */
    QStatus status = ER_OK;
    testObj->setOutput(0);
    testObj->RegisterForNameAcquiredSignals();
    status = g_msgBus->Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status);
    // wait for signal
    for (int i = 0; i < 200; ++i) {
        if (testObj->getOutput() == SUCCESS) {
            break;
        }
        qcc::Sleep(WAIT_TIME_10);
    }
    ASSERT_EQ(SUCCESS, testObj->getOutput());
}

TEST_F(NameAcquiredTest, NameAcquiredSignal_WellKnownName) {

    QStatus status =  ER_OK;
    testObj->setOutput(0);
    testObj->RegisterForNameAcquiredSignals();
    ASSERT_EQ(ER_OK, status);
    status = g_msgBus->Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status);
    // wait for unique name signal
    for (int i = 0; i < 100; ++i) {
        if (testObj->getOutput() == SUCCESS) {
            break;
        }
        qcc::Sleep(WAIT_TIME_10);
    }
    ASSERT_EQ(SUCCESS, testObj->getOutput());
    testObj->setOutput(0);
    ASSERT_EQ(ER_OK, status);
    status = g_msgBus->RequestName("com.cool", DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    ASSERT_EQ(ER_OK, status);
    // wait for signal
    for (int i = 0; i < 100; ++i) {
        if (testObj->getOutput() == SUCCESS) {
            break;
        }
        qcc::Sleep(WAIT_TIME_10);
    }

    ASSERT_EQ(SUCCESS, testObj->getOutput());
}


