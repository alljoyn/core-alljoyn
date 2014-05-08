/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/Util.h>

using namespace ajn;
using namespace qcc;

static const char* interface1 = "org.alljoyn.alljoyn_test.interface1";
static const char* interface2 = "org.alljoyn.alljoyn_test.interface2";
static const char* object_path = "/org/alljoyn/alljoyn_test";

class SvcTestObject : public BusObject {

  public:

    SvcTestObject(const char* path, BusAttachment& mBus) :
        BusObject(path),
        msgEncrypted(false),
        objectRegistered(false),
        get_property_called(false),
        set_property_called(false),
        prop_val(420),
        bus(mBus)
    {
        QStatus status = ER_OK;
        //const BusAttachment &mBus = GetBusAttachment();
        /* Add interface1 to the BusObject. */
        const InterfaceDescription* Intf1 = mBus.GetInterface(interface1);
        EXPECT_TRUE(Intf1 != NULL);
        if (Intf1 != NULL) {
            AddInterface(*Intf1);
            /* Add interface2 to the BusObject. */
            const InterfaceDescription* Intf2 = mBus.GetInterface(interface2);
            EXPECT_TRUE(Intf2 != NULL);
            AddInterface(*Intf2);

            /* Register the method handlers with the object */
            const MethodEntry methodEntries[] = {
                { Intf1->GetMember("my_ping"), static_cast<MessageReceiver::MethodHandler>(&SvcTestObject::Ping) },
            };
            status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    void ObjectRegistered(void)
    {
        objectRegistered = true;
    }

    void Ping(const InterfaceDescription::Member* member, Message& msg)
    {
        char* value = NULL;
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = arg->Get("s", &value);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (msg->IsEncrypted()) {
            msgEncrypted = true;
        }
        status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "Ping: Error sending reply,  Actual Status: " << QCC_StatusText(status);
    }

    void GetProp(const InterfaceDescription::Member* member, Message& msg)
    {
        QStatus status = ER_OK;
        const InterfaceDescription* Intf2 = bus.GetInterface(interface2);
        EXPECT_TRUE(Intf2 != NULL);
        if (Intf2 != NULL) {
            if (!msg->IsEncrypted() &&  (this->IsSecure() && Intf2->GetSecurityPolicy() != AJ_IFC_SECURITY_OFF)) {
                status = ER_BUS_MESSAGE_NOT_ENCRYPTED;
                status = MethodReply(msg, status);
                EXPECT_EQ(ER_OK, status) << "Actual Status: " << QCC_StatusText(status);
            } else {
                get_property_called = true;
                MsgArg prop("v", new MsgArg("i", prop_val));
                prop.SetOwnershipFlags(MsgArg::OwnsArgs);
                if (msg->IsEncrypted()) {
                    msgEncrypted = true;
                }
                status = MethodReply(msg, &prop, 1);
                EXPECT_EQ(ER_OK, status) << "Error getting property, Actual Status: " << QCC_StatusText(status);
            }
        }
    }

    void SetProp(const InterfaceDescription::Member* member, Message& msg)
    {
        QStatus status = ER_OK;
        const InterfaceDescription* Intf2 = bus.GetInterface(interface2);
        EXPECT_TRUE(Intf2 != NULL);
        if (Intf2 != NULL) {
            if (!msg->IsEncrypted() &&  (this->IsSecure() && Intf2->GetSecurityPolicy() != AJ_IFC_SECURITY_OFF)) {
                status = ER_BUS_MESSAGE_NOT_ENCRYPTED;
                status = MethodReply(msg, status);
                EXPECT_EQ(ER_OK, status) << "Actual Status: " << QCC_StatusText(status);
            } else {
                set_property_called = true;
                int32_t integer = 0;
                if (msg->IsEncrypted()) {
                    msgEncrypted = true;
                }
                const MsgArg* val = msg->GetArg(2);
                MsgArg value = *(val->v_variant.val);
                status = value.Get("i", &integer);
                EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
                prop_val = integer;
                status = MethodReply(msg, status);
                EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            }
        }
    }


    bool msgEncrypted;
    bool objectRegistered;
    bool get_property_called;
    bool set_property_called;
    int32_t prop_val;
    BusAttachment& bus;

};


class ObjectSecurityTest : public testing::Test, public AuthListener {
  public:
    ObjectSecurityTest() :
        clientbus("ObjectSecurityTestClient", false),
        servicebus("ObjectSecurityTestService", false),
        status(ER_OK),
        authComplete(false)
    { };

    virtual void SetUp() {
        status = clientbus.Start();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = clientbus.Connect(ajn::getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        clientbus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", this, NULL, false);
        clientbus.ClearKeyStore();

        status = servicebus.Start();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = servicebus.Connect(ajn::getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        servicebus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", this, NULL, false);
        servicebus.ClearKeyStore();
    }

    virtual void TearDown() {

        clientbus.ClearKeyStore();
        servicebus.ClearKeyStore();
        status = clientbus.Disconnect(ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = servicebus.Disconnect(ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = clientbus.Stop();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = servicebus.Stop();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = clientbus.Join();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = servicebus.Join();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    BusAttachment clientbus;
    BusAttachment servicebus;
    QStatus status;
    bool authComplete;

  private:

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {
        EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
        if (credMask & AuthListener::CRED_PASSWORD) {
            creds.SetPassword("123456");
        }
        return true;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
        EXPECT_TRUE(success);
        authComplete = true;
    }

};

/*
 *  Service object level = false
 *  Client object level = false
 *  service creates interface with AJ_IFC_SECURITY_OFF.
 *  client creates interface with AJ_IFC_SECURITY_OFF.
 *  client makes method call.
 *  expected that no encryption is used.
 */
TEST_F(ObjectSecurityTest, Test1) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    status = servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);

    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    InterfaceDescription* clienttestIntf2 = NULL;
    status = clientbus.CreateInterface(interface2, clienttestIntf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf2 != NULL);
    status = clienttestIntf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf2->Activate();


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.AddInterface(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    status = clientProxyObject.AddInterface(interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}



/*
 *   Service object level = false
 *   Client object level = false
 *   service creates interface with AJ_IFC_SECURITY_INHERIT.
 *   client creates interface with AJ_IFC_SECURITY_INHERIT.
 *   client makes method call.
 *   expected that no encryption is used.
 */
TEST_F(ObjectSecurityTest, Test2) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    InterfaceDescription* clienttestIntf2 = NULL;
    status = clientbus.CreateInterface(interface2, clienttestIntf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf2 != NULL);
    status = clienttestIntf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf2->Activate();

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.AddInterface(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    status = clientProxyObject.AddInterface(interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}

/*
 *  Service object level = false
 *  Client object level = false
 *  service creates interface with REQUIRED.
 *  client creates interface with REQUIRED.
 *  client makes method call.
 *  expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test3) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    InterfaceDescription* clienttestIntf2 = NULL;
    status = clientbus.CreateInterface(interface2, clienttestIntf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf2 != NULL);
    status = clienttestIntf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf2->Activate();

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.AddInterface(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    status = clientProxyObject.AddInterface(interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}


/*
 *   Service object level = true
 *   Client object level = true
 *   service creates interface with AJ_IFC_SECURITY_OFF.
 *   client creates interface with AJ_IFC_SECURITY_OFF.
 *   client makes method call.
 *  expected that no encryption is used because interfaces with N/A security level should NOT use security.
 */
TEST_F(ObjectSecurityTest, Test4) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    InterfaceDescription* clienttestIntf2 = NULL;
    status = clientbus.CreateInterface(interface2, clienttestIntf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf2 != NULL);
    status = clienttestIntf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf2->Activate();

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.AddInterface(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    status = clientProxyObject.AddInterface(interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());

}

/*
 *  Service object level = true
 *  Client object level = true
 *  service creates interface with AJ_IFC_SECURITY_INHERIT.
 *  client creates interface with AJ_IFC_SECURITY_INHERIT.
 *  client makes method call.
 *  expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test5) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    InterfaceDescription* clienttestIntf2 = NULL;
    status = clientbus.CreateInterface(interface2, clienttestIntf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf2 != NULL);
    status = clienttestIntf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf2->Activate();

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.AddInterface(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    status = clientProxyObject.AddInterface(interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());

}


/*
 * Service object level = true
 * Client object level = true
 * service creates interface with REQUIRED.
 * client creates interface with REQUIRED.
 * client makes method call.
 * expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test6) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    InterfaceDescription* clienttestIntf2 = NULL;
    status = clientbus.CreateInterface(interface2, clienttestIntf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf2 != NULL);
    status = clienttestIntf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf2->Activate();

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.AddInterface(interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    status = clientProxyObject.AddInterface(interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());

}

/*
 *  Service object level = false
 *  Client object level = false
 *  service creates interface with AJ_IFC_SECURITY_OFF.
 *  client Introspects.
 *  client makes method call.
 *  expected that no encryption is used.
 */
TEST_F(ObjectSecurityTest, Test7) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.IntrospectRemoteObject();

    serviceObject.msgEncrypted = false;
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}



/*
 *   Service object level = false
 *   Client object level = false
 *   service creates interface with AJ_IFC_SECURITY_INHERIT.
 *   client Introspects.
 *   client makes method call.
 *   expected that no encryption is used.
 */
TEST_F(ObjectSecurityTest, Test8) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}



/*
 *  Service object level = false
 *  Client object level = false
 *  service creates interface with REQUIRED.
 *  client Introspects.
 *  client makes method call.
 *  expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test9) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}

/*
 *  Service object level = false
 *  Client object level = true
 *   service creates interface with AJ_IFC_SECURITY_OFF.
 *  client Introspects.
 *  client makes method call.
 *  expected that no encryption is used because interfaces with AJ_IFC_SECURITY_OFF security level should NOT use security.
 */
TEST_F(ObjectSecurityTest, Test10) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* clienttestIntf2 = NULL;
    status = servicebus.CreateInterface(interface2, clienttestIntf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf2 != NULL);
    status = clienttestIntf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());
}


/*
 * Service object level = false
 * Client object level = true
 * service creates interface with AJ_IFC_SECURITY_INHERIT.
 * client Introspects.
 * client makes method call.
 * expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test11) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());
}


/*
 * Service object level = false
 * Client object level = true
 * service creates interface with REQUIRED.
 * client Introspects.
 * client makes method call.
 * expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test12) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());
}


/*
 * Service object level = true
 * Client object level = false
 * service creates interface with AJ_IFC_SECURITY_OFF.
 * client Introspects.
 * client makes method call.
 * expected that no encryption is used because interfaces with N/A security level should NOT use security.
 */
TEST_F(ObjectSecurityTest, Test13) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    /* Before introspect, proxybusobject is unsecure. */
    ASSERT_FALSE(clientProxyObject.IsSecure());

    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    /* After introspect, proxybusobject becomes secure. */
    ASSERT_TRUE(clientProxyObject.IsSecure());

}


/*
 *  Service object level = true
 *  Client object level = false
 *  service creates interface with AJ_IFC_SECURITY_INHERIT.
 *  client Introspects.
 *  client makes method call.
 *  expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test14) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    /* Before introspect, proxybusobject is unsecure. */
    ASSERT_FALSE(clientProxyObject.IsSecure());

    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    /* After introspect, proxybusobject becomes secure. */
    ASSERT_TRUE(clientProxyObject.IsSecure());

}

/*
 *   Service object level = true
 *   Client object level = false
 *   service creates interface with REQUIRED.
 *   client Introspects.
 *   client makes method call.
 *   expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test15) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    /* Before introspect, proxybusobject is unsecure. */
    ASSERT_FALSE(clientProxyObject.IsSecure());

    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    /* After introspect, proxybusobject becomes secure. */
    ASSERT_TRUE(clientProxyObject.IsSecure());

}



/*
 *  Service object level = true
 *  Client object level = true
 *  service creates interface with AJ_IFC_SECURITY_OFF.
 *  client Introspects.
 *  client makes method call.
 *  expected that no encryption is used because interfaces with N/A security level should NOT use security.
 */
TEST_F(ObjectSecurityTest, Test16) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());

}


/*
 *   Service object level = true
 *   Client object level = true
 *   service creates interface with AJ_IFC_SECURITY_INHERIT.
 *   client Introspects.
 *   client makes method call.
 *   expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test17) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());

}


/*
 *  Service object level = true
 *  Client object level = true
 *  service creates interface with REQUIRED.
 *  client Introspects.
 *  client makes method call.
 *  expected that encryption is used.
 */
TEST_F(ObjectSecurityTest, Test18) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_TRUE(serviceObject.IsSecure());
    ASSERT_TRUE(clientProxyObject.IsSecure());

}

/*
 *  Client object level = true.
 *  service creates interface with REQUIRED.
 *  Client Introspect should not trigger security.
 */
TEST_F(ObjectSecurityTest, Test19) {

    QStatus status = ER_OK;

    InterfaceDescription* servicetestIntf = NULL;
    status = servicebus.CreateInterface(interface1, servicetestIntf, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();
    InterfaceDescription* servicetestIntf2 = NULL;
    status = servicebus.CreateInterface(interface2, servicetestIntf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf2 != NULL);
    status = servicetestIntf2->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);


    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, true);
    status = clientProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(authComplete);
}



class SignalSecurityTestObject : public BusObject {

  public:

    SignalSecurityTestObject(const char* path, InterfaceDescription& intf) :
        BusObject(path),
        objectRegistered(false),
        intf(intf)  { }

    void ObjectRegistered(void)
    {
        objectRegistered = true;
    }

    QStatus SendSignal() {
        const InterfaceDescription::Member*  signal_member = intf.GetMember("my_signal");
        MsgArg arg("s", "Signal");
        QStatus status = Signal(NULL, 0, *signal_member, &arg, 1, 0, 0);
        return status;
    }


    bool objectRegistered;
    InterfaceDescription& intf;
};

class ObjectSecurityTestSignalReceiver : public MessageReceiver {

  public:

    ObjectSecurityTestSignalReceiver() {
        signalReceived = false;
        msgEncrypted = false;
    }

    void SignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg) {
        if (msg->IsEncrypted()) {
            msgEncrypted = true;;
        }
        signalReceived = true;
    }

    bool signalReceived;
    bool msgEncrypted;

};

/*
 *  signal sender object level = false.
 *  service creates interface with AJ_IFC_SECURITY_OFF.
 *  Signal is not encrypted.
 */
TEST_F(ObjectSecurityTest, Test20) {

    QStatus status = ER_OK;

    InterfaceDescription* servicetestIntf = NULL;
    status = servicebus.CreateInterface(interface1, servicetestIntf, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();

    SignalSecurityTestObject serviceObject(object_path, *servicetestIntf);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    const InterfaceDescription::Member*  signal_member = clienttestIntf->GetMember("my_signal");

    ObjectSecurityTestSignalReceiver signalReceiver;
    status = clientbus.RegisterSignalHandler(&signalReceiver,
                                             static_cast<MessageReceiver::SignalHandler>(&ObjectSecurityTestSignalReceiver::SignalHandler),
                                             signal_member,
                                             NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientbus.AddMatch("type='signal',interface='org.alljoyn.alljoyn_test.interface1',member='my_signal'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    EXPECT_FALSE(clientProxyObject.IsSecure());
    status = clientProxyObject.SecureConnection();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(clientProxyObject.IsSecure());

    status = serviceObject.SendSignal();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 3 sec for signal to be arrived
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered && signalReceiver.signalReceived) {
            break;
        }
    }

    EXPECT_TRUE(serviceObject.objectRegistered);
    EXPECT_TRUE(signalReceiver.signalReceived);
    EXPECT_FALSE(signalReceiver.msgEncrypted);
}


/*
 *  signal sender object level = false.
 *  service creates interface with AJ_IFC_SECURITY_INHERIT.
 *  Signal is not encrypted.
 */

TEST_F(ObjectSecurityTest, Test21) {

    QStatus status = ER_OK;

    InterfaceDescription* servicetestIntf = NULL;
    status = servicebus.CreateInterface(interface1, servicetestIntf, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();

    SignalSecurityTestObject serviceObject(object_path, *servicetestIntf);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    const InterfaceDescription::Member*  signal_member = clienttestIntf->GetMember("my_signal");

    ObjectSecurityTestSignalReceiver signalReceiver;
    status = clientbus.RegisterSignalHandler(&signalReceiver,
                                             static_cast<MessageReceiver::SignalHandler>(&ObjectSecurityTestSignalReceiver::SignalHandler),
                                             signal_member,
                                             NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientbus.AddMatch("type='signal',interface='org.alljoyn.alljoyn_test.interface1',member='my_signal'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    EXPECT_FALSE(clientProxyObject.IsSecure());
    status = clientProxyObject.SecureConnection();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Even though we called ProxyBusObject.SecureConnection(), that is not going to mark the object as secure.*/
    EXPECT_FALSE(clientProxyObject.IsSecure());

    status = serviceObject.SendSignal();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 3 sec for signal to be arrived
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered && signalReceiver.signalReceived) {
            break;
        }
    }

    EXPECT_TRUE(serviceObject.objectRegistered);
    EXPECT_TRUE(signalReceiver.signalReceived);
    EXPECT_FALSE(signalReceiver.msgEncrypted);
}


/*
 *  signal sender object level = false.
 *  service creates interface with REQUIRED.
 *  Signal is  encrypted.
 */

TEST_F(ObjectSecurityTest, Test22) {

    QStatus status = ER_OK;

    InterfaceDescription* servicetestIntf = NULL;
    status = servicebus.CreateInterface(interface1, servicetestIntf, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();

    SignalSecurityTestObject serviceObject(object_path, *servicetestIntf);
    servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    const InterfaceDescription::Member*  signal_member = clienttestIntf->GetMember("my_signal");

    ObjectSecurityTestSignalReceiver signalReceiver;
    status = clientbus.RegisterSignalHandler(&signalReceiver,
                                             static_cast<MessageReceiver::SignalHandler>(&ObjectSecurityTestSignalReceiver::SignalHandler),
                                             signal_member,
                                             NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientbus.AddMatch("type='signal',interface='org.alljoyn.alljoyn_test.interface1',member='my_signal'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    EXPECT_FALSE(clientProxyObject.IsSecure());
    status = clientProxyObject.SecureConnection();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(clientProxyObject.IsSecure());

    status = serviceObject.SendSignal();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 3 sec for signal to be arrived
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered && signalReceiver.signalReceived) {
            break;
        }
    }

    EXPECT_TRUE(serviceObject.objectRegistered);
    EXPECT_TRUE(signalReceiver.signalReceived);
    EXPECT_TRUE(signalReceiver.msgEncrypted);
}


/*
 *  signal sender object level = true.
 *  service creates interface with AJ_IFC_SECURITY_OFF.
 *  Signal is not encrypted.
 */

TEST_F(ObjectSecurityTest, Test23) {

    QStatus status = ER_OK;

    InterfaceDescription* servicetestIntf = NULL;
    status = servicebus.CreateInterface(interface1, servicetestIntf, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();

    SignalSecurityTestObject serviceObject(object_path, *servicetestIntf);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    const InterfaceDescription::Member*  signal_member = clienttestIntf->GetMember("my_signal");

    ObjectSecurityTestSignalReceiver signalReceiver;
    status = clientbus.RegisterSignalHandler(&signalReceiver,
                                             static_cast<MessageReceiver::SignalHandler>(&ObjectSecurityTestSignalReceiver::SignalHandler),
                                             signal_member,
                                             NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientbus.AddMatch("type='signal',interface='org.alljoyn.alljoyn_test.interface1',member='my_signal'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    EXPECT_FALSE(clientProxyObject.IsSecure());
    status = clientProxyObject.SecureConnection();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(clientProxyObject.IsSecure());

    status = serviceObject.SendSignal();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 3 sec for signal to be arrived
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered && signalReceiver.signalReceived) {
            break;
        }
    }

    EXPECT_TRUE(serviceObject.objectRegistered);
    EXPECT_TRUE(signalReceiver.signalReceived);
    EXPECT_FALSE(signalReceiver.msgEncrypted);
}


/*
 *  signal sender object level = true.
 *  service creates interface with AJ_IFC_SECURITY_INHERIT.
 *  Signal is encrypted.
 */

TEST_F(ObjectSecurityTest, Test24) {

    QStatus status = ER_OK;

    InterfaceDescription* servicetestIntf = NULL;
    status = servicebus.CreateInterface(interface1, servicetestIntf, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();

    SignalSecurityTestObject serviceObject(object_path, *servicetestIntf);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    const InterfaceDescription::Member*  signal_member = clienttestIntf->GetMember("my_signal");

    ObjectSecurityTestSignalReceiver signalReceiver;
    status = clientbus.RegisterSignalHandler(&signalReceiver,
                                             static_cast<MessageReceiver::SignalHandler>(&ObjectSecurityTestSignalReceiver::SignalHandler),
                                             signal_member,
                                             NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientbus.AddMatch("type='signal',interface='org.alljoyn.alljoyn_test.interface1',member='my_signal'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    EXPECT_FALSE(clientProxyObject.IsSecure());
    status = clientProxyObject.SecureConnection();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(clientProxyObject.IsSecure());

    status = serviceObject.SendSignal();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 3 sec for signal to be arrived
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered && signalReceiver.signalReceived) {
            break;
        }
    }

    EXPECT_TRUE(serviceObject.objectRegistered);
    EXPECT_TRUE(signalReceiver.signalReceived);
    EXPECT_TRUE(signalReceiver.msgEncrypted);
}



/*
 *  signal sender object level = true.
 *  service creates interface with AJ_IFC_SECURITY_REQUIRED.
 *  Signal is encrypted.
 */

TEST_F(ObjectSecurityTest, Test25) {

    QStatus status = ER_OK;

    InterfaceDescription* servicetestIntf = NULL;
    status = servicebus.CreateInterface(interface1, servicetestIntf, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(servicetestIntf != NULL);
    status = servicetestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    servicetestIntf->Activate();

    SignalSecurityTestObject serviceObject(object_path, *servicetestIntf);
    servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }


    InterfaceDescription* clienttestIntf = NULL;
    status = clientbus.CreateInterface(interface1, clienttestIntf, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(clienttestIntf != NULL);
    status = clienttestIntf->AddSignal("my_signal", "s", NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    clienttestIntf->Activate();
    const InterfaceDescription::Member*  signal_member = clienttestIntf->GetMember("my_signal");

    ObjectSecurityTestSignalReceiver signalReceiver;
    status = clientbus.RegisterSignalHandler(&signalReceiver,
                                             static_cast<MessageReceiver::SignalHandler>(&ObjectSecurityTestSignalReceiver::SignalHandler),
                                             signal_member,
                                             NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientbus.AddMatch("type='signal',interface='org.alljoyn.alljoyn_test.interface1',member='my_signal'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    EXPECT_FALSE(clientProxyObject.IsSecure());
    status = clientProxyObject.SecureConnection();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(clientProxyObject.IsSecure());

    status = serviceObject.SendSignal();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 3 sec for signal to be arrived
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered && signalReceiver.signalReceived) {
            break;
        }
    }

    EXPECT_TRUE(serviceObject.objectRegistered);
    EXPECT_TRUE(signalReceiver.signalReceived);
    EXPECT_TRUE(signalReceiver.msgEncrypted);
}


const char* grand_parent_interface1 = "org.alljoyn.alljoyn_test.grand_parent.interface1";
const char* parent_interface1 = "org.alljoyn.alljoyn_test.parent.interface1";
const char* child_interface1 = "org.alljoyn.alljoyn_test.child.interface1";
const char* grand_parent_object_path = "/grandparent";
const char* parent_object_path = "/grandparent/parent";
const char* child_object_path = "/grandparent/parent/child";

class GrandParentTestObject : public BusObject {

  public:

    GrandParentTestObject(const char* path, BusAttachment& mBus) :
        BusObject(path),
        msgEncrypted(false),
        objectRegistered(false),
        bus(mBus)
    {
        QStatus status = ER_OK;
        /* Add interface1 to the BusObject. */
        const InterfaceDescription* Intf1 = mBus.GetInterface(grand_parent_interface1);
        EXPECT_TRUE(Intf1 != NULL);
        if (Intf1 != NULL) {
            AddInterface(*Intf1);

            /* Register the method handlers with the object */
            const MethodEntry methodEntries[] = {
                { Intf1->GetMember("grand_parent_ping"), static_cast<MessageReceiver::MethodHandler>(&GrandParentTestObject::GrandParentPing) },
            };
            status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    void ObjectRegistered(void)
    {
        objectRegistered = true;
    }

    void GrandParentPing(const InterfaceDescription::Member* member, Message& msg)
    {
        char* value = NULL;
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = arg->Get("s", &value);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (msg->IsEncrypted()) {
            msgEncrypted = true;
        }
        status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "GrandParentPing: Error sending reply,  Actual Status: " << QCC_StatusText(status);
    }


    bool msgEncrypted;
    bool objectRegistered;
    BusAttachment& bus;

};


class ParentTestObject : public BusObject {

  public:

    ParentTestObject(const char* path, BusAttachment& mBus) :
        BusObject(path),
        msgEncrypted(false),
        objectRegistered(false),
        bus(mBus)
    {
        QStatus status = ER_OK;
        /* Add interface1 to the BusObject. */
        const InterfaceDescription* Intf1 = mBus.GetInterface(parent_interface1);
        EXPECT_TRUE(Intf1 != NULL);
        if (Intf1 != NULL) {
            AddInterface(*Intf1);

            /* Register the method handlers with the object */
            const MethodEntry methodEntries[] = {
                { Intf1->GetMember("parent_ping"), static_cast<MessageReceiver::MethodHandler>(&ParentTestObject::ParentPing) },
            };
            status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    void ObjectRegistered(void)
    {
        objectRegistered = true;
    }

    void ParentPing(const InterfaceDescription::Member* member, Message& msg)
    {
        char* value = NULL;
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = arg->Get("s", &value);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (msg->IsEncrypted()) {
            msgEncrypted = true;
        }
        status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "ParentPing: Error sending reply,  Actual Status: " << QCC_StatusText(status);
    }


    bool msgEncrypted;
    bool objectRegistered;
    BusAttachment& bus;

};

class ChildTestObject : public BusObject {

  public:

    ChildTestObject(const char* path, BusAttachment& mBus) :
        BusObject(path),
        msgEncrypted(false),
        objectRegistered(false),
        bus(mBus)
    {
        QStatus status = ER_OK;
        /* Add interface1 to the BusObject. */
        const InterfaceDescription* Intf1 = mBus.GetInterface(child_interface1);
        EXPECT_TRUE(Intf1 != NULL);
        if (Intf1 != NULL) {
            AddInterface(*Intf1);

            /* Register the method handlers with the object */
            const MethodEntry methodEntries[] = {
                { Intf1->GetMember("child_ping"), static_cast<MessageReceiver::MethodHandler>(&ChildTestObject::ChildPing) },
            };
            status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    void ObjectRegistered(void)
    {
        objectRegistered = true;
    }

    void ChildPing(const InterfaceDescription::Member* member, Message& msg)
    {
        char* value = NULL;
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = arg->Get("s", &value);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (msg->IsEncrypted()) {
            msgEncrypted = true;
        }
        status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "ChildPing: Error sending reply,  Actual Status: " << QCC_StatusText(status);
    }


    bool msgEncrypted;
    bool objectRegistered;
    BusAttachment& bus;

};


/*
 *  GrandParentBusObject level = true
 *  ParentBusObject level = false
 *  ChildBusObject level = false
 *  GrandParentBusObject adds interface AJ_IFC_SECURITY_INHERIT
 *  ParentBusObject adds interface AJ_IFC_SECURITY_OFF
 *  ChildBusObject adds interface AJ_IFC_SECURITY_INHERIT
 *  Client introspects
 */
TEST_F(ObjectSecurityTest, Test26) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(grand_parent_interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("grand_parent_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();

    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(parent_interface1, Intf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddMethod("parent_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    InterfaceDescription* Intf3 = NULL;
    status = servicebus.CreateInterface(child_interface1, Intf3, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf3 != NULL);
    status = Intf3->AddMethod("child_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf3->Activate();


    GrandParentTestObject grandParentTestObject(grand_parent_object_path, servicebus);
    status = servicebus.RegisterBusObject(grandParentTestObject, true);
    ParentTestObject parentTestObject(parent_object_path, servicebus);
    status = servicebus.RegisterBusObject(parentTestObject, false);
    ChildTestObject childTestObject(child_object_path, servicebus);
    status = servicebus.RegisterBusObject(childTestObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (grandParentTestObject.objectRegistered &&  parentTestObject.objectRegistered && childTestObject.objectRegistered) {
            break;
        }
    }
    EXPECT_TRUE(grandParentTestObject.objectRegistered);
    EXPECT_TRUE(parentTestObject.objectRegistered);
    EXPECT_TRUE(childTestObject.objectRegistered);

    ProxyBusObject grandParentProxyObject(clientbus, servicebus.GetUniqueName().c_str(), grand_parent_object_path, 0, false);
    status = grandParentProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(grandParentProxyObject.IsSecure());

    ProxyBusObject parentProxyObject(clientbus, servicebus.GetUniqueName().c_str(), parent_object_path, 0, false);
    status = parentProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(parentProxyObject.IsSecure());

    ProxyBusObject childProxyObject(clientbus, servicebus.GetUniqueName().c_str(), child_object_path, 0, false);
    status = childProxyObject.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(childProxyObject.IsSecure());

    grandParentTestObject.msgEncrypted = false;
    parentTestObject.msgEncrypted = false;
    childTestObject.msgEncrypted = false;

    Message reply(clientbus);
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");

    /* Method call on grandparent object. Encryption must be used. */
    const InterfaceDescription::Member* grandParentPingMethod;
    const InterfaceDescription* ifc = grandParentProxyObject.GetInterface(grand_parent_interface1);
    grandParentPingMethod = ifc->GetMember("grand_parent_ping");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = grandParentProxyObject.MethodCall(*grandParentPingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(grandParentTestObject.msgEncrypted);

    /* Method call on parent object. Encryption must not be used, interface is not applicable.*/
    const InterfaceDescription::Member* parentPingMethod;
    const InterfaceDescription* ifc2 = parentProxyObject.GetInterface(parent_interface1);
    parentPingMethod = ifc2->GetMember("parent_ping");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = parentProxyObject.MethodCall(*parentPingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(parentTestObject.msgEncrypted);

    /* Method call on child object. Encryption must be used.*/
    const InterfaceDescription::Member* childPingMethod;
    const InterfaceDescription* ifc3 = childProxyObject.GetInterface(child_interface1);
    childPingMethod = ifc3->GetMember("child_ping");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = childProxyObject.MethodCall(*childPingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(childTestObject.msgEncrypted);

}



/*
 *  Service object level = false
 *  Client object level = false
 *  service creates interface with AJ_IFC_SECURITY_OFF.
 *  client populates proxybusobject from xml
 *  client makes method call.
 *  expected that no encryption is used.
 */

static const char* Test27XML =  {
    "<node>\n"
    "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"false\"/>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface1\">\n"
    "        <method name=\"my_ping\">\n"
    "          <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "          <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "     </interface>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface2\">\n"
    "        <property name=\"integer_property\" type=\"i\" access=\"readwrite\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "     </interface>\n"
    "</node>\n"
};

TEST_F(ObjectSecurityTest, Test27) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_OFF);
    ASSERT_TRUE(Intf2 != NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    status = servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.ParseXml(Test27XML);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}

/*
 *  Service object level = false
 *  Client object level = false
 *  service creates interface with REQUIRED.
 *  client populates proxybusobject from xml
 *  client makes method call.
 *  expected that encryption is used.
 */
static const char* Test28XML =  {
    "<node>\n"
    "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"false\"/>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface1\">\n"
    "        <method name=\"my_ping\">\n"
    "          <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "          <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n"
    "     </interface>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface2\">\n"
    "        <property name=\"integer_property\" type=\"i\" access=\"readwrite\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n"
    "     </interface>\n"
    "</node>\n"
};

TEST_F(ObjectSecurityTest, Test28) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    status = servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.ParseXml(Test28XML);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}


/*
 *  Service object level = false
 *  Client object level = false
 *  service creates interface with AJ_IFC_SECURITY_INHERIT.
 *  client populates proxybusobject from xml
 *  client makes method call.
 *  expected that no encryption is used.
 *  No annotation means NONE
 */

static const char* Test29XML =  {
    "<node>\n"
    "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"false\"/>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface1\">\n"
    "        <method name=\"my_ping\">\n"
    "          <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "          <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "     </interface>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface2\">\n"
    "        <property name=\"integer_property\" type=\"i\" access=\"readwrite\"/>\n"
    "     </interface>\n"
    "</node>\n"
};

TEST_F(ObjectSecurityTest, Test29) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    status = servicebus.RegisterBusObject(serviceObject, false);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.ParseXml(Test29XML);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    ASSERT_FALSE(serviceObject.IsSecure());
    ASSERT_FALSE(clientProxyObject.IsSecure());
}


/*
 *  Service object level = true
 *  Client object level = true
 *  service creates interface with AJ_IFC_SECURITY_OFF.
 *  client populates proxybusobject from xml
 *  client makes method call.
 *  expected that no encryption is used.
 */
static const char* Test30XML =  {
    "<node>\n"
    "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface1\">\n"
    "        <method name=\"my_ping\">\n"
    "          <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "          <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "     </interface>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface2\">\n"
    "        <property name=\"integer_property\" type=\"i\" access=\"readwrite\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n"
    "     </interface>\n"
    "</node>\n"
};

TEST_F(ObjectSecurityTest, Test30) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    status = servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.ParseXml(Test30XML);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_FALSE(serviceObject.msgEncrypted);

    EXPECT_TRUE(serviceObject.IsSecure());
    EXPECT_TRUE(clientProxyObject.IsSecure());
}

/*
 *  Service object level = true
 *  Client object level = true
 *  service creates interface with REQUIRED.
 *  client populates proxybusobject from xml
 *  client makes method call.
 *  expected that encryption is used.
 */
static const char* Test31XML =  {
    "<node>\n"
    "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface1\">\n"
    "        <method name=\"my_ping\">\n"
    "          <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "          <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n"
    "     </interface>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface2\">\n"
    "        <property name=\"integer_property\" type=\"i\" access=\"readwrite\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n"
    "     </interface>\n"
    "</node>\n"
};

TEST_F(ObjectSecurityTest, Test31) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_REQUIRED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    status = servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.ParseXml(Test31XML);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    EXPECT_TRUE(serviceObject.IsSecure());
    EXPECT_TRUE(clientProxyObject.IsSecure());
}


/*
 *  Service object level = true
 *  Client object level = true
 *  service creates interface with AJ_IFC_SECURITY_INHERIT.
 *  client populates proxybusobject from xml
 *  client makes method call.
 *  expected that encryption is used.
 *  Inherit is by default. it does not need any annotation
 */
static const char* Test32XML =  {
    "<node>\n"
    "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface1\">\n"
    "        <method name=\"my_ping\">\n"
    "          <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "          <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "     </interface>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface2\">\n"
    "        <property name=\"integer_property\" type=\"i\" access=\"readwrite\"/>\n"
    "     </interface>\n"
    "</node>\n"
};

TEST_F(ObjectSecurityTest, Test32) {

    QStatus status = ER_OK;

    InterfaceDescription* Intf1 = NULL;
    status = servicebus.CreateInterface(interface1, Intf1, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf1 != NULL);
    status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf1->Activate();
    InterfaceDescription* Intf2 = NULL;
    status = servicebus.CreateInterface(interface2, Intf2, AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(Intf2 != NULL);
    status = Intf2->AddProperty("integer_property", "i", PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Intf2->Activate();

    SvcTestObject serviceObject(object_path, servicebus);
    status = servicebus.RegisterBusObject(serviceObject, true);
    //Wait for a maximum of 3 sec for object to be registered
    for (int i = 0; i < 300; ++i) {
        qcc::Sleep(10);
        if (serviceObject.objectRegistered) {
            break;
        }
    }
    ASSERT_TRUE(serviceObject.objectRegistered);

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    status = clientProxyObject.ParseXml(Test32XML);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject.GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.MethodCall(*pingMethod, &pingArgs, 1, reply, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    MsgArg val;
    status = val.Set("i", 421);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = clientProxyObject.SetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    serviceObject.msgEncrypted = false;
    status = clientProxyObject.GetProperty(interface2, "integer_property", val);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int iVal = 0;
    status = val.Get("i", &iVal);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(421, iVal);
    EXPECT_TRUE(serviceObject.msgEncrypted);

    EXPECT_TRUE(serviceObject.IsSecure());
    EXPECT_TRUE(clientProxyObject.IsSecure());
}


/*
 * Test that undefined annotaions do not cause crashes. The interface level sec policy should be INHERIT
 * Object level policy should be "false" by default.
 */
static const char* Test33XML =  {
    "<node>\n"
    "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"hello\"/>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface1\">\n"
    "        <method name=\"my_ping\">\n"
    "          <arg name=\"inStr\" type=\"s\" direction=\"in\"/>\n"
    "          <arg name=\"outStr\" type=\"s\" direction=\"out\"/>\n"
    "        </method>\n"
    "        <annotation name=\"org.alljoyn.iBus.Secure\" value=\"alice\"/>\n"
    "     </interface>\n"
    "     <interface name=\"org.alljoyn.alljoyn_test.interface2\">\n"
    "        <property name=\"integer_property\" type=\"i\" access=\"readwrite\"/>\n"
    "        <annotation name=\"org.alljoyn.Bus.Secure\" value=\"bob\"/>\n"
    "     </interface>\n"
    "</node>\n"
};

TEST_F(ObjectSecurityTest, Test33) {

    QStatus status = ER_OK;

    ProxyBusObject clientProxyObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
    EXPECT_FALSE(clientProxyObject.IsSecure());
    status = clientProxyObject.ParseXml(Test33XML);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const InterfaceDescription* Intf1 = clientProxyObject.GetInterface(interface1);
    const InterfaceDescription* Intf2 = clientProxyObject.GetInterface(interface2);
    EXPECT_EQ(Intf1->GetSecurityPolicy(), AJ_IFC_SECURITY_INHERIT);
    EXPECT_EQ(Intf2->GetSecurityPolicy(), AJ_IFC_SECURITY_INHERIT);
    EXPECT_FALSE(clientProxyObject.IsSecure());
}
