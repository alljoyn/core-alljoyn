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
#include <gtest/gtest.h>
#include <deque>
#include "ajTestCommon.h"
#include <qcc/Condition.h>
#include <qcc/Mutex.h>
#include <qcc/Util.h>
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <qcc/Thread.h>

using namespace ajn;
using namespace qcc;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.test.ProxyBusObjectTest";
static const char* OBJECT_NAME =    "org.alljoyn.test.ProxyBusObjectTest";
static const char* OBJECT_PATH =   "/org/alljoyn/test/ProxyObjectTest";

class ProxyBusObjectTestMethodHandlers {
  public:
    static void Ping(const InterfaceDescription::Member* member, Message& msg)
    {

    }

    static void Chirp(const InterfaceDescription::Member* member, Message& msg)
    {

    }
};

bool auth_complete_listener1_flag;
bool auth_complete_listener2_flag;
class ProxyBusObjectTestAuthListenerOne : public AuthListener {

    QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, void* context)
    {
        Credentials creds;
        EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return RequestCredentialsResponse(context, true, creds);
        }
        return RequestCredentialsResponse(context, false, creds);
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
        EXPECT_TRUE(success);
        auth_complete_listener1_flag = true;
    }
};

class ProxyBusObjectTestAuthListenerTwo : public AuthListener {

    QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, void* context)
    {
        Credentials creds;
        creds.SetPassword("123456");
        return RequestCredentialsResponse(context, true, creds);
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {

        EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
        EXPECT_TRUE(success);
        auth_complete_listener2_flag = true;
    }
};

class ProxyBusObjectTest : public testing::Test {
  public:
    ProxyBusObjectTest() :
        status(ER_FAIL),
        bus("ProxyBusObjectTest", false),
        servicebus("ProxyBusObjectTestservice", false),
        proxyBusObjectTestAuthListenerOne(NULL),
        proxyBusObjectTestAuthListenerTwo(NULL)
    { }

    virtual void SetUp() {
        status = bus.Start();
        EXPECT_EQ(ER_OK, status);
        status = bus.Connect(ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status);
    }

    virtual void TearDown() {
        bus.Stop();
        bus.Join();
        delete proxyBusObjectTestAuthListenerOne;
        delete proxyBusObjectTestAuthListenerTwo;
    }

    void SetUpProxyBusObjectTestService()
    {
        buslistener.name_owner_changed_flag = false;
        /* create/start/connect alljoyn_busattachment */
        status = servicebus.Start();
        EXPECT_EQ(ER_OK, status);
        status = servicebus.Connect(ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status);

        /* create/activate alljoyn_interface */
        InterfaceDescription* testIntf = NULL;
        status = servicebus.CreateInterface(INTERFACE_NAME, testIntf, false);
        EXPECT_EQ(ER_OK, status);
        EXPECT_TRUE(testIntf != NULL);
        if (testIntf != NULL) {
            status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
            EXPECT_EQ(ER_OK, status);
            status = testIntf->AddMember(MESSAGE_METHOD_CALL, "chirp", "s", "", "chirp", 0);
            EXPECT_EQ(ER_OK, status);
            testIntf->Activate();
        }
        servicebus.RegisterBusListener(buslistener);

        ProxyBusObjectTestBusObject testObj(OBJECT_PATH);

        const InterfaceDescription* exampleIntf = servicebus.GetInterface(INTERFACE_NAME);
        EXPECT_TRUE(exampleIntf);
        if (exampleIntf != NULL) {
            testObj.SetUp(*exampleIntf);
        }

        status = servicebus.RegisterBusObject(testObj);
        EXPECT_EQ(ER_OK, status);
        buslistener.name_owner_changed_flag = false; //make sure the flag is false

        /* request name */
        uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
        status = servicebus.RequestName(OBJECT_NAME, flags);
        EXPECT_EQ(ER_OK, status);
        for (size_t i = 0; i < 200; ++i) {
            if (buslistener.name_owner_changed_flag) {
                break;
            }
            qcc::Sleep(5);
        }
        EXPECT_TRUE(buslistener.name_owner_changed_flag);
    }

    void TearDownProxyBusObjectTestService()
    {
//        alljoyn_busattachment_unregisterbuslistener(servicebus, buslistener);
    }

    class ProxyBusObjectTestBusListener : public BusListener {
      public:
        ProxyBusObjectTestBusListener() : name_owner_changed_flag(false) { };
        void    NameOwnerChanged(const char*busName, const char*previousOwner, const char*newOwner) {
            if (strcmp(busName, OBJECT_NAME) == 0) {
                name_owner_changed_flag = true;
            }
        }

        bool name_owner_changed_flag;
    };

    class ProxyBusObjectTestBusObject : public BusObject {
      public:
        ProxyBusObjectTestBusObject(const char* path) :
            BusObject(path)
        {

        }

        void SetUp(const InterfaceDescription& intf)
        {
            QStatus status = ER_OK;
            status = AddInterface(intf);
            EXPECT_EQ(ER_OK, status);

            /* register method handlers */
            const InterfaceDescription::Member* ping_member = intf.GetMember("ping");
            ASSERT_TRUE(ping_member);

            const InterfaceDescription::Member* chirp_member = intf.GetMember("chirp");
            ASSERT_TRUE(chirp_member);

            /** Register the method handlers with the object */
            const MethodEntry methodEntries[] = {
                { ping_member, static_cast<MessageReceiver::MethodHandler>(&ProxyBusObjectTest::ProxyBusObjectTestBusObject::Ping) },
                { chirp_member, static_cast<MessageReceiver::MethodHandler>(&ProxyBusObjectTest::ProxyBusObjectTestBusObject::Chirp) }
            };
            status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
            EXPECT_EQ(ER_OK, status);
        }

        void Ping(const InterfaceDescription::Member* member, Message& msg)
        {

        }

        void Chirp(const InterfaceDescription::Member* member, Message& msg)
        {

        }
    };

    QStatus status;
    BusAttachment bus;

    BusAttachment servicebus;

    ProxyBusObjectTestBusListener buslistener;

    ProxyBusObjectTestAuthListenerOne* proxyBusObjectTestAuthListenerOne;
    ProxyBusObjectTestAuthListenerTwo* proxyBusObjectTestAuthListenerTwo;
};


TEST_F(ProxyBusObjectTest, ParseXml) {
    const char* busObjectXML =
        "<node name=\"/org/alljoyn/test/ProxyObjectTest\">"
        "  <interface name=\"org.alljoyn.test.ProxyBusObjectTest\">\n"
        "    <signal name=\"chirp\">\n"
        "      <arg name=\"chirp\" type=\"s\"/>\n"
        "    </signal>\n"
        "    <signal name=\"chirp2\">\n"
        "      <arg name=\"chirp\" type=\"s\" direction=\"out\"/>\n"
        "    </signal>\n"
        "    <method name=\"ping\">\n"
        "      <arg name=\"in\" type=\"s\" direction=\"in\"/>\n"
        "      <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
        "    </method>\n"
        "  </interface>\n"
        "</node>\n";
    QStatus status;

    ProxyBusObject proxyObj(bus, NULL, NULL, 0);
    status = proxyObj.ParseXml(busObjectXML, NULL);
    EXPECT_EQ(ER_OK, status);

    EXPECT_TRUE(proxyObj.ImplementsInterface("org.alljoyn.test.ProxyBusObjectTest"));

    const InterfaceDescription* testIntf = proxyObj.GetInterface("org.alljoyn.test.ProxyBusObjectTest");
    qcc::String introspect = testIntf->Introspect(0);

    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.ProxyBusObjectTest\">\n"
        "  <signal name=\"chirp\">\n"
        "    <arg name=\"chirp\" type=\"s\" direction=\"out\"/>\n"
        "  </signal>\n"
        "  <signal name=\"chirp2\">\n"
        "    <arg name=\"chirp\" type=\"s\" direction=\"out\"/>\n"
        "  </signal>\n"
        "  <method name=\"ping\">\n"
        "    <arg name=\"in\" type=\"s\" direction=\"in\"/>\n"
        "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n";
    EXPECT_STREQ(expectedIntrospect, introspect.c_str());
}

TEST_F(ProxyBusObjectTest, SecureConnection) {
    auth_complete_listener1_flag = false;
    auth_complete_listener2_flag = false;
    /* create/activate alljoyn_interface */
    InterfaceDescription* testIntf = NULL;
    status = servicebus.CreateInterface(INTERFACE_NAME, testIntf, false);
    EXPECT_EQ(ER_OK, status);
    ASSERT_TRUE(testIntf != NULL);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "chirp", "s", "", "chirp", 0);
    EXPECT_EQ(ER_OK, status);
    testIntf->Activate();

    ProxyBusObjectTestBusObject testObj(OBJECT_PATH);

    status = servicebus.Start();
    EXPECT_EQ(ER_OK, status);
    status = servicebus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status);

    status = servicebus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status);

    status = servicebus.RequestName(OBJECT_NAME, 0);

    proxyBusObjectTestAuthListenerOne = new ProxyBusObjectTestAuthListenerOne();
    status = servicebus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", proxyBusObjectTestAuthListenerOne);
    EXPECT_EQ(ER_OK, status);
    servicebus.ClearKeyStore();

    proxyBusObjectTestAuthListenerTwo = new ProxyBusObjectTestAuthListenerTwo();
    status = bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", proxyBusObjectTestAuthListenerTwo);
    EXPECT_EQ(ER_OK, status);
    bus.ClearKeyStore();

    ProxyBusObject proxy(bus, OBJECT_NAME, OBJECT_PATH, 0);

    status = proxy.SecureConnection();
    EXPECT_EQ(ER_OK, status);
}

TEST_F(ProxyBusObjectTest, SecureConnectionAsync) {
    auth_complete_listener1_flag = false;
    auth_complete_listener2_flag = false;

    /* create/activate alljoyn_interface */
    InterfaceDescription* testIntf = NULL;
    status = servicebus.CreateInterface(INTERFACE_NAME, testIntf, false);
    EXPECT_EQ(ER_OK, status);
    ASSERT_TRUE(testIntf != NULL);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "chirp", "s", "", "chirp", 0);
    EXPECT_EQ(ER_OK, status);
    testIntf->Activate();

    ProxyBusObjectTestBusObject testObj(OBJECT_PATH);

    status = servicebus.Start();
    EXPECT_EQ(ER_OK, status);
    status = servicebus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status);

    status = servicebus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status);

    status = servicebus.RequestName(OBJECT_NAME, 0);

    proxyBusObjectTestAuthListenerOne = new ProxyBusObjectTestAuthListenerOne();
    status = servicebus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", proxyBusObjectTestAuthListenerOne);
    EXPECT_EQ(ER_OK, status);
    servicebus.ClearKeyStore();

    proxyBusObjectTestAuthListenerTwo = new ProxyBusObjectTestAuthListenerTwo();
    status = bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", proxyBusObjectTestAuthListenerTwo);
    EXPECT_EQ(ER_OK, status);
    bus.ClearKeyStore();

    ProxyBusObject proxy(bus, OBJECT_NAME, OBJECT_PATH, 0);

    status = proxy.SecureConnectionAsync();
    EXPECT_EQ(ER_OK, status);
    for (size_t i = 0; i < 200; ++i) {
        if (auth_complete_listener1_flag && auth_complete_listener2_flag) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(auth_complete_listener1_flag);
    EXPECT_TRUE(auth_complete_listener2_flag);
}

TEST_F(ProxyBusObjectTest, GetChildren) {
    QStatus status = ER_FAIL;

    InterfaceDescription* testIntf = NULL;
    bus.CreateInterface("org.alljoyn.test.ProxyBusObjectTest", testIntf, false);
    ASSERT_TRUE(testIntf != NULL);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status);

    ProxyBusObject proxyObjChildOne(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest/ChildOne", 0);
    ProxyBusObject proxyObjChildTwo(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest/ChildTwo", 0);

    status = proxyObjChildOne.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status);
    status = proxyObjChildTwo.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status);

    ProxyBusObject proxyObj(bus, NULL, NULL, 0);

    status = proxyObj.AddChild(proxyObjChildOne);
    EXPECT_EQ(ER_OK, status);
    status = proxyObj.AddChild(proxyObjChildTwo);
    EXPECT_EQ(ER_OK, status);

    EXPECT_TRUE(proxyObj.IsValid());

    ProxyBusObject* proxyObjSub = proxyObj.GetChild("/org/alljoyn/test/ProxyObjectTest");
    ASSERT_TRUE(proxyObjSub != NULL);

    size_t numChildren = 0;
    ProxyBusObject** children = NULL;
    numChildren = proxyObjSub->GetChildren();
    EXPECT_EQ((size_t)2, numChildren);

    children = new ProxyBusObject*[numChildren];
    proxyObjSub->GetChildren(children, numChildren);

    for (size_t i = 0; i < numChildren; ++i) {
        ASSERT_TRUE(children[i]) << "Test interface for children[" << i << "] should not be NULL.";
        ASSERT_TRUE(children[i]->IsValid()) << "Test interface for children[" << i << "] should a valid ProxyBusObject.";
        EXPECT_TRUE(children[i]->ImplementsInterface("org.alljoyn.test.ProxyBusObjectTest")) <<
            "Test interface for children[" << i << "] should implement the org.alljoyn.test.ProxyBusObjectTest interface.";

        const InterfaceDescription* childIntf = children[i]->GetInterface("org.alljoyn.test.ProxyBusObjectTest");
        qcc::String introspect = childIntf->Introspect();

        const char* expectedIntrospect =
            "<interface name=\"org.alljoyn.test.ProxyBusObjectTest\">\n"
            "  <method name=\"ping\">\n"
            "    <arg name=\"in\" type=\"s\" direction=\"in\"/>\n"
            "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
            "  </method>\n"
            "</interface>\n";
        EXPECT_STREQ(expectedIntrospect, introspect.c_str()) <<
            "Test interface for children[" << i << "] did not have expected introspection.";
    }

    status = proxyObj.RemoveChild("/org/alljoyn/test/ProxyObjectTest/ChildOne");
    EXPECT_EQ(ER_OK, status);

    ProxyBusObject* removedProxyChild = proxyObj.GetChild("/org/alljoyn/test/ProxyObjectTest/ChildOne");
    EXPECT_EQ(NULL, removedProxyChild);
    delete [] children;
}

// ALLJOYN-1908
TEST_F(ProxyBusObjectTest, AddChild_regressionTest) {
    QStatus status = ER_FAIL;

    InterfaceDescription* testIntf = NULL;
    bus.CreateInterface("org.alljoyn.test.ProxyBusObjectTest", testIntf, false);
    ASSERT_TRUE(testIntf != NULL);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status);

    ProxyBusObject proxyObjChildOne(bus, "org.alljoyn.test.ProxyBusObjectTest", "/aa/a", 0);
    ProxyBusObject proxyObjChildTwo(bus, "org.alljoyn.test.ProxyBusObjectTest", "/ab/a", 0);

    status = proxyObjChildOne.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status);
    status = proxyObjChildTwo.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status);

    ProxyBusObject proxyObj(bus, NULL, NULL, 0);

    status = proxyObj.AddChild(proxyObjChildOne);
    EXPECT_EQ(ER_OK, status);
    status = proxyObj.AddChild(proxyObjChildTwo);
    EXPECT_EQ(ER_OK, status);

    EXPECT_TRUE(proxyObj.IsValid());

    size_t numChildren = 0;
    numChildren = proxyObj.GetChildren();
    //if ALLJOYN-1908 were not fixed this would return 1
    EXPECT_EQ((size_t)2, numChildren);
}

// ALLJOYN-2043
TEST_F(ProxyBusObjectTest, AddPropertyInterfaceError) {
    const InterfaceDescription* propIntf = bus.GetInterface(::ajn::org::freedesktop::DBus::Properties::InterfaceName);

    InterfaceDescription* testIntf = NULL;
    bus.CreateInterface("org.alljoyn.test.ProxyBusObjectTest", testIntf, false);
    ASSERT_TRUE(testIntf != NULL);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status);
    status = testIntf->AddProperty("stringProp", "s", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status);
    testIntf->Activate();

    ProxyBusObject proxyObj(bus, NULL, NULL, 0);

    status = proxyObj.AddInterface(*propIntf);
    EXPECT_EQ(ER_OK, status);

    status = proxyObj.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status);
}

// ASACORE-1521
class ChangeListener : public ProxyBusObject::PropertiesChangedListener {
  public:
    enum Operation { WAIT, REMOVE };

    ChangeListener(BusAttachment& bus, Operation op, Mutex& testLock, Condition& testCond) :
        running(false),
        didRun(false),
        unregStatus(ER_NONE),
        bus(bus),
        op(op),
        remObj(NULL),
        remIfaceName(NULL),
        remListener(NULL),
        removed(false),
        testLock(testLock),
        testCond(testCond)
    { }

    void SetRemoveListener(ProxyBusObject& remObj,
                           const char* remIfaceName,
                           ChangeListener& remListener)
    {
        this->remObj = &remObj;
        this->remIfaceName = remIfaceName;
        this->remListener = &remListener;
    }

    void RemoveListener()
    {
        unregStatus = remObj->UnregisterPropertiesChangedListener(remIfaceName, *remListener);

        remListener->lock.Lock();
        remListener->removed = true;
        remListener->lock.Unlock();

        lock.Lock();
        removed = true;
        cond.Signal();
        lock.Unlock();
    }

    void PropertiesChanged(ProxyBusObject& obj,
                           const char* ifaceName,
                           const MsgArg& changed,
                           const MsgArg& invalidated,
                           void* context)
    {
        testLock.Lock();
        running = true;
        testCond.Signal();
        testLock.Unlock();

        /*
         * Need to enable concurrent callbacks so that listeners can be
         * unregistered from other threads.
         */
        bus.EnableConcurrentCallbacks();

        switch (op) {
        case WAIT:
            lock.Lock();
            while (!removed) {
                cond.Wait(lock);
            }
            lock.Unlock();
            break;

        case REMOVE:
            RemoveListener();
            break;
        }

        testLock.Lock();
        didRun = true;
        running = false;
        testCond.Signal();
        testLock.Unlock();
    }

    bool running;
    bool didRun;
    QStatus unregStatus;

  private:
    BusAttachment& bus;
    Operation op;
    ProxyBusObject* remObj;
    const char* remIfaceName;
    ChangeListener* remListener;
    Mutex lock;
    Condition cond;
    bool removed;
    Mutex& testLock;
    Condition& testCond;
};

class UnregisterThread : public Thread {
  public:
    UnregisterThread(Mutex& testLock, Condition& testCond) :
        Thread("UnregisterTestThread"),
        testLock(testLock),
        testCond(testCond)
    { }

    void AddChangeListener(ChangeListener& listener) { listeners.push_back(&listener); }

    ThreadReturn STDCALL Run(void* arg)
    {
        QStatus status = ER_OK;
        ChangeListener* l = NULL;
        bool wait = true;
        testLock.Lock();
        while (wait && (status == ER_OK)) {
            for (std::deque<ChangeListener*>::iterator it = listeners.begin(); it != listeners.end(); ++it) {
                l = *it;
                if (l->running) {
                    wait = false;
                    break;
                }
            }
            if (wait) {
                status = testCond.TimedWait(testLock, 1000);
            }
        }
        testLock.Unlock();
        if (l && status == ER_OK) {
            l->RemoveListener();
        }
        return (ThreadReturn)status;
    }

  private:
    std::deque<ChangeListener*> listeners;
    Mutex& testLock;
    Condition& testCond;
};

TEST_F(ProxyBusObjectTest, UnregisterPropertiesChangedListenerRaceTest1) {
    /*
     * First test for ASACORE-1521: removing listener from within a handler.
     *
     * In this test case, we setup 2 listeners that will unregister the other
     * when run.  A passing condition is that only one of the listeners will run
     * successfully.
     */
    Mutex lock;
    Condition cond;

    BusObject testObj(OBJECT_PATH);
    bus.RegisterBusObject(testObj);

    InterfaceDescription* testIntf1 = NULL;
    bus.CreateInterface(INTERFACE_NAME, testIntf1, false);
    ASSERT_TRUE(testIntf1 != NULL);

    status = testIntf1->AddProperty("stringProp1", "s", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status);
    testIntf1->AddPropertyAnnotation("stringProp1", org::freedesktop::DBus::AnnotateEmitsChanged, "true");
    testIntf1->Activate();

    ProxyBusObject proxyObj(bus, bus.GetUniqueName().c_str(), OBJECT_PATH, 0);
    MsgArg arg("s", "foo");

    status = proxyObj.AddInterface(*testIntf1);
    EXPECT_EQ(ER_OK, status);

    ChangeListener removeListener1(bus, ChangeListener::REMOVE, lock, cond);
    ChangeListener removeListener2(bus, ChangeListener::REMOVE, lock, cond);

    proxyObj.RegisterPropertiesChangedListener(INTERFACE_NAME, NULL, 0, removeListener1, NULL);
    proxyObj.RegisterPropertiesChangedListener(INTERFACE_NAME, NULL, 0, removeListener2, NULL);

    /* Set each listener to remove the other. */
    removeListener1.SetRemoveListener(proxyObj, INTERFACE_NAME, removeListener2);
    removeListener2.SetRemoveListener(proxyObj, INTERFACE_NAME, removeListener1);

    testObj.EmitPropChanged(INTERFACE_NAME, "stringProp1", arg, 0);

    lock.Lock();
    status = ER_OK;
    /* First wait for at least one listener to run. */
    while (!removeListener1.didRun && !removeListener2.didRun) {
        status = cond.TimedWait(lock, 1000);  // Should not timeout
        if (status != ER_OK) {
            break;
        }
    }
    EXPECT_EQ(ER_OK, status);

    /*
     * If only only one listener ran, git the second a chance to run and verify
     * that it did not.
     */
    if (!(removeListener1.didRun && removeListener2.didRun)) {
        status = cond.TimedWait(lock, 100);  // Should timeout
        EXPECT_EQ(ER_TIMEOUT, status) << "Second listener still called.";
    }
    lock.Unlock();

    /* Verify that only one listener ran. */
    EXPECT_TRUE((removeListener1.didRun && !removeListener2.didRun) ||
                (!removeListener1.didRun && removeListener2.didRun)) << ((removeListener1.didRun && removeListener2.didRun) ?
                                                                         "both ran" :
                                                                         "neither ran");

    /*
     * We don't know which removeListener was called.  Thus the value of
     * unregStatus for each of the listeners depends on which listener was
     * called.  In some respects this is a bit redundant with the test above,
     * but it does ensure that the correct status code is set.
     */
    if (removeListener1.didRun) {
        EXPECT_EQ(ER_OK, removeListener1.unregStatus);
        EXPECT_EQ(ER_NONE, removeListener2.unregStatus);
    }
    if (removeListener2.didRun) {
        EXPECT_EQ(ER_NONE, removeListener1.unregStatus);
        EXPECT_EQ(ER_OK, removeListener2.unregStatus);
    }

    proxyObj.UnregisterPropertiesChangedListener(INTERFACE_NAME, removeListener1);
    proxyObj.UnregisterPropertiesChangedListener(INTERFACE_NAME, removeListener2);
}

TEST_F(ProxyBusObjectTest, UnregisterPropertiesChangedListenerRaceTest2) {
    /*
     * Second test for ASACORE-1521: remove listener from other thread.
     *
     * In this test case, we setup 2 listeners that block waiting for a
     * condition variable to be signaled.  We also setup a secondary thread that
     * will wait for one of the listeners to start so that it can unregister the
     * other listener.  A passing condition is that only one of the listeners is
     * called.
     */
    Mutex lock;
    Condition cond;

    BusObject testObj(OBJECT_PATH);
    bus.RegisterBusObject(testObj);

    InterfaceDescription* testIntf1 = NULL;
    bus.CreateInterface(INTERFACE_NAME, testIntf1, false);
    ASSERT_TRUE(testIntf1 != NULL);

    status = testIntf1->AddProperty("stringProp1", "s", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status);
    testIntf1->AddPropertyAnnotation("stringProp1", org::freedesktop::DBus::AnnotateEmitsChanged, "true");
    testIntf1->Activate();

    ProxyBusObject proxyObj(bus, bus.GetUniqueName().c_str(), OBJECT_PATH, 0);
    MsgArg arg("s", "foo");

    status = proxyObj.AddInterface(*testIntf1);
    EXPECT_EQ(ER_OK, status);

    ChangeListener waitListener1(bus, ChangeListener::WAIT, lock, cond);
    ChangeListener waitListener2(bus, ChangeListener::WAIT, lock, cond);
    UnregisterThread thread(lock, cond);

    proxyObj.RegisterPropertiesChangedListener(INTERFACE_NAME, NULL, 0, waitListener1, NULL);
    proxyObj.RegisterPropertiesChangedListener(INTERFACE_NAME, NULL, 0, waitListener2, NULL);

    /* Identify which listener to remove according to which runs first. */
    waitListener1.SetRemoveListener(proxyObj, INTERFACE_NAME, waitListener2);
    waitListener2.SetRemoveListener(proxyObj, INTERFACE_NAME, waitListener1);

    /* Let the thread know what listeners to wait for. */
    thread.AddChangeListener(waitListener1);
    thread.AddChangeListener(waitListener2);

    /* Start the thread waiting for a listener to start. */
    thread.Start();

    testObj.EmitPropChanged(INTERFACE_NAME, "stringProp1", arg, 0);

    /* The thread will exit automatically once it has removed a listener. */
    thread.Join();

    EXPECT_EQ(ER_OK, (QStatus)(intptr_t)thread.GetExitValue());

    lock.Lock();
    status = ER_OK;
    /* Wait for at least one listener to complete. */
    while (!waitListener1.didRun && !waitListener2.didRun) {
        status = cond.TimedWait(lock, 1000);  // Should not timeout
        if (status != ER_OK) {
            break;
        }
    }
    EXPECT_EQ(ER_OK, status);

    /* Make sure the second listener did not run. */
    if (!(waitListener1.didRun && waitListener2.didRun)) {
        status = cond.TimedWait(lock, 100);  // Should timeout
        EXPECT_EQ(ER_TIMEOUT, status) << "Second listener still called.";
    }
    lock.Unlock();

    /* Verify that only one listener ran. */
    EXPECT_TRUE((waitListener1.didRun && !waitListener2.didRun) ||
                (!waitListener1.didRun && waitListener2.didRun)) << ((waitListener1.didRun && waitListener2.didRun) ?
                                                                     "both ran" :
                                                                     "neither ran");

    proxyObj.UnregisterPropertiesChangedListener(INTERFACE_NAME, waitListener1);
    proxyObj.UnregisterPropertiesChangedListener(INTERFACE_NAME, waitListener2);
}


TEST_F(ProxyBusObjectTest, UnregisterPropertiesChangedListenerRaceTest3) {
    /* Third test for ASACORE-1521: attempt to have listener remove itself (fail case)
     *
     * In this test case, we setup a listener to attempt to remove itself.  A
     * passing condition is that the listener fails to remove itself.
     */
    Mutex lock;
    Condition cond;

    BusObject testObj(OBJECT_PATH);
    bus.RegisterBusObject(testObj);

    InterfaceDescription* testIntf1 = NULL;
    bus.CreateInterface(INTERFACE_NAME, testIntf1, false);
    ASSERT_TRUE(testIntf1 != NULL);

    status = testIntf1->AddProperty("stringProp1", "s", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status);
    testIntf1->AddPropertyAnnotation("stringProp1", org::freedesktop::DBus::AnnotateEmitsChanged, "true");
    testIntf1->Activate();

    ProxyBusObject proxyObj(bus, bus.GetUniqueName().c_str(), OBJECT_PATH, 0);
    MsgArg arg("s", "foo");

    status = proxyObj.AddInterface(*testIntf1);
    EXPECT_EQ(ER_OK, status);

    ChangeListener failListener(bus, ChangeListener::REMOVE, lock, cond);

    proxyObj.RegisterPropertiesChangedListener(INTERFACE_NAME, NULL, 0, failListener, NULL);

    /* Setup the listener to remove itself. */
    failListener.SetRemoveListener(proxyObj, INTERFACE_NAME, failListener);

    testObj.EmitPropChanged(INTERFACE_NAME, "stringProp1", arg, 0);

    lock.Lock();
    /* Wait for the listener to run. */
    while (!failListener.didRun) {
        status = cond.TimedWait(lock, 1000);  // Should not timeout
        if (status != ER_OK) {
            break;
        }
    }
    lock.Unlock();

    /* Verify that it did run and that it failed. */
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(failListener.didRun);
    EXPECT_EQ(ER_DEADLOCK, failListener.unregStatus);

    proxyObj.UnregisterPropertiesChangedListener(INTERFACE_NAME, failListener);
}


TEST_F(ProxyBusObjectTest, UnregisterPropertiesChangedListenerRaceTest4) {
    /* Fourth test for ASACORE-1521: forget to unregister listener before destroying ProxyBusObject
     *
     * In this test case, we setup a listener to remove itself but destroy
     * associated ProxyBusObject before the PropertiesChangedSignal is sent.
     */
    Mutex lock;
    Condition cond;

    BusObject testObj(OBJECT_PATH);
    bus.RegisterBusObject(testObj);

    InterfaceDescription* testIntf1 = NULL;
    bus.CreateInterface(INTERFACE_NAME, testIntf1, false);
    ASSERT_TRUE(testIntf1 != NULL);

    status = testIntf1->AddProperty("stringProp1", "s", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status);
    testIntf1->AddPropertyAnnotation("stringProp1", org::freedesktop::DBus::AnnotateEmitsChanged, "true");
    testIntf1->Activate();

    ProxyBusObject* proxyObj = new ProxyBusObject(bus, bus.GetUniqueName().c_str(), OBJECT_PATH, 0);
    MsgArg arg("s", "foo");

    status = proxyObj->AddInterface(*testIntf1);
    EXPECT_EQ(ER_OK, status);

    ChangeListener neverCalledListener(bus, ChangeListener::REMOVE, lock, cond);

    proxyObj->RegisterPropertiesChangedListener(INTERFACE_NAME, NULL, 0, neverCalledListener, NULL);

    /* Setup the listener to remove itself. */
    neverCalledListener.SetRemoveListener(*proxyObj, INTERFACE_NAME, neverCalledListener);

    delete proxyObj;

    testObj.EmitPropChanged(INTERFACE_NAME, "stringProp1", arg, 0);

    lock.Lock();
    /* Wait for the listener to be called (which should not happen). */
    while (!neverCalledListener.didRun) {
        status = cond.TimedWait(lock, 100);  // Should timeout
        if (status != ER_OK) {
            break;
        }
    }
    lock.Unlock();

    /* Verify that the listener was never called. */
    EXPECT_EQ(ER_TIMEOUT, status);
    EXPECT_FALSE(neverCalledListener.didRun);
}
