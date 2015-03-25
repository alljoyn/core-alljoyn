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

#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/Observer.h>

#include <alljoyn/Status.h>

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "ajTestCommon.h"

namespace test_unit_observer {

using namespace std;
using namespace qcc;
using namespace ajn;

#define INTF_A  "org.test.a"
#define INTF_B  "org.test.b"
#define METHOD  "Identify"

#define PATH_PREFIX "/test/"

#define MAX_WAIT_MS 6000

#define STRESS_FACTOR 5

class TestObject : public BusObject {
  public:
    TestObject(BusAttachment& bus, qcc::String path, const vector<qcc::String>& _interfaces) :
        BusObject(path.c_str()), bus(bus), path(path), interfaces(_interfaces) {
        busname = bus.GetUniqueName();
        vector<qcc::String>::iterator it;
        for (it = interfaces.begin(); it != interfaces.end(); ++it) {
            const InterfaceDescription* intf = bus.GetInterface(it->c_str());
            EXPECT_TRUE(intf != NULL);
            AddInterface(*intf, ANNOUNCED);

            QStatus status = AddMethodHandler(intf->GetMember(METHOD),
                                              static_cast<MessageReceiver::MethodHandler>(&TestObject::HandleIdentify));
            EXPECT_EQ(ER_OK, status) << "Method handler registration failed";
        }
    }

    virtual ~TestObject() { }

    void HandleIdentify(const InterfaceDescription::Member* member, Message& message) {
        UNREFERENCED_PARAMETER(member);

        MsgArg args[2] = { MsgArg("s", busname.c_str()), MsgArg("s", path.c_str()) };
        EXPECT_EQ(ER_OK, MethodReply(message, args, 2)) << "Method reply failed";
    }

    BusAttachment& bus;
    qcc::String busname;
    qcc::String path;
    vector<qcc::String> interfaces;
};

class Participant : public SessionPortListener, public SessionListener {
  public:

    BusAttachment bus;
    String uniqueBusName;

    typedef std::pair<TestObject*, bool> ObjectState;
    typedef std::map<qcc::String, ObjectState> ObjectMap;
    ObjectMap objects;

    SessionOpts opts;
    typedef std::map<qcc::String, SessionId> SessionMap;
    SessionMap hostedSessionMap;
    qcc::Mutex hsmLock;
    SessionPort port;
    bool acceptSessions;

    AboutData aboutData;
    AboutObj aboutObj;

    Participant() : bus("Participant"),
        opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY),
        port(42), acceptSessions(true), aboutData("en"), aboutObj(bus)
    {
        Init();
    }

    void StartBus() {
        ASSERT_EQ(ER_OK, bus.Start());
        ASSERT_EQ(ER_OK, bus.Connect(getConnectArg().c_str()));
        ASSERT_EQ(ER_OK, bus.BindSessionPort(port, opts, *this));
        uniqueBusName = bus.GetUniqueName();
    }

    void PublishAbout() {
        ASSERT_EQ(ER_OK, aboutObj.Announce(port, aboutData));
    }

    void Init() {
        QStatus status;

        StartBus();

        /* create interfaces */
        InterfaceDescription* intf = NULL;
        status = bus.CreateInterface(INTF_A, intf);
        ASSERT_EQ(ER_OK, status);
        ASSERT_TRUE(intf != NULL);
        status = intf->AddMethod(METHOD, "", "ss", "busname,path");
        ASSERT_EQ(ER_OK, status);
        intf->Activate();

        intf = NULL;
        status = bus.CreateInterface(INTF_B, intf);
        ASSERT_EQ(ER_OK, status);
        ASSERT_TRUE(intf != NULL);
        status = intf->AddMethod(METHOD, "", "ss", "busname,path");
        ASSERT_EQ(ER_OK, status);
        intf->Activate();

        /* set up totally uninteresting about data */
        //AppId is a 128bit uuid
        uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                            0x1E, 0x82, 0x11, 0xE4,
                            0x86, 0x51, 0xD1, 0x56,
                            0x1D, 0x5D, 0x46, 0xB0 };
        aboutData.SetAppId(appId, 16);
        aboutData.SetDeviceName("My Device Name");
        //DeviceId is a string encoded 128bit UUID
        aboutData.SetDeviceId("93c06771-c725-48c2-b1ff-6a2a59d445b8");
        aboutData.SetAppName("Application");
        aboutData.SetManufacturer("Manufacturer");
        aboutData.SetModelNumber("123456");
        aboutData.SetDescription("A poetic description of this application");
        aboutData.SetDateOfManufacture("2014-03-24");
        aboutData.SetSoftwareVersion("0.1.2");
        aboutData.SetHardwareVersion("0.0.1");
        aboutData.SetSupportUrl("http://www.example.org");

        PublishAbout();
    }

    virtual ~Participant() {
        Fini();
    }

    void Fini() {
        ObjectMap::iterator it;
        for (it = objects.begin(); it != objects.end(); ++it) {
            if (it->second.second) {
                bus.UnregisterBusObject(*(it->second.first));
            }
            delete it->second.first;
        }
        StopBus();
    }

    void StopBus() {
        ASSERT_EQ(ER_OK, bus.Disconnect());
        ASSERT_EQ(ER_OK, bus.Stop());
        ASSERT_EQ(ER_OK, bus.Join());
    }

    void CreateObject(qcc::String name, vector<qcc::String> interfaces) {
        qcc::String path = qcc::String(PATH_PREFIX) + name;
        TestObject* obj = new TestObject(bus, path, interfaces);
        ObjectState os(obj, false);
        objects.insert(make_pair(name, os));
    }

    void RegisterObject(qcc::String name) {
        ObjectMap::iterator it = objects.find(name);
        ASSERT_NE(objects.end(), it) << "No such object.";
        ASSERT_FALSE(it->second.second) << "Object already on bus.";
        ASSERT_EQ(ER_OK, bus.RegisterBusObject(*(it->second.first)));
        it->second.second = true;
        ASSERT_EQ(ER_OK, aboutObj.Announce(port, aboutData));
    }

    void UnregisterObject(qcc::String name) {
        ObjectMap::iterator it = objects.find(name);
        ASSERT_NE(objects.end(), it) << "No such object.";
        ASSERT_TRUE(it->second.second) << "Object not on bus.";
        bus.UnregisterBusObject(*(it->second.first));
        it->second.second = false;
        ASSERT_EQ(ER_OK, aboutObj.Announce(port, aboutData));
    }

    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        UNREFERENCED_PARAMETER(sessionPort);
        UNREFERENCED_PARAMETER(joiner);
        UNREFERENCED_PARAMETER(opts);
        return acceptSessions;
    }

    virtual void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {
        UNREFERENCED_PARAMETER(sessionPort);

        hsmLock.Lock(MUTEX_CONTEXT);
        hostedSessionMap[joiner] = id;
        bus.SetHostedSessionListener(id, this);
        hsmLock.Unlock(MUTEX_CONTEXT);
    }

    virtual void SessionLost(SessionId sessionId, SessionLostReason reason) {
        UNREFERENCED_PARAMETER(reason);
        /* we only set a session listener on the hosted sessions */
        hsmLock.Lock(MUTEX_CONTEXT);
        SessionMap::iterator iter = hostedSessionMap.begin();
        while (iter != hostedSessionMap.end()) {
            if (iter->second == sessionId) {
                hostedSessionMap.erase(iter);
                iter = hostedSessionMap.begin();
            } else {
                ++iter;
            }
        }
        hsmLock.Unlock(MUTEX_CONTEXT);
    }

    void CloseSession(Participant& joiner) {
        hsmLock.Lock(MUTEX_CONTEXT);
        SessionMap::iterator iter = hostedSessionMap.find(joiner.uniqueBusName);
        bool foundOngoingSession = true;
        if (hostedSessionMap.end() == iter) {
            foundOngoingSession = false;
        }
        if (!foundOngoingSession) {
            hsmLock.Unlock(MUTEX_CONTEXT);
            ASSERT_TRUE(foundOngoingSession) << "Could not find ongoing session.";
        }
        bus.LeaveHostedSession(iter->second);
        hostedSessionMap.erase(iter);
        hsmLock.Unlock(MUTEX_CONTEXT);
    }

  private:
    //Private copy constructor to prevent copying the class and double freeing of memory
    Participant(const Participant& rhs);
    //Private assignment operator to prevent copying the class and double freeing of memory
    Participant& operator=(const Participant& rhs);
};

class PendingParticipant1 : public Participant {

  public:
    qcc::String objectToDrop;
    uint32_t sleepAfter;

    PendingParticipant1() : sleepAfter(0) {
    }

    // Participant removes the object that was originally interesting for the consuming observer
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        UNREFERENCED_PARAMETER(sessionPort);
        UNREFERENCED_PARAMETER(joiner);
        UNREFERENCED_PARAMETER(opts);

        UnregisterObject(objectToDrop);
        qcc::Sleep(sleepAfter);
        return acceptSessions;
    }

    virtual ~PendingParticipant1() { }

};

class PendingParticipant2 : public Participant {

  public:
    qcc::String newObjectToAnnounce;
    vector<qcc::String> objInterfaces;
    bool once;

    PendingParticipant2() : once(true) {
    }
    //Participant announces another object that is interesting for the calling consuming observer
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        UNREFERENCED_PARAMETER(sessionPort);
        UNREFERENCED_PARAMETER(joiner);
        UNREFERENCED_PARAMETER(opts);

        if (once) {
            CreateObject(newObjectToAnnounce, objInterfaces);
            RegisterObject(newObjectToAnnounce);
            once = false;
        }
        return acceptSessions;
    }

    virtual ~PendingParticipant2() { }

};

class ObserverTest : public testing::Test {
  public:
    vector<String> intfA;
    vector<String> intfB;
    vector<String> intfAB;

    const char* cintfAB[2];
    const char** cintfA;
    const char** cintfB;

    virtual void SetUp() {
        intfA.push_back(INTF_A);
        intfB.push_back(INTF_B);
        intfAB.push_back(INTF_A);
        intfAB.push_back(INTF_B);
        cintfAB[0] = INTF_A;
        cintfAB[1] = INTF_B;
        cintfA = &(cintfAB[0]);
        cintfB = &(cintfAB[1]);

    }
    virtual void TearDown() { }

    void SimpleScenario(Participant& provider, Participant& consumer);
};

class ObserverListener : public Observer::Listener {
  public:
    BusAttachment& bus;

    typedef vector<ManagedProxyBusObject> ProxyVector;
    ProxyVector proxies;

    int counter;
    Event event;
    bool strict;

    ObserverListener(BusAttachment& bus) : bus(bus), counter(0), strict(true) { }

    void ExpectInvocations(int newCounter) {
        /* first, check whether the counter was really 0 from last invocation */
        EXPECT_EQ(0, counter) << "In the previous test case, the listener was triggered an invalid number of times";

        event.ResetEvent();
        counter = newCounter;
    }

    ProxyVector::iterator FindProxy(ManagedProxyBusObject proxy) {
        ProxyVector::iterator it;
        for (it = proxies.begin(); it != proxies.end(); ++it) {
            if (it->iden(proxy)) {
                break;
            }
        }
        return it;
    }

    void CheckReentrancy(ManagedProxyBusObject& proxy) {
        Message reply(bus);
        QStatus status;

        /* proxy object must implement at least one of A or B */
        const char* intfname = INTF_A;
        if (!proxy->ImplementsInterface(INTF_A)) {
            intfname = INTF_B;
            EXPECT_TRUE(proxy->ImplementsInterface(INTF_B));
        }

        status = proxy->MethodCall(intfname, METHOD, NULL, 0, reply);
        EXPECT_TRUE((status == ER_OK) || (status == ER_BUS_BLOCKING_CALL_NOT_ALLOWED));

        bus.EnableConcurrentCallbacks();
        status = proxy->MethodCall(intfname, METHOD, NULL, 0, reply);
        EXPECT_EQ(ER_OK, status);
        if (ER_OK == status) {
            String ubn(reply->GetArg(0)->v_string.str), path(reply->GetArg(1)->v_string.str);
            if (strict) {
                EXPECT_EQ(proxy->GetUniqueName(), ubn);
            }
            EXPECT_EQ(proxy->GetPath(), path);
        }
    }

    virtual void ObjectDiscovered(ManagedProxyBusObject& proxy) {
        ProxyVector::iterator it = FindProxy(proxy);
        if (strict) {
            EXPECT_EQ(it, proxies.end()) << "Discovering an already-discovered object";
        }
        proxies.push_back(proxy);
        CheckReentrancy(proxy);
        if (--counter == 0) {
            event.SetEvent();
        }

    }

    virtual void ObjectLost(ManagedProxyBusObject& proxy) {
        ProxyVector::iterator it = FindProxy(proxy);
        EXPECT_NE(it, proxies.end()) << "Lost a not-discovered object";
        proxies.erase(it);
        if (--counter == 0) {
            event.SetEvent();
        }
    }
};

static bool WaitForAll(vector<Event*>& events, uint32_t wait_ms = MAX_WAIT_MS)
{
    uint32_t final = qcc::GetTimestamp() + wait_ms;
    vector<Event*> remaining = events;

    while (remaining.size() > 0) {
        vector<Event*> triggered;
        uint32_t now = qcc::GetTimestamp();

        if (now >= final) {
            return false;
        }

        QStatus status = Event::Wait(remaining, triggered, final - now);
        if (status != ER_OK && status != ER_TIMEOUT) {
            return false;
        }

        vector<Event*>::iterator trigit, remit;
        for (trigit = triggered.begin(); trigit != triggered.end(); ++trigit) {
            for (remit = remaining.begin(); remit != remaining.end(); ++remit) {
                if (*remit == *trigit) {
                    remaining.erase(remit);
                    break;
                }
            }
        }
    }

    return true;
}

static int CountProxies(Observer& obs)
{
    int count;
    ManagedProxyBusObject iter;
    for (count = 0, iter = obs.GetFirst(); iter->IsValid(); iter = obs.GetNext(iter)) {
        ++count;
    }
    return count;
}

void ObserverTest::SimpleScenario(Participant& provider, Participant& consumer)
{
    vector<qcc::String> interfaces;

    provider.CreateObject("justA", intfA);
    provider.CreateObject("justB", intfB);
    provider.CreateObject("both", intfAB);

    ObserverListener listenerA(consumer.bus), listenerB(consumer.bus), listenerAB(consumer.bus);
    Observer obsA(consumer.bus, cintfA, 1);
    obsA.RegisterListener(listenerA);
    Observer obsB(consumer.bus, cintfB, 1);
    obsB.RegisterListener(listenerB);
    Observer obsAB(consumer.bus, cintfAB, 2);
    obsAB.RegisterListener(listenerAB);

    vector<Event*> events;
    vector<Event*> allEvents;
    allEvents.push_back(&(listenerA.event));
    allEvents.push_back(&(listenerB.event));
    allEvents.push_back(&(listenerAB.event));

    /* let provider publish objects on the bus */
    listenerA.ExpectInvocations(2);
    listenerB.ExpectInvocations(2);
    listenerAB.ExpectInvocations(1);

    provider.RegisterObject("justA");
    provider.RegisterObject("justB");
    provider.RegisterObject("both");
    EXPECT_TRUE(WaitForAll(allEvents));

    /* remove justA from the bus */
    listenerA.ExpectInvocations(1);
    listenerB.ExpectInvocations(0);
    listenerAB.ExpectInvocations(0);

    provider.UnregisterObject("justA");
    events.clear();
    events.push_back(&(listenerA.event));
    EXPECT_TRUE(WaitForAll(events));

    /* remove "both" from the bus */
    listenerA.ExpectInvocations(1);
    listenerB.ExpectInvocations(1);
    listenerAB.ExpectInvocations(1);

    provider.UnregisterObject("both");
    EXPECT_TRUE(WaitForAll(allEvents));

    /* count the number of proxies left in the Observers.
     * There should be 0 in A, 1 in B, 0 in AB */
    EXPECT_EQ(0, CountProxies(obsA));
    EXPECT_EQ(1, CountProxies(obsB));
    EXPECT_EQ(0, CountProxies(obsAB));

    /* remove all listeners */
    obsA.UnregisterAllListeners();
    obsB.UnregisterAllListeners();
    obsAB.UnregisterListener(listenerAB);

    /* remove "justB" and reinstate the other objects */
    listenerA.ExpectInvocations(0);
    listenerB.ExpectInvocations(0);
    listenerAB.ExpectInvocations(0);
    provider.UnregisterObject("justB");
    provider.RegisterObject("justA");
    provider.RegisterObject("both");

    /* busy-wait for a second at most */
    for (int i = 0; i < 50; ++i) {
        if (CountProxies(obsA) == 2 &&
            CountProxies(obsB) == 1 &&
            CountProxies(obsAB) == 1) {
            break;
        }
        qcc::Sleep(20);
    }
    EXPECT_EQ(2, CountProxies(obsA));
    EXPECT_EQ(1, CountProxies(obsB));
    EXPECT_EQ(1, CountProxies(obsAB));

    /* reinstate listeners & test triggerOnExisting functionality */
    listenerA.ExpectInvocations(2);
    listenerB.ExpectInvocations(1);
    listenerAB.ExpectInvocations(1);
    obsA.RegisterListener(listenerA);
    obsB.RegisterListener(listenerB);
    obsAB.RegisterListener(listenerAB);

    EXPECT_TRUE(WaitForAll(allEvents));

    /* test multiple listeners for same observer */
    ObserverListener listenerB2(consumer.bus);
    listenerB2.ExpectInvocations(0);
    obsB.RegisterListener(listenerB2, false);

    listenerA.ExpectInvocations(0);
    listenerB.ExpectInvocations(1);
    listenerB2.ExpectInvocations(1);
    listenerAB.ExpectInvocations(0);
    provider.RegisterObject("justB");
    events.clear();
    events.push_back(&(listenerB.event));
    events.push_back(&(listenerB2.event));
    EXPECT_TRUE(WaitForAll(events));

    /* are all objects back where they belong? */
    EXPECT_EQ(2, CountProxies(obsA));
    EXPECT_EQ(2, CountProxies(obsB));
    EXPECT_EQ(1, CountProxies(obsAB));

    /* test multiple observers for the same set of interfaces */
    Observer obsB2(consumer.bus, cintfB, 1);
    obsB.UnregisterListener(listenerB2); /* unregister listenerB2 from obsB so we can reuse it here */
    listenerA.ExpectInvocations(0);
    listenerB.ExpectInvocations(0);
    listenerB2.ExpectInvocations(2);
    listenerAB.ExpectInvocations(0);
    obsB2.RegisterListener(listenerB2);
    events.clear();
    events.push_back(&(listenerB2.event));
    EXPECT_TRUE(WaitForAll(events));

    /* test Observer::Get() and the proxy creation functionality */
    ManagedProxyBusObject proxy;
    ObjectId oid(provider.uniqueBusName, PATH_PREFIX "justA");
    proxy = obsA.Get(oid);
    EXPECT_TRUE(proxy->IsValid());
    EXPECT_EQ((size_t)2, proxy->GetInterfaces()); // always one more than expected because of org.freedesktop.DBus.Peer
    oid.objectPath = PATH_PREFIX "both";
    proxy = obsA.Get(oid);
    EXPECT_TRUE(proxy->IsValid());
    EXPECT_EQ((size_t)3, proxy->GetInterfaces());

    /* verify that we can indeed perform method calls */
    Message reply(consumer.bus);
    EXPECT_EQ(ER_OK, proxy->MethodCall(INTF_A, METHOD, NULL, 0, reply));
    String ubn(reply->GetArg(0)->v_string.str), path(reply->GetArg(1)->v_string.str);
    EXPECT_EQ(provider.uniqueBusName, ubn);
    EXPECT_EQ(qcc::String(PATH_PREFIX "both"), path);
}

TEST_F(ObserverTest, Simple)
{
    Participant provider;
    Participant consumer;

    SimpleScenario(provider, consumer);
}

TEST_F(ObserverTest, SimpleSelf)
{
    Participant provcons;

    SimpleScenario(provcons, provcons);
}

TEST_F(ObserverTest, Rejection)
{
    Participant willing, doubtful, unwilling, consumer;
    willing.CreateObject("a", intfA);
    doubtful.CreateObject("a", intfAB);
    unwilling.CreateObject("a", intfAB);

    unwilling.acceptSessions = false;

    ObserverListener listener(consumer.bus);
    Observer obs(consumer.bus, cintfA, 1);
    obs.RegisterListener(listener);
    vector<Event*> events;
    events.push_back(&(listener.event));

    listener.ExpectInvocations(2);
    willing.RegisterObject("a");
    doubtful.RegisterObject("a");
    unwilling.RegisterObject("a");

    EXPECT_TRUE(WaitForAll(events));

    /* now let doubtful kill the connection */
    qcc::Sleep(100); // This sleep is necessary to make sure the provider
                     // knows it has a session. Otherwise, CloseSession
                     // sporadically fails.
    listener.ExpectInvocations(1);
    doubtful.CloseSession(consumer);
    EXPECT_TRUE(WaitForAll(events));

    /* there should only be one object left */
    EXPECT_EQ(1, CountProxies(obs));

    /* unannounce and reannounce, connection should be restored */
    listener.ExpectInvocations(1);
    doubtful.UnregisterObject("a");
    doubtful.RegisterObject("a");
    EXPECT_TRUE(WaitForAll(events));

    /* now there should only be two objects */
    EXPECT_EQ(2, CountProxies(obs));
}

TEST_F(ObserverTest, CreateDelete)
{
    Participant provider, consumer;
    provider.CreateObject("a", intfA);
    provider.CreateObject("ab", intfAB);
    provider.CreateObject("ab2", intfAB);

    ObserverListener listener(consumer.bus);
    Observer obs(consumer.bus, cintfA, 1);
    obs.RegisterListener(listener);
    vector<Event*> events;
    events.push_back(&(listener.event));

    listener.ExpectInvocations(3);
    provider.RegisterObject("a");
    provider.RegisterObject("ab");
    provider.RegisterObject("ab2");

    EXPECT_TRUE(WaitForAll(events));

    /* now create and destroy some observers */
    Observer* spark;
    Observer* flame;
    ObserverListener dummy(consumer.bus);

    spark = new Observer(consumer.bus, cintfA, 1);
    delete spark;
    flame = new Observer(consumer.bus, cintfA, 1);
    flame->RegisterListener(dummy);
    delete flame;

    spark = new Observer(consumer.bus, cintfA, 1);
    flame = new Observer(consumer.bus, cintfA, 1);
    flame->RegisterListener(dummy);
    delete flame;
    delete spark;

    flame = new Observer(consumer.bus, cintfA, 1);
    spark = new Observer(consumer.bus, cintfA, 1);
    flame->RegisterListener(dummy);
    delete flame;
    delete spark;

    /* create some movement on the bus to see if there are any lingering
     * traces of spark and flame that create problems */
    listener.ExpectInvocations(3);
    provider.UnregisterObject("a");
    provider.UnregisterObject("ab");
    provider.UnregisterObject("ab2");

    EXPECT_TRUE(WaitForAll(events));
}

TEST_F(ObserverTest, ListenTwice)
{
    /* reuse the same listener for two observers */
    Participant provider, consumer;
    provider.CreateObject("a", intfA);
    provider.CreateObject("ab", intfAB);
    provider.CreateObject("ab2", intfAB);

    ObserverListener listener(consumer.bus);
    Observer obs(consumer.bus, cintfA, 1);
    obs.RegisterListener(listener);

    vector<Event*> events;
    events.push_back(&(listener.event));

    {
        /* use listener for 2 observers, so we expect to
         * see all events twice */
        Observer obs2(consumer.bus, cintfA, 1);
        obs2.RegisterListener(listener);

        listener.ExpectInvocations(6);
        provider.RegisterObject("a");
        provider.RegisterObject("ab");
        provider.RegisterObject("ab2");

        EXPECT_TRUE(WaitForAll(events));
    }

    /* one observer is gone, so we expect to see
     * every event just once. */
    listener.ExpectInvocations(3);
    provider.UnregisterObject("a");
    provider.UnregisterObject("ab");
    provider.UnregisterObject("ab2");

    EXPECT_TRUE(WaitForAll(events));
}

TEST_F(ObserverTest, Multi)
{
    /* multiple providers, multiple consumers */
    Participant one, two;
    one.CreateObject("a", intfA);
    one.CreateObject("b", intfB);
    one.CreateObject("ab", intfAB);
    two.CreateObject("a", intfA);
    two.CreateObject("b", intfB);
    two.CreateObject("ab", intfAB);

    vector<Event*> events;

    Observer obsAone(one.bus, cintfA, 1);
    ObserverListener lisAone(one.bus);
    obsAone.RegisterListener(lisAone);
    events.push_back(&(lisAone.event));
    Observer obsBone(one.bus, cintfB, 1);
    ObserverListener lisBone(one.bus);
    obsBone.RegisterListener(lisBone);
    events.push_back(&(lisBone.event));
    Observer obsABone(one.bus, cintfAB, 2);
    ObserverListener lisABone(one.bus);
    obsABone.RegisterListener(lisABone);
    events.push_back(&(lisABone.event));

    Observer obsAtwo(two.bus, cintfA, 1);
    ObserverListener lisAtwo(two.bus);
    obsAtwo.RegisterListener(lisAtwo);
    events.push_back(&(lisAtwo.event));
    Observer obsBtwo(two.bus, cintfB, 1);
    ObserverListener lisBtwo(two.bus);
    obsBtwo.RegisterListener(lisBtwo);
    events.push_back(&(lisBtwo.event));
    Observer obsABtwo(two.bus, cintfAB, 2);
    ObserverListener lisABtwo(two.bus);
    obsABtwo.RegisterListener(lisABtwo);
    events.push_back(&(lisABtwo.event));

    /* put objects on the bus */
    lisAone.ExpectInvocations(4);
    lisBone.ExpectInvocations(4);
    lisABone.ExpectInvocations(2);
    lisAtwo.ExpectInvocations(4);
    lisBtwo.ExpectInvocations(4);
    lisABtwo.ExpectInvocations(2);

    one.RegisterObject("a");
    one.RegisterObject("b");
    one.RegisterObject("ab");
    two.RegisterObject("a");
    two.RegisterObject("b");
    two.RegisterObject("ab");

    EXPECT_TRUE(WaitForAll(events, 2 * MAX_WAIT_MS));
    EXPECT_EQ(4, CountProxies(obsAone));
    EXPECT_EQ(4, CountProxies(obsBone));
    EXPECT_EQ(2, CountProxies(obsABone));
    EXPECT_EQ(4, CountProxies(obsAtwo));
    EXPECT_EQ(4, CountProxies(obsBtwo));
    EXPECT_EQ(2, CountProxies(obsABtwo));

    /* now drop all objects */
    lisAone.ExpectInvocations(4);
    lisBone.ExpectInvocations(4);
    lisABone.ExpectInvocations(2);
    lisAtwo.ExpectInvocations(4);
    lisBtwo.ExpectInvocations(4);
    lisABtwo.ExpectInvocations(2);

    one.UnregisterObject("a");
    one.UnregisterObject("b");
    one.UnregisterObject("ab");
    two.UnregisterObject("a");
    two.UnregisterObject("b");
    two.UnregisterObject("ab");

    EXPECT_TRUE(WaitForAll(events, 2 * MAX_WAIT_MS));
    EXPECT_EQ(0, CountProxies(obsAone));
    EXPECT_EQ(0, CountProxies(obsBone));
    EXPECT_EQ(0, CountProxies(obsABone));
    EXPECT_EQ(0, CountProxies(obsAtwo));
    EXPECT_EQ(0, CountProxies(obsBtwo));
    EXPECT_EQ(0, CountProxies(obsABtwo));
}

TEST_F(ObserverTest, ObjectIdSanity) {

    //Simple tests to exercise ObjectId constructors and operators

    //Default
    ObjectId emptyObjId;
    EXPECT_FALSE(emptyObjId.IsValid());     // Empty unique busname and object path

    //Basic construction
    qcc::String busName(":org.alljoyn.observer");
    qcc::String objectPath("/org/alljoyn/observer/test");
    ObjectId objId(busName, objectPath);
    EXPECT_TRUE(objId.IsValid()); // Filled-in unique busname and object path
    ObjectId objId1("", "");
    EXPECT_FALSE(emptyObjId.IsValid()); // Empty unique busname and object path

    //Copy constructor and ==
    ObjectId cpObjId(objId);
    EXPECT_TRUE(cpObjId.IsValid());
    EXPECT_EQ(cpObjId.objectPath, objId.objectPath);
    EXPECT_EQ(cpObjId.uniqueBusName, objId.uniqueBusName);
    EXPECT_TRUE(cpObjId == objId);

    //Construction with ManagedProxyBusObject
    ManagedProxyBusObject mgdProxyBusObj;
    ObjectId emptyObj1(mgdProxyBusObj);
    EXPECT_FALSE(emptyObj1.IsValid()); // Empty unique busname and object path

    //Construction with ProxyBusObject* and ProxyBusObject
    ProxyBusObject proxyBusObj;
    ObjectId emptyObjId2(&proxyBusObj);
    EXPECT_FALSE(emptyObjId2.IsValid()); // Empty unique busname and object path
    ObjectId emptyObjId3(proxyBusObj);
    EXPECT_FALSE(emptyObjId3.IsValid()); // Empty unique busname and object path

    //Construction with dummy ProxyBusObject
    BusAttachment bus("Dummy");
    const uint32_t dummySessionId = 123456789;
    SessionId someSessionId(dummySessionId);
    ProxyBusObject validProxyBusObj(bus, "Dummy", busName.c_str(), objectPath.c_str(), someSessionId);
    ObjectId validObjId(validProxyBusObj);
    EXPECT_TRUE(validObjId.IsValid());
    EXPECT_TRUE(validObjId.uniqueBusName == validProxyBusObj.GetUniqueName());
    EXPECT_TRUE(validObjId.objectPath == validProxyBusObj.GetPath());

    //Null test
    ProxyBusObject* nullProxyBusObj = NULL;
    EXPECT_FALSE(ObjectId(nullProxyBusObj).IsValid()); // Empty unique busname and object path

    //Operator <
    ObjectId cmpObjId(busName, "/A/B/C");
    ObjectId cmpObjId1(busName, "/D/E/F");
    EXPECT_TRUE(cmpObjId.IsValid());
    EXPECT_TRUE(cmpObjId1.IsValid());
    EXPECT_TRUE(cmpObjId < cmpObjId1);

    ObjectId cmpObjId2(busName + ".A", objectPath);
    ObjectId cmpObjId3(busName + ".B", objectPath);
    EXPECT_TRUE(cmpObjId2.IsValid());
    EXPECT_TRUE(cmpObjId3.IsValid());
    EXPECT_TRUE(cmpObjId2 < cmpObjId3);

    EXPECT_FALSE(cmpObjId2 < cmpObjId);
}

TEST_F(ObserverTest, ObserverSanity) {

    /* Test basic construction with NULLs of the Observer.
     * If the number of interfaces is not matching the actual number of interfaces in the array,
     * then it's inevitable not to segfault.*/

    Participant one;
    const char* mandIntf[1] = { NULL };
    const char* mandIntf2[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    Observer* obs  = NULL;
    obs = new Observer(one.bus, mandIntf, 1); // Should not crash although the resulting observer is not useful
    EXPECT_FALSE(NULL == obs);
    Observer* obs2  = NULL;
    obs2 = new Observer(one.bus, mandIntf2, 10); // Should not crash although the resulting observer is not useful
    EXPECT_FALSE(NULL == obs2);

    Observer* obs3 = NULL;
    obs3 = new Observer(one.bus, NULL, 0);
    EXPECT_FALSE(NULL == obs3);

    Observer* obs4 = NULL;
    obs4 = new Observer(one.bus, mandIntf, 0);
    EXPECT_FALSE(NULL == obs4);

    /* Test using same interface name twice */
    vector<qcc::String> doubleIntf;
    const char*doubleIntfA[] = { intfA[0].c_str(), intfA[0].c_str() };
    doubleIntf.push_back(intfA[0]);
    doubleIntf.push_back(intfA[0]); // Intentional

    ObserverListener listener(one.bus);
    Observer* obs5 = NULL;
    obs5 = new Observer(one.bus, doubleIntfA, 2);
    EXPECT_FALSE(NULL == obs5);
    obs5->RegisterListener(listener);

    vector<qcc::String> oneIntfA;
    oneIntfA.push_back(intfA[0]);
    one.CreateObject("doubleIntfA", oneIntfA);

    vector<Event*> events;
    events.push_back(&(listener.event));

    listener.ExpectInvocations(1); // Should be triggered only once on object registration although we have duplicate interfaces
    one.RegisterObject("doubleIntfA");

    EXPECT_TRUE(WaitForAll(events));

    EXPECT_EQ(1, CountProxies(*obs5)); // Make sure we have only one proxy for the remote object implementing duplicate interfaces

    listener.ExpectInvocations(1); // Should be triggered only once on object un-registration although we have duplicate interfaces
    one.UnregisterObject("doubleIntfA");

    EXPECT_TRUE(WaitForAll(events));

    obs5->UnregisterListener(listener);

    delete obs;
    delete obs2;
    delete obs3;
    delete obs4;
    delete obs5;

}

TEST_F(ObserverTest, RegisterListenerTwice) {

    /* Reuse the same listener for the same observer */
    Participant provider, consumer;
    provider.CreateObject("a", intfA);

    ObserverListener listener(consumer.bus);
    listener.strict = false;
    Observer obs(consumer.bus, cintfA, 1);

    obs.RegisterListener(listener);
    obs.RegisterListener(listener); // Intentional

    vector<Event*> events;
    events.push_back(&(listener.event));

    listener.ExpectInvocations(2); // Should be triggered twice on object registration as we registered the listener twice
    provider.RegisterObject("a");

    EXPECT_TRUE(WaitForAll(events));

    listener.ExpectInvocations(2); // Should be triggered twice on object un-registration as we registered the listener twice
    provider.UnregisterObject("a");

    EXPECT_TRUE(WaitForAll(events));

    obs.UnregisterListener(listener);

    listener.ExpectInvocations(1); // Should be triggered once on object registration as we removed one listener
    provider.RegisterObject("a");

    EXPECT_TRUE(WaitForAll(events));

    obs.UnregisterListener(listener);

}

TEST_F(ObserverTest, AnnounceLogicSanity) {

    Participant provider, consumer;
    ObserverListener listenerA(consumer.bus);
    ObserverListener listenerB(consumer.bus);

    provider.CreateObject("a", intfA);
    provider.CreateObject("b", intfB);

    provider.RegisterObject("a");
    provider.RegisterObject("b");

    vector<Event*> events;

    {
        Observer obsA(consumer.bus, cintfA, 1);

        events.push_back(&(listenerA.event));

        listenerA.ExpectInvocations(1); // Object with intfA was at least discovered
        obsA.RegisterListener(listenerA);

        EXPECT_TRUE(WaitForAll(events));

        events.clear();
        events.push_back(&(listenerB.event));
        Observer obsB(consumer.bus, cintfB, 1);
        listenerB.ExpectInvocations(1); // Object with intfB was at least discovered
        obsB.RegisterListener(listenerB);

        EXPECT_TRUE(WaitForAll(events));

    }

    events.clear();

    // Try creating Observer on IntfB after destroying Observer on InftA

    {
        Observer obsA(consumer.bus, cintfA, 1);
        events.push_back(&(listenerA.event));
        listenerA.ExpectInvocations(1); // Object with intfA was at least discovered

        obsA.RegisterListener(listenerA);
        EXPECT_TRUE(WaitForAll(events));
        obsA.UnregisterAllListeners();
    }

    events.clear();

    Observer obsB(consumer.bus, cintfB, 1);
    events.push_back(&(listenerB.event));

    listenerB.ExpectInvocations(1);     // Object with intfB was at least discovered
    obsB.RegisterListener(listenerB);

    EXPECT_TRUE(WaitForAll(events));
    obsB.UnregisterAllListeners();

    provider.UnregisterObject("a");
    provider.UnregisterObject("b");
}

TEST_F(ObserverTest, GetFirstGetNext) {
    // set up two participants
    Participant one, two;
    one.CreateObject("a", intfA);
    two.CreateObject("a", intfA);

    // set up one observer
    Participant obs;
    Observer obsA(obs.bus, cintfA, 1);
    ObserverListener lisA(obs.bus);
    obsA.RegisterListener(lisA);

    // register objects
    vector<Event*> events;
    events.push_back(&(lisA.event));
    lisA.ExpectInvocations(2);
    one.RegisterObject("a");
    two.RegisterObject("a");
    EXPECT_TRUE(WaitForAll(events));

    // basic iterator access
    ManagedProxyBusObject proxy = obsA.GetFirst();
    EXPECT_TRUE(proxy->IsValid());
    proxy = obsA.GetNext(proxy);
    EXPECT_TRUE(proxy->IsValid());
    proxy = obsA.GetNext(proxy);
    EXPECT_FALSE(proxy->IsValid());

    // start iterating
    proxy = obsA.GetFirst();
    EXPECT_TRUE(proxy->IsValid());
    ManagedProxyBusObject proxy2 = obsA.GetFirst();
    EXPECT_TRUE(proxy2->IsValid());

    // unregister objects
    lisA.ExpectInvocations(2);
    one.UnregisterObject("a");
    two.UnregisterObject("a");

    // don't wait for the listener notification; should not crash either way.
    proxy2 = obsA.GetNext(proxy2);
    if (proxy2->IsValid()) {
        Message reply(obs.bus);
        // the object is no longer on the bus so the method call must not succeed
        EXPECT_NE(ER_OK, proxy2->MethodCall(INTF_A, METHOD, NULL, 0, reply));
    }

    // wait for events and check iterator
    EXPECT_TRUE(WaitForAll(events));
    proxy = obsA.GetNext(proxy);
    EXPECT_FALSE(proxy->IsValid());
}

TEST_F(ObserverTest, RestartObserver) {
    // set up two participants
    Participant one, two;
    one.CreateObject("a", intfA);
    two.CreateObject("a", intfA);

    // set up observer
    Participant obs;
    Observer* obsA = new Observer(obs.bus, cintfA, 1);
    ObserverListener lisA(obs.bus);
    obsA->RegisterListener(lisA);

    // register objects
    vector<Event*> events;
    events.push_back(&(lisA.event));
    lisA.ExpectInvocations(2);
    one.RegisterObject("a");
    two.RegisterObject("a");
    EXPECT_TRUE(WaitForAll(events));

    // destroy observer
    obsA->UnregisterAllListeners();
    delete obsA;

    // create new observer
    obsA = new Observer(obs.bus, cintfA, 1);
    lisA.ExpectInvocations(2);
    obsA->RegisterListener(lisA);
    EXPECT_TRUE(WaitForAll(events));

    // clean up observer
    obsA->UnregisterAllListeners();
    delete obsA;
}

TEST_F(ObserverTest, DiscoverWhileRunning) {
    // set up observer
    Participant obs;
    Observer* obsA = new Observer(obs.bus, cintfA, 1);
    ObserverListener lisA(obs.bus);
    obsA->RegisterListener(lisA);
    vector<Event*> events;
    events.push_back(&(lisA.event));

    // set up participant
    Participant one;
    one.CreateObject("a", intfA);
    lisA.ExpectInvocations(1);
    one.RegisterObject("a");
    EXPECT_TRUE(WaitForAll(events));

    // set up another participant
    Participant two;
    two.CreateObject("a", intfA);
    lisA.ExpectInvocations(1);
    two.RegisterObject("a");
    EXPECT_TRUE(WaitForAll(events));

    // removal of participants
    lisA.ExpectInvocations(2);
    one.UnregisterObject("a");
    two.UnregisterObject("a");
    EXPECT_TRUE(WaitForAll(events));
}

TEST_F(ObserverTest, StopBus) {
    // set up two participants
    Participant one, two;
    one.CreateObject("a", intfA);
    two.CreateObject("a", intfA);

    // set up observer
    Participant obs;
    Observer obsA(obs.bus, cintfA, 1);

    // register listener
    ObserverListener lisA(obs.bus);
    lisA.strict = false;
    obsA.RegisterListener(lisA);
    vector<Event*> events;
    events.push_back(&(lisA.event));

    // register two objects
    lisA.ExpectInvocations(2);
    one.RegisterObject("a");
    two.RegisterObject("a");
    EXPECT_TRUE(WaitForAll(events));

    // stop participant buses
    lisA.ExpectInvocations(2);
    one.StopBus();
    two.StopBus();
    EXPECT_TRUE(WaitForAll(events));

    // start participant buses
    lisA.ExpectInvocations(2);
    one.StartBus();
    two.StartBus();
    one.PublishAbout();
    two.PublishAbout();
    EXPECT_TRUE(WaitForAll(events));
}
TEST_F(ObserverTest, DISABLED_StressNumPartObjects) {

    // Stress the number of participants, observers and consumers

    vector<Participant*> providers(STRESS_FACTOR);
    vector<Participant*> consumers(STRESS_FACTOR);
    vector<ObserverListener*> listeners(STRESS_FACTOR);
    vector<Observer*> observers(STRESS_FACTOR);

    vector<Event*> events;

    for (int i = 0; i < STRESS_FACTOR; i++) {

        providers.push_back(NULL);
        providers[i] = new Participant();
        EXPECT_TRUE(NULL != providers[i]);

        consumers.push_back(NULL);
        consumers[i] = new Participant();
        EXPECT_TRUE(NULL != consumers[i]);

        if (consumers[i] == NULL || providers[i] == NULL) {
            break;     //clean up
        }

        providers[i]->CreateObject("a", intfAB);
        providers[i]->CreateObject("b", intfAB);

        providers[i]->RegisterObject("a");
        providers[i]->RegisterObject("b");

        listeners[i] = new ObserverListener(consumers[i]->bus);
        events.push_back(&(listeners[i]->event));
        listeners[i]->ExpectInvocations(2 * STRESS_FACTOR);

        if (listeners[i] == NULL) {
            break;     //clean up;
        }

        observers.push_back(NULL);
        observers[i] = new Observer(consumers[i]->bus, cintfAB, 2);
        EXPECT_TRUE(NULL != observers[i]);
        if (observers[i] == NULL) {
            break;     //clean up;
        }
        observers[i]->RegisterListener(*(listeners[i]));

        qcc::Sleep(20);
    }

    EXPECT_TRUE(WaitForAll(events, MAX_WAIT_MS * (1 + STRESS_FACTOR / 2)));

    //clean up
    for (int i = 0; i < STRESS_FACTOR; i++) {
        observers[i]->UnregisterAllListeners();
        delete listeners[i];
        delete observers[i];
        providers[i]->UnregisterObject("a");
        providers[i]->UnregisterObject("b");
        delete consumers[i];
        delete providers[i];
    }
}

TEST_F(ObserverTest, PendingStateObjectLost) {

    // Set-up an observer
    Participant partObs;
    Observer obs(partObs.bus, cintfAB, 1);
    ObserverListener listener(partObs.bus);
    obs.RegisterListener(listener);
    vector<Event*> events;
    events.push_back(&(listener.event));

    // This provider will remove the object upon accepting the session join callback
    PendingParticipant1 provider;
    provider.objectToDrop = "a";
    provider.CreateObject("a", intfAB);
    provider.RegisterObject("a");

    // No sessions should have been established (on both sides)
    // as the object of interest was removed
    EXPECT_TRUE(provider.hostedSessionMap.size() == 0);
    EXPECT_TRUE(partObs.hostedSessionMap.size() == 0);

}

TEST_F(ObserverTest, PendingStateNewObjectAnnounced) {

    // Set-up an observer
    Participant partObs;
    Observer obs(partObs.bus, cintfAB, 1);
    ObserverListener listener(partObs.bus);
    obs.RegisterListener(listener);
    vector<Event*> events;
    events.push_back(&(listener.event));

    // This provider will announce a new object once it receives the initial accept session callback
    PendingParticipant2 provider;

    provider.newObjectToAnnounce = "b";
    provider.objInterfaces = intfAB;

    provider.CreateObject("a", intfAB); // Initial object to trigger the accept session callback
    provider.RegisterObject("a");

    listener.ExpectInvocations(2);
    EXPECT_TRUE(WaitForAll(events));

}

}
