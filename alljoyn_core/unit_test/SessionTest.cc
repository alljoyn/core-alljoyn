/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>

#include <qcc/String.h>
#include <qcc/Thread.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "ajTestCommon.h"

using namespace std;
using namespace qcc;
using namespace ajn;

class SessionTest : public testing::Test {
  public:
    virtual void SetUp() { }
    virtual void TearDown() { }

};

class TwoMultipointSessionsSessionPortListener : public SessionPortListener {
  public:
    virtual ~TwoMultipointSessionsSessionPortListener() { }
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
};

TEST_F(SessionTest, TwoMultipointSessions)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort portA = 27;
    SessionPort portB = portA;

    BusAttachment busA("A");
    TwoMultipointSessionsSessionPortListener listenerA;
    ASSERT_EQ(ER_OK, busA.Start());
    ASSERT_EQ(ER_OK, busA.Connect(getConnectArg().c_str()));
    ASSERT_EQ(ER_OK, busA.BindSessionPort(portA, opts, listenerA));
    ASSERT_EQ(ER_OK, busA.RequestName("bus.A", DBUS_NAME_FLAG_DO_NOT_QUEUE));
    ASSERT_EQ(ER_OK, busA.AdvertiseName("bus.A", TRANSPORT_ANY));

    BusAttachment busB("B");
    TwoMultipointSessionsSessionPortListener listenerB;
    ASSERT_EQ(ER_OK, busB.Start());
    ASSERT_EQ(ER_OK, busB.Connect(getConnectArg().c_str()));
    ASSERT_EQ(ER_OK, busB.BindSessionPort(portB, opts, listenerB));
    ASSERT_EQ(ER_OK, busB.RequestName("bus.B", DBUS_NAME_FLAG_DO_NOT_QUEUE));
    ASSERT_EQ(ER_OK, busB.AdvertiseName("bus.B", TRANSPORT_ANY));

    SessionId outIdA;
    SessionOpts outOptsA;
    ASSERT_EQ(ER_OK, busA.JoinSession("bus.B", portB, NULL, outIdA, opts));

    SessionId outIdB;
    SessionOpts outOptsB;
    ASSERT_EQ(ER_OK, busB.JoinSession("bus.A", portA, NULL, outIdB, opts));

    /*
     * The bug is that joining two multipoint sessions with the same port resulted in only one
     * session, not two.  This asserts that there are in fact two different sessions created above.
     */
    ASSERT_NE(outIdA, outIdB);
}

bool sessionMemberAddedFlagA = false;
bool sessionMemberRemovedFlagA = false;
bool sessionMemberAddedFlagB = false;
bool sessionMemberRemovedFlagB = false;
bool sessionMemberAddedFlagC = false;
bool sessionMemberRemovedFlagC = false;
bool sessionJoinerAcceptedFlag = false;
bool sessionJoinedFlag = false;
bool sessionJoinedCBFlag = false;
SessionId bindMemberSessionId = 0;

class BindMemberSessionListenerA : public SessionListener {
  public:
    //virtual void SessionLost(SessionId sessionId, SessionLostReason reason) { }
    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) {
        sessionMemberAddedFlagA = true;
    }
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        sessionMemberRemovedFlagA = true;
    }
};

class BindMemberSessionListenerB : public SessionListener {
  public:
    //virtual void SessionLost(SessionId sessionId, SessionLostReason reason) { }
    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) {
        sessionMemberAddedFlagB = true;
    }
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        sessionMemberRemovedFlagB = true;
    }
};

class BindMemberSessionListenerC : public SessionListener {
  public:
    //virtual void SessionLost(SessionId sessionId, SessionLostReason reason) { }
    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) {
        sessionMemberAddedFlagC = true;
    }
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        sessionMemberRemovedFlagC = true;
    }
};

class BindMemberSessionPortListener : public SessionPortListener {
  public:
    BindMemberSessionPortListener(BusAttachment* bus, BindMemberSessionListenerA* bindMemberSessionListener) :
        bus(bus),
        sessionListener(bindMemberSessionListener) { }
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        sessionJoinerAcceptedFlag = true;
        return true;
    }
    virtual void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {
        bindMemberSessionId = id;
        sessionJoinedFlag = true;
        QStatus status =  bus->SetSessionListener(id, sessionListener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }
    BusAttachment* bus;
    BindMemberSessionListenerA* sessionListener;
};

class BindMemberJoinSessionAsyncCB : public ajn::BusAttachment::JoinSessionAsyncCB {
  public:
    virtual void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        sessionJoinedCBFlag = true;
    }
};
TEST_F(SessionTest, BindMemberAddedRemoved) {
    QStatus status = ER_FAIL;
    /* make sure global flags are initialized */
    sessionMemberAddedFlagA = false;
    sessionMemberRemovedFlagA = false;
    sessionMemberAddedFlagB = false;
    sessionMemberRemovedFlagB = false;
    sessionMemberAddedFlagC = false;
    sessionMemberRemovedFlagC = false;
    sessionJoinerAcceptedFlag = false;
    sessionJoinedFlag = false;
    sessionJoinedCBFlag = false;
    bindMemberSessionId = 0;

    SessionId multipointSessionId = 0;

    BusAttachment busA("bus.Aa", false);
    BusAttachment busB("bus.Bb", false);
    BusAttachment busC("bus.Cc", false);

    status = busA.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busA.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busB.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busB.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busC.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busC.Connect(getConnectArg().c_str());

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    BindMemberSessionListenerA sessionListenerA;
    BindMemberSessionPortListener sessionPortListener(&busA, &sessionListenerA);
    SessionPort port = 0;

    status = busA.BindSessionPort(port, opts, sessionPortListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    BindMemberJoinSessionAsyncCB joinSessionCB;
    BindMemberSessionListenerB sessionListenerB;
    status = busB.JoinSessionAsync(busA.GetUniqueName().c_str(), port, &sessionListenerB, opts, &joinSessionCB);

    //Wait upto 5 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 500; ++i) {
        if (sessionJoinedCBFlag && sessionJoinedFlag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(sessionJoinedCBFlag);
    multipointSessionId = bindMemberSessionId;

    status = busA.SetSessionListener(bindMemberSessionId, &sessionListenerA);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 5 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 500; ++i) {
        if (sessionMemberAddedFlagB &&
            sessionJoinerAcceptedFlag &&
            sessionJoinedFlag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(sessionJoinerAcceptedFlag);
    EXPECT_TRUE(sessionJoinedFlag);
    EXPECT_TRUE(sessionMemberAddedFlagB);

    sessionMemberAddedFlagA = false;
    sessionMemberAddedFlagB = false;
    sessionJoinerAcceptedFlag = false;
    sessionJoinedFlag = false;
    sessionJoinedCBFlag = false;

    BindMemberSessionListenerC sessionListenerC;
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busC.JoinSessionAsync(busA.GetUniqueName().c_str(), port, &sessionListenerC, opts, &joinSessionCB);


    //Wait upto 5 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 500; ++i) {
        if (sessionJoinedCBFlag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_EQ(multipointSessionId, bindMemberSessionId);

    //Wait upto 5 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 500; ++i) {
        if (sessionMemberAddedFlagA &&
            sessionMemberAddedFlagB &&
            sessionMemberAddedFlagC &&
            sessionJoinerAcceptedFlag &&
            sessionJoinedFlag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(sessionMemberAddedFlagA);
    EXPECT_TRUE(sessionMemberAddedFlagB);
    EXPECT_TRUE(sessionMemberAddedFlagC);
    EXPECT_TRUE(sessionJoinerAcceptedFlag);
    EXPECT_TRUE(sessionJoinedFlag);

    status = busB.LeaveSession(bindMemberSessionId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 5 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 500; ++i) {
        if (sessionMemberRemovedFlagA && sessionMemberRemovedFlagC) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(sessionMemberRemovedFlagA);
    EXPECT_FALSE(sessionMemberRemovedFlagB);
    EXPECT_TRUE(sessionMemberRemovedFlagA);

    sessionMemberRemovedFlagA = false;
    sessionMemberRemovedFlagB = false;
    sessionMemberRemovedFlagC = false;

    status = busC.LeaveSession(bindMemberSessionId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 5 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 500; ++i) {
        if (sessionMemberRemovedFlagA) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(sessionMemberRemovedFlagA);
    EXPECT_FALSE(sessionMemberRemovedFlagB);
    EXPECT_FALSE(sessionMemberRemovedFlagC);
}

qcc::String sessionJoinedTestJoiner = "";

class SessionJoinedSessionPortListener : public SessionPortListener {
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        printf("AcceptSessionJoiner sessionPort = %d, joiner = %s\n", sessionPort, joiner);
        sessionJoinerAcceptedFlag = true;
        return true;
    }
    virtual void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {
        printf("SessionJoined sessionPort = %d, SessionId=%d, joiner = %s\n", sessionPort, id, joiner);
        bindMemberSessionId = id;
        sessionJoinedTestJoiner = joiner;
        sessionJoinedFlag = true;

    }
};

//ALLJOYN-1602
TEST_F(SessionTest, SessionJoined) {
    bindMemberSessionId = 0;
    sessionJoinerAcceptedFlag = false;
    sessionJoinedFlag = false;
    QStatus status = ER_FAIL;

    BusAttachment busA("busAA", false);
    BusAttachment busB("busBB", false);

    status = busA.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busA.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busB.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busB.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    SessionJoinedSessionPortListener sessionPortListener;
    SessionPort port = 0;

    status = busA.BindSessionPort(port, opts, sessionPortListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    SessionListener blankSessionListener;
    SessionId sessionId;

    status = busB.JoinSession(busA.GetUniqueName().c_str(), port, &blankSessionListener, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(sessionJoinerAcceptedFlag);
    //Wait upto 3 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 300; ++i) {
        if (sessionJoinedFlag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(sessionJoinedFlag);
    EXPECT_EQ(bindMemberSessionId, sessionId);
    EXPECT_STRNE(busA.GetUniqueName().c_str(), sessionJoinedTestJoiner.c_str()) <<
    "The Joiner name " << sessionJoinedTestJoiner.c_str() <<
    " should be different than " << busA.GetUniqueName().c_str();
    EXPECT_STREQ(busB.GetUniqueName().c_str(), sessionJoinedTestJoiner.c_str()) <<
    "The Joiner name " << sessionJoinedTestJoiner.c_str() <<
    " should be the same as " << busB.GetUniqueName().c_str();;

    status = busA.RemoveSessionMember(sessionId, busB.GetUniqueName());
    EXPECT_EQ(ER_ALLJOYN_REMOVESESSIONMEMBER_NOT_MULTIPOINT, status) << "  Actual Status: " << QCC_StatusText(status);
}

bool sessionLostFlagA;
bool sessionLostFlagB;
class RemoveSessionMemberBusAListener : public SessionPortListener, public SessionListener {
  public:
    RemoveSessionMemberBusAListener(BusAttachment* bus) : bus(bus) { }

    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        printf("AcceptSessionJoiner sessionPort = %d, joiner = %s\n", sessionPort, joiner);
        return true;
    }
    virtual void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {
        printf("SessionJoined sessionPort = %d, SessionId=%d, joiner = %s\n", sessionPort, id, joiner);
        bindMemberSessionId = id;
        sessionJoinedTestJoiner = joiner;
        sessionJoinedFlag = true;
        bus->SetSessionListener(id, this);
    }
    virtual void SessionLost(SessionId sessionId) {
        sessionLostFlagA = true;
    }
    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) {
        sessionMemberAddedFlagA = true;
    }
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        sessionMemberRemovedFlagA = true;
    }
    BusAttachment* bus;
};
class RemoveSessionMemberBusBListener : public SessionListener {

    virtual void SessionLost(SessionId sessionId) {
        sessionLostFlagB = true;
    }
    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) {
        sessionMemberAddedFlagB = true;
    }
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        sessionMemberRemovedFlagB = true;
    }
};

TEST_F(SessionTest, RemoveSessionMember) {
    QStatus status = ER_FAIL;
    /* make sure global flags are initialized */
    sessionJoinedFlag = false;
    sessionLostFlagA = false;
    sessionMemberAddedFlagA = false;
    sessionMemberRemovedFlagA = false;
    sessionLostFlagB = false;
    sessionMemberAddedFlagB = false;
    sessionMemberRemovedFlagB = false;

    BusAttachment busA("bus.Aa", false);
    BusAttachment busB("bus.Bb", false);

    status = busA.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busA.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busB.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busB.Connect(getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Multi-point session */
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    RemoveSessionMemberBusAListener sessionPortListener(&busA);
    SessionPort port = 1;

    status = busA.BindSessionPort(port, opts, sessionPortListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    RemoveSessionMemberBusBListener sessionListener;
    SessionId sessionId;

    status = busB.JoinSession(busA.GetUniqueName().c_str(), port, &sessionListener, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(sessionJoinerAcceptedFlag);
    //Wait upto 3 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 300; ++i) {
        if (sessionJoinedFlag && sessionMemberAddedFlagA && sessionMemberAddedFlagB) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(sessionJoinedFlag);
    EXPECT_TRUE(sessionMemberAddedFlagA);
    EXPECT_TRUE(sessionMemberAddedFlagB);

    status = busB.RemoveSessionMember(sessionId, busA.GetUniqueName());
    EXPECT_EQ(ER_ALLJOYN_REMOVESESSIONMEMBER_NOT_BINDER, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busA.RemoveSessionMember(sessionId, busA.GetUniqueName());
    EXPECT_EQ(ER_ALLJOYN_REMOVESESSIONMEMBER_REPLY_FAILED, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busA.RemoveSessionMember(sessionId, ":Invalid");
    EXPECT_EQ(ER_ALLJOYN_REMOVESESSIONMEMBER_NOT_FOUND, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busA.RemoveSessionMember(sessionId, busB.GetUniqueName());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 2 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 200; ++i) {
        if (sessionLostFlagA && sessionLostFlagB && sessionMemberRemovedFlagA && sessionMemberRemovedFlagB) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(sessionLostFlagA);
    EXPECT_TRUE(sessionLostFlagB);
    EXPECT_TRUE(sessionMemberRemovedFlagA);
    EXPECT_TRUE(sessionMemberRemovedFlagB);

}
