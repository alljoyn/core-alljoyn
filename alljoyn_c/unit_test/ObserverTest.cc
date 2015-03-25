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

#include <alljoyn_c/Observer.h>

#include <alljoyn/Status.h>

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "ajTestCommon.h"

namespace test_unit_observerC {

/*
 * This test suite is a straight-up port of the C++ Observer test suite.
 * Observer is a consumer-only concept, so only the consumer-related bits have
 * been converted to the C API to save some effort.
 */

using namespace std;
using namespace qcc;
using namespace ajn;

#define INTF_A  "org.test.a"
#define INTF_B  "org.test.b"
#define METHOD  "Identify"

#define PATH_PREFIX "/test/"

#define MAX_WAIT_MS 3000

#define STRESS_FACTOR 5

class TestObject : public BusObject {
  public:
    TestObject(BusAttachment& bus, qcc::String path, const vector<qcc::String>& _interfaces) :
        BusObject(path.c_str()), bus(bus), path(path), interfaces(_interfaces) {
        busname = bus.GetUniqueName();
        vector<qcc::String>::iterator it;
        for (it = interfaces.begin(); it != interfaces.end(); ++it) {
            const InterfaceDescription* intf = bus.GetInterface(it->c_str());
            EXPECT_TRUE(intf != NULL) << "failed to get interface " << it->c_str();
            if (intf == NULL) {
                continue;
            }
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
    alljoyn_busattachment cbus;
    BusAttachment& bus;

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
    AboutObj* aboutObj;

    Participant() : cbus(alljoyn_busattachment_create("Participant", QCC_TRUE)), bus(*(BusAttachment*)cbus),
        opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY),
        port(42), acceptSessions(true), aboutData("en"), aboutObj(new AboutObj(bus))
    {
        Init();
    }

    void Init() {
        QStatus status;

        ASSERT_EQ(ER_OK, bus.Start());
        ASSERT_EQ(ER_OK, bus.Connect(getConnectArg().c_str()));

        ASSERT_EQ(ER_OK, bus.BindSessionPort(port, opts, *this));

        uniqueBusName = bus.GetUniqueName();

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

        ASSERT_EQ(ER_OK, aboutObj->Announce(port, aboutData));
    }

    ~Participant() {
        Fini();
    }

    void Fini() {
        ObjectMap::iterator it;
        for (it = objects.begin(); it != objects.end(); ++it) {
            if (it->second.second) {
                bus.UnregisterBusObject(*(it->second.first));
                delete it->second.first;
            }
        }

        delete aboutObj;

        ASSERT_EQ(ER_OK, bus.Disconnect());
        ASSERT_EQ(ER_OK, bus.Stop());
        ASSERT_EQ(ER_OK, bus.Join());

        alljoyn_busattachment_destroy(cbus);
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
        ASSERT_EQ(ER_OK, aboutObj->Announce(port, aboutData));
    }

    void UnregisterObject(qcc::String name) {
        ObjectMap::iterator it = objects.find(name);
        ASSERT_NE(objects.end(), it) << "No such object.";
        ASSERT_TRUE(it->second.second) << "Object not on bus.";
        bus.UnregisterBusObject(*(it->second.first));
        it->second.second = false;
        ASSERT_EQ(ER_OK, aboutObj->Announce(port, aboutData));
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
        ASSERT_NE(hostedSessionMap.end(), iter) << "Could not find ongoing session.";
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

static void object_discovered(const void* ctx, alljoyn_proxybusobject_ref proxyref);
static void object_lost(const void* ctx, alljoyn_proxybusobject_ref proxyref);

static alljoyn_observerlistener_callback listener_cbs = { object_discovered, object_lost };

struct ObserverListener {
    alljoyn_busattachment bus;

    typedef vector<alljoyn_proxybusobject_ref> ProxyVector;
    ProxyVector proxies;

    int counter;
    Event event;
    bool tolerateAlreadyDiscoveredObjects;
    alljoyn_observerlistener listener;

    ObserverListener(alljoyn_busattachment bus) : bus(bus), counter(0), tolerateAlreadyDiscoveredObjects(false) {
        listener = alljoyn_observerlistener_create(&listener_cbs, this);
    }

    virtual ~ObserverListener() {
        alljoyn_observerlistener_destroy(listener);
    }

    void ExpectInvocations(int newCounter) {
        /* first, check whether the counter was really 0 from last invocation */
        EXPECT_EQ(0, counter) << "In the previous test case, the listener was triggered an invalid number of times";

        event.ResetEvent();
        counter = newCounter;
    }

    ProxyVector::iterator FindProxy(alljoyn_proxybusobject_ref proxyref) {
        ProxyVector::iterator it;
        for (it = proxies.begin(); it != proxies.end(); ++it) {
            if (*it == proxyref) {
                break;
            }
        }
        return it;
    }

    void CheckReentrancy(alljoyn_proxybusobject_ref proxyref) {
        alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
        alljoyn_message reply = alljoyn_message_create(bus);
        QStatus status;

        /* proxy object must implement at least one of A or B */
        const char* intfname = INTF_A;
        if (!alljoyn_proxybusobject_implementsinterface(proxy, INTF_A)) {
            intfname = INTF_B;
            EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(proxy, INTF_B));
        }

        status = alljoyn_proxybusobject_methodcall(proxy, intfname, METHOD, NULL, 0, reply, MAX_WAIT_MS, 0);
        EXPECT_TRUE((status == ER_OK) || (status == ER_BUS_BLOCKING_CALL_NOT_ALLOWED));

        alljoyn_busattachment_enableconcurrentcallbacks(bus);
        status = alljoyn_proxybusobject_methodcall(proxy, intfname, METHOD, NULL, 0, reply, MAX_WAIT_MS, 0);
        EXPECT_EQ(ER_OK, status);
        if (ER_OK == status) {
            const char* ubn = NULL;
            const char* path = NULL;
            alljoyn_message_parseargs(reply, "ss", &ubn, &path);
            EXPECT_STREQ(alljoyn_proxybusobject_getuniquename(proxy), ubn);
            EXPECT_STREQ(alljoyn_proxybusobject_getpath(proxy), path);
        }
        alljoyn_message_destroy(reply);
    }

    virtual void ObjectDiscovered(alljoyn_proxybusobject_ref proxyref) {
        ProxyVector::iterator it = FindProxy(proxyref);
        if (!tolerateAlreadyDiscoveredObjects) {
            EXPECT_EQ(it, proxies.end()) << "Discovering an already-discovered object";
        }
        proxies.push_back(proxyref);
        alljoyn_proxybusobject_ref_incref(proxyref);
        CheckReentrancy(proxyref);
        if (--counter == 0) {
            event.SetEvent();
        }
    }

    virtual void ObjectLost(alljoyn_proxybusobject_ref proxyref) {
        ProxyVector::iterator it = FindProxy(proxyref);
        EXPECT_NE(it, proxies.end()) << "Lost a not-discovered object";
        proxies.erase(it);
        alljoyn_proxybusobject_ref_decref(proxyref);
        if (--counter == 0) {
            event.SetEvent();
        }
    }
};

static void object_discovered(const void* ctx, alljoyn_proxybusobject_ref proxyref)
{
    EXPECT_TRUE(ctx != NULL);
    ObserverListener* listener = (ObserverListener*) ctx;
    listener->ObjectDiscovered(proxyref);
}

static void object_lost(const void* ctx, alljoyn_proxybusobject_ref proxyref)
{
    EXPECT_TRUE(ctx != NULL);
    ObserverListener* listener = (ObserverListener*) ctx;
    listener->ObjectLost(proxyref);
}

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

static int CountProxies(alljoyn_observer observer)
{
    int count;
    alljoyn_proxybusobject_ref iter;
    for (count = 0, iter = alljoyn_observer_getfirst(observer);
         iter != NULL;
         iter = alljoyn_observer_getnext(observer, iter)) {
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

    ObserverListener listenerA(consumer.cbus),
    listenerB(consumer.cbus),
    listenerAB(consumer.cbus);
    alljoyn_observer obsA = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    alljoyn_observer_registerlistener(obsA, listenerA.listener, QCC_TRUE);
    alljoyn_observer obsB = alljoyn_observer_create(consumer.cbus, cintfB, 1);
    alljoyn_observer_registerlistener(obsB, listenerB.listener, QCC_TRUE);
    alljoyn_observer obsAB = alljoyn_observer_create(consumer.cbus, cintfAB, 2);
    alljoyn_observer_registerlistener(obsAB, listenerAB.listener, QCC_TRUE);

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
    alljoyn_observer_unregisteralllisteners(obsA);
    alljoyn_observer_unregisteralllisteners(obsB);
    alljoyn_observer_unregisterlistener(obsAB, listenerAB.listener);

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
    alljoyn_observer_registerlistener(obsA, listenerA.listener, QCC_TRUE);
    alljoyn_observer_registerlistener(obsB, listenerB.listener, QCC_TRUE);
    alljoyn_observer_registerlistener(obsAB, listenerAB.listener, QCC_TRUE);

    EXPECT_TRUE(WaitForAll(allEvents));

    /* test multiple listeners for same observer */
    ObserverListener listenerB2(consumer.cbus);
    listenerB2.ExpectInvocations(0);
    alljoyn_observer_registerlistener(obsB, listenerB2.listener, QCC_FALSE);

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
    alljoyn_observer obsB2 = alljoyn_observer_create(consumer.cbus, cintfB, 1);
    alljoyn_observer_unregisterlistener(obsB, listenerB2.listener); /* unregister listenerB2 from obsB so we can reuse it here */
    listenerA.ExpectInvocations(0);
    listenerB.ExpectInvocations(0);
    listenerB2.ExpectInvocations(2);
    listenerAB.ExpectInvocations(0);
    alljoyn_observer_registerlistener(obsB2, listenerB2.listener, QCC_TRUE);
    events.clear();
    events.push_back(&(listenerB2.event));
    EXPECT_TRUE(WaitForAll(events));
    alljoyn_observer_unregisterlistener(obsB2, listenerB2.listener);
    alljoyn_observer_destroy(obsB2);

    /* test Observer::Get() and the proxy creation functionality */
    alljoyn_proxybusobject_ref proxyref = alljoyn_observer_get(obsA, provider.uniqueBusName.c_str(), PATH_PREFIX "justA");
    ASSERT_TRUE(proxyref != NULL);
    alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
    ASSERT_TRUE(proxy != NULL);
    EXPECT_EQ((size_t)2, alljoyn_proxybusobject_getinterfaces(proxy, NULL, 0)); // always one more than expected because of org.freedesktop.DBus.Peer
    alljoyn_proxybusobject_ref_decref(proxyref);

    proxyref = alljoyn_observer_get(obsA, provider.uniqueBusName.c_str(), PATH_PREFIX "both");
    ASSERT_TRUE(proxyref != NULL);
    proxy = alljoyn_proxybusobject_ref_get(proxyref);
    ASSERT_TRUE(proxy != NULL);
    EXPECT_EQ((size_t)3, alljoyn_proxybusobject_getinterfaces(proxy, NULL, 0));
    /* verify that we can indeed perform method calls */
    alljoyn_message reply = alljoyn_message_create(consumer.cbus);
    EXPECT_EQ(ER_OK, alljoyn_proxybusobject_methodcall(proxy, INTF_A, METHOD, NULL, 0, reply, MAX_WAIT_MS, 0));
    const char* ubn;
    const char* path;
    EXPECT_EQ(ER_OK, alljoyn_message_parseargs(reply, "ss", &ubn, &path));
    EXPECT_STREQ(provider.uniqueBusName.c_str(), ubn);
    EXPECT_STREQ(PATH_PREFIX "both", path);
    alljoyn_message_destroy(reply);
    alljoyn_proxybusobject_ref_decref(proxyref);

    alljoyn_observer_destroy(obsA);
    alljoyn_observer_destroy(obsB);
    alljoyn_observer_destroy(obsAB);
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

    ObserverListener listener(consumer.cbus);
    alljoyn_observer obs = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    alljoyn_observer_registerlistener(obs, listener.listener, QCC_TRUE);
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

    alljoyn_observer_destroy(obs);
}

TEST_F(ObserverTest, CreateDelete)
{
    Participant provider, consumer;
    provider.CreateObject("a", intfA);
    provider.CreateObject("ab", intfAB);
    provider.CreateObject("ab2", intfAB);

    ObserverListener listener(consumer.cbus);
    alljoyn_observer obs = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    alljoyn_observer_registerlistener(obs, listener.listener, QCC_TRUE);
    vector<Event*> events;
    events.push_back(&(listener.event));

    listener.ExpectInvocations(3);
    provider.RegisterObject("a");
    provider.RegisterObject("ab");
    provider.RegisterObject("ab2");

    EXPECT_TRUE(WaitForAll(events));

    /* now create and destroy some observers */
    alljoyn_observer spark;
    alljoyn_observer flame;
    ObserverListener dummy(consumer.cbus);

    spark = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    alljoyn_observer_destroy(spark);;
    flame = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    alljoyn_observer_registerlistener(flame, dummy.listener, QCC_TRUE);;
    alljoyn_observer_destroy(flame);;

    spark = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    flame = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    alljoyn_observer_registerlistener(flame, dummy.listener, QCC_TRUE);;
    alljoyn_observer_destroy(flame);;
    alljoyn_observer_destroy(spark);;

    flame = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    spark = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    alljoyn_observer_registerlistener(flame, dummy.listener, QCC_TRUE);;
    alljoyn_observer_destroy(flame);;
    alljoyn_observer_destroy(spark);;

    /* create some movement on the bus to see if there are any lingering
     * traces of spark and flame that create problems */
    listener.ExpectInvocations(3);
    provider.UnregisterObject("a");
    provider.UnregisterObject("ab");
    provider.UnregisterObject("ab2");

    EXPECT_TRUE(WaitForAll(events));

    alljoyn_observer_destroy(obs);
}

TEST_F(ObserverTest, ListenTwice)
{
    /* reuse the same listener for two observers */
    Participant provider, consumer;
    provider.CreateObject("a", intfA);
    provider.CreateObject("ab", intfAB);
    provider.CreateObject("ab2", intfAB);

    ObserverListener listener(consumer.cbus);
    alljoyn_observer obs = alljoyn_observer_create(consumer.cbus, cintfA, 1);
    alljoyn_observer_registerlistener(obs, listener.listener, QCC_TRUE);

    vector<Event*> events;
    events.push_back(&(listener.event));

    {
        /* use listener for 2 observers, so we expect to
         * see all events twice */
        alljoyn_observer obs2 = alljoyn_observer_create(consumer.cbus, cintfA, 1);
        alljoyn_observer_registerlistener(obs2, listener.listener, QCC_TRUE);

        listener.ExpectInvocations(6);
        provider.RegisterObject("a");
        provider.RegisterObject("ab");
        provider.RegisterObject("ab2");

        EXPECT_TRUE(WaitForAll(events));

        alljoyn_observer_destroy(obs2);
    }

    /* one observer is gone, so we expect to see
     * every event just once. */
    listener.ExpectInvocations(3);
    provider.UnregisterObject("a");
    provider.UnregisterObject("ab");
    provider.UnregisterObject("ab2");

    EXPECT_TRUE(WaitForAll(events));

    alljoyn_observer_destroy(obs);
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

    alljoyn_observer obsAone = alljoyn_observer_create(one.cbus, cintfA, 1);
    ObserverListener lisAone(one.cbus);
    alljoyn_observer_registerlistener(obsAone, lisAone.listener, QCC_TRUE);
    events.push_back(&(lisAone.event));
    alljoyn_observer obsBone = alljoyn_observer_create(one.cbus, cintfB, 1);
    ObserverListener lisBone(one.cbus);
    alljoyn_observer_registerlistener(obsBone, lisBone.listener, QCC_TRUE);
    events.push_back(&(lisBone.event));
    alljoyn_observer obsABone = alljoyn_observer_create(one.cbus, cintfAB, 2);
    ObserverListener lisABone(one.cbus);
    alljoyn_observer_registerlistener(obsABone, lisABone.listener, QCC_TRUE);
    events.push_back(&(lisABone.event));

    alljoyn_observer obsAtwo = alljoyn_observer_create(two.cbus, cintfA, 1);
    ObserverListener lisAtwo(two.cbus);
    alljoyn_observer_registerlistener(obsAtwo, lisAtwo.listener, QCC_TRUE);
    events.push_back(&(lisAtwo.event));
    alljoyn_observer obsBtwo = alljoyn_observer_create(two.cbus, cintfB, 1);
    ObserverListener lisBtwo(two.cbus);
    alljoyn_observer_registerlistener(obsBtwo, lisBtwo.listener, QCC_TRUE);
    events.push_back(&(lisBtwo.event));
    alljoyn_observer obsABtwo = alljoyn_observer_create(two.cbus, cintfAB, 2);
    ObserverListener lisABtwo(two.cbus);
    alljoyn_observer_registerlistener(obsABtwo, lisABtwo.listener, QCC_TRUE);
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

    EXPECT_TRUE(WaitForAll(events));
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

    EXPECT_TRUE(WaitForAll(events));
    EXPECT_EQ(0, CountProxies(obsAone));
    EXPECT_EQ(0, CountProxies(obsBone));
    EXPECT_EQ(0, CountProxies(obsABone));
    EXPECT_EQ(0, CountProxies(obsAtwo));
    EXPECT_EQ(0, CountProxies(obsBtwo));
    EXPECT_EQ(0, CountProxies(obsABtwo));

    alljoyn_observer_destroy(obsAone);
    alljoyn_observer_destroy(obsBone);
    alljoyn_observer_destroy(obsABone);
    alljoyn_observer_destroy(obsAtwo);
    alljoyn_observer_destroy(obsBtwo);
    alljoyn_observer_destroy(obsABtwo);
}

TEST_F(ObserverTest, ObserverSanity) {

    /* Test basic construction with NULLs of the Observer.
     * If the number of interfaces is not matching the actual number of interfaces in the array,
     * then it's inevitable not to segfault.*/

    Participant one;
    const char* mandIntf[1] = { NULL };
    const char* mandIntf2[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    // The following should not crash although the resulting observers are not useful
    EXPECT_EQ(NULL, alljoyn_observer_create(one.cbus, mandIntf, 1));
    EXPECT_EQ(NULL, alljoyn_observer_create(one.cbus, mandIntf2, 10));
    EXPECT_EQ(NULL, alljoyn_observer_create(one.cbus, NULL, 0));
    EXPECT_EQ(NULL, alljoyn_observer_create(one.cbus, mandIntf, 0));

    /* Test using same interface name twice */
    const char*doubleIntfA[] = { intfA[0].c_str(), intfA[0].c_str() };

    ObserverListener listener(one.cbus);
    alljoyn_observer obs5 = alljoyn_observer_create(one.cbus, doubleIntfA, 2);
    alljoyn_observer_registerlistener(obs5, listener.listener, QCC_TRUE);

    vector<qcc::String> oneIntfA;
    oneIntfA.push_back(intfA[0]);
    one.CreateObject("doubleIntfA", oneIntfA);

    vector<Event*> events;
    events.push_back(&(listener.event));

    listener.ExpectInvocations(1); // Should be triggered only once on object registration although we have duplicate interfaces
    one.RegisterObject("doubleIntfA");

    EXPECT_TRUE(WaitForAll(events));

    EXPECT_EQ(1, CountProxies(obs5)); // Make sure we have only one proxy for the remote object implementing duplicate interfaces

    listener.ExpectInvocations(1); // Should be triggered only once on object un-registration although we have duplicate interfaces

    one.UnregisterObject("doubleIntfA");

    EXPECT_TRUE(WaitForAll(events));

    alljoyn_observer_unregisterlistener(obs5, listener.listener);
    alljoyn_observer_destroy(obs5);
}

TEST_F(ObserverTest, RegisterListenerTwice) {

    /* Reuse the same listener for the same observer */
    Participant provider, consumer;
    provider.CreateObject("a", intfA);

    ObserverListener listener(consumer.cbus);
    listener.tolerateAlreadyDiscoveredObjects = true;
    alljoyn_observer obs = alljoyn_observer_create(consumer.cbus, cintfA, 1);

    alljoyn_observer_registerlistener(obs, listener.listener, QCC_TRUE);
    alljoyn_observer_registerlistener(obs, listener.listener, QCC_TRUE); // Intentional

    vector<Event*> events;
    events.push_back(&(listener.event));

    listener.ExpectInvocations(2); // Should be triggered twice on object registration as we registered the listener twice
    provider.RegisterObject("a");

    EXPECT_TRUE(WaitForAll(events));

    listener.ExpectInvocations(2); // Should be triggered twice on object un-registration as we registered the listener twice
    provider.UnregisterObject("a");

    EXPECT_TRUE(WaitForAll(events));

    alljoyn_observer_unregisterlistener(obs, listener.listener);

    listener.ExpectInvocations(1); // Should be triggered once on object registration as we removed one listener
    provider.RegisterObject("a");

    EXPECT_TRUE(WaitForAll(events));

    alljoyn_observer_unregisterlistener(obs, listener.listener);

    alljoyn_observer_destroy(obs);
}

TEST_F(ObserverTest, AnnounceLogicSanity) {

    Participant provider, consumer;
    ObserverListener listenerA(consumer.cbus);
    ObserverListener listenerB(consumer.cbus);

    provider.CreateObject("a", intfA);
    provider.CreateObject("b", intfB);

    provider.RegisterObject("a");
    provider.RegisterObject("b");

    vector<Event*> events;

    {
        alljoyn_observer obsA = alljoyn_observer_create(consumer.cbus, cintfA, 1);

        events.push_back(&(listenerA.event));

        listenerA.ExpectInvocations(1); // Object with intfA was at least discovered
        alljoyn_observer_registerlistener(obsA, listenerA.listener, QCC_TRUE);

        EXPECT_TRUE(WaitForAll(events));

        events.clear();
        events.push_back(&(listenerB.event));
        alljoyn_observer obsB = alljoyn_observer_create(consumer.cbus, cintfB, 1);
        listenerB.ExpectInvocations(1); // Object with intfB was at least discovered
        alljoyn_observer_registerlistener(obsB, listenerB.listener, QCC_TRUE);

        EXPECT_TRUE(WaitForAll(events));

        alljoyn_observer_destroy(obsA);
        alljoyn_observer_destroy(obsB);
    }

    events.clear();

    // Try creating Observer on IntfB after destroying Observer on InftA

    {
        alljoyn_observer obsA = alljoyn_observer_create(consumer.cbus, cintfA, 1);
        events.push_back(&(listenerA.event));
        listenerA.ExpectInvocations(1); // Object with intfA was at least discovered

        alljoyn_observer_registerlistener(obsA, listenerA.listener, QCC_TRUE);
        EXPECT_TRUE(WaitForAll(events));
        alljoyn_observer_unregisterlistener(obsA, listenerA.listener);
        alljoyn_observer_destroy(obsA);
    }

    events.clear();

    alljoyn_observer obsB = alljoyn_observer_create(consumer.cbus, cintfB, 1);
    events.push_back(&(listenerB.event));

    listenerB.ExpectInvocations(1);     // Object with intfB was at least discovered
    alljoyn_observer_registerlistener(obsB, listenerB.listener, QCC_TRUE);

    EXPECT_TRUE(WaitForAll(events));
    alljoyn_observer_unregisterlistener(obsB, listenerB.listener);

    provider.UnregisterObject("a");
    provider.UnregisterObject("b");

    alljoyn_observer_destroy(obsB);

}

TEST_F(ObserverTest, StressNumPartObjects) {

    // Stress the number of participants, observers and consumers

    vector<Participant*> providers(STRESS_FACTOR);
    vector<Participant*> consumers(STRESS_FACTOR);
    vector<ObserverListener*> listeners(STRESS_FACTOR);
    vector<alljoyn_observer> observers(STRESS_FACTOR);

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

        listeners[i] = new ObserverListener(consumers[i]->cbus);
        events.push_back(&(listeners[i]->event));
        listeners[i]->ExpectInvocations(2 * STRESS_FACTOR);

        if (listeners[i] == NULL) {
            break;     //clean up;
        }

        observers.push_back(NULL);
        observers[i] = alljoyn_observer_create(consumers[i]->cbus, cintfAB, 2);
        if (observers[i] == NULL) {
            break;       //clean up;
        }
        alljoyn_observer_registerlistener(observers[i], listeners[i]->listener, QCC_TRUE);
    }

    EXPECT_TRUE(WaitForAll(events));

    //clean up
    for (int i = 0; i < STRESS_FACTOR; i++) {
        alljoyn_observer_unregisterlistener(observers[i], listeners[i]->listener);
        alljoyn_observerlistener_destroy(listeners[i]->listener);
        alljoyn_observer_destroy(observers[i]);
        providers[i]->UnregisterObject("a");
        providers[i]->UnregisterObject("b");
        delete consumers[i];
        delete providers[i];
    }

}


}
