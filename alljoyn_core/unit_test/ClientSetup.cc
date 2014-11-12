/*******************************************************************************
 * Copyright (c) 2011, 2014, AllSeen Alliance. All rights reserved.
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
#include "ClientSetup.h"
#include <qcc/Thread.h>
#include <gtest/gtest.h>

namespace cl {
namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* InterfaceName = "org.alljoyn.test_services.Interface";
namespace dummy {
const char* InterfaceName1 = "org.alljoyn.test_services.dummy.Interface1";
const char* InterfaceName2 = "org.alljoyn.test_services.dummy.Interface2";
const char* InterfaceName3 = "org.alljoyn.test_services.dummy.Interface3";
}
const char* ObjectPath = "/org/alljoyn/test_services";
namespace values {
const char* InterfaceName = "org.alljoyn.test_services.Interface.values";
namespace dummy {
const char* InterfaceName1 = "org.alljoyn.test_services.values.dummy.Interface1";
const char* InterfaceName2 = "org.alljoyn.test_services.values.dummy.Interface2";
const char* InterfaceName3 = "org.alljoyn.test_services.values.dummy.Interface3";
}                     // end of dummy
}                   // end of values
}               // end of alljoyn_test
}           // end of alljoyn
}       // end of org
}   // end of cl

/* Client setup */
ClientSetup::ClientSetup(const char* default_bus_addr, const char* wellKnownName) :
    clientMsgBus("clientSetup", true), wellKnownName(wellKnownName)
{
    QStatus status = ER_OK;
    {
        /* Get env vars */
        Environ*env = Environ::GetAppEnviron();
        clientArgs = env->Find("BUS_ADDRESS", default_bus_addr);

        /* Start the msg bus */
        status = clientMsgBus.Start();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Client bus start failed";

        if (status == ER_OK) {
            /* Connect to the Bus */
            status = clientMsgBus.Connect(clientArgs.c_str());
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Client Bus connect failed";
        }
    }
    return;
}

BusAttachment* ClientSetup::getClientMsgBus()
{
    return &clientMsgBus;
}

qcc::String ClientSetup::getClientArgs()
{
    return clientArgs;
}

QStatus ClientSetup::MethodCall(int noOfCalls, int type)
{
    QStatus status = ER_OK;

    ProxyBusObject remoteObj(clientMsgBus,
                             wellKnownName.c_str(),
                             ::cl::org::alljoyn::alljoyn_test::ObjectPath,
                             0);

    Message reply(clientMsgBus);

    status = remoteObj.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Problem while introspecting remote object";
    if (status != ER_OK) {
        return status;
    }

    if (type == 1) {
        MsgArg pingStr("s", "Test Ping");
        for (int i = 0; i < noOfCalls; i++) {
            status = remoteObj.MethodCall(
                ::cl::org::alljoyn::alljoyn_test::InterfaceName,
                "my_ping",
                &pingStr,
                1,
                reply,
                5000);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Problem while calling remote method.";
            if (status != ER_OK) {
                //exit the if status not ER_OK
                return status;
            }
        }         // end for loop
    }     //end if type == 1
    else if (type == 2) {

        MsgArg inputArgs[10];

        inputArgs[0].Set("s", "one");
        inputArgs[1].Set("s", "two");
        inputArgs[2].Set("s", "three");
        inputArgs[3].Set("s", "four");
        inputArgs[4].Set("s", "five");
        inputArgs[5].Set("s", "six");
        inputArgs[6].Set("s", "seven");
        inputArgs[7].Set("s", "eight");
        inputArgs[8].Set("s", "nine");
        inputArgs[9].Set("s", "ten");


        for (int i = 0; i < noOfCalls; i++) {

            status = remoteObj.MethodCall(
                ::cl::org::alljoyn::alljoyn_test::InterfaceName,
                "my_param_test",
                inputArgs,
                10,
                reply,
                5000);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Problem while calling remote method";
            if (status != ER_OK) {
                return status;
            }
        }
    }     // end if type == 2
    else if (type == 3) {

        MsgArg inputArgs[10];

        inputArgs[0].Set("s", "");
        inputArgs[1].Set("s", "");
        inputArgs[2].Set("s", "");
        inputArgs[3].Set("s", "");
        inputArgs[4].Set("s", "");
        inputArgs[5].Set("s", "");
        inputArgs[6].Set("s", "");
        inputArgs[7].Set("s", "");
        inputArgs[8].Set("s", "");
        inputArgs[9].Set("s", "");

        for (int i = 0; i < noOfCalls; i++) {

            status = remoteObj.MethodCall(
                ::cl::org::alljoyn::alljoyn_test::InterfaceName,
                "my_param_test",
                inputArgs,
                10,
                reply,
                5000);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Problem while calling remote method";
            if (status != ER_OK) {
                return status;
            }
        }

        const MsgArg*replyArgs[10];
        size_t numArgs;
        reply->GetArgs(numArgs, *replyArgs);
        EXPECT_EQ(static_cast<size_t>(10), numArgs);
    }     // end if type == 3
    else if (type == 4) {

        MsgArg inputArgs[10];

        inputArgs[0].Set("i", "");
        inputArgs[1].Set("i", "");
        inputArgs[2].Set("i", "");
        inputArgs[3].Set("i", "");
        inputArgs[4].Set("i", "");
        inputArgs[5].Set("i", "");
        inputArgs[6].Set("i", "");
        inputArgs[7].Set("i", "");
        inputArgs[8].Set("i", "");
        inputArgs[9].Set("i", "");

        for (int i = 0; i < noOfCalls; i++) {
            status = remoteObj.MethodCall(
                ::cl::org::alljoyn::alljoyn_test::InterfaceName,
                "my_param_test",
                inputArgs,
                10,
                reply,
                5000);
            EXPECT_EQ(ER_BUS_UNEXPECTED_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
            if (status != ER_OK) {
                return status;
            }
        }
        const MsgArg*replyArgs[10];
        size_t numArgs;
        reply->GetArgs(numArgs, *replyArgs);
        printf("\n Reply received : %s", replyArgs[0]->v_string.str);

    }     // end if type == 4
    return status;
}

QStatus ClientSetup::AsyncMethodCall(int noOfCalls, int type)
{
    QStatus status = ER_OK;

    ProxyBusObject remoteObj(
        clientMsgBus,
        wellKnownName.c_str(),
        ::cl::org::alljoyn::alljoyn_test::ObjectPath,
        0);

    Message reply(clientMsgBus);

    status = remoteObj.IntrospectRemoteObject();

    if (type == 1) {

        MsgArg pingStr("s", "Test Ping");
        for (int i = 0; i < noOfCalls; i++) {
            status = remoteObj.MethodCallAsync(::cl::org::alljoyn::alljoyn_test::InterfaceName, "my_ping",
                                               this, static_cast<MessageReceiver::ReplyHandler>(&ClientSetup::AsyncCallReplyHandler),
                                               &pingStr, 1);
            //don't clog up the the queue sending signals too quickly
            qcc::Sleep(1);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Problem while calling remote method";
            if (status != ER_OK) {
                return status;
            }
        }
    } else if (type == 2) {

        MsgArg inputArgs[10];

        inputArgs[0] = "one";
        inputArgs[1] = "two";
        inputArgs[2] = "three";
        inputArgs[3] = "four";
        inputArgs[4] = "five";
        inputArgs[5] = "six";
        inputArgs[6] = "seven";
        inputArgs[7] = "eight";
        inputArgs[8] = "nine";
        inputArgs[9] = "ten";

        for (int i = 0; i < noOfCalls; i++) {
            status = remoteObj.MethodCall(::cl::org::alljoyn::alljoyn_test::InterfaceName, "my_param_test", inputArgs, 10, reply, 5000);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Problem while calling remote method";
            if (status != ER_OK) {
                return status;
            }
        }
    } // end of type==2
    return status;
}

/*
 * Function to handle Asynchronous method call reply
 */
void ClientSetup::AsyncCallReplyHandler(Message& msg, void* context)
{
    //static int count = 0;
    //printf("Reply no %d Received : %s\n", count++, msg->GetArg(0)->v_string.str);
    g_Signal_flag++;
}

/*
 * Function to test signals
 */
QStatus ClientSetup::SignalHandler(int noOfCalls, int type)
{
    QStatus status = ER_OK;
    const InterfaceDescription* intf = NULL;
    const InterfaceDescription::Member* mysignal = NULL;
    const InterfaceDescription::Member* mysignal_2 = NULL;

    /* Create a remote object */
    ProxyBusObject remoteObj(clientMsgBus, wellKnownName.c_str(), ::cl::org::alljoyn::alljoyn_test::ObjectPath, 0);
    Message reply(clientMsgBus);

    status = remoteObj.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Problem while introspecting the remote object";
    if (status != ER_OK) {
        return status;
    }

    intf = remoteObj.GetInterface(::cl::org::alljoyn::alljoyn_test::InterfaceName);
    assert(intf);
    remoteObj.AddInterface(*intf);
    mysignal = intf->GetMember("my_signal");
    assert("mysignal");
    mysignal_2 = intf->GetMember("my_signal_string");
    assert("mysignal_2");

    /* register the signal handler for the the 'my_signal' signal */
    status =  clientMsgBus.RegisterSignalHandler(this,
                                                 static_cast<MessageReceiver::SignalHandler>(&ClientSetup::MySignalHandler),
                                                 mysignal,
                                                 NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: "
                             << QCC_StatusText(status)
                             << " Problem while registering signal handler";
    if (status != ER_OK) {
        return status;
    }

    /* register the signal handler for the the 'my_signal' signal */
    status =  clientMsgBus.RegisterSignalHandler(this,
                                                 static_cast<MessageReceiver::SignalHandler>(&ClientSetup::MySignalHandler2),
                                                 mysignal_2,
                                                 NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: "
                             << QCC_StatusText(status)
                             << " Problem while registering signal handler";

    /* add the match rules */
    status = clientMsgBus.AddMatch("type='signal',interface='org.alljoyn.test_services.Interface',member='my_signal1'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: "
                             << QCC_StatusText(status)
                             << "Failed to register Match rule for 'org.alljoyn.test_services.my_signal1'";

    status = clientMsgBus.AddMatch("type='signal',interface='org.alljoyn.test_services.Interface',member='my_signal_string'");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: "
                             << QCC_StatusText(status)
                             << "Failed to register Match rule for 'org.alljoyn.test_services.my_signal_string'";

    if (type == 1) {
        MsgArg singStr("s", "Sing String");
        status = remoteObj.MethodCall(::cl::org::alljoyn::alljoyn_test::InterfaceName, "my_sing", &singStr, 1, reply, 5000);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: "
                                 << QCC_StatusText(status) << "\n"
                                 << "MethodCall on " << ::cl::org::alljoyn::alljoyn_test::InterfaceName << "." << "my_sing";
        EXPECT_STREQ("Sing String", reply->GetArg(0)->v_string.str);
    } // end if type == 1
    else if (type == 2) {
        MsgArg singStr("s", "Huge String");
        status = remoteObj.MethodCall(::cl::org::alljoyn::alljoyn_test::InterfaceName, "my_sing", &singStr, 1, reply, 5000);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: "
                                 << QCC_StatusText(status) << "\n"
                                 << "MethodCall on " << ::cl::org::alljoyn::alljoyn_test::InterfaceName << "." << "my_sing";
    } // end if type == 2
    return status;
}

void ClientSetup::MySignalHandler(
    const InterfaceDescription::Member*member,
    const char* sourcePath,
    Message& msg)
{
    printf("\n Inside the signal handler ");
}

void ClientSetup::MySignalHandler2(
    const InterfaceDescription::Member*member,
    const char* sourcePath,
    Message& msg)
{
    //printf("\n Inside the signal handler 2");
    const MsgArg*replyArgs[2];
    size_t numArgs;
    msg->GetArgs(numArgs, *replyArgs);
    EXPECT_EQ(static_cast<size_t>(2), numArgs);
    if (replyArgs[0]->v_uint32 == 5) {
        EXPECT_STREQ("hello", msg->GetArg(1)->v_string.str);
        EXPECT_EQ(static_cast<uint32_t>(5), msg->GetArg(1)->v_string.len);
    } else if (msg->GetArg(1)->v_uint32 == 4096) {
        EXPECT_EQ(static_cast<uint32_t>(4096), msg->GetArg(1)->v_string.len);
        qcc::String hugeA(4096, 'a');
        EXPECT_STREQ(hugeA.c_str(), msg->GetArg(1)->v_string.str);
    } else {
        EXPECT_TRUE(false) << "Received unexpected signal.";
    }
//    printf("\n Reply received : %d", replyArgs[0]->v_uint32);
//    //printf("\n Reply received : %s",replyArgs[1]->v_string.str);
//    printf("\n Reply received : %s", msg->GetArg(1)->v_string.str);

    g_Signal_flag = replyArgs[0]->v_uint32;

}

int ClientSetup::getSignalFlag()
{
    return g_Signal_flag;
}

void ClientSetup::setSignalFlag(int flag)
{
    g_Signal_flag = flag;
}

const char* ClientSetup::getClientInterfaceName() const
{
    return ::cl::org::alljoyn::alljoyn_test::InterfaceName;
}

const char* ClientSetup::getClientDummyInterfaceName1() const
{
    return ::cl::org::alljoyn::alljoyn_test::dummy::InterfaceName1;
}

const char* ClientSetup::getClientDummyInterfaceName2() const
{
    return ::cl::org::alljoyn::alljoyn_test::dummy::InterfaceName2;
}

const char* ClientSetup::getClientDummyInterfaceName3() const
{
    return ::cl::org::alljoyn::alljoyn_test::dummy::InterfaceName3;
}

const char* ClientSetup::getClientObjectPath() const
{
    return ::cl::org::alljoyn::alljoyn_test::ObjectPath;
}

const char* ClientSetup::getClientValuesInterfaceName() const
{
    return ::cl::org::alljoyn::alljoyn_test::values::InterfaceName;
}

const char* ClientSetup::getClientValuesDummyInterfaceName1() const
{
    return ::cl::org::alljoyn::alljoyn_test::values::dummy::InterfaceName1;
}

const char* ClientSetup::getClientValuesDummyInterfaceName2() const
{
    return ::cl::org::alljoyn::alljoyn_test::values::dummy::InterfaceName2;
}

const char* ClientSetup::getClientValuesDummyInterfaceName3() const
{
    return ::cl::org::alljoyn::alljoyn_test::values::dummy::InterfaceName3;
}

