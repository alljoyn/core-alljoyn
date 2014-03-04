/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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
#include "ajTestCommon.h"
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
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
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = bus.Connect(ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = servicebus.Connect(ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* create/activate alljoyn_interface */
        InterfaceDescription* testIntf = NULL;
        status = servicebus.CreateInterface(INTERFACE_NAME, testIntf, false);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(testIntf != NULL);
        if (testIntf != NULL) {
            status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = testIntf->AddMember(MESSAGE_METHOD_CALL, "chirp", "s", "", "chirp", 0);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        buslistener.name_owner_changed_flag = false; //make sure the flag is false

        /* request name */
        uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
        status = servicebus.RequestName(OBJECT_NAME, flags);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "chirp", "s", "", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    testIntf->Activate();

    ProxyBusObjectTestBusObject testObj(OBJECT_PATH);

    status = servicebus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = servicebus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = servicebus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = servicebus.RequestName(OBJECT_NAME, 0);

    proxyBusObjectTestAuthListenerOne = new ProxyBusObjectTestAuthListenerOne();
    status = servicebus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", proxyBusObjectTestAuthListenerOne);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicebus.ClearKeyStore();

    proxyBusObjectTestAuthListenerTwo = new ProxyBusObjectTestAuthListenerTwo();
    status = bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", proxyBusObjectTestAuthListenerTwo);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    bus.ClearKeyStore();

    ProxyBusObject proxy(bus, OBJECT_NAME, OBJECT_PATH, 0);

    status = proxy.SecureConnection();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(ProxyBusObjectTest, SecureConnectionAsync) {
    auth_complete_listener1_flag = false;
    auth_complete_listener2_flag = false;

    /* create/activate alljoyn_interface */
    InterfaceDescription* testIntf = NULL;
    status = servicebus.CreateInterface(INTERFACE_NAME, testIntf, false);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = testIntf->AddMember(MESSAGE_METHOD_CALL, "chirp", "s", "", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    testIntf->Activate();

    ProxyBusObjectTestBusObject testObj(OBJECT_PATH);

    status = servicebus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = servicebus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = servicebus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = servicebus.RequestName(OBJECT_NAME, 0);

    proxyBusObjectTestAuthListenerOne = new ProxyBusObjectTestAuthListenerOne();
    status = servicebus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", proxyBusObjectTestAuthListenerOne);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicebus.ClearKeyStore();

    proxyBusObjectTestAuthListenerTwo = new ProxyBusObjectTestAuthListenerTwo();
    status = bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", proxyBusObjectTestAuthListenerTwo);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    bus.ClearKeyStore();

    ProxyBusObject proxy(bus, OBJECT_NAME, OBJECT_PATH, 0);

    status = proxy.SecureConnectionAsync();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject proxyObjChildOne(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest/ChildOne", 0);
    ProxyBusObject proxyObjChildTwo(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest/ChildTwo", 0);

    status = proxyObjChildOne.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = proxyObjChildTwo.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject proxyObj(bus, NULL, NULL, 0);

    status = proxyObj.AddChild(proxyObjChildOne);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = proxyObj.AddChild(proxyObjChildTwo);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(proxyObj.IsValid());

    ProxyBusObject* proxyObjSub = proxyObj.GetChild("/org/alljoyn/test/ProxyObjectTest");
    ASSERT_TRUE(proxyObjSub != NULL);

    size_t numChildren = 0;
    ProxyBusObject** children = NULL;
    numChildren = proxyObjSub->GetChildren();
    EXPECT_EQ((size_t)2, numChildren);

    children = new ProxyBusObject *[numChildren];
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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject proxyObjChildOne(bus, "org.alljoyn.test.ProxyBusObjectTest", "/aa/a", 0);
    ProxyBusObject proxyObjChildTwo(bus, "org.alljoyn.test.ProxyBusObjectTest", "/ab/a", 0);

    status = proxyObjChildOne.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = proxyObjChildTwo.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject proxyObj(bus, NULL, NULL, 0);

    status = proxyObj.AddChild(proxyObjChildOne);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = proxyObj.AddChild(proxyObjChildTwo);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = testIntf->AddProperty("stringProp", "s", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    testIntf->Activate();

    ProxyBusObject proxyObj(bus, NULL, NULL, 0);

    status = proxyObj.AddInterface(*propIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = proxyObj.AddInterface(*testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
