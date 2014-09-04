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

  private:
    BusAttachment& bus;
    SessionListener*sl;



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
        bus.SetHostedSessionListener(id, sl);

    }

  public:
    SessionJoinedSessionPortListener(BusAttachment& _bus, SessionListener*_sl) : bus(_bus), sl(_sl) { }
};

class SessionJoinTestSessionListener : public SessionListener {

  public:
    SessionId lastSessionId;
    int sessionLostCalled;
    SessionListener::SessionLostReason lastReason;

    SessionId sessionMemberAddedSessionId;
    int sessionMemberAddedCalled;
    qcc::String sessionMemberAddedUniqueName;

    SessionId sessionMemberRemovedSessionId;
    int sessionMemberRemovedCalled;
    qcc::String sessionMemberRemovedUniqueName;

    const char*name;

    SessionJoinTestSessionListener(const char*_name) :
        lastSessionId(0),
        sessionLostCalled(0),
        lastReason(ALLJOYN_SESSIONLOST_INVALID),
        sessionMemberAddedSessionId(0),
        sessionMemberAddedCalled(0),
        sessionMemberAddedUniqueName(""),
        sessionMemberRemovedSessionId(0),
        sessionMemberRemovedCalled(0),
        sessionMemberRemovedUniqueName(""),
        name(_name)
    { }


    virtual void SessionLost(SessionId sessionId, SessionLostReason reason) {
        printf("%s SessionLost %" PRIu32 ", reason = %d \r\n",  name, sessionId, reason);
        lastSessionId = sessionId;
        lastReason = reason;
        ++sessionLostCalled;

    }

    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) {
        printf("%s SessionMemberAdded %" PRIu32 ", uniqueName = %s \r\n",  name, sessionId, uniqueName);
        sessionMemberAddedSessionId = sessionId;
        sessionMemberAddedUniqueName = uniqueName;
        ++sessionMemberAddedCalled;

    }

    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        printf("%s SessionMemberRemoved %" PRIu32 ", uniqueName = %s \r\n",  name, sessionId, uniqueName);
        sessionMemberRemovedSessionId = sessionId;
        sessionMemberRemovedUniqueName = uniqueName;
        ++sessionMemberRemovedCalled;
    }

};

static bool SessionJoinLeaveTest(BusAttachment& busHost, BusAttachment& busJoiner, bool joinerLeaves, bool multipoint) {

    QStatus status = ER_FAIL;
    bindMemberSessionId = 0;
    sessionJoinerAcceptedFlag = false;
    sessionJoinedFlag = false;

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, multipoint, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionJoinTestSessionListener sessionListenerHost("host");
    SessionJoinTestSessionListener sessionListenerJoiner("joiner");

    SessionJoinedSessionPortListener sessionPortListener(busHost, &sessionListenerHost);
    SessionPort port = 0;

    status = busHost.BindSessionPort(port, opts, sessionPortListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    SessionId sessionId;

    status = busJoiner.JoinSession(busHost.GetUniqueName().c_str(), port, &sessionListenerJoiner, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(sessionJoinerAcceptedFlag);
    //Wait upto 3 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 300; ++i) {
        if (sessionJoinedFlag) {
            break;
        }
        qcc::Sleep(10);
    }
    qcc::Sleep(10);

    EXPECT_TRUE(sessionJoinedFlag);
    EXPECT_EQ(bindMemberSessionId, sessionId);
    if (&busHost == &busJoiner) { /* self join case */
        EXPECT_STREQ(busHost.GetUniqueName().c_str(), sessionJoinedTestJoiner.c_str()) <<
        "The Joiner name " << sessionJoinedTestJoiner.c_str() <<
        " should be the same as" << busHost.GetUniqueName().c_str();


    } else {
        EXPECT_STRNE(busHost.GetUniqueName().c_str(), sessionJoinedTestJoiner.c_str()) <<
        "The Joiner name " << sessionJoinedTestJoiner.c_str() <<
        " should be different than " << busHost.GetUniqueName().c_str();
    }
    EXPECT_STREQ(busJoiner.GetUniqueName().c_str(), sessionJoinedTestJoiner.c_str()) <<
    "The Joiner name " << sessionJoinedTestJoiner.c_str() <<
    " should be the same as " << busJoiner.GetUniqueName().c_str();;

    if (!multipoint) {
        status = busHost.RemoveSessionMember(sessionId, busJoiner.GetUniqueName());
        EXPECT_EQ(ER_ALLJOYN_REMOVESESSIONMEMBER_NOT_MULTIPOINT, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    SessionJoinTestSessionListener*signalledListener;
    SessionJoinTestSessionListener*notSignalledListener;
    SessionListener::SessionLostReason sessionLostReason;
    if (joinerLeaves) {
        EXPECT_EQ(ER_OK, busJoiner.LeaveJoinedSession(sessionId));
        if (&busHost != &busJoiner) { /* other join case */
            EXPECT_EQ(ER_BUS_NO_SESSION, busJoiner.LeaveHostedSession(sessionId));
        }
        signalledListener = &sessionListenerHost;
        notSignalledListener = &sessionListenerJoiner;
        sessionLostReason = SessionListener::SessionLostReason::ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION;

    } else {
        EXPECT_EQ(ER_OK, busHost.LeaveHostedSession(sessionId));
        if (&busHost != &busJoiner) { /* other join case */
            EXPECT_EQ(ER_BUS_NO_SESSION, busHost.LeaveJoinedSession(sessionId));
        }
        signalledListener = &sessionListenerJoiner;
        notSignalledListener = &sessionListenerHost;
        sessionLostReason = SessionListener::SessionLostReason::ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION;
    }

    qcc::Sleep(100); /* not sure if this is needed */
    EXPECT_EQ(sessionId, signalledListener->lastSessionId);
    EXPECT_EQ(1, signalledListener->sessionLostCalled);
    EXPECT_EQ(sessionLostReason, signalledListener->lastReason);
    EXPECT_EQ(0, notSignalledListener->lastSessionId);
    EXPECT_EQ(0, notSignalledListener->sessionLostCalled);
    EXPECT_EQ(SessionListener::SessionLostReason::ALLJOYN_SESSIONLOST_INVALID, notSignalledListener->lastReason);
    if (multipoint) {
        EXPECT_EQ(sessionId, sessionListenerHost.sessionMemberAddedSessionId);
        EXPECT_EQ(1, sessionListenerHost.sessionMemberAddedCalled);
        EXPECT_STREQ(busJoiner.GetUniqueName().c_str(), sessionListenerHost.sessionMemberAddedUniqueName.c_str());
        EXPECT_EQ(sessionId, sessionListenerJoiner.sessionMemberAddedSessionId);
        EXPECT_EQ(1, sessionListenerJoiner.sessionMemberAddedCalled);
        EXPECT_STREQ(busHost.GetUniqueName().c_str(), sessionListenerJoiner.sessionMemberAddedUniqueName.c_str());

        EXPECT_EQ(sessionId, signalledListener->sessionMemberRemovedSessionId);
        EXPECT_EQ(1, signalledListener->sessionMemberRemovedCalled);
        EXPECT_STREQ(joinerLeaves ? busJoiner.GetUniqueName().c_str() : busHost.GetUniqueName().c_str(), signalledListener->sessionMemberRemovedUniqueName.c_str());
        EXPECT_EQ(0, notSignalledListener->sessionMemberRemovedSessionId);
        EXPECT_EQ(0, notSignalledListener->sessionMemberRemovedCalled);
        EXPECT_STREQ("", notSignalledListener->sessionMemberRemovedUniqueName.c_str());
    }

    qcc::Sleep(200); /* let all callbacks finish */

    return true;
}

//ALLJOYN-1602
TEST_F(SessionTest, SessionJoined) {
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

    /* other-join */
    printf("Other join PP - joiner leaves\n");
    EXPECT_TRUE(SessionJoinLeaveTest(busA, busB, true, false));
    printf("Other join PP - host leaves\n");
    EXPECT_TRUE(SessionJoinLeaveTest(busA, busB, false, false));

    printf("Other join MP - joiner leaves\n");
    EXPECT_TRUE(SessionJoinLeaveTest(busA, busB, true, true));
    printf("Other join MP - host leaves\n");
    EXPECT_TRUE(SessionJoinLeaveTest(busA, busB, false, true));


    /* self-join */
    printf("self join PP- 'joiner' leaves");
    EXPECT_TRUE(SessionJoinLeaveTest(busA, busA, true, false));
    printf("self join PP- 'host' leaves\n");
    EXPECT_TRUE(SessionJoinLeaveTest(busA, busA, false, false));

    printf("self join MP- 'joiner' leaves");
    EXPECT_TRUE(SessionJoinLeaveTest(busA, busA, true, true));
    printf("self join MP- 'host' leaves\n");
    EXPECT_TRUE(SessionJoinLeaveTest(busA, busA, false, true));
}

bool sessionLostFlagA;
bool sessionLostFlagB;
static int sessionMemberAddedCounter;
static int sessionMemberRemovedCounter;
static int sessionLostCounter;
static int sessionJoinedCounter;

class RemoveSessionMemberBusAListener : public SessionPortListener, public SessionListener {
  public:
    RemoveSessionMemberBusAListener(BusAttachment* bus) : bus(bus) {
    }

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
        ++sessionJoinedCounter;
        bus->SetSessionListener(id, this);
    }
    virtual void SessionLost(SessionId sessionId, SessionLostReason reason) {
        printf("Session lost SessionId=%d, reason=%d\n", sessionId, reason);
        sessionLostFlagA = true;
        ++sessionLostCounter;
    }
    virtual void SessionMemberAdded(SessionId sessionId, const char* uniqueName) {
        printf("Session member added SessionId=%d, uniqueName=%s\n", sessionId, uniqueName);
        sessionMemberAddedFlagA = true;
        ++sessionMemberAddedCounter;
    }
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        printf("Session member removed SessionId=%d, uniqueName=%s\n",  sessionId, uniqueName);
        sessionMemberRemovedFlagA = true;
        ++sessionMemberRemovedCounter;
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
    sessionJoinerAcceptedFlag = false;

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
    EXPECT_EQ(ER_ALLJOYN_REMOVESESSIONMEMBER_NOT_FOUND, status) << "  Actual Status: " << QCC_StatusText(status);

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

static void MultipointMultipeerTest(BusAttachment& busHost, BusAttachment& busJoiner, BusAttachment& busJoiner2, bool joinerLeaves) {

    assert(&busJoiner != &busJoiner2); /* this would not make sense for this test */

    QStatus status = ER_FAIL;
    bindMemberSessionId = 0;
    sessionJoinerAcceptedFlag = false;
    sessionJoinedFlag = false;

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionJoinTestSessionListener sessionListenerHost("host");
    SessionJoinTestSessionListener sessionListenerJoiner("joiner");
    SessionJoinTestSessionListener sessionListenerJoiner2("joiner2");

    SessionJoinedSessionPortListener sessionPortListener(busHost, &sessionListenerHost);
    SessionPort port = 0;

    status = busHost.BindSessionPort(port, opts, sessionPortListener);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    SessionId sessionId;

    /* Joiner 1 */
    status = busJoiner.JoinSession(busHost.GetUniqueName().c_str(), port, &sessionListenerJoiner, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(sessionJoinerAcceptedFlag);
    //Wait upto 3 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 300; ++i) {
        if (sessionJoinedFlag) {
            break;
        }
        qcc::Sleep(10);
    }
    qcc::Sleep(10);

    EXPECT_TRUE(sessionJoinedFlag);
    EXPECT_EQ(sessionId, bindMemberSessionId);

    sessionJoinerAcceptedFlag = false;
    sessionJoinedFlag = false;

    EXPECT_STREQ(busJoiner.GetUniqueName().c_str(), sessionListenerHost.sessionMemberAddedUniqueName.c_str());
    EXPECT_EQ(1, sessionListenerHost.sessionMemberAddedCalled);
    EXPECT_STREQ(busHost.GetUniqueName().c_str(), sessionListenerJoiner.sessionMemberAddedUniqueName.c_str());
    EXPECT_EQ(1, sessionListenerJoiner.sessionMemberAddedCalled);
    EXPECT_STREQ("", sessionListenerJoiner2.sessionMemberAddedUniqueName.c_str());
    EXPECT_EQ(0, sessionListenerJoiner2.sessionMemberAddedCalled);

    printf("joiner 2\n");
    /* joiner 2 */
    status = busJoiner2.JoinSession(busHost.GetUniqueName().c_str(), port, &sessionListenerJoiner2, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait upto 3 seconds all callbacks and listeners to be called.
    for (int i = 0; i < 300; ++i) {
        if (sessionJoinedFlag) {
            break;
        }
        qcc::Sleep(10);
    }
    qcc::Sleep(100);

    EXPECT_STREQ(busJoiner2.GetUniqueName().c_str(), sessionListenerHost.sessionMemberAddedUniqueName.c_str());
    EXPECT_EQ(2, sessionListenerHost.sessionMemberAddedCalled);
    EXPECT_STREQ(busJoiner2.GetUniqueName().c_str(), sessionListenerJoiner.sessionMemberAddedUniqueName.c_str());
    EXPECT_EQ(2, sessionListenerJoiner.sessionMemberAddedCalled);
    EXPECT_EQ(2, sessionListenerJoiner2.sessionMemberAddedCalled);


    if (joinerLeaves) {
        printf("joiner leaving\n");
        EXPECT_EQ(ER_OK, busJoiner.LeaveJoinedSession(sessionId));
        qcc::Sleep(200);

        EXPECT_EQ(sessionId, sessionListenerHost.sessionMemberRemovedSessionId);
        EXPECT_EQ(0, sessionListenerJoiner.sessionMemberRemovedSessionId);
        EXPECT_EQ(sessionId, sessionListenerJoiner2.sessionMemberRemovedSessionId);

        EXPECT_STREQ(busJoiner.GetUniqueName().c_str(), sessionListenerHost.sessionMemberRemovedUniqueName.c_str());
        EXPECT_STREQ("", sessionListenerJoiner.sessionMemberRemovedUniqueName.c_str());
        EXPECT_STREQ(busJoiner.GetUniqueName().c_str(), sessionListenerJoiner2.sessionMemberRemovedUniqueName.c_str());

        EXPECT_EQ(1, sessionListenerHost.sessionMemberRemovedCalled);
        EXPECT_EQ(0, sessionListenerJoiner.sessionMemberRemovedCalled);
        EXPECT_EQ(1, sessionListenerJoiner2.sessionMemberRemovedCalled);
    } else {
        printf("host leaving\n");
        EXPECT_EQ(ER_OK, busHost.LeaveHostedSession(sessionId));
        qcc::Sleep(200);

        EXPECT_EQ(0, sessionListenerHost.sessionMemberRemovedSessionId);
        EXPECT_EQ(sessionId, sessionListenerJoiner.sessionMemberRemovedSessionId);
        EXPECT_EQ(sessionId, sessionListenerJoiner2.sessionMemberRemovedSessionId);

        EXPECT_STREQ("", sessionListenerHost.sessionMemberRemovedUniqueName.c_str());
        EXPECT_STREQ(busHost.GetUniqueName().c_str(), sessionListenerJoiner.sessionMemberRemovedUniqueName.c_str());
        EXPECT_STREQ(busHost.GetUniqueName().c_str(), sessionListenerJoiner2.sessionMemberRemovedUniqueName.c_str());

        EXPECT_EQ(0, sessionListenerHost.sessionMemberRemovedCalled);
        EXPECT_EQ(1, sessionListenerJoiner.sessionMemberRemovedCalled);
        EXPECT_EQ(1, sessionListenerJoiner2.sessionMemberRemovedCalled);
    }

    /* clean up the session to avoid callbacks after this function returns */
    busHost.LeaveSession(sessionId);
    busJoiner.LeaveSession(sessionId);
    busJoiner2.LeaveSession(sessionId);

    qcc::Sleep(100); /* let all callbacks finish */

}

TEST_F(SessionTest, MultipointExtended) {

    BusAttachment busA("busAA", false);
    BusAttachment busB("busBB", false);
    BusAttachment busC("busCC", false);

    QStatus status = busA.Start();
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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


    printf("A - B , C as 2nd joiner, B leaves\n");
    MultipointMultipeerTest(busA, busB, busC, true);


    printf("A - B , C as 2nd joiner, A leaves\n");
    MultipointMultipeerTest(busA, busB, busC, false);

    printf("A - B , A as 2nd joiner (self-join), B leaves r\n");
    MultipointMultipeerTest(busA, busB, busA, true);

    printf("A - A , B as 2nd joiner (self-join), A leaves as joiner\n");
    MultipointMultipeerTest(busA, busA, busB, true);

    printf("A - A , B as 2nd joiner (self-join), A leaves as host\n");
    MultipointMultipeerTest(busA, busA, busB, false);

}
