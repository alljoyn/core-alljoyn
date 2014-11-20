/**
 * @file
 * This program tests the basic features of Alljoyn.It uses google test as the test
 * automation framework.
 */
/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include "ServiceSetup.h"
#include "ClientSetup.h"
#include "ServiceTestObject.h"
#include "ajTestCommon.h"

#include <qcc/time.h>
/* Header files included for Google Test Framework */
#include <gtest/gtest.h>

/* client waits for this event during findAdvertisedName */
static Event g_discoverEvent;

/** Client Listener to receive advertisements  */
class ClientBusListener : public BusListener {
  public:
    ClientBusListener() : BusListener() { }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        //QCC_SyncPrintf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);
        g_discoverEvent.SetEvent();
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        //QCC_SyncPrintf("LostAdvertisedName(name=%s, transport=0x%x, namePrefix=%s)\n", name, transport, namePrefix);
    }
};

class PerfTest : public::testing::Test {
  protected:
    PerfTest() : serviceBus(NULL),
        myService(NULL),
        serviceTestObject(NULL),
        myBusListener(NULL),
        clientListener(NULL)
    { }

    virtual void SetUp()
    {
        QStatus status = ER_OK;
        InterfaceDescription* regTestIntf = NULL;

        serviceBus = new BusAttachment("bbtestservices", true);

        if (!serviceBus->IsStarted()) {
            status = serviceBus->Start();
        }

        if (ER_OK == status && !serviceBus->IsConnected()) {
            /* Connect to the daemon and wait for the bus to exit */
            status = serviceBus->Connect(getConnectArg().c_str());
        }
        myService = new ServiceObject(*serviceBus, "/org/alljoyn/test_services");

        /* Invoking the Bus Listener */
        myBusListener = new MyBusListener();
        serviceBus->RegisterBusListener(*myBusListener);


        status = serviceBus->CreateInterface(myService->getAlljoynInterfaceName(), regTestIntf);
        assert(regTestIntf);

        /* Adding a signal to no */
        status = regTestIntf->AddSignal("my_signal", "s", NULL, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf->AddSignal("my_signal_string", "us", NULL, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf->AddMember(MESSAGE_METHOD_CALL, "my_ping", "s",  "s", "o,i", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf->AddMember(MESSAGE_METHOD_CALL, "my_sing", "s",  "s", "o,i", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf->AddMember(MESSAGE_METHOD_CALL, "my_param_test", "ssssssssss", "ssssssssss", "iiiiiiiiii,oooooooooo", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        regTestIntf->Activate();
        status = myService->AddInterfaceToObject(regTestIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        InterfaceDescription* valuesIntf = NULL;
        status = serviceBus->CreateInterface(myService->getAlljoynValuesInterfaceName(), valuesIntf);
        assert(valuesIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf->AddProperty("int_val", "i", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf->AddProperty("str_val", "s", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf->AddProperty("ro_str", "s", PROP_ACCESS_READ);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf->AddProperty("prop_signal", "s", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        valuesIntf->Activate();
        status = myService->AddInterfaceToObject(valuesIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Populate the signal handler members */
        myService->PopulateSignalMembers();
        status = myService->InstallMethodHandlers();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status =  serviceBus->RegisterBusObject(*myService);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Request a well-known name */
        status = serviceBus->RequestName(myService->getAlljoynWellKnownName(),
                                         DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Creating a session */
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY,
                         TRANSPORT_ANY);
        SessionPort sessionPort = 550;
        status = serviceBus->BindSessionPort(sessionPort, opts, *myBusListener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Advertising this well known name */
        status = serviceBus->AdvertiseName(myService->getAlljoynWellKnownName(), TRANSPORT_ANY);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Adding the second object */
        status = ER_OK;
        serviceTestObject = new ServiceTestObject(*serviceBus, myService->getServiceObjectPath());

        InterfaceDescription* regTestIntf2 = NULL;
        status = serviceBus->CreateInterface(myService->getServiceInterfaceName(), regTestIntf2);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        assert(regTestIntf2);

        /* Adding a signal to no */
        status = regTestIntf2->AddSignal("my_signal", "s", NULL, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "my_ping", "s",  "s", "o,i", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "ByteArrayTest", "ay", "ay", "i,o", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "my_sing", "s",  "s", "o,i", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "my_king", "s", "s", "i,o", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "DoubleArrayTest", "ad", "ad", "i,o", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        regTestIntf2->Activate();
        status = serviceTestObject->AddInterfaceToObject(regTestIntf2);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        InterfaceDescription* valuesIntf2 = NULL;
        status = serviceBus->CreateInterface(myService->getServiceValuesInterfaceName(), valuesIntf2);
        assert(valuesIntf2);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf2->AddProperty("int_val", "i", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf2->AddProperty("str_val", "s", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf2->AddProperty("ro_str", "s", PROP_ACCESS_READ);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        valuesIntf2->Activate();
        status = serviceTestObject->AddInterfaceToObject(valuesIntf2);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Populate the signal handler members */
        serviceTestObject->PopulateSignalMembers(myService->getServiceInterfaceName());
        status = serviceTestObject->InstallMethodHandlers(myService->getServiceInterfaceName());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status =  serviceBus->RegisterBusObject(*serviceTestObject);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        BusAttachment* deleteMe = serviceBus;
        serviceBus = NULL;
        delete deleteMe;
        ServiceTestObject* deleteTestObj = serviceTestObject;
        serviceTestObject = NULL;
        delete deleteTestObj;
        ServiceObject* deleteServiceObject = myService;
        myService = NULL;
        delete deleteServiceObject;
        MyBusListener* deleteBusListener = myBusListener;
        myBusListener = NULL;
        delete deleteBusListener;
        delete clientListener;
    }

    BusAttachment* serviceBus;

    ServiceObject* myService;
    ServiceTestObject* serviceTestObject;
    MyBusListener* myBusListener;
    ClientBusListener* clientListener;
};

//BusAttachment* PerfTest::serviceBus = NULL;

TEST_F(PerfTest, Introspect_CorrectParameters)
{
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    ProxyBusObject remoteObj(*(testclient.getClientMsgBus()),
                             myService->getAlljoynWellKnownName(),
                             testclient.getClientObjectPath(),
                             0);

    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(PerfTest, ErrorMsg_Error_invalid_path) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    /* Invalid path  - does not begin with '/' */
    ProxyBusObject remoteObj(*(testclient.getClientMsgBus()),
                             myService->getAlljoynWellKnownName(),
                             "org/alljoyn/alljoyn_test1",
                             0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_BUS_BAD_OBJ_PATH, status) << "  Actual Status: " << QCC_StatusText(status);


}

TEST_F(PerfTest, ErrorMsg_Error_no_such_object) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    /* Valid path but-existant  */
    ProxyBusObject remoteObj(*test_msgBus,
                             myService->getAlljoynWellKnownName(),
                             "/org/alljoyn/alljoyn_test1",
                             0);

    QStatus status = remoteObj.IntrospectRemoteObject();
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Instead of directly making an introspect...make a method call and get the error reply */
    const InterfaceDescription* introIntf = test_msgBus->GetInterface(ajn::org::freedesktop::DBus::Introspectable::InterfaceName);
    remoteObj.AddInterface(*introIntf);
    ASSERT_TRUE(introIntf);

    /* Attempt to retrieve introspection from the remote object using sync call */
    Message reply(*test_msgBus);
    const InterfaceDescription::Member* introMember = introIntf->GetMember("Introspect");
    ASSERT_TRUE(introMember);
    status = remoteObj.MethodCall(*introMember, NULL, 0, reply, 5000);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);

    String errMsg;
    reply->GetErrorName(&errMsg);

    EXPECT_STREQ(QCC_StatusText(ER_BUS_NO_SUCH_OBJECT), errMsg.c_str());
}

TEST_F(PerfTest, ErrorMsg_does_not_exist_interface) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    /* Valid well known name - But does not exist  */
    ProxyBusObject remoteObj(*test_msgBus,
                             "org.alljoyn.alljoyn_test.Interface1",
                             testclient.getClientObjectPath(),
                             0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    EXPECT_TRUE(status == ER_BUS_REPLY_IS_ERROR_MESSAGE) <<
    "Expected ER_BUS_REPLY_IS_ERROR_MESSAGE \n\tActual Status: " << QCC_StatusText(status);

    /* Instead of directly making an introspect...make a method call and get the error reply */
    const InterfaceDescription* introIntf = test_msgBus->GetInterface(ajn::org::freedesktop::DBus::Introspectable::InterfaceName);
    remoteObj.AddInterface(*introIntf);
    ASSERT_TRUE(introIntf);

    /* Attempt to retrieve introspection from the remote object using sync call */
    Message reply(*test_msgBus);
    const InterfaceDescription::Member* introMember = introIntf->GetMember("Introspect");
    ASSERT_TRUE(introMember);
    status = remoteObj.MethodCall(*introMember, NULL, 0, reply, 5000);
    EXPECT_TRUE(status == ER_BUS_REPLY_IS_ERROR_MESSAGE) <<
    "Expected ER_BUS_REPLY_IS_ERROR_MESSAGE \n\tActual Status: " << QCC_StatusText(status);

    if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
        String errMsg;
        reply->GetErrorName(&errMsg);
        EXPECT_STREQ("Unknown bus name: org.alljoyn.alljoyn_test.Interface1", errMsg.c_str());
    }
}

TEST_F(PerfTest, ErrorMsg_MethodCallOnNonExistantMethod) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    ProxyBusObject remoteObj(*test_msgBus, myService->getAlljoynWellKnownName(), testclient.getClientObjectPath(), 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(*test_msgBus);
    MsgArg pingStr("s", "Test Ping");
    status = remoteObj.MethodCall(testclient.getClientInterfaceName(), "my_unknown", &pingStr, 1, reply, 5000);
    EXPECT_EQ(ER_BUS_INTERFACE_NO_SUCH_MEMBER, status) << "  Actual Status: " << QCC_StatusText(status);
}


/* Test LargeParameters for a synchronous method call */

TEST_F(PerfTest, MethodCallTest_LargeParameters) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());
    QStatus status = testclient.MethodCall(100, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

/* Test for synchronous method call */

TEST_F(PerfTest, MethodCallTest_SimpleCall) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    QStatus status = testclient.MethodCall(1, 1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST_F(PerfTest, MethodCallTest_EmptyParameters) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    QStatus status = testclient.MethodCall(1, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST_F(PerfTest, MethodCallTest_InvalidParameters) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    QStatus status = testclient.MethodCall(1, 4);
    EXPECT_EQ(ER_BUS_UNEXPECTED_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
}

/* Test signals */
TEST_F(PerfTest, Properties_SimpleSignal) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    ProxyBusObject remoteObj(*test_msgBus, myService->getAlljoynWellKnownName(), testclient.getClientObjectPath(), 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    MsgArg newName("s", "New returned name");
    status = remoteObj.SetProperty(testclient.getClientValuesInterfaceName(), "prop_signal", newName);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST_F(PerfTest, Properties_SettingNoSuchProperty) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    ProxyBusObject remoteObj(*test_msgBus, myService->getAlljoynWellKnownName(), testclient.getClientObjectPath(), 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(*test_msgBus);
    MsgArg inArgs[3];
    MsgArg newName("s", "New returned name");
    size_t numArgs = ArraySize(inArgs);
    MsgArg::Set(inArgs, numArgs, "ssv", testclient.getClientValuesInterfaceName(), "prop_signall", &newName);
    const InterfaceDescription* propIface = test_msgBus->GetInterface(ajn::org::freedesktop::DBus::Properties::InterfaceName);
    ASSERT_TRUE(propIface != NULL);

    status = remoteObj.MethodCall(*(propIface->GetMember("Set")), inArgs, numArgs, reply, 5000, 0);
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);
    String errMsg;
    reply->GetErrorName(&errMsg);
    EXPECT_STREQ(QCC_StatusText(ER_BUS_NO_SUCH_PROPERTY), errMsg.c_str());

}

TEST_F(PerfTest, Properties_SettingReadOnlyProperty) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    ProxyBusObject remoteObj(*test_msgBus, myService->getAlljoynWellKnownName(), testclient.getClientObjectPath(), 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(*test_msgBus);
    MsgArg newName("s", "New returned name");
    MsgArg inArgs[3];
    size_t numArgs = ArraySize(inArgs);
    MsgArg::Set(inArgs, numArgs, "ssv", testclient.getClientValuesInterfaceName(), "ro_str", &newName);
    const InterfaceDescription* propIface = test_msgBus->GetInterface(ajn::org::freedesktop::DBus::Properties::InterfaceName);
    ASSERT_TRUE(propIface != NULL);

    status = remoteObj.MethodCall(*(propIface->GetMember("Set")), inArgs, numArgs, reply, 5000, 0);
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);
    String errMsg;
    reply->GetErrorName(&errMsg);
    EXPECT_STREQ(QCC_StatusText(ER_BUS_PROPERTY_ACCESS_DENIED), errMsg.c_str());
}

TEST_F(PerfTest, Signals_With_Two_Parameters) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    testclient.setSignalFlag(0);
    QStatus status = testclient.SignalHandler(0, 1);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait upto 2 seconds for the signal to complete.
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (testclient.getSignalFlag() != 0) {
            break;
        }
    }
    ASSERT_EQ(5, testclient.getSignalFlag());

}


TEST_F(PerfTest, Signals_With_Huge_String_Param) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    testclient.setSignalFlag(0);
    QCC_StatusText(testclient.SignalHandler(0, 2));
    //Wait upto 2 seconds for the signal to complete.
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (testclient.getSignalFlag() != 0) {
            break;
        }
    }
    ASSERT_EQ(4096, testclient.getSignalFlag());

}



/* Test Asynchronous method calls */
TEST_F(PerfTest, AsyncMethodCallTest_SimpleCall) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    testclient.setSignalFlag(0);
    QStatus status = testclient.AsyncMethodCall(1000, 1);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait upto 2 seconds for the AsyncMethodCalls to complete;
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (testclient.getSignalFlag() == 1000) {
            break;
        }
    }
    EXPECT_EQ(1000, testclient.getSignalFlag());

}

TEST_F(PerfTest, BusObject_ALLJOYN_328_BusObject_destruction)
{
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());



    qcc::String clientArgs = testclient.getClientArgs();

    /* Create a Bus Attachment Object */
    BusAttachment* serviceBus = new BusAttachment("ALLJOYN-328", true);
    ASSERT_TRUE(serviceBus != NULL);
    serviceBus->Start();

    /* Dynamically create a BusObject and register it with the Bus */
    BusObject* obj1 = new  BusObject("/home/narasubr", true);

    QStatus status = serviceBus->RegisterBusObject(*obj1);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = serviceBus->Connect(clientArgs.c_str());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /*Delete the bus object..Now as per fix for ALLJOYN-328 deregisterbusobject will be called */
    BusObject* obj2 = obj1;
    obj1 = NULL;
    delete obj2;
    //TODO nothing is checked after Deleting the busObject what should this test be looking for?
    //status = serviceBus->Stop();
    //ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    // why is this sleep here when nothing is pending
    //qcc::Sleep(2000);
    /* Clean up msg bus */
    if (serviceBus) {
        BusAttachment* deleteMe = serviceBus;
        serviceBus = NULL;
        delete deleteMe;
    }


}

TEST_F(PerfTest, BusObject_GetChildTest) {
    QStatus status = ER_OK;
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    /* The Client side */
    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    /* No session required since client and service on same daemon */
    ProxyBusObject remoteObj = ProxyBusObject(*client_msgBus,
                                              myService->getAlljoynWellKnownName(),
                                              "/",
                                              0);

    status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject* absolutePathChildObj = remoteObj.GetChild("/org");
    ProxyBusObject* relativePathChildObj = remoteObj.GetChild("org");

    /* Should get the same child object whether using absolute or relative path */
    EXPECT_TRUE(absolutePathChildObj == relativePathChildObj);

    /* RemoveChild with absolute path */
    status = remoteObj.RemoveChild("/org");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(remoteObj.GetChild("/org") == NULL);

    /* RemoveChild with relative path, need to reset remoteObj first */
    remoteObj = ProxyBusObject(*client_msgBus,
                               myService->getAlljoynWellKnownName(),
                               "/",
                               0);

    status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = remoteObj.RemoveChild("org");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(remoteObj.GetChild("org") == NULL);
}

TEST_F(PerfTest, Marshal_ByteArrayTest) {
    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());


    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    /* Create a remote object */
    ProxyBusObject remoteObj(*test_msgBus, myService->getAlljoynWellKnownName(), "/org/alljoyn/service_test", 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Call the remote method */
    Message reply(*test_msgBus);

    const size_t max_array_size = 1024 * 128;
    /* 1. Testing the Max Array Size  */
    uint8_t* big = new uint8_t[max_array_size];
    memset(big, 0xaa, max_array_size);
    MsgArg arg;
    status = arg.Set("ay", max_array_size, big);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = remoteObj.MethodCall("org.alljoyn.service_test.Interface", "ByteArrayTest", &arg, 1, reply, 500000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int res = memcmp(big, (uint8_t*)reply->GetArg(0)->v_string.str, max_array_size);

    ASSERT_EQ(0, res);
    delete [] big;

    /* Testing The Max Packet Length  */
    MsgArg arg1;
    double* bigd = new double[max_array_size];
    status = arg1.Set("ad", max_array_size, bigd);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = remoteObj.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = remoteObj.MethodCall("org.alljoyn.service_test.Interface", "DoubleArrayTest", &arg1, 1, reply, 500000);
    ASSERT_EQ(ER_BUS_BAD_BODY_LEN, status) << "  Actual Status: " << QCC_StatusText(status);

    delete [] bigd;
}

TEST_F(PerfTest, FindAdvertisedName_MatchAll_Success)
{
    QStatus status = ER_OK;

    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    g_discoverEvent.ResetEvent();

    /* Initializing the bus listener */
    clientListener = new ClientBusListener();
    client_msgBus->RegisterBusListener(*clientListener);

    /* find every name */
    status = client_msgBus->FindAdvertisedName("");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = Event::Wait(g_discoverEvent, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(PerfTest, FindAdvertisedName_MatchExactName_Success)
{
    QStatus status = ER_OK;
    Timespec startTime;
    Timespec endTime;


    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    g_discoverEvent.ResetEvent();

    /* Initializing the bus listener */
    clientListener = new ClientBusListener();
    client_msgBus->RegisterBusListener(*clientListener);

    /* find every name */
    //GetTimeNow(&startTime);
    status = client_msgBus->FindAdvertisedName(myService->getAlljoynWellKnownName());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = Event::Wait(g_discoverEvent, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //GetTimeNow(&endTime);
    //QCC_SyncPrintf("FindAdvertisedName takes %d ms\n", (endTime - startTime));
}

TEST_F(PerfTest, FindAdvertisedName_InvalidName_Fail)
{
    QStatus status = ER_OK;

    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    g_discoverEvent.ResetEvent();

    /* Initializing the bus listener */
    clientListener = new ClientBusListener();
    client_msgBus->RegisterBusListener(*clientListener);

    /* invalid name find */
    status = client_msgBus->FindAdvertisedName("org.alljoyn.test_invalid");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    QCC_SyncPrintf("Waiting FoundAdvertisedName 3 seconds...\n");
    status = Event::Wait(g_discoverEvent, 3000);
    ASSERT_EQ(ER_TIMEOUT, status);
}

TEST_F(PerfTest, JoinSession_BusNotConnected_Fail)
{
    QStatus status = ER_OK;
    BusAttachment* client_msgBus = NULL;

    client_msgBus = new BusAttachment("clientSetup", true);
    ASSERT_TRUE(client_msgBus != NULL);
    client_msgBus->Start();
    /* Join session failed because not connected yet*/
    SessionId sessionid;
    SessionOpts qos(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = client_msgBus->JoinSession("org.alljoyn.invalid_services", 550, NULL, sessionid, qos);
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, status) << "  Actual Status: " << QCC_StatusText(status);

    delete client_msgBus;
}
TEST_F(PerfTest, JoinSession_InvalidPort_Fail)
{
    QStatus status = ER_OK;
    BusAttachment* client_msgBus = NULL;

    // Have to wait for some time till service well-known name is ready
    //qcc::Sleep(5000);

    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    client_msgBus = testclient.getClientMsgBus();

    /* Join the session */
    SessionId sessionid;
    SessionOpts qos(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    /* port 450 is invalid */
    status = client_msgBus->JoinSession(myService->getAlljoynWellKnownName(), 450, NULL, sessionid, qos);
    ASSERT_EQ(ER_ALLJOYN_JOINSESSION_REPLY_NO_SESSION, status);

}

TEST_F(PerfTest, JoinSession_RecordTime_Success)
{
    QStatus status = ER_OK;
    BusAttachment* client_msgBus = NULL;
    Timespec startTime;
    Timespec endTime;


    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    client_msgBus = testclient.getClientMsgBus();

    /* Join the session */
    SessionId sessionid;
    SessionOpts qos(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    //GetTimeNow(&startTime);
    status = client_msgBus->JoinSession(myService->getAlljoynWellKnownName(), 550, NULL, sessionid, qos);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_NE(static_cast<SessionId>(0), sessionid) << "SessionID should not be '0'";

    //GetTimeNow(&endTime);
    //QCC_SyncPrintf("JoinSession takes %d ms\n", (endTime - startTime));

    status = client_msgBus->LeaveSession(sessionid);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST_F(PerfTest, ClientTest_BasicDiscovery) {
    QStatus status = ER_OK;


    ClientSetup testclient(ajn::getConnectArg().c_str(), myService->getAlljoynWellKnownName());

    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    g_discoverEvent.ResetEvent();

    /* Initializing the bus listener */
    clientListener = new ClientBusListener();
    client_msgBus->RegisterBusListener(*clientListener);

    //QCC_SyncPrintf("Finding AdvertisedName\n");
    /* Do a Find Name  */
    status = client_msgBus->FindAdvertisedName(myService->getAlljoynWellKnownName());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = Event::Wait(g_discoverEvent, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Join the session */
    SessionId sessionid;
    SessionOpts qos(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = client_msgBus->JoinSession(myService->getAlljoynWellKnownName(), 550, NULL, sessionid, qos);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_NE(static_cast<SessionId>(0), sessionid) << "SessionID should not be '0'";
    /* Checking id name is on the bus */
    bool hasOwner = false;
    status = client_msgBus->NameHasOwner(myService->getAlljoynWellKnownName(), hasOwner);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(hasOwner, true);

    ProxyBusObject remoteObj = ProxyBusObject(*client_msgBus, myService->getAlljoynWellKnownName(), "/org/alljoyn/test_services", sessionid);
    status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message replyc(*client_msgBus);
    MsgArg pingStr("s", "Hello World");
    status = remoteObj.MethodCall("org.alljoyn.test_services.Interface", "my_ping", &pingStr, 1, replyc, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_STREQ("Hello World", replyc->GetArg(0)->v_string.str);

    Message replyd(*client_msgBus);
    status = remoteObj.MethodCall("org.alljoyn.test_services.Interface", "my_ping", &pingStr, 1, replyd, 5000, ALLJOYN_FLAG_NO_REPLY_EXPECTED);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
