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
        const InterfaceDescription* Intf1 = bus.GetInterface("org.test");
        EXPECT_TRUE(Intf1 != NULL);
        AddInterface(*Intf1);
    }

    virtual ~TestObject() { }

    QStatus SendSignal(const char* dest, SessionId id, uint8_t flags = 0) {
        const InterfaceDescription::Member*  signal_member = bus.GetInterface("org.test")->GetMember("my_signal");
        MsgArg arg("s", "Signal");
        QStatus status = Signal(dest, id, *signal_member, &arg, 1, 0, flags);
        return status;
    }

    BusAttachment& bus;
};

class AlwaysAcceptSessionPortListener : public SessionPortListener {
  public:
    virtual ~AlwaysAcceptSessionPortListener() { }
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
};

class Participant : public MessageReceiver  {
  public:
    SessionPort port;
    SessionPort mpport;

    const char* name;
    BusAttachment bus;
    TestObject* busobj;

    AlwaysAcceptSessionPortListener spl;
    AlwaysAcceptSessionPortListener mpspl;

    SessionOpts opts;
    SessionOpts mpopts;

    int signalReceived;

    std::map<std::pair<qcc::String, bool>, SessionId> sessionMap;

    Participant(const char* name) : port(42), mpport(84), name(name), bus(name),
        opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY),
        mpopts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY)
    {
        signalReceived = 0;
        Init();
    }

    void Init() {
        QStatus status;

        ASSERT_EQ(ER_OK, bus.Start());
        ASSERT_EQ(ER_OK, bus.Connect(getConnectArg().c_str()));

        ASSERT_EQ(ER_OK, bus.BindSessionPort(port, opts, spl));
        ASSERT_EQ(ER_OK, bus.BindSessionPort(mpport, mpopts, mpspl));

        ASSERT_EQ(ER_OK, bus.RequestName(name, DBUS_NAME_FLAG_DO_NOT_QUEUE));
        ASSERT_EQ(ER_OK, bus.AdvertiseName(name, TRANSPORT_ANY));

        /* create test interface */
        InterfaceDescription* servicetestIntf = NULL;
        status = bus.CreateInterface("org.test", servicetestIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(servicetestIntf != NULL);
        status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        servicetestIntf->Activate();
        const InterfaceDescription::Member* signal_member = servicetestIntf->GetMember("my_signal");

        status = bus.RegisterSignalHandler(this,
                                           static_cast<MessageReceiver::SignalHandler>(&Participant::SignalHandler),
                                           signal_member,
                                           NULL);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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

    void SignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg) {
        signalReceived++;
    }

    void AddMatch() {
        QStatus status = bus.AddMatch("type='signal',interface='org.test',member='my_signal'");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    void RemoveMatch() {
        QStatus status = bus.RemoveMatch("type='signal',interface='org.test',member='my_signal'");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    void JoinSession(const char* name, bool multipoint) {
        SessionId outIdA;
        SessionOpts outOptsA;
        SessionPort p = multipoint ? mpport : port;
        ASSERT_EQ(ER_OK, bus.JoinSession(name, p, NULL, outIdA, multipoint ? mpopts : opts));
        sessionMap[std::make_pair(qcc::String(name), multipoint)] = outIdA;
    }

    void LeaveSession(const char* name, bool multipoint) {
        std::pair<qcc::String, bool> key = std::make_pair(qcc::String(name), multipoint);
        SessionId id = sessionMap[key];
        sessionMap.erase(key);
        ASSERT_EQ(ER_OK, bus.LeaveSession(id));
    }

    SessionId GetSessionId(const char* name, bool multipoint) {
        std::pair<qcc::String, bool> key = std::make_pair(qcc::String(name), multipoint);
        return sessionMap[key];
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

void verify_recv_ex(Participant& participant, int expect_recv)
{
    EXPECT_EQ(expect_recv, participant.signalReceived);
    participant.signalReceived = 0;
}

void verify_recv(Participant& participant) {
    verify_recv_ex(participant, 1);
}

void verify_norecv(Participant& participant) {
    verify_recv_ex(participant, 0);
}

TEST_F(SignalTest, Point2PointSimple)
{
    Participant A("A.A");
    Participant B("B.B");

    B.JoinSession("A.A", false);

    /* directed signal */
    B.busobj->SendSignal("A.A", 0, 0);
    wait_for_signal();
    verify_recv(A);
    verify_norecv(B);

    A.busobj->SendSignal("B.B", 0, 0);
    wait_for_signal();
    verify_norecv(A);
    verify_recv(B);

    /* dbus broadcast signal */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, 0);
    A.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    B.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    verify_recv(A);
    verify_recv(B);
    A.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    verify_recv(A);
    verify_recv(B);
    A.RemoveMatch();
    B.RemoveMatch();

    /* global broadcast signal */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    verify_recv(A);
    verify_recv(B);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    verify_recv(A);
    verify_recv(B);
    A.RemoveMatch();
    B.RemoveMatch();

    /* all-session broadcast */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
    wait_for_signal();
    verify_norecv(A);
    verify_recv(B);
    A.RemoveMatch();
    B.RemoveMatch();

    /* session multicast */
    B.busobj->SendSignal(NULL, B.GetSessionId("A.A", false), 0);
    wait_for_signal();
    verify_recv(A);
    verify_norecv(B);
    A.busobj->SendSignal(NULL, B.GetSessionId("A.A", false), 0);
    wait_for_signal();
    verify_norecv(A);
    verify_recv(B);

    B.LeaveSession("A.A", false);
}

TEST_F(SignalTest, MultiPointSimple)
{
    Participant A("A.A");
    Participant B("B.B");
    Participant C("C.C");

    B.JoinSession("A.A", true);
    C.JoinSession("A.A", true);

    /* directed signal */
    B.busobj->SendSignal("A.A", 0, 0);
    wait_for_signal();
    verify_recv(A);
    verify_norecv(B);
    verify_norecv(C);

    A.busobj->SendSignal("B.B", 0, 0);
    wait_for_signal();
    verify_norecv(A);
    verify_recv(B);
    verify_norecv(C);

    /* dbus broadcast signal */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, 0);
    A.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);
    verify_norecv(C);

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    C.AddMatch();
    B.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    verify_recv(A);
    verify_recv(B);
    verify_recv(C);
    A.busobj->SendSignal(NULL, 0, 0);
    wait_for_signal();
    verify_recv(A);
    verify_recv(B);
    verify_recv(C);
    A.RemoveMatch();
    B.RemoveMatch();
    C.RemoveMatch();

    /* global broadcast signal */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);
    verify_norecv(C);

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    C.AddMatch();
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    verify_recv(A);
    verify_recv(B);
    verify_recv(C);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_GLOBAL_BROADCAST);
    wait_for_signal();
    verify_recv(A);
    verify_recv(B);
    verify_recv(C);
    A.RemoveMatch();
    B.RemoveMatch();
    C.RemoveMatch();

    /* all-session broadcast */
    /* no addmatches */
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);
    verify_norecv(C);

    /* with addmatches */
    A.AddMatch();
    B.AddMatch();
    C.AddMatch();
    B.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);
    verify_norecv(C);
    A.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
    wait_for_signal();
    verify_norecv(A);
    verify_recv(B);
    verify_recv(C);
    A.RemoveMatch();
    B.RemoveMatch();
    C.RemoveMatch();

    /* session multicast */
    B.busobj->SendSignal(NULL, B.GetSessionId("A.A", true), 0);
    wait_for_signal();
    verify_recv(A);
    verify_norecv(B);
    verify_recv(C);
    A.busobj->SendSignal(NULL, B.GetSessionId("A.A", true), 0);
    wait_for_signal();
    verify_norecv(A);
    verify_recv(B);
    verify_recv(C);

    B.LeaveSession("A.A", true);
    C.LeaveSession("A.A", true);
}

void SendASB(Participant& participant)
{
    participant.busobj->SendSignal(NULL, 0, ALLJOYN_FLAG_ALLSESSION_BROADCAST);
}

TEST_F(SignalTest, Point2PointComplex) {

    Participant A("A.A");
    Participant B("B.B");
    Participant C("C.C");

    /* x -> y means "x hosts p2p session for y"
     * x m> y means "x hosts mp session for y"
     */

    /* A -> B, B -> C, C -> A */
    B.JoinSession("A.A", false);
    C.JoinSession("B.B", false);
    A.JoinSession("C.C", false);

    /* all-session broadcast */
    /* without addmatch */
    SendASB(A);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);
    verify_norecv(C);

    /* with addmatch */
    A.AddMatch();
    B.AddMatch();
    C.AddMatch();
    SendASB(A);
    wait_for_signal();
    verify_norecv(A);
    verify_recv(B);
    verify_norecv(C);
    A.RemoveMatch();
    B.RemoveMatch();
    C.RemoveMatch();

    /* A -> B, A -> C, B -> C, C -> A */
    C.JoinSession("A.A", false);
    /* without addmatch */
    SendASB(A);
    wait_for_signal();
    verify_norecv(A);
    verify_norecv(B);
    verify_norecv(C);

    /* with addmatch */
    A.AddMatch();
    B.AddMatch();
    C.AddMatch();
    SendASB(A);
    wait_for_signal();
    verify_norecv(A);
    verify_recv(B);
    verify_recv(C);
    A.RemoveMatch();
    B.RemoveMatch();
    C.RemoveMatch();

    B.LeaveSession("A.A", false);
    C.LeaveSession("A.A", false);
    C.LeaveSession("B.B", false);
}

TEST_F(SignalTest, MultiSession) {
    Participant A("A.A");
    Participant B("B.B");

    B.AddMatch();

    /* Enter in 2 sessions with A */
    B.JoinSession("A.A", false);
    B.JoinSession("A.A", true);
    SendASB(A);
    wait_for_signal();
    /* verify B received the signal just once */
    verify_norecv(A);
    verify_recv(B);

    /* leave one of the sessions */
    B.LeaveSession("A.A", false);
    SendASB(A);
    wait_for_signal();
    /* verify B still received the signal */
    verify_norecv(A);
    verify_recv(B);

    /* leave the last session */
    B.LeaveSession("A.A", true);
    SendASB(A);
    wait_for_signal();
    /* verify B did not received the signal */
    verify_norecv(A);
    verify_norecv(B);

    B.RemoveMatch();
}
