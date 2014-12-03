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
static bool presenceFoundAdvReq;
static bool presenceFoundNotAdvReq;
static bool presenceFoundAdvNotReq;
static bool presenceFoundNotAdvNotReq;
static bool presenceFoundReqAdvLocalOnly;

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
String wellKnownNameAdvReq;
String wellKnownNameNotAdvReq;
String wellKnownNameAdvNotReq;
String wellKnownNameNotAdvNotReq;
String wellKnownNameReqAdvLocalOnly;

// find name listener class
class PresenceTestFindNameListener : public BusListener {
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        printf("FoundAdvertisedName %s\n", name);
        if (wellKnownNameAdvReq == name) {
            presenceFoundAdvReq = true;
        } else if (wellKnownNameAdvNotReq == name) {
            presenceFoundAdvNotReq = true;
        } else if (wellKnownNameReqAdvLocalOnly == name) {
            presenceFoundReqAdvLocalOnly = true;
        }
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        printf("LostAdvertisedName %s\n", name);

        if (wellKnownNameAdvReq == name) {
            presenceFoundAdvReq = false;
        } else if (wellKnownNameAdvNotReq == name) {
            presenceFoundAdvNotReq = false;
        } else if (wellKnownNameReqAdvLocalOnly == name) {
            presenceFoundReqAdvLocalOnly = false;
        }
    }
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner) {
        printf("NameOwnerChanged %s newOwner %s\n", busName, newOwner);

        if (wellKnownNameNotAdvReq == busName) {
            if (newOwner != NULL) {
                presenceFoundNotAdvReq = true;
            } else {
                presenceFoundNotAdvReq = false;
            }

        }
    }
};

//Wellknown names - advertised and requested, advertised not requested, requested not advertised, not advertised not requested.
TEST_F(PresenceTest, PresenceWellKnownNames) {
    QStatus status = ER_OK;
    presenceFoundAdvReq = false;
    presenceFoundNotAdvReq = false;
    presenceFoundAdvNotReq = false;
    presenceFoundNotAdvNotReq = false;
    wellKnownNameAdvReq = genUniqueName(bus);
    wellKnownNameNotAdvReq = genUniqueName(bus);
    wellKnownNameAdvNotReq = genUniqueName(bus);
    wellKnownNameNotAdvNotReq = genUniqueName(bus);
    wellKnownNameReqAdvLocalOnly = genUniqueName(bus);

    // start other bus attachment
    BusAttachment otherBus("BusAttachmentTestOther", true);
    status = otherBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = otherBus.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // register listener bus
    PresenceTestFindNameListener testBusListener;
    otherBus.RegisterBusListener(testBusListener);

    //
    // wellKnownNameAdvReq
    status = bus.RequestName(wellKnownNameAdvReq.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.AdvertiseName(wellKnownNameAdvReq.c_str(), TRANSPORT_ANY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //
    // wellKnownNameAdvNotReq
    status = bus.AdvertiseName(wellKnownNameAdvNotReq.c_str(), TRANSPORT_ANY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //
    // wellKnownNameNotAdvReq
    status = bus.RequestName(wellKnownNameNotAdvReq.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // find advertised name
    status = otherBus.FindAdvertisedName(getUniqueNamePrefix(bus).c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //
    // wellKnownNameReqAdvLocalOnly
    status = bus.RequestName(wellKnownNameReqAdvLocalOnly.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.AdvertiseName(wellKnownNameReqAdvLocalOnly.c_str(), TRANSPORT_ANY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // wait for up to 10 seconds to find name
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (presenceFoundAdvReq && presenceFoundAdvNotReq && presenceFoundNotAdvReq && presenceFoundReqAdvLocalOnly) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    ASSERT_TRUE(presenceFoundAdvReq) << "failed to find advertised name: " << wellKnownNameAdvReq.c_str();
    ASSERT_TRUE(presenceFoundAdvNotReq) << "failed to find advertised name: " << wellKnownNameAdvNotReq.c_str();
    ASSERT_TRUE(presenceFoundNotAdvReq) << "failed to get NOC for requested name: " << wellKnownNameNotAdvReq.c_str();
    ASSERT_TRUE(presenceFoundReqAdvLocalOnly) << "failed to find advertised name: " << wellKnownNameReqAdvLocalOnly.c_str();
    ASSERT_FALSE(presenceFoundNotAdvNotReq) << "Found name incorrectly: " << wellKnownNameNotAdvNotReq.c_str();

    // ping
    status = otherBus.Ping(wellKnownNameAdvReq.c_str(), 3000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping
    status = otherBus.Ping(wellKnownNameAdvNotReq.c_str(), 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNREACHABLE, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping
    status = otherBus.Ping(wellKnownNameNotAdvReq.c_str(), 3000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping
    status = otherBus.Ping(wellKnownNameNotAdvNotReq.c_str(), 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping
    status = otherBus.Ping(wellKnownNameReqAdvLocalOnly.c_str(), 3000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // stop second bus
    otherBus.Stop();
    otherBus.Join();
}
//test Unique name.
TEST_F(PresenceTest, PresenceUniqueNames) {
    QStatus status = ER_OK;

    // start other bus attachment
    BusAttachment otherBus("BusAttachmentTestOther", true);
    status = otherBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = otherBus.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // register listener bus
    PresenceTestFindNameListener testBusListener;
    otherBus.RegisterBusListener(testBusListener);

    //
    // advertise unique name
    status = bus.AdvertiseName(bus.GetUniqueName().c_str(), TRANSPORT_ANY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping
    status = otherBus.Ping(bus.GetUniqueName().c_str(), 3000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // stop second bus
    otherBus.Stop();
    otherBus.Join();
}

//test Unique name advertised.
TEST_F(PresenceTest, PresenceUniqueNamesAdvertised) {
    QStatus status = ER_OK;

    // start other bus attachment
    BusAttachment otherBus("BusAttachmentTestOther", true);
    status = otherBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = otherBus.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // register listener bus
    PresenceTestFindNameListener testBusListener;
    otherBus.RegisterBusListener(testBusListener);

    // ping
    status = otherBus.Ping(bus.GetUniqueName().c_str(), 3000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // stop second bus
    otherBus.Stop();
    otherBus.Join();
}

TEST_F(PresenceTest, PingBogusUniqueNames) {
    QStatus status = ER_OK;

    // ping bogusUqns with same guid
    status = bus.Ping(String(bus.GetUniqueName() + "0").c_str(), 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Ping(String(bus.GetUniqueName() + ".li").c_str(), 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping bogusUqn with invalid guid
    status = bus.Ping(":xyz.40", 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME, status) << "  Actual Status: " << QCC_StatusText(status);


    //
    // advertise bogusUniqueName with local guid
    String bogusUniqueName = bus.GetUniqueName() + "1";
    status = bus.AdvertiseName(bogusUniqueName.c_str(), TRANSPORT_ANY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //
    // advertise bogusUniqueName with invalid guid
    String bogusUniqueNamewithInvalidGuid = ":abc.100";
    status = bus.AdvertiseName(bogusUniqueNamewithInvalidGuid.c_str(), TRANSPORT_ANY);
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

    // ping bogusUniqueName with local guid
    status = otherBus.Ping(bogusUniqueName.c_str(), 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping bogusUniqueName with invalid guid
    status = otherBus.Ping(bogusUniqueNamewithInvalidGuid.c_str(), 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNKNOWN_NAME, status) << "  Actual Status: " << QCC_StatusText(status);

    // stop second bus
    otherBus.Stop();
    otherBus.Join();
}
TEST_F(PresenceTest, PingExitedApp) {
    QStatus status = ER_OK;

    // start other bus attachment
    BusAttachment otherBus("BusAttachmentTestOther", true);
    status = otherBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = otherBus.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    String otherUqn = otherBus.GetUniqueName();
    // stop second bus
    otherBus.Stop();
    otherBus.Join();

    // ping bogusUniqueName with local guid
    status = bus.Ping(otherUqn.c_str(), 3000);
    EXPECT_EQ(ER_ALLJOYN_PING_REPLY_UNREACHABLE, status) << "  Actual Status: " << QCC_StatusText(status);

}
class PresenceSessionPortListener : public SessionPortListener {
  public:
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
};
TEST_F(PresenceTest, PingSessionNames) {
    QStatus status = ER_OK;
    presenceFoundAdvReq = false;
    wellKnownNameAdvReq = genUniqueName(bus);

    // start other bus attachment
    BusAttachment otherBus("BusAttachmentTestOther", true);
    status = otherBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = otherBus.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // register listener bus
    PresenceTestFindNameListener testBusListener;
    otherBus.RegisterBusListener(testBusListener);

    //
    // wellKnownNameAdvReq
    status = bus.RequestName(wellKnownNameAdvReq.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.AdvertiseName(wellKnownNameAdvReq.c_str(), TRANSPORT_ANY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort portA = 27;
    PresenceSessionPortListener listenerA;
    EXPECT_EQ(ER_OK, bus.BindSessionPort(portA, opts, listenerA));


    // find advertised name
    status = otherBus.FindAdvertisedName(getUniqueNamePrefix(bus).c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // wait for up to 10 seconds to find name
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (presenceFoundAdvReq) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    ASSERT_TRUE(presenceFoundAdvReq) << "failed to find advertised name: " << wellKnownNameAdvReq.c_str();

    //joinsession
    SessionId outIdA;
    status = otherBus.JoinSession(wellKnownNameAdvReq.c_str(), portA, NULL, outIdA, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //canceladvertise
    status = bus.CancelAdvertiseName(wellKnownNameAdvReq.c_str(), TRANSPORT_ANY);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping wkn from joiner
    status = otherBus.Ping(wellKnownNameAdvReq.c_str(), 3000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping unique name from joiner
    status = otherBus.Ping(bus.GetUniqueName().c_str(), 3000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // ping unique name from host
    status = bus.Ping(otherBus.GetUniqueName().c_str(), 3000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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


