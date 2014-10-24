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

#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/Message.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/ProxyBusObject.h>
#include <qcc/Thread.h>
#include <qcc/String.h>

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.test.MessageTest";
static const char* OBJECT_NAME =    "org.alljoyn.test.MessageTest";
static const char* OBJECT_PATH =   "/org/alljoyn/test/MessageTest";

static QCC_BOOL name_owner_changed_flag = QCC_FALSE;

/* Exposed methods */
static void AJ_CALL ping_method(alljoyn_busobject bus, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{
    alljoyn_msgarg outArg = alljoyn_msgarg_create();
    alljoyn_msgarg inArg = alljoyn_message_getarg(msg, 0);
    const char* str;
    alljoyn_msgarg_get(inArg, "s", &str);
    alljoyn_msgarg_set(outArg, "s", str);
    QStatus status = alljoyn_busobject_methodreply_args(bus, msg, outArg, 1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(outArg);
}

/* NameOwnerChanged callback */
static void AJ_CALL name_owner_changed(const void* context, const char* busName, const char* previousOwner, const char* newOwner)
{
    if (strcmp(busName, OBJECT_NAME) == 0) {
        name_owner_changed_flag = QCC_TRUE;
    }
}

class MessageTest : public testing::Test {
  public:
    virtual void SetUp() {
        bus = alljoyn_busattachment_create("ProxyBusObjectTest", false);
        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        EXPECT_NO_FATAL_FAILURE(alljoyn_busattachment_destroy(bus));
    }

    void SetUpMessageTestService()
    {
        /* create/start/connect alljoyn_busattachment */
        servicebus = alljoyn_busattachment_create("MessageTestservice", false);
        status = alljoyn_busattachment_start(servicebus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(servicebus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* create/activate alljoyn_interface */
        alljoyn_interfacedescription testIntf = NULL;
        status = alljoyn_busattachment_createinterface(servicebus, INTERFACE_NAME, &testIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(testIntf != NULL);
        status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        alljoyn_interfacedescription_activate(testIntf);

        /* register bus listener */
        alljoyn_buslistener_callbacks buslistenerCbs = {
            NULL,
            NULL,
            NULL,
            NULL,
            &name_owner_changed,
            NULL,
            NULL,
            NULL
        };
        buslistener = alljoyn_buslistener_create(&buslistenerCbs, NULL);
        alljoyn_busattachment_registerbuslistener(servicebus, buslistener);

        /* Set up bus object */
        alljoyn_busobject_callbacks busObjCbs = {
            NULL,
            NULL,
            NULL,
            NULL
        };
        testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
        const alljoyn_interfacedescription exampleIntf = alljoyn_busattachment_getinterface(servicebus, INTERFACE_NAME);
        ASSERT_TRUE(exampleIntf);

        status = alljoyn_busobject_addinterface(testObj, exampleIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* register method handlers */
        alljoyn_interfacedescription_member ping_member;
        QCC_BOOL foundMember = alljoyn_interfacedescription_getmember(exampleIntf, "ping", &ping_member);
        EXPECT_TRUE(foundMember);

        /* add methodhandlers */
//        alljoyn_busobject_methodentry methodEntries[] = {,
//            { &ping_member, ping_method },
//        };
//        status = alljoyn_busobject_addmethodhandlers(testObj, methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        status = alljoyn_busobject_addmethodhandler(testObj, ping_member, &ping_method, NULL);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = alljoyn_busattachment_registerbusobject(servicebus, testObj);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        name_owner_changed_flag = QCC_FALSE;

        /* request name */
        uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
        status = alljoyn_busattachment_requestname(servicebus, OBJECT_NAME, flags);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        for (size_t i = 0; i < 200; ++i) {
            if (name_owner_changed_flag) {
                break;
            }
            qcc::Sleep(5);
        }
        EXPECT_TRUE(name_owner_changed_flag);
    }

    void TearDownMessageTestService()
    {
        alljoyn_busattachment_unregisterbuslistener(servicebus, buslistener);
        /*
         * must destroy the busattachment before destroying the buslistener or
         * the code will segfault when the code tries to call the bus_stopping
         * callback.
         */
        alljoyn_busattachment_destroy(servicebus);
        alljoyn_buslistener_destroy(buslistener);
        alljoyn_busobject_destroy(testObj);
    }

    QStatus status;
    alljoyn_busattachment bus;

    alljoyn_busattachment servicebus;
    alljoyn_buslistener buslistener;
    alljoyn_busobject testObj;
};

TEST_F(MessageTest, getarg__getargs_parseargs) {
    SetUpMessageTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_message reply = alljoyn_message_create(bus);
    alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");
    status = alljoyn_proxybusobject_methodcall(proxyObj, INTERFACE_NAME, "ping", input, 1, reply, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* str;
    alljoyn_msgarg_get(alljoyn_message_getarg(reply, 0), "s", &str);
    EXPECT_STREQ("AllJoyn", str);

    str = NULL;
    alljoyn_msgarg output;
    size_t numArgs;
    alljoyn_message_getargs(reply, &numArgs, &output);
    EXPECT_EQ((size_t)1, numArgs);
    alljoyn_msgarg arg = alljoyn_msgarg_array_element(output, 0);
    size_t buf = alljoyn_msgarg_signature(arg, NULL, 0);
    char* val = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_signature(arg, val, buf);
    EXPECT_STREQ("s", val);
    free(val);

    status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(output, 0), "s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("AllJoyn", str);

    str = NULL;
    status = alljoyn_message_parseargs(reply, "s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("AllJoyn", str);

    alljoyn_message_destroy(reply);
    alljoyn_msgarg_destroy(input);
    alljoyn_proxybusobject_destroy(proxyObj);

    TearDownMessageTestService();
}

/*
 * The alljoyn_message_description and alljoyn_message_tostring function are some
 * of the few functions that behave differently with the release variant vs.
 * the debug variant.
 */
TEST_F(MessageTest, message_properties) {
    SetUpMessageTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_message reply = alljoyn_message_create(bus);
    alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");
    status = alljoyn_proxybusobject_methodcall(proxyObj, INTERFACE_NAME, "ping", input, 1, reply, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(input);

    EXPECT_FALSE(alljoyn_message_isbroadcastsignal(reply));
    EXPECT_FALSE(alljoyn_message_isglobalbroadcast(reply));
    EXPECT_FALSE(alljoyn_message_issessionless(reply));
    uint32_t timeLeft;
    EXPECT_FALSE(alljoyn_message_isexpired(reply, &timeLeft));
    EXPECT_NE((uint32_t)0, timeLeft);
    EXPECT_FALSE(alljoyn_message_isunreliable(reply));
    EXPECT_FALSE(alljoyn_message_isencrypted(reply));

    /* we don't expect any of the flags to be set */
    EXPECT_EQ(0, alljoyn_message_getflags(reply));

    /* no security is being used so there should not be an auth mechanism specified */
    EXPECT_STREQ("", alljoyn_message_getauthmechanism(reply));

    EXPECT_EQ(ALLJOYN_MESSAGE_METHOD_RET, alljoyn_message_gettype(reply));

    /* The serial is unknown but it should not be zero */
    EXPECT_NE((uint32_t)0, alljoyn_message_getcallserial(reply));
    EXPECT_NE((uint32_t)0, alljoyn_message_getreplyserial(reply));

    EXPECT_STREQ("s", alljoyn_message_getsignature(reply));
    /* in this instance we can not find objectpathm interface name , or member name from the message*/
    EXPECT_STREQ("", alljoyn_message_getobjectpath(reply));
    EXPECT_STREQ("", alljoyn_message_getinterface(reply));
    EXPECT_STREQ("", alljoyn_message_getmembername(reply));

    const char* destination_uniqueName;
    destination_uniqueName = alljoyn_busattachment_getuniquename(bus);
    EXPECT_STREQ(destination_uniqueName, alljoyn_message_getdestination(reply));
    EXPECT_STREQ(destination_uniqueName, alljoyn_message_getreceiveendpointname(reply));

    const char* sender_uniqueName;
    sender_uniqueName = alljoyn_busattachment_getuniquename(servicebus);
    EXPECT_STREQ(sender_uniqueName, alljoyn_message_getsender(reply));

    EXPECT_EQ((uint32_t)0, alljoyn_message_getcompressiontoken(reply));
    EXPECT_EQ((alljoyn_sessionid)0, alljoyn_message_getsessionid(reply));

#if NDEBUG
    char* str;
    size_t buf;
    buf = alljoyn_message_tostring(reply, NULL, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_message_tostring(reply, str, buf);
    qcc::String strTest = str;
    free(str);

    buf = alljoyn_message_description(reply, NULL, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_message_description(reply, str, buf);
    EXPECT_STREQ("", str);
    free(str);
#else
    char* str;
    size_t buf;
    buf = alljoyn_message_tostring(reply, NULL, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_message_tostring(reply, str, buf);
    qcc::String strTest = str;
    /* all messages should start by stating the endianness */
    EXPECT_EQ((size_t)0, strTest.find_first_of("<message endianness="));
    free(str);

    buf = alljoyn_message_description(reply, NULL, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_message_description(reply, str, buf);
    strTest = str;
    /* this call to description should return 'METHID_RET[<reply serial>](s)' */
    EXPECT_EQ((size_t)0, strTest.find_first_of("METHOD_RET["));
    free(str);
#endif
    alljoyn_message_destroy(reply);
    alljoyn_proxybusobject_destroy(proxyObj);
    TearDownMessageTestService();
}
