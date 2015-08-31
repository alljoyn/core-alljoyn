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
#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>
#include <memory>

#include <qcc/String.h>
#include <qcc/time.h>
#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include <qcc/Condition.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

#include "TestSecureApplication.h"
#include "TestSecurityManager.h"

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "ajTestCommon.h"

using namespace std;
using namespace qcc;
using namespace ajn;

const uint32_t SLEEP_TIME = 2000;
const uint32_t BACKPRESSURE_TEST_NUM_SIGNALS = 12;

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

    Participant(qcc::String connectArg = "", uint8_t annotation = 0, InterfaceDescription** intf = NULL) : port(42), mpport(84), bus("Participant"), name(genUniqueName(bus)),
        opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY),
        mpopts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY),
        inited(false)
    {
        Init(connectArg, annotation, intf);
    }

    void Init(qcc::String connectArg, uint8_t annotation, InterfaceDescription** intf)
    {
        QStatus status;

        if (connectArg == "") {
            connectArg = getConnectArg();
        }
        ASSERT_EQ(ER_OK, bus.Start());
        status = bus.Connect(connectArg.c_str());

        if ((status != ER_OK) && (connectArg == "null:")) {
            printf("Skipping test. Could not connect to Null transport.\n");
            return;
        }

        ASSERT_EQ(ER_OK, status);

        ASSERT_EQ(ER_OK, bus.BindSessionPort(port, opts, *this));
        ASSERT_EQ(ER_OK, bus.BindSessionPort(mpport, mpopts, *this));

        ASSERT_EQ(ER_OK, bus.RequestName(name.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE));
        ASSERT_EQ(ER_OK, bus.AdvertiseName(name.c_str(), TRANSPORT_ANY));

        /* create test interface */
        InterfaceDescription* servicetestIntf = NULL;
        status = bus.CreateInterface("org.test", servicetestIntf);
        EXPECT_EQ(ER_OK, status);
        ASSERT_TRUE(servicetestIntf != NULL);
        status = servicetestIntf->AddSignal("my_signal", "s", NULL, annotation);
        EXPECT_EQ(ER_OK, status);
        if (intf == nullptr) {
            servicetestIntf->Activate();
            CreateBusObject();
        } else {
            *intf = servicetestIntf;
        }
    }

    void CreateBusObject()
    {
        busobj = new TestObject(bus);
        bus.RegisterBusObject(*busobj);
        inited = true;
        return;
    }

    ~Participant() {
        Fini();
    }

    void Fini() {
        if (inited) {
            bus.UnregisterBusObject(*busobj);
            delete busobj;
            ASSERT_EQ(ER_OK, bus.Disconnect());
        }
        ASSERT_EQ(ER_OK, bus.Stop());
        ASSERT_EQ(ER_OK, bus.Join());
    }

    SessionMap::key_type SessionMapKey(qcc::String participant, bool multipoint) {
        return std::make_pair(participant, multipoint);
    }

    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& sessionOpts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(sessionOpts);
        return true;
    }
    virtual void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {
        hostedSessionMap[SessionMapKey(joiner, (sessionPort == mpport))] = id;
        bus.SetHostedSessionListener(id, this);
    }
    virtual void SessionLost(SessionId sessionId, SessionLostReason reason) {
        QCC_UNUSED(reason);
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
        EXPECT_EQ(ER_OK, status);
    }

    void RemoveMatch() {
        QStatus status = bus.RemoveMatch("type='signal',interface='org.test',member='my_signal'");
        EXPECT_EQ(ER_OK, status);
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

    QStatus SendSessioncastSignal()
    {
        return busobj->SendSignal(NULL, SESSION_ID_ALL_HOSTED, 0);
    }

    QStatus SendSessionlessSignal()
    {
        return busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_SESSIONLESS);
    }

    QStatus SendUnicastSignal()
    {
        return busobj->SendSignal(name.c_str(), 0, 0);
    }

    QStatus SendGlobalBroadcastSignal()
    {
        return busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    }

    SessionId GetJoinedSessionId(Participant& part, bool multipoint) {
        return joinedSessionMap[SessionMapKey(part.name.c_str(), multipoint)];
    }
    bool inited;
  private:
    //Private copy constructor to prevent copying the class and double freeing of memory
    Participant(const Participant& rhs) : bus("Participant") {
        QCC_UNUSED(rhs);
    }
    //Private assignment operator to prevent copying the class and double freeing of memory
    Participant& operator=(const Participant&) { return *this; }
};

class SignalReceiver : public MessageReceiver {
  protected:
    Participant* participant;
    int signalReceived;
    bool blocking;
  public:
    SignalReceiver(bool blocking = false) : signalReceived(0), blocking(blocking) { }
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
        EXPECT_EQ(ER_OK, status);
    }

    void SignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg) {
        QCC_UNUSED(member);
        QCC_UNUSED(sourcePath);
        QCC_UNUSED(msg);

        if (blocking) {
            blocking = false;
            qcc::Sleep(SLEEP_TIME);
        }
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
    PathReceiver(const char* path, bool blocking = false) : SignalReceiver(blocking), senderpath(path) { }
    virtual ~PathReceiver() { }

    virtual void RegisterSignalHandler(const InterfaceDescription::Member* member) {
        QStatus status = participant->bus.RegisterSignalHandler(this,
                                                                static_cast<MessageReceiver::SignalHandler>(&PathReceiver::SignalHandler),
                                                                member,
                                                                senderpath.c_str());
        EXPECT_EQ(ER_OK, status);
    }
};

class RuleReceiver : public SignalReceiver {
    qcc::String matchRule;
  public:
    RuleReceiver(const char* rule, bool blocking = false) : SignalReceiver(blocking), matchRule(rule) { }
    virtual ~RuleReceiver() { }

    virtual void RegisterSignalHandler(const InterfaceDescription::Member* member) {
        QStatus status = participant->bus.RegisterSignalHandlerWithRule(this,
                                                                        static_cast<MessageReceiver::SignalHandler>(&RuleReceiver::SignalHandler),
                                                                        member,
                                                                        matchRule.c_str());
        EXPECT_EQ(ER_OK, status);
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

/* This is a blocking test. The idea is to send out 12 signals, the first signal handler
   will sleep for SLEEP_TIME, as a result of which the SendSignal should block for approx
   SLEEP_TIME ms until that signal handler returns.
 */
TEST_F(SignalTest, BackPressure) {
    Participant A("null:");
    Participant B("null:");

    if (A.inited && B.inited) {
        // Set blocking to true.
        PathReceiver recvBy("/signals/test", true);
        recvBy.Register(&B);

        B.JoinSession(A, false);
        uint64_t start_time = qcc::GetTimestamp64();
        for (uint32_t i = 0; i < BACKPRESSURE_TEST_NUM_SIGNALS; i++) {
            A.busobj->SendSignal(NULL, B.GetJoinedSessionId(A, false), 0);
        }
        uint64_t elapsed = qcc::GetTimestamp64() - start_time;

        EXPECT_GE(elapsed, SLEEP_TIME - QCC_TIMESTAMP_GRANULARITY);
        wait_for_signal();
        recvBy.verify_recv(BACKPRESSURE_TEST_NUM_SIGNALS);
    }
}

QStatus SetMemberDescription(InterfaceDescription* intf)
{
    return intf->SetMemberDescription("my_signal", "my_signal description");
}

// Test the deprecated API to set a member description and mark a signal
// a sessionless at the same time.
QStatus SetSessionlessMemberDescription(InterfaceDescription* intf)
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
    return intf->SetMemberDescription("my_signal", "my_signal description", true);
#if defined(QCC_OS_GROUP_WINDOWS)
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

TEST_F(SignalTest, SignalTypeEnforcement)
{
    InterfaceDescription* legacyNonSessionlessIntf;
    InterfaceDescription* legacySessionlessIntf;
    InterfaceDescription* sessioncastIntf;
    InterfaceDescription* sessionlessIntf;
    InterfaceDescription* unicastIntf;
    InterfaceDescription* globalBroadcastIntf;

    // Create a participant to test each signal type.
    Participant legacyParticipant;
    Participant legacyNonSessionlessParticipant("", 0, &legacyNonSessionlessIntf);
    Participant legacySessionlessParticipant("", 0, &legacySessionlessIntf);
    Participant sessioncastParticipant("", MEMBER_ANNOTATE_SESSIONCAST, &sessioncastIntf);
    Participant sessionlessParticipant("", MEMBER_ANNOTATE_SESSIONLESS, &sessionlessIntf);
    Participant unicastParticipant("", MEMBER_ANNOTATE_UNICAST, &unicastIntf);
    Participant globalBroadcastParticipant("", MEMBER_ANNOTATE_GLOBAL_BROADCAST, &globalBroadcastIntf);

    // Try to add a simple signal description to all interfaces
    // except legacyIntf and legacySessionlessIntf.
    ASSERT_EQ(ER_OK, SetMemberDescription(legacyNonSessionlessIntf));
    ASSERT_EQ(ER_OK, SetMemberDescription(sessioncastIntf));
    ASSERT_EQ(ER_OK, SetMemberDescription(sessionlessIntf));
    ASSERT_EQ(ER_OK, SetMemberDescription(unicastIntf));
    ASSERT_EQ(ER_OK, SetMemberDescription(globalBroadcastIntf));

    // Try to add an obsolete sessionless signal description to all interfaces
    // except legacyIntf and legacyNonSessionlessIntf.
    ASSERT_EQ(ER_OK, SetSessionlessMemberDescription(legacySessionlessIntf));
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, SetSessionlessMemberDescription(sessioncastIntf));
    ASSERT_EQ(ER_OK, SetSessionlessMemberDescription(sessionlessIntf));
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, SetSessionlessMemberDescription(unicastIntf));
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, SetSessionlessMemberDescription(globalBroadcastIntf));

    // Activate interfaces.
    legacyNonSessionlessIntf->Activate();
    legacySessionlessIntf->Activate();
    sessioncastIntf->Activate();
    sessionlessIntf->Activate();
    unicastIntf->Activate();
    globalBroadcastIntf->Activate();

    // Finish initializing the participant objects.
    legacyNonSessionlessParticipant.CreateBusObject();
    legacySessionlessParticipant.CreateBusObject();
    sessioncastParticipant.CreateBusObject();
    sessionlessParticipant.CreateBusObject();
    unicastParticipant.CreateBusObject();
    globalBroadcastParticipant.CreateBusObject();

    // Verify that legacy code is unaffected.
    ASSERT_EQ(ER_OK, legacyParticipant.SendSessioncastSignal());
    ASSERT_EQ(ER_OK, legacyParticipant.SendSessionlessSignal());
    ASSERT_EQ(ER_OK, legacyParticipant.SendUnicastSignal());
    ASSERT_EQ(ER_OK, legacyParticipant.SendGlobalBroadcastSignal());

    ASSERT_EQ(ER_OK, legacyNonSessionlessParticipant.SendSessioncastSignal());
    ASSERT_EQ(ER_OK, legacyNonSessionlessParticipant.SendSessionlessSignal());
    ASSERT_EQ(ER_OK, legacyNonSessionlessParticipant.SendUnicastSignal());
    ASSERT_EQ(ER_OK, legacyNonSessionlessParticipant.SendGlobalBroadcastSignal());

    // Verify that any legacy caller that explicitly set SetMemberDescription
    // with isSessionless=true will fail if it tries to send a signal of
    // another type.
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, legacySessionlessParticipant.SendSessioncastSignal());
    ASSERT_EQ(ER_OK, legacySessionlessParticipant.SendSessionlessSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, legacySessionlessParticipant.SendUnicastSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, legacySessionlessParticipant.SendGlobalBroadcastSignal());

    // Verify that each member explicitly marked with one signal type cannot
    // send signals of other types.
    ASSERT_EQ(ER_OK, sessioncastParticipant.SendSessioncastSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, sessioncastParticipant.SendSessionlessSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, sessioncastParticipant.SendUnicastSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, sessioncastParticipant.SendGlobalBroadcastSignal());

    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, sessionlessParticipant.SendSessioncastSignal());
    ASSERT_EQ(ER_OK, sessionlessParticipant.SendSessionlessSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, sessionlessParticipant.SendUnicastSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, sessionlessParticipant.SendGlobalBroadcastSignal());

    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, unicastParticipant.SendSessioncastSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, unicastParticipant.SendSessionlessSignal());
    ASSERT_EQ(ER_OK, unicastParticipant.SendUnicastSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, unicastParticipant.SendGlobalBroadcastSignal());

    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, globalBroadcastParticipant.SendSessioncastSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, globalBroadcastParticipant.SendSessionlessSignal());
    ASSERT_EQ(ER_INVALID_SIGNAL_EMISSION_TYPE, globalBroadcastParticipant.SendUnicastSignal());
    ASSERT_EQ(ER_OK, globalBroadcastParticipant.SendGlobalBroadcastSignal());
}

class SecSignalTest :
    public::testing::Test,
    public MessageReceiver {
  public:
    SecSignalTest() : prov("provider"), cons("consumer"), proxy(NULL), evented(false), lastValue(false) { }
    ~SecSignalTest() { }

    TestSecurityManager tsm;
    TestSecureApplication prov;
    TestSecureApplication cons;
    shared_ptr<ProxyBusObject> proxy;
    Mutex lock;
    Condition condition;
    bool evented;
    bool lastValue;

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, tsm.Init());
        ASSERT_EQ(ER_OK, prov.Init(tsm));
        ASSERT_EQ(ER_OK, cons.Init(tsm));

        ASSERT_EQ(ER_OK, prov.HostSession());
        SessionId sid = 0;
        ASSERT_EQ(ER_OK, cons.JoinSession(prov, sid));

        proxy = shared_ptr<ProxyBusObject>(cons.GetProxyObject(prov, sid));
        ASSERT_TRUE(proxy != NULL);
        cons.GetBusAttachement().RegisterSignalHandlerWithRule(this,
                                                               static_cast<MessageReceiver::SignalHandler>(&SecSignalTest::EventHandler),
                                                               cons.GetBusAttachement().GetInterface(TEST_INTERFACE)->GetMember(TEST_SIGNAL_NAME),
                                                               TEST_SIGNAL_MATCH_RULE);
    }

    void EventHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
    {
        QCC_UNUSED(msg);
        QCC_UNUSED(srcPath);
        QCC_UNUSED(member);
        lock.Lock();
        evented = true;
        condition.Signal();
        bool value = lastValue;
        QStatus status = msg->GetArgs("b", &value);
        EXPECT_EQ(ER_OK, status) << "Failed to Get bool value out of MsgArg";

        lastValue = value;
        lock.Unlock();

        cout << "received message ..." << endl;
    }


    QStatus SendAndWaitForEvent(TestSecureApplication& prov, bool newValue = true)
    {
        lock.Lock();
        evented = false;
        lastValue = !newValue;
        QStatus status = prov.SendSignal(newValue, cons);
        if (ER_OK != status) {
            cout << "Failed to send event" << __FILE__ << "@" << __LINE__ << endl;
        } else {
            condition.TimedWait(lock, 5000);
            EXPECT_TRUE(evented) << " No event received";
            EXPECT_EQ(newValue, lastValue) << "Signal value";
            status = evented  && (newValue == lastValue) ? ER_OK : ER_FAIL;
            evented = false;
        }
        lock.Unlock();
        return status;
    }

};

TEST_F(SecSignalTest, DISABLED_SendSignalAllowed) //See asacore-2432
{
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY));
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));

    ASSERT_EQ(ER_OK, SendAndWaitForEvent(prov, true));
}

TEST_F(SecSignalTest, DISABLED_SendSignalNotAllowedAfterConsumerPolicyUpdate) //See asacore-2432
{
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY));
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));

    ASSERT_EQ(ER_OK, SendAndWaitForEvent(prov, true));

    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE, "wrong.interface"));
    ASSERT_EQ(ER_OK, prov.SendSignal(true, cons));
    ASSERT_EQ(ER_OK, prov.SendSignal(true, cons));

    qcc::Sleep(500);
    ASSERT_FALSE(evented);

}

TEST_F(SecSignalTest, DISABLED_SendSignalAllowedAfterConsumerPolicyUpdate) //See asacore-2432
{
    ASSERT_EQ(ER_OK, prov.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_OBSERVE | PermissionPolicy::Rule::Member::ACTION_MODIFY));
    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE));

    ASSERT_EQ(ER_OK, SendAndWaitForEvent(prov, true));

    ASSERT_EQ(ER_OK, cons.SetAnyTrustedUserPolicy(tsm, PermissionPolicy::Rule::Member::ACTION_PROVIDE));
    ASSERT_EQ(ER_OK, prov.SendSignal(true, cons));
    ASSERT_EQ(ER_OK, SendAndWaitForEvent(prov, true));

}
