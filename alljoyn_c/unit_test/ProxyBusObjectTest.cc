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

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.test.ProxyBusObjectTest";
static const char* OBJECT_NAME =    "org.alljoyn.test.ProxyBusObjectTest";
static const char* OBJECT_PATH =   "/org/alljoyn/test/ProxyObjectTest";

static QCC_BOOL chirp_method_flag = QCC_FALSE;
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

static void AJ_CALL chirp_method(alljoyn_busobject bus, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{
    chirp_method_flag = QCC_TRUE;
    alljoyn_msgarg outArg = alljoyn_msgarg_create();
    alljoyn_msgarg inArg = alljoyn_message_getarg(msg, 0);
    const char* str;
    alljoyn_msgarg_get(inArg, "s", &str);
    alljoyn_msgarg_set(outArg, "s", str);
    QStatus status = alljoyn_busobject_methodreply_args(bus, msg, NULL, 0);
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

class ProxyBusObjectTest : public testing::Test {
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

    void SetUpProxyBusObjectTestService()
    {
        /* create/start/connect alljoyn_busattachment */
        servicebus = alljoyn_busattachment_create("ProxyBusObjectTestservice", false);
        status = alljoyn_busattachment_start(servicebus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(servicebus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* create/activate alljoyn_interface */
        alljoyn_interfacedescription testIntf = NULL;
        status = alljoyn_busattachment_createinterface(servicebus, INTERFACE_NAME, &testIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(testIntf != NULL);
        if (testIntf != NULL) {
            status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "chirp", "s", "", "chirp", 0);
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

            alljoyn_interfacedescription_member chirp_member;
            foundMember = alljoyn_interfacedescription_getmember(exampleIntf, "chirp", &chirp_member);
            EXPECT_TRUE(foundMember);

            /* add methodhandlers */
            alljoyn_busobject_methodentry methodEntries[] = {
                { &chirp_member, chirp_method },
                { &ping_member, ping_method },
            };
            status = alljoyn_busobject_addmethodhandlers(testObj, methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
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
    }

    void TearDownProxyBusObjectTestService()
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

    alljoyn_busobject testObj;
    alljoyn_busattachment servicebus;
    alljoyn_buslistener buslistener;
};

TEST_F(ProxyBusObjectTest, create_destroy) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);
    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
}

TEST_F(ProxyBusObjectTest, introspectremoteobject) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription intf = alljoyn_proxybusobject_getinterface(proxyObj, "org.freedesktop.DBus.Introspectable");
    size_t buf;
    char* str;
    buf = alljoyn_interfacedescription_introspect(intf, NULL, 0, 0);
    buf++;
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(intf, str, buf, 0);
    EXPECT_STREQ(
        "<interface name=\"org.freedesktop.DBus.Introspectable\">\n"
        "  <method name=\"Introspect\">\n"
        "    <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n",
        str);
    free(str);
    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
}

QCC_BOOL introspect_callback_flag = QCC_FALSE;

void AJ_CALL introspect_callback(QStatus status, alljoyn_proxybusobject obj, void* context)
{
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription intf = alljoyn_proxybusobject_getinterface(obj, "org.freedesktop.DBus.Introspectable");
    size_t buf;
    char* str;
    buf = alljoyn_interfacedescription_introspect(intf, NULL, 0, 0);
    buf++;
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(intf, str, buf, 0);
    EXPECT_STREQ(
        "<interface name=\"org.freedesktop.DBus.Introspectable\">\n"
        "  <method name=\"Introspect\">\n"
        "    <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n",
        str);
    free(str);
    introspect_callback_flag = QCC_TRUE;
}

TEST_F(ProxyBusObjectTest, introspectremoteobjectasync) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);
    introspect_callback_flag = QCC_FALSE;
    status = alljoyn_proxybusobject_introspectremoteobjectasync(proxyObj, &introspect_callback, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (introspect_callback_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(introspect_callback_flag);
    alljoyn_interfacedescription intf = alljoyn_proxybusobject_getinterface(proxyObj, "org.freedesktop.DBus.Introspectable");
    size_t buf;
    char* str;
    buf = alljoyn_interfacedescription_introspect(intf, NULL, 0, 0);
    buf++;
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(intf, str, buf, 0);
    EXPECT_STREQ(
        "<interface name=\"org.freedesktop.DBus.Introspectable\">\n"
        "  <method name=\"Introspect\">\n"
        "    <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n",
        str);
    free(str);
    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
}


TEST_F(ProxyBusObjectTest, getinterface_getinterfaces) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription intf = alljoyn_proxybusobject_getinterface(proxyObj, "org.freedesktop.DBus.Introspectable");
    char* intf_introspect;
    size_t buf  = alljoyn_interfacedescription_introspect(intf, NULL, 0, 0);
    buf++;
    intf_introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(intf, intf_introspect, buf, 0);
    EXPECT_STREQ(
        "<interface name=\"org.freedesktop.DBus.Introspectable\">\n"
        "  <method name=\"Introspect\">\n"
        "    <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n",
        intf_introspect);
    free(intf_introspect);

    alljoyn_interfacedescription intf_array[6];
    size_t count = alljoyn_proxybusobject_getinterfaces(proxyObj, intf_array, 6);
    /*
     * the org.alljoyn.Bus object should contain 5 interfaces
     * org.alljoyn.Bus
     * org.alljoyn.Deamon
     * org.freedesktop.DBus.Introspectable
     * org.allseen.Introspectable
     * org.freedesktop.DBus.Peer
     */
    ASSERT_EQ((size_t)5, count);
    EXPECT_STREQ("org.alljoyn.Bus", alljoyn_interfacedescription_getname(intf_array[0]));
    EXPECT_STREQ("org.alljoyn.Daemon", alljoyn_interfacedescription_getname(intf_array[1]));
    EXPECT_STREQ("org.allseen.Introspectable", alljoyn_interfacedescription_getname(intf_array[2]));
    EXPECT_STREQ("org.freedesktop.DBus.Introspectable", alljoyn_interfacedescription_getname(intf_array[3]));
    EXPECT_STREQ("org.freedesktop.DBus.Peer", alljoyn_interfacedescription_getname(intf_array[4]));

    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
}

TEST_F(ProxyBusObjectTest, getpath) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);
    EXPECT_STREQ("/org/alljoyn/Bus", alljoyn_proxybusobject_getpath(proxyObj));

    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
}

TEST_F(ProxyBusObjectTest, getservicename) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);
    EXPECT_STREQ("org.alljoyn.Bus", alljoyn_proxybusobject_getservicename(proxyObj));

    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
}

TEST_F(ProxyBusObjectTest, getsessionid) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);
    EXPECT_EQ((alljoyn_sessionid)0, alljoyn_proxybusobject_getsessionid(proxyObj));

    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
    /*
     * TODO set up a session with a real session and make sure that proxyObj has
     * will return the proper sessionid.
     */

}

TEST_F(ProxyBusObjectTest, implementsinterface) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(proxyObj, "org.alljoyn.Bus"));
    EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(proxyObj, "org.alljoyn.Daemon"));
    EXPECT_FALSE(alljoyn_proxybusobject_implementsinterface(proxyObj, "org.alljoyn.Invalid"));
    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
}

TEST_F(ProxyBusObjectTest, addinterface_by_name) {
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
    EXPECT_TRUE(proxyObj);

    status = alljoyn_proxybusobject_addinterface_by_name(proxyObj, "org.freedesktop.DBus.Introspectable");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription intf = alljoyn_proxybusobject_getinterface(proxyObj, "org.freedesktop.DBus.Introspectable");
    char* intf_introspect;
    size_t buf  = alljoyn_interfacedescription_introspect(intf, NULL, 0, 0);
    buf++;
    intf_introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(intf, intf_introspect, buf, 0);
    EXPECT_STREQ(
        "<interface name=\"org.freedesktop.DBus.Introspectable\">\n"
        "  <method name=\"Introspect\">\n"
        "    <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n",
        intf_introspect);
    free(intf_introspect);

    EXPECT_FALSE(alljoyn_proxybusobject_implementsinterface(proxyObj, "org.alljoyn.Bus"));
    EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(proxyObj, "org.freedesktop.DBus.Introspectable"));
    EXPECT_NO_FATAL_FAILURE(alljoyn_proxybusobject_destroy(proxyObj));
}

TEST_F(ProxyBusObjectTest, addinterface) {
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.ProxyBusObjectTest", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_SIGNAL, "chirp", "", "s", "chirp", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest", 0);
    EXPECT_TRUE(proxyObj);

    status = alljoyn_proxybusobject_addinterface(proxyObj, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(proxyObj, "org.alljoyn.test.ProxyBusObjectTest"));
    char* introspect;
    size_t buf  = alljoyn_interfacedescription_introspect(testIntf, NULL, 0, 0);
    buf++;
    introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(testIntf, introspect, buf, 0);
    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.ProxyBusObjectTest\">\n"
        "  <signal name=\"chirp\">\n"
        "    <arg name=\"chirp\" type=\"s\" direction=\"out\"/>\n"
        "  </signal>\n"
        "  <method name=\"ping\">\n"
        "    <arg name=\"in\" type=\"s\" direction=\"in\"/>\n"
        "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n";
    EXPECT_STREQ(expectedIntrospect, introspect);
    free(introspect);
    alljoyn_proxybusobject_destroy(proxyObj);
}

TEST_F(ProxyBusObjectTest, methodcall) {
    SetUpProxyBusObjectTestService();

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

    alljoyn_message_destroy(reply);
    alljoyn_msgarg_destroy(input);
    alljoyn_proxybusobject_destroy(proxyObj);

    TearDownProxyBusObjectTestService();
}

TEST_F(ProxyBusObjectTest, methodcall_member) {
    SetUpProxyBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_message reply = alljoyn_message_create(bus);
    alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");

    /* register method handlers */
    alljoyn_interfacedescription_member ping_member_from_proxy;
    QCC_BOOL foundMember = alljoyn_interfacedescription_getmember(alljoyn_proxybusobject_getinterface(proxyObj, INTERFACE_NAME), "ping", &ping_member_from_proxy);
    EXPECT_TRUE(foundMember);

    status = alljoyn_proxybusobject_methodcall_member(proxyObj, ping_member_from_proxy, input, 1, reply, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char* str;
    alljoyn_msgarg_get(alljoyn_message_getarg(reply, 0), "s", &str);
    EXPECT_STREQ("AllJoyn", str);

    alljoyn_message_destroy(reply);
    alljoyn_msgarg_destroy(input);
    alljoyn_proxybusobject_destroy(proxyObj);

    TearDownProxyBusObjectTestService();
}

TEST_F(ProxyBusObjectTest, methodcall_noreply) {
    SetUpProxyBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    chirp_method_flag = QCC_FALSE;

    alljoyn_message reply = alljoyn_message_create(bus);
    alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");
    status = alljoyn_proxybusobject_methodcall_noreply(proxyObj, INTERFACE_NAME, "chirp", input, 1, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (size_t i = 0; i < 200; ++i) {
        if (chirp_method_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(chirp_method_flag);

    alljoyn_message_destroy(reply);
    alljoyn_msgarg_destroy(input);
    alljoyn_proxybusobject_destroy(proxyObj);

    TearDownProxyBusObjectTestService();
}

TEST_F(ProxyBusObjectTest, methodcall_member_noreply) {
    SetUpProxyBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_message reply = alljoyn_message_create(bus);
    alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");

    /* register method handlers */
    alljoyn_interfacedescription_member chirp_member_from_proxy;
    QCC_BOOL foundMember = alljoyn_interfacedescription_getmember(alljoyn_proxybusobject_getinterface(proxyObj, INTERFACE_NAME), "chirp", &chirp_member_from_proxy);
    EXPECT_TRUE(foundMember);

    chirp_method_flag = QCC_FALSE;

    status = alljoyn_proxybusobject_methodcall_member_noreply(proxyObj, chirp_member_from_proxy, input, 1, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (size_t i = 0; i < 200; ++i) {
        if (chirp_method_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(chirp_method_flag);

    alljoyn_message_destroy(reply);
    alljoyn_msgarg_destroy(input);
    alljoyn_proxybusobject_destroy(proxyObj);

    TearDownProxyBusObjectTestService();
}
QCC_BOOL ping_methodcall_reply_handler_flag = QCC_FALSE;
QCC_BOOL chirp_methodcall_reply_handler_flag = QCC_FALSE;
void AJ_CALL ping_methodcall_reply_handler(alljoyn_message message, void* context)
{
    // TODO add alljoyn_message_gettype()
    EXPECT_EQ(ALLJOYN_MESSAGE_METHOD_RET, alljoyn_message_gettype(message));
    EXPECT_STREQ("Input String to test context", (char*)context);
    const char* str;
    alljoyn_msgarg_get(alljoyn_message_getarg(message, 0), "s", &str);
    EXPECT_STREQ("AllJoyn", str);

    alljoyn_message_parseargs(message, "s", &str);
    EXPECT_STREQ("AllJoyn", str);

    ping_methodcall_reply_handler_flag = QCC_TRUE;
}

void chirp_methodcall_reply_handler(alljoyn_message message, void* context)
{
    //not yet used in any tests
}

TEST_F(ProxyBusObjectTest, methodcallasync) {
    SetUpProxyBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");

    ping_methodcall_reply_handler_flag = QCC_FALSE;

    char context_str[64] = "Input String to test context";
    status = alljoyn_proxybusobject_methodcallasync(proxyObj,
                                                    INTERFACE_NAME,
                                                    "ping",
                                                    &ping_methodcall_reply_handler,
                                                    input,
                                                    1,
                                                    context_str,
                                                    ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(input);
    alljoyn_proxybusobject_destroy(proxyObj);

    for (size_t i = 0; i < 200; ++i) {
        if (ping_methodcall_reply_handler_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(ping_methodcall_reply_handler_flag);

    TearDownProxyBusObjectTestService();
}

TEST_F(ProxyBusObjectTest, methodcallasync_member) {
    SetUpProxyBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");

    ping_methodcall_reply_handler_flag = QCC_FALSE;

    /* register method handlers */
    alljoyn_interfacedescription_member ping_member_from_proxy;
    QCC_BOOL foundMember = alljoyn_interfacedescription_getmember(alljoyn_proxybusobject_getinterface(proxyObj, INTERFACE_NAME), "ping", &ping_member_from_proxy);
    EXPECT_TRUE(foundMember);
    char context_str[64] = "Input String to test context";
    status = alljoyn_proxybusobject_methodcallasync_member(proxyObj,
                                                           ping_member_from_proxy,
                                                           &ping_methodcall_reply_handler,
                                                           input,
                                                           1,
                                                           context_str,
                                                           ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(input);
    alljoyn_proxybusobject_destroy(proxyObj);

    for (size_t i = 0; i < 200; ++i) {
        if (ping_methodcall_reply_handler_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(ping_methodcall_reply_handler_flag);

    TearDownProxyBusObjectTestService();
}

TEST_F(ProxyBusObjectTest, parsexml) {
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

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, NULL, NULL, 0);
    EXPECT_TRUE(proxyObj);

    status = alljoyn_proxybusobject_parsexml(proxyObj, busObjectXML, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(proxyObj, "org.alljoyn.test.ProxyBusObjectTest"));

    alljoyn_interfacedescription testIntf = alljoyn_proxybusobject_getinterface(proxyObj, "org.alljoyn.test.ProxyBusObjectTest");
    char* introspect;
    size_t buf  = alljoyn_interfacedescription_introspect(testIntf, NULL, 0, 0);
    buf++;
    introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(testIntf, introspect, buf, 0);

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
    EXPECT_STREQ(expectedIntrospect, introspect);
    free(introspect);
    alljoyn_proxybusobject_destroy(proxyObj);
}

TEST_F(ProxyBusObjectTest, Add_Get_Remove_Child) {
    QStatus status;
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.ProxyBusObjectTest", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject proxyObjChildOne = alljoyn_proxybusobject_create(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest/ChildOne", 0);
    EXPECT_TRUE(proxyObjChildOne);

    status = alljoyn_proxybusobject_addinterface(proxyObjChildOne, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject proxyObjChildTwo = alljoyn_proxybusobject_create(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest/ChildTwo", 0);
    EXPECT_TRUE(proxyObjChildOne);

    status = alljoyn_proxybusobject_addinterface(proxyObjChildTwo, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, NULL, NULL, 0);
    EXPECT_TRUE(proxyObj);

    status = alljoyn_proxybusobject_addchild(proxyObj, proxyObjChildOne);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_proxybusobject_addchild(proxyObj, proxyObjChildTwo);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_TRUE(alljoyn_proxybusobject_isvalid(proxyObj));

    alljoyn_proxybusobject proxyObjChild1 = alljoyn_proxybusobject_getchild(proxyObj, "/org/alljoyn/test/ProxyObjectTest/ChildOne");
    ASSERT_TRUE(proxyObjChild1);
    ASSERT_TRUE(alljoyn_proxybusobject_isvalid(proxyObjChild1));
    EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(proxyObjChild1, "org.alljoyn.test.ProxyBusObjectTest"));

    alljoyn_interfacedescription testIntfChild1 = alljoyn_proxybusobject_getinterface(proxyObjChild1, "org.alljoyn.test.ProxyBusObjectTest");
    char* introspect;
    size_t buf  = alljoyn_interfacedescription_introspect(testIntfChild1, NULL, 0, 0);
    buf++;
    introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(testIntfChild1, introspect, buf, 0);

    const char* expectedIntrospect =
        "<interface name=\"org.alljoyn.test.ProxyBusObjectTest\">\n"
        "  <method name=\"ping\">\n"
        "    <arg name=\"in\" type=\"s\" direction=\"in\"/>\n"
        "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
        "  </method>\n"
        "</interface>\n";
    EXPECT_STREQ(expectedIntrospect, introspect);
    free(introspect);

    alljoyn_proxybusobject proxyObjChild2 = alljoyn_proxybusobject_getchild(proxyObj, "/org/alljoyn/test/ProxyObjectTest/ChildTwo");
    ASSERT_TRUE(proxyObjChild2 != NULL);
    EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(proxyObjChild2, "org.alljoyn.test.ProxyBusObjectTest"));

    alljoyn_interfacedescription testIntfChild2 = alljoyn_proxybusobject_getinterface(proxyObjChild2, "org.alljoyn.test.ProxyBusObjectTest");
    buf  = alljoyn_interfacedescription_introspect(testIntfChild2, NULL, 0, 0);
    buf++;
    introspect = (char*)malloc(sizeof(char) * buf);
    alljoyn_interfacedescription_introspect(testIntfChild2, introspect, buf, 0);

    EXPECT_STREQ(expectedIntrospect, introspect);
    free(introspect);

    status = alljoyn_proxybusobject_removechild(proxyObj, "/org/alljoyn/test/ProxyObjectTest/ChildOne");
    alljoyn_proxybusobject proxyObjChildRemoved = alljoyn_proxybusobject_getchild(proxyObj, "/org/alljoyn/test/ProxyObjectTest/ChildOne");
    EXPECT_FALSE(proxyObjChildRemoved);

    alljoyn_proxybusobject_destroy(proxyObjChildOne);
    alljoyn_proxybusobject_destroy(proxyObjChildTwo);
    alljoyn_proxybusobject_destroy(proxyObj);
}

TEST_F(ProxyBusObjectTest, getchildren) {
    QStatus status;
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.ProxyBusObjectTest", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject proxyObjChildOne = alljoyn_proxybusobject_create(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest/ChildOne", 0);
    EXPECT_TRUE(proxyObjChildOne);

    status = alljoyn_proxybusobject_addinterface(proxyObjChildOne, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject proxyObjChildTwo = alljoyn_proxybusobject_create(bus, "org.alljoyn.test.ProxyBusObjectTest", "/org/alljoyn/test/ProxyObjectTest/ChildTwo", 0);
    EXPECT_TRUE(proxyObjChildOne);

    status = alljoyn_proxybusobject_addinterface(proxyObjChildTwo, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, NULL, NULL, 0);
    EXPECT_TRUE(proxyObj);

    status = alljoyn_proxybusobject_addchild(proxyObj, proxyObjChildOne);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_proxybusobject_addchild(proxyObj, proxyObjChildTwo);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_TRUE(alljoyn_proxybusobject_isvalid(proxyObj));

    alljoyn_proxybusobject proxyObjSub = alljoyn_proxybusobject_getchild(proxyObj, "/org/alljoyn/test/ProxyObjectTest");
    ASSERT_TRUE(proxyObjSub != NULL);
    size_t numChildren;
    alljoyn_proxybusobject* children;
    numChildren = alljoyn_proxybusobject_getchildren(proxyObjSub, NULL, 0);
    EXPECT_EQ((size_t)2, numChildren);
    children = (alljoyn_proxybusobject*) malloc(sizeof(alljoyn_proxybusobject) * numChildren);
    alljoyn_proxybusobject_getchildren(proxyObjSub, children, numChildren);

    for (size_t i = 0; i < numChildren; ++i) {
        ASSERT_TRUE(children[i]) << "Test interface for child " << i << " should not be NULL";
        ASSERT_TRUE(alljoyn_proxybusobject_isvalid(children[i])) << "Test interface for child " << i << " should be a valid alljoyn_proxybusobject.";
        EXPECT_TRUE(alljoyn_proxybusobject_implementsinterface(children[i], "org.alljoyn.test.ProxyBusObjectTest")) <<
        "Test interface for child " << i << " should implement the org.alljoyn.test.ProxyBusObjectTest interface.";

        alljoyn_interfacedescription testIntfChild = alljoyn_proxybusobject_getinterface(children[i], "org.alljoyn.test.ProxyBusObjectTest");
        char* introspect;
        size_t buf  = alljoyn_interfacedescription_introspect(testIntfChild, NULL, 0, 0);
        buf++;
        introspect = (char*)malloc(sizeof(char) * buf);
        alljoyn_interfacedescription_introspect(testIntfChild, introspect, buf, 0);

        const char* expectedIntrospect =
            "<interface name=\"org.alljoyn.test.ProxyBusObjectTest\">\n"
            "  <method name=\"ping\">\n"
            "    <arg name=\"in\" type=\"s\" direction=\"in\"/>\n"
            "    <arg name=\"out\" type=\"s\" direction=\"out\"/>\n"
            "  </method>\n"
            "</interface>\n";
        EXPECT_STREQ(expectedIntrospect, introspect) << "Test interface for child " << i << " did not have expected introspection.";
        free(introspect);
    }
    free(children);
    alljoyn_proxybusobject_destroy(proxyObjChildOne);
    alljoyn_proxybusobject_destroy(proxyObjChildTwo);
    alljoyn_proxybusobject_destroy(proxyObj);
}
