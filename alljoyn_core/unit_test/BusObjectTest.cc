/******************************************************************************
 * Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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
#include "BusObjectTestBusObject.h"
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>

using namespace ajn;
using namespace qcc;

/*constants*/
static const char* OBJECT_PATH =   "/org/alljoyn/test/BusObjectTest";

/* ASACORE-189 */
TEST(BusObjectTest, ObjectRegisteredUnregistered) {
    BusAttachment bus("test1");
    QStatus status = ER_OK;
    BusObjectTestBusObject testObj(bus, OBJECT_PATH);
    status = bus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    status = bus.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered && testObj.wasUnregistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    EXPECT_TRUE(testObj.wasUnregistered);

    status = bus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}


/* ASACORE-189 */
TEST(BusObjectTest, ObjectRegisteredUnregisteredMultipleConnectDisconnect) {
    BusAttachment bus("test4");
    QStatus status = ER_OK;
    BusObjectTestBusObject testObj(bus, OBJECT_PATH);
    status = bus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    status = bus.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered && testObj.wasUnregistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    EXPECT_TRUE(testObj.wasUnregistered);

    testObj.wasRegistered = false;
    testObj.wasUnregistered = false;
    status = bus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    status = bus.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered && testObj.wasUnregistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    EXPECT_TRUE(testObj.wasUnregistered);

    status = bus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}


/* ASACORE-189 */
TEST(BusObjectTest, ObjectRegisteredAfterConnect) {
    BusAttachment bus("test5");
    QStatus status = ER_OK;
    BusObjectTestBusObject testObj(bus, OBJECT_PATH);
    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    status = bus.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered && testObj.wasUnregistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    EXPECT_TRUE(testObj.wasUnregistered);

    status = bus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

/* ASACORE-189 */
TEST(BusObjectTest, ObjectRegisteredAfterConnectUnregisteredBeforDisconnect) {
    BusAttachment bus("test6");
    QStatus status = ER_OK;
    BusObjectTestBusObject testObj(bus, OBJECT_PATH);
    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);

    bus.UnregisterBusObject(testObj);
    for (int msec = 0; msec < 5000; msec += 10) {
        if (testObj.wasRegistered && testObj.wasUnregistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(testObj.wasRegistered);
    EXPECT_TRUE(testObj.wasUnregistered);

    status = bus.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    testObj.wasRegistered = false;
    testObj.wasUnregistered = false;
    // We don't expect to get a second ObjectUnregistered signal wait for two
    // seconds and check that we did not get an ObjectUntegistered signal
    for (int msec = 0; msec < 2000; msec += 10) {
        if (testObj.wasRegistered && testObj.wasUnregistered) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_FALSE(testObj.wasRegistered);
    EXPECT_FALSE(testObj.wasUnregistered);
    status = bus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
TEST(BusObjectTest, DISABLED_Send_Signal_After_BusObject_Unregister)
{
    BusAttachment bus("test2");
    QStatus status = ER_OK;
    BusObjectTestBusObject testObj(bus, OBJECT_PATH);
    //Start a bus attachment
    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //add an interface to it
    InterfaceDescription* servicetestIntf = NULL;
    status = bus.CreateInterface("org.test", servicetestIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();

    //send a signal before registering the signal will result in error: ER_BUS_OBJECT_NOT_REGISTERED
    status = testObj.SendSignal();
    EXPECT_EQ(ER_BUS_OBJECT_NOT_REGISTERED, status) << "  Actual Status: " << QCC_StatusText(status);

    //register the bus object and check it was registered
    status = bus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int i = 0; i < 500; ++i) {
        qcc::Sleep(10);
        if (testObj.wasRegistered) {
            break;
        }
    }
    EXPECT_TRUE(testObj.wasRegistered);

    //Unregister the bus object and check that it was indeed unregistered
    bus.UnregisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int i = 0; i < 500; ++i) {
        qcc::Sleep(10);
        if (testObj.wasUnregistered) {
            break;
        }
    }
    EXPECT_TRUE(testObj.wasUnregistered);

    //Send a signal on the unregistered bus object. This should fail with ER_BUS_OBJECT_NOT_REGISTERED

    status = testObj.SendSignal();
    EXPECT_EQ(ER_BUS_OBJECT_NOT_REGISTERED, status) << "  Actual Status: " << QCC_StatusText(status);

}

//Test that signal is received because of register signal handler. Then signal is not received because of unregister signal handler.
TEST(BusObjectTest, SendSignalAfterUnregistersignalHandler)
{
    BusAttachment busService("test3Service");
    BusAttachment busClient("test3Client");
    QStatus status = ER_OK;
    BusObjectTestBusObject testObj(busService, OBJECT_PATH);

    //Start a service bus attachment
    status = busService.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busService.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Start a client bus attachment
    status = busClient.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busClient.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


    //add an interface to service bus attachment
    InterfaceDescription* servicetestIntf = NULL;
    status = busService.CreateInterface("org.test", servicetestIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();


    //add interface to client bus attachment
    InterfaceDescription* clienttestIntf = NULL;
    status = busClient.CreateInterface("org.test", clienttestIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    const InterfaceDescription::Member*  signal_member = clienttestIntf->GetMember("my_signal");

    //register the service bus object and check it was registered
    status = busService.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int i = 0; i < 500; ++i) {
        qcc::Sleep(10);
        if (testObj.wasRegistered) {
            break;
        }
    }
    EXPECT_TRUE(testObj.wasRegistered);

    //Register signal handler with client ane prepare it for receiving signal
    BusObjectTestSignalReceiver signalReceiver;
    status = busClient.RegisterSignalHandler(&signalReceiver,
                                             static_cast<MessageReceiver::SignalHandler>(&BusObjectTestSignalReceiver::SignalHandler),
                                             signal_member,
                                             NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busClient.AddMatch("type='signal',interface='org.test',member='my_signal'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


    //Service side emits the signal
    status = testObj.SendSignal();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //verify that client received the signal
    for (int i = 0; i < 500; ++i) {
        qcc::Sleep(10);
        if (signalReceiver.signalReceived) {
            break;
        }
    }
    EXPECT_EQ(1U, signalReceiver.signalReceived);

    signalReceiver.signalReceived = 0;
    //client side unregisters the signal handler
    status = busClient.UnregisterSignalHandler(&signalReceiver,
                                               static_cast<MessageReceiver::SignalHandler>(&BusObjectTestSignalReceiver::SignalHandler),
                                               signal_member,
                                               NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


    //Service side emits the signal again
    status = testObj.SendSignal();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //verify that client HAS NOT received the signal
    for (int i = 0; i < 500; ++i) {
        qcc::Sleep(10);
        if (signalReceiver.signalReceived) {
            break;
        }
    }
    EXPECT_EQ(0U, signalReceiver.signalReceived);

    status = busService.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busService.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busService.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busClient.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busClient.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busClient.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


}

class TestBusObject : public BusObject {
  public:
    TestBusObject(BusAttachment& bus, const char* path)
        : BusObject(path), bus(bus), wasRegistered(false), wasUnregistered(false) {

        QStatus status = ER_OK;
        const InterfaceDescription* Intf1 = bus.GetInterface("org.test");
        EXPECT_TRUE(Intf1 != NULL);
        AddInterface(*Intf1);
        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { Intf1->GetMember("pasta"), static_cast<MessageReceiver::MethodHandler>(&BusObjectTestBusObject::Pasta) }
        };
        status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual ~TestBusObject() { }

    void ObjectRegistered(void) {
        wasRegistered = true;
    }
    void ObjectUnregistered(void) {
        wasUnregistered = true;
    }

    void Pasta(const InterfaceDescription::Member* member, Message& msg)
    {
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "Pasta: Error sending reply,  Actual Status: " << QCC_StatusText(status);
    }

    BusAttachment& bus;
    bool wasRegistered, wasUnregistered;
};


//Test that you can make a method call and then the method call should fail gracefully when you unregister the bus object on the service side.
TEST(BusObjectTest, Make_methodcall_after_unregister_bus_object)
{
    BusAttachment busService("test4Service");
    BusAttachment busClient("test4Client");
    QStatus status = ER_OK;

    //Start a service bus attachment
    status = busService.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busService.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Start a client bus attachment
    status = busClient.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busClient.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //add an interface to service bus attachment
    InterfaceDescription* servicetestIntf = NULL;
    status = busService.CreateInterface("org.test", servicetestIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddMethod("pasta", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();

    TestBusObject testObj(busService, OBJECT_PATH);
    //register the service bus object and check it was registered
    status = busService.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int i = 0; i < 500; ++i) {
        qcc::Sleep(10);
        if (testObj.wasRegistered) {
            break;
        }
    }
    EXPECT_TRUE(testObj.wasRegistered);

    //create client proxy bus object and introspect
    ProxyBusObject clientProxyObject(busClient, busService.GetUniqueName().c_str(), OBJECT_PATH, 0, false);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Make a method call
    Message reply(busClient);
    const InterfaceDescription::Member* pastaMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface("org.test");
    pastaMethod = ifc->GetMember("pasta");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Pasta String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pastaMethod, &pingArgs, 1, reply, 5000);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Pasta String", reply->GetArg(0)->v_string.str);


    //Unregister the service bus object and check it was unregistered
    busService.UnregisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int i = 0; i < 500; ++i) {
        qcc::Sleep(10);
        if (testObj.wasUnregistered) {
            break;
        }
    }
    EXPECT_TRUE(testObj.wasUnregistered);

    //Make a method call and it should fail gracefully as the service side bus object is unregistsred.
    status = clientProxyObject.MethodCall(*pastaMethod, &pingArgs, 1, reply, 5000);
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("ER_BUS_NO_SUCH_OBJECT", reply->GetArg(0)->v_string.str);



    //Clean up
    status = busService.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busService.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busService.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = busClient.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busClient.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busClient.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}


class PropsTestBusObject : public BusObject {
  public:
    PropsTestBusObject(BusAttachment& bus, const char* path) : BusObject(path)
    {
        const InterfaceDescription* Intf1 = bus.GetInterface("org.test");
        EXPECT_TRUE(Intf1 != NULL);
        AddInterface(*Intf1);
        arrayStructData[0].Set("(is)", 42, "sorbet");
        arrayStructData[1].Set("(is)", 2112, "calamari");
    }

  protected:
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        if (strcmp(propName, "arrayStruct") == 0) {
            // Returning a local, non-new'd MsgArg array will trigger
            // the ASACORE-1009 bug with an attempt to free an invalid
            // pointer.
            val.Set("a(is)", ArraySize(arrayStructData), arrayStructData);
        }
        return ER_OK;
    }

  private:
    MsgArg arrayStructData[2];
};

/* ASACORE-1009 */
TEST(BusObjectTest, GetAllPropsWithStaticMsgArgProp)
{
    QStatus status;
    BusAttachment busService("test7service");
    BusAttachment busClient("test7client");
    InterfaceDescription* intf = NULL;
    status = busService.CreateInterface("org.test", intf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(intf != NULL);
    status = intf->AddProperty("arrayStruct", "a(is)", PROP_ACCESS_READ);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    intf->Activate();

    //Start a service bus attachment
    status = busService.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busService.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Start a client bus attachment
    status = busClient.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busClient.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropsTestBusObject bo(busService, OBJECT_PATH);
    status = busService.RegisterBusObject(bo);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject pbo(busClient, busService.GetUniqueName().c_str(), OBJECT_PATH, 0, false);

    status = pbo.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    MsgArg props;
    status = pbo.GetAllProperties("org.test", props);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
