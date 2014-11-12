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

class TestObject : public BusObject {
  public:
    TestObject(BusAttachment& bus)
        : BusObject("/signals/test"), bus(bus) {
        const InterfaceDescription* intf1 = bus.GetInterface("org.test");
        EXPECT_TRUE(intf1 != NULL) << "bus.GetInterface(\"org.test\") returned null";
        if (intf1 == NULL) {
            return;
        }
        AddInterface(*intf1);
    }

    virtual ~TestObject() { }

    QStatus SendSignal(const char* dest, SessionId id, uint8_t flags = 0) {
        const InterfaceDescription* intf1 = bus.GetInterface("org.test");
        EXPECT_TRUE(intf1 != NULL);
        if (intf1 == NULL) {
            return ER_FAIL;
        }
        const InterfaceDescription::Member*  signal_member = intf1->GetMember("my_signal");
        MsgArg arg("s", "Signal");
        QStatus status = Signal(dest, id, *signal_member, &arg, 1, 0, flags);
        return status;
    }

    BusAttachment& bus;
};

class Participant : public SessionPortListener, public SessionListener {
  public:
    SessionPort port;
    SessionPort mpport;

    BusAttachment bus;
    String name;
    TestObject* busobj;

    SessionOpts opts;
    SessionOpts mpopts;

    typedef std::map<std::pair<qcc::String, bool>, SessionId> SessionMap;
    SessionMap hostedSessionMap;
    SessionMap joinedSessionMap;

    Participant() : port(42), mpport(84), bus("Participant"), name(genUniqueName(bus)),
        opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY),
        mpopts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY)
    {
        Init();
    }

    void Init() {
        QStatus status;

        ASSERT_EQ(ER_OK, bus.Start());
        ASSERT_EQ(ER_OK, bus.Connect(getConnectArg().c_str()));

        ASSERT_EQ(ER_OK, bus.BindSessionPort(port, opts, *this));
        ASSERT_EQ(ER_OK, bus.BindSessionPort(mpport, mpopts, *this));

        ASSERT_EQ(ER_OK, bus.RequestName(name.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE));
        ASSERT_EQ(ER_OK, bus.AdvertiseName(name.c_str(), TRANSPORT_ANY));

        /* create test interface */
        InterfaceDescription* servicetestIntf = NULL;
        status = bus.CreateInterface("org.test", servicetestIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(servicetestIntf != NULL);
        status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        servicetestIntf->Activate();

        /* create bus object */
        busobj = new TestObject(bus);
        bus.RegisterBusObject(*busobj);
    }

    ~Participant() {
        Fini();
    }

    void Fini() {
        bus.UnregisterBusObject(*busobj);
        delete busobj;
        ASSERT_EQ(ER_OK, bus.Disconnect());
        ASSERT_EQ(ER_OK, bus.Stop());
        ASSERT_EQ(ER_OK, bus.Join());
    }

    SessionMap::key_type SessionMapKey(qcc::String participant, bool multipoint) {
        return std::make_pair(participant, multipoint);
    }

    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
    virtual void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {
        hostedSessionMap[SessionMapKey(joiner, (sessionPort == mpport))] = id;
        bus.SetHostedSessionListener(id, this);
    }
    virtual void SessionLost(SessionId sessionId, SessionLostReason reason) {
        /* we only set a session listener on the hosted sessions */
        SessionMap::iterator iter = hostedSessionMap.begin();
        while (iter != hostedSessionMap.end()) {
            if (iter->second == sessionId) {
                hostedSessionMap.erase(iter);
                iter = hostedSessionMap.begin();
            } else {
                ++iter;
            }
        }
    }
    virtual void SessionMemberRemoved(SessionId sessionId, const char* uniqueName) {
        /* we only set a session listener on the hosted sessions */
        SessionMap::iterator iter = hostedSessionMap.begin();
        while (iter != hostedSessionMap.end()) {
            if (iter->first.first == qcc::String(uniqueName) && iter->second == sessionId) {
                hostedSessionMap.erase(iter);
                iter = hostedSessionMap.begin();
            } else {
                ++iter;
            }
        }
    }

    void AddMatch() {
        QStatus status = bus.AddMatch("type='signal',interface='org.test',member='my_signal'");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    void RemoveMatch() {
        QStatus status = bus.RemoveMatch("type='signal',interface='org.test',member='my_signal'");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    void JoinSession(Participant& part, bool multipoint) {
        SessionId outIdA;
        SessionOpts outOptsA;
        SessionPort p = multipoint ? mpport : port;
        ASSERT_EQ(ER_OK, bus.JoinSession(part.name.c_str(), p, NULL, outIdA, multipoint ? mpopts : opts));
        joinedSessionMap[SessionMapKey(part.name.c_str(), multipoint)] = outIdA;

        /* make sure both sides know we're in session before we continue */
        int count = 0;
        while (part.hostedSessionMap.find(SessionMapKey(bus.GetUniqueName(), multipoint)) == part.hostedSessionMap.end()) {
            qcc::Sleep(10);
            if (++count > 200) {
                ADD_FAILURE() << "JoinSession: joiner got OK reply, but host did not receive SessionJoined.";
                break;
            }
        }
    }

    void LeaveSession(Participant& part, bool multipoint) {
        SessionMap::key_type key = SessionMapKey(part.name.c_str(), multipoint);
        SessionId id = joinedSessionMap[key];
        joinedSessionMap.erase(key);
        ASSERT_EQ(ER_OK, bus.LeaveJoinedSession(id));

        /* make sure both sides know the session is lost before we continue */
        int count = 0;
        while (part.hostedSessionMap.find(SessionMapKey(bus.GetUniqueName(), multipoint)) != part.hostedSessionMap.end()) {
            qcc::Sleep(10);
            if (++count > 200) {
                ADD_FAILURE() << "LeaveSession: joiner got OK reply, but host did not receive SessionLost.";
                break;
            }
        }
    }

    SessionId GetJoinedSessionId(Participant& part, bool multipoint) {
        return joinedSessionMap[SessionMapKey(part.name.c_str(), multipoint)];
    }
  private:
    //Private copy constructor to prevent copying the class and double freeing of memory
    Participant(const Participant& rhs) : bus("Participant") { }
    //Private assignment operator to prevent copying the class and double freeing of memory
    Participant& operator=(const Participant& rhs) { return *this; }
};

class SignalReceiver : public MessageReceiver {
  protected:
    Participant* participant;
    int signalReceived;
  public:
    SignalReceiver() : signalReceived(0) { }
    virtual ~SignalReceiver() { }

    void Register(Participant* part) {
        participant = part;

        const InterfaceDescription* servicetestIntf = part->bus.GetInterface("org.test");
        EXPECT_TRUE(servicetestIntf != NULL) << "bus.GetInterface(\"org.test\") returned NULL";
        if (servicetestIntf == NULL) {
            return;
        }
        const InterfaceDescription::Member* signal_member = servicetestIntf->GetMember("my_signal");
        EXPECT_TRUE(signal_member != NULL) << "intf->GetMember(\"my_signal\") returned NULL";
        if (signal_member == NULL) {
            return;
        }
        RegisterSignalHandler(signal_member);
    }

    /* override this for non-standard signal subscriptions */
    virtual void RegisterSignalHandler(const InterfaceDescription::Member* member) {
        QStatus status = participant->bus.RegisterSignalHandler(this,
                                                                static_cast<MessageReceiver::SignalHandler>(&SignalReceiver::SignalHandler),
                                                                member,
                                                                NULL);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    void SignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg) {
        signalReceived++;
    }

    void verify_recv(int expected = 1) {
// #define PRINT
#ifdef PRINT
        fprintf(stderr, "VERIFICATION: FOR %s EXPECTED %d\n", participant->name.c_str(), expected);
#endif
        EXPECT_EQ(expected, signalReceived);
        signalReceived = 0;
    }

    void verify_norecv() {
        verify_recv(0);
    }

};

class PathReceiver : public SignalReceiver {
    qcc::String senderpath;
  public:
    PathReceiver(const char* path) : SignalReceiver(), senderpath(path) { }
    virtual ~PathReceiver() { }

    virtual void RegisterSignalHandler(const InterfaceDescription::Member* member) {
        QStatus status = participant->bus.RegisterSignalHandler(this,
                                                                static_cast<MessageReceiver::SignalHandler>(&PathReceiver::SignalHandler),
                                                                member,
                                                                senderpath.c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }
};

class RuleReceiver : public SignalReceiver {
    qcc::String matchRule;
  public:
    RuleReceiver(const char* rule) : SignalReceiver(), matchRule(rule) { }
    virtual ~RuleReceiver() { }

    virtual void RegisterSignalHandler(const InterfaceDescription::Member* member) {
        QStatus status = participant->bus.RegisterSignalHandlerWithRule(this,
                                                                        static_cast<MessageReceiver::SignalHandler>(&RuleReceiver::SignalHandler),
                                                                        member,
                                                                        matchRule.c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }
};

class SignalTest : public testing::Test {
  public:
    virtual void SetUp() { }
    virtual void TearDown() { }
};

void wait_for_signal()
{
    qcc::Sleep(1000);
}

TEST_F(SignalTest, Point2PointSimple)
{
    Participant A;
    Participant B;
    SignalReceiver recvA, recvB;
    recvA.Register(&A);
    recvB.Register(&B);

    B.JoinSession(A, false);

    /* unicast signal */
    B.busobj->SendSignal(A.name.c_str(), 0, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_norecv();

    A.busobj->SendSignal(B.name.c_str(), 0, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_recv();

    /* dbus broadcast signal */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, 0);
    A.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_norecv();

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    B.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();
    A.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();
    A.RemoveMatch();
    B.RemoveMatch();

    /* global broadcast signal */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_norecv();

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();
    A.RemoveMatch();
    B.RemoveMatch();

    /* sessioncast */
    B.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, false), 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_norecv();
    A.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, false), 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_recv();

    /* sessioncast on all sessions */
    B.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_norecv();
    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_recv();

    B.LeaveSession(A, false);
}

TEST_F(SignalTest, MultiPointSimple)
{
    Participant A;
    Participant B;
    Participant C;
    SignalReceiver recvA, recvB, recvC;
    recvA.Register(&A);
    recvB.Register(&B);
    recvC.Register(&C);

    B.JoinSession(A, true);
    C.JoinSession(A, true);

    /* unicast signal */
    B.busobj->SendSignal(A.name.c_str(), 0, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_norecv();
    recvC.verify_norecv();

    A.busobj->SendSignal(B.name.c_str(), 0, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_recv();
    recvC.verify_norecv();

    /* dbus broadcast signal */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, 0);
    A.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_norecv();
    recvC.verify_norecv();

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    C.AddMatch();
    B.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();
    recvC.verify_recv();
    A.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();
    recvC.verify_recv();
    A.RemoveMatch();
    B.RemoveMatch();
    C.RemoveMatch();

    /* global broadcast signal */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_norecv();
    recvC.verify_norecv();

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    C.AddMatch();
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();
    recvC.verify_recv();
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();
    recvC.verify_recv();
    A.RemoveMatch();
    B.RemoveMatch();
    C.RemoveMatch();

    /* sessioncast */
    B.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, true), 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_norecv();
    recvC.verify_recv();
    A.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, true), 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_recv();
    recvC.verify_recv();

    /* sessioncast on all hosted sessions */
    B.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_norecv();
    recvC.verify_norecv();
    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_recv();
    recvC.verify_recv();

    B.LeaveSession(A, true);
    C.LeaveSession(A, true);
}

TEST_F(SignalTest, Point2PointComplex) {
    Participant A;
    Participant B;
    Participant C;
    SignalReceiver recvA, recvB, recvC;
    recvA.Register(&A);
    recvB.Register(&B);
    recvC.Register(&C);

    /* x -> y means "x hosts p2p session for y" */

    /* A -> B, B -> C, C -> A */
    B.JoinSession(A, false);
    C.JoinSession(B, false);
    A.JoinSession(C, false);

    /* sessioncast on all hosted sessions */
    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_recv();
    recvC.verify_norecv();

    /* A -> B, A -> C, B -> C, C -> A */
    C.JoinSession(A, false);
    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_recv();
    recvC.verify_recv();

    B.LeaveSession(A, false);
    C.LeaveSession(A, false);
    C.LeaveSession(B, false);
}

TEST_F(SignalTest, MultiSession) {
    Participant A;
    Participant B;
    SignalReceiver recvA, recvB;
    recvA.Register(&A);
    recvB.Register(&B);

    /* Enter in 2 sessions with A */
    B.JoinSession(A, false);
    B.JoinSession(A, true);
    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    /* verify B received the signal twice */
    recvA.verify_norecv();
    recvB.verify_recv(2);

    /* leave one of the sessions */
    B.LeaveSession(A, false);
    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    /* verify B still received the signal */
    recvA.verify_norecv();
    recvB.verify_recv();

    /* leave the last session */
    B.LeaveSession(A, true);
    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    /* verify B did not received the signal */
    recvA.verify_norecv();
    recvB.verify_norecv();
}

TEST_F(SignalTest, SelfJoinPointToPoint) {
    Participant A;
    Participant B;
    SignalReceiver recvA, recvB;
    recvA.Register(&A);
    recvB.Register(&B);

    B.JoinSession(A, false);
    A.JoinSession(A, false);

    A.busobj->SendSignal(A.name.c_str(), 0, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_norecv();

    B.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, false), 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_norecv();

    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();

    B.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_norecv();

    B.LeaveSession(A, false);
    A.LeaveSession(A, false);
}

TEST_F(SignalTest, SelfJoinMultiPoint) {
    Participant A;
    Participant B;
    SignalReceiver recvA, recvB;
    recvA.Register(&A);
    recvB.Register(&B);

    B.JoinSession(A, true);
    A.JoinSession(A, true);

    A.busobj->SendSignal(A.name.c_str(), 0, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_norecv();

    B.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, true), 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_norecv();

    A.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_recv();
    recvB.verify_recv();

    B.busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    wait_for_signal();
    recvA.verify_norecv();
    recvB.verify_norecv();

    B.LeaveSession(A, true);
    A.LeaveSession(A, true);
}

TEST_F(SignalTest, Paths) {
    Participant A;
    Participant B;
    PathReceiver recvAy("/signals/test");
    PathReceiver recvAn("/not/right");
    PathReceiver recvBy("/signals/test");
    PathReceiver recvBn("/not/right");
    recvAy.Register(&A);
    recvAn.Register(&A);
    recvBy.Register(&B);
    recvBn.Register(&B);

    B.JoinSession(A, false);
    A.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, false), 0);
    B.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, false), 0);
    wait_for_signal();
    recvAy.verify_recv();
    recvBy.verify_recv();
    recvAn.verify_norecv();
    recvBn.verify_norecv();
}

TEST_F(SignalTest, Rules) {
    Participant A;
    Participant B;
    RuleReceiver recvAy("type='signal'");
    RuleReceiver recvAn("type='signal',member='nonexistent'");
    RuleReceiver recvBy("type='signal'");
    RuleReceiver recvBn("type='signal',member='nonexistent'");
    recvAy.Register(&A);
    recvAn.Register(&A);
    recvBy.Register(&B);
    recvBn.Register(&B);

    B.JoinSession(A, false);
    A.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, false), 0);
    B.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, false), 0);
    wait_for_signal();
    recvAy.verify_recv();
    recvBy.verify_recv();
    recvAn.verify_norecv();
    recvBn.verify_norecv();
}
