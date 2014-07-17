/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "ajTestCommon.h"


#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/platform.h>
#include <qcc/String.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>
#include <alljoyn/Status.h>

/*
 * The unit test use many busy wait loops.  The busy wait loops were chosen
 * over thread sleeps because of the ease of understanding the busy wait loops.
 * Also busy wait loops do not require any platform specific threading code.
 */
#define WAIT_TIME 5

using namespace std;
using namespace qcc;
using namespace ajn;

// globals
static bool presenceFound;


// PresenceTest test class
class PresenceTest : public testing::Test {
  public:
    BusAttachment bus;

    PresenceTest() : bus("PresenceTest", false) { };

    virtual void SetUp() {
        QStatus status = ER_OK;
        status = bus.Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = bus.Connect(getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        bus.Stop();
        bus.Join();
    }

};

// find name listener class
class PresenceTestFindNameListener : public BusListener {
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        presenceFound = true;
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        presenceFound = false;
    }
};

//ASACORE-682
// Negative test - Verify that Presence detection does not work if the name is not requested
TEST_F(PresenceTest, DISABLED_NegativePresenceNameNotRequested) {
    QStatus status = ER_OK;
    presenceFound = false;
    const char* wellKnowName = "org.test.presence";

    // advertise name
    status = bus.AdvertiseName(wellKnowName, TRANSPORT_ANY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // start other bus attachment
    BusAttachment otherBus("BusAttachmentTestOther", true);
    status = otherBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = otherBus.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // register listener bus
    PresenceTestFindNameListener testBusListener;
    otherBus.RegisterBusListener(testBusListener);

    // find advertised name
    status = otherBus.FindAdvertisedName(wellKnowName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // wait for up to 10 seconds to find name
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (presenceFound) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    ASSERT_TRUE(presenceFound) << "failed to find advertised name: " << wellKnowName;

    // ping
    status = otherBus.Ping(wellKnowName, 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNREACHABLE, status) << "  Actual Status: " << QCC_StatusText(status);

    // stop second bus
    otherBus.Stop();
    otherBus.Join();
}

// ping with invalid "null" name as an argument, NOTE: ER_BUS_BAD_BUS_NAME is returned instead of ER_BAD_ARG_1
TEST_F(PresenceTest, PingWithBadArgument) {
    QStatus status = ER_OK;

    status = bus.Ping(NULL, 1000);
    EXPECT_EQ(ER_BUS_BAD_BUS_NAME, status) << "  Actual Status: " << QCC_StatusText(status);
}

// ping with no valid bus attachment
TEST_F(PresenceTest, PingWithNoBusAttachment) {
    QStatus status = ER_OK;
    BusAttachment otherBus("BusAttachmentTestOther", true);

    status = otherBus.Ping("asdf.asdf", 100);
    EXPECT_EQ(ER_BUS_NOT_CONNECTED, status) << "  Actual Status: " << QCC_StatusText(status);
}

