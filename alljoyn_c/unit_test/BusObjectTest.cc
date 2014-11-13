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

#include <qcc/Thread.h>

#include <string.h>
#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/BusObject.h>
#include <alljoyn_c/MsgArg.h>

#include "ajTestCommon.h"

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.test.BusObjectTest";
static const char* OBJECT_NAME = "org.alljoyn.test.BusObjectTest";
static const char* OBJECT_PATH = "/org/alljoyn/test/BusObjectTest";

/*********** BusObject callback functions **************/
static QCC_BOOL object_registered_flag = QCC_FALSE;
static QCC_BOOL object_unregistered_flag = QCC_FALSE;
static QCC_BOOL name_owner_changed_flag = QCC_FALSE;
static QCC_BOOL prop_changed_flag = QCC_FALSE;

static const char* prop1 = "AllJoyn BusObject Test"; //read only property
static int32_t prop2; //write only property
static uint32_t prop3; //RW property
static QStatus AJ_CALL get_property(const void* context, const char* ifcName, const char* propName, alljoyn_msgarg val)
{
    EXPECT_STREQ(INTERFACE_NAME, ifcName);
    QStatus status = ER_OK;
    if (0 == strcmp("prop1", propName)) {
        alljoyn_msgarg_set(val, "s", prop1);
    } else if (0 == strcmp("prop2", propName)) {
        alljoyn_msgarg_set(val, "i", prop2);
    } else if (0 == strcmp("prop3", propName)) {
        alljoyn_msgarg_set(val, "u", prop3);
    } else {
        status = ER_BUS_NO_SUCH_PROPERTY;
    }
    return status;
}

static QStatus AJ_CALL set_property(const void* context, const char* ifcName, const char* propName, alljoyn_msgarg val)
{
    EXPECT_STREQ(INTERFACE_NAME, ifcName);
    QStatus status = ER_OK;
    if (0 == strcmp("prop1", propName)) {
        alljoyn_msgarg_get(val, "s", &prop1);
    } else if (0 == strcmp("prop2", propName)) {
        alljoyn_msgarg_get(val, "i", &prop2);
    } else if (0 == strcmp("prop3", propName)) {
        alljoyn_msgarg_get(val, "u", &prop3);
    } else {
        status = ER_BUS_NO_SUCH_PROPERTY;
    }
    return status;
}

static void AJ_CALL busobject_registered(const void* context)
{
    object_registered_flag = QCC_TRUE;
}

static void AJ_CALL busobject_unregistered(const void* context)
{
    object_unregistered_flag = QCC_TRUE;
}

/* NameOwnerChanged callback */
static void AJ_CALL name_owner_changed(const void* context, const char* busName, const char* previousOwner, const char* newOwner)
{
    if (strcmp(busName, OBJECT_NAME) == 0) {
        name_owner_changed_flag = QCC_TRUE;
    }
}

/* Property changed callback */
static void AJ_CALL obj_prop_changed(alljoyn_proxybusobject obj, const char* iface_name, alljoyn_msgarg changed, alljoyn_msgarg invalidated, void* context)
{
    alljoyn_msgarg argList;
    size_t argListSize;
    QStatus status = ER_FAIL; //default state is failure

    ASSERT_TRUE(invalidated);
    // Invalidated properties
    status = alljoyn_msgarg_get(invalidated, "as", &argListSize, &argList);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    if (argListSize > 0) {
        EXPECT_EQ(1UL, argListSize);
        ASSERT_TRUE(argList);
        for (size_t index = 0; index < argListSize; index++) {
            char* propName;
            ASSERT_TRUE(alljoyn_msgarg_array_element(argList, index));
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(argList, index), "s", &propName);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            ASSERT_TRUE(propName);
            EXPECT_STREQ("prop2", propName);
        }
    }

    ASSERT_TRUE(changed);
    // Changed properties
    status = alljoyn_msgarg_get(changed, "a{sv}", &argListSize, &argList);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    if (argListSize > 0) {
        EXPECT_EQ(1UL, argListSize);
        ASSERT_TRUE(argList);
        for (size_t index = 0; index < argListSize; index++) {
            char* propName;
            alljoyn_msgarg valueArg;
            uint32_t value;
            ASSERT_TRUE(alljoyn_msgarg_array_element(argList, index));
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(argList, index), "{sv}", &propName, &valueArg);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            ASSERT_TRUE(propName);
            EXPECT_STREQ("prop3", propName);
            status = alljoyn_msgarg_get(valueArg, "u", &value);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            EXPECT_EQ(prop3, value);
        }
    }

    prop_changed_flag = QCC_TRUE;
}
/************* Method handlers *************/
static QCC_BOOL chirp_method_flag = QCC_FALSE;

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
    alljoyn_msgarg outArg = alljoyn_msgarg_create();
    alljoyn_msgarg inArg = alljoyn_message_getarg(msg, 0);
    const char* str;
    alljoyn_msgarg_get(inArg, "s", &str);
    alljoyn_msgarg_set(outArg, "s", str);
    QStatus status = alljoyn_busobject_methodreply_args(bus, msg, NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    chirp_method_flag = QCC_TRUE;
    alljoyn_msgarg_destroy(outArg);
}

class BusObjectTest : public testing::Test {
  public:
    virtual void SetUp() {
        object_registered_flag = QCC_FALSE;
        object_unregistered_flag = QCC_TRUE;

        bus = alljoyn_busattachment_create("ProxyBusObjectTest", false);
        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        EXPECT_NO_FATAL_FAILURE(alljoyn_busattachment_destroy(bus));
    }

    void SetUpBusObjectTestService()
    {
        /* create/start/connect alljoyn_busattachment */
        servicebus = alljoyn_busattachment_create("ProxyBusObjectTestservice", false);
        status = alljoyn_busattachment_start(servicebus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(servicebus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_interfacedescription testIntf = NULL;
        status = alljoyn_busattachment_createinterface(servicebus, INTERFACE_NAME, &testIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(testIntf != NULL);
        status = alljoyn_interfacedescription_addproperty(testIntf, "prop1", "s", ALLJOYN_PROP_ACCESS_READ);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_interfacedescription_addproperty(testIntf, "prop2", "i", ALLJOYN_PROP_ACCESS_WRITE);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_interfacedescription_addproperty(testIntf, "prop3", "u", ALLJOYN_PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        alljoyn_interfacedescription_activate(testIntf);
        /* Initialize properties to a known value*/
        prop2 = -32;
        prop3 =  42;
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
            &get_property,
            &set_property,
            &busobject_registered,
            &busobject_unregistered
        };
        testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);

        status = alljoyn_busobject_addinterface(testObj, testIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = alljoyn_busattachment_registerbusobject(servicebus, testObj);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        for (size_t i = 0; i < 200; ++i) {
            if (object_registered_flag) {
                break;
            }
            qcc::Sleep(5);
        }
        EXPECT_TRUE(object_registered_flag);

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

    void TearDownBusObjectTestService()
    {
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

TEST_F(BusObjectTest, object_registered_unregistered)
{
    /* Set up bus object */
    alljoyn_busobject_callbacks busObjCbs = {
        &get_property,
        &set_property,
        &busobject_registered,
        &busobject_unregistered
    };
    alljoyn_busobject testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
    status = alljoyn_busattachment_registerbusobject(bus, testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (object_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(object_registered_flag);

    alljoyn_busattachment_unregisterbusobject(bus, testObj);
    for (size_t i = 0; i < 200; ++i) {
        if (object_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(object_unregistered_flag);

    alljoyn_busobject_destroy(testObj);
    alljoyn_busattachment_stop(bus);
    alljoyn_busattachment_join(bus);
}

TEST_F(BusObjectTest, get_property_handler)
{
    SetUpBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg value = alljoyn_msgarg_create();
    status = alljoyn_proxybusobject_getproperty(proxyObj, INTERFACE_NAME, "prop1", value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const char*str;
    status = alljoyn_msgarg_get(value, "s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(prop1, str);
    alljoyn_msgarg_destroy(value);

    /* should fail to read a write only property*/
    value = alljoyn_msgarg_create();
    status = alljoyn_proxybusobject_getproperty(proxyObj, INTERFACE_NAME, "prop2", value);
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(value);

    value = alljoyn_msgarg_create();
    status = alljoyn_proxybusobject_getproperty(proxyObj, INTERFACE_NAME, "prop3", value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    uint32_t return_value;
    status = alljoyn_msgarg_get(value, "u", &return_value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((uint32_t)42, return_value);
    alljoyn_msgarg_destroy(value);
    alljoyn_proxybusobject_destroy(proxyObj);
    TearDownBusObjectTestService();
}

TEST_F(BusObjectTest, set_property_handler)
{
    SetUpBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* should fail to write a read only property*/
    alljoyn_msgarg value = alljoyn_msgarg_create_and_set("s", "This should not work.");
    status = alljoyn_proxybusobject_setproperty(proxyObj, INTERFACE_NAME, "prop1", value);
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(value);

    value = alljoyn_msgarg_create_and_set("i", -888);
    status = alljoyn_proxybusobject_setproperty(proxyObj, INTERFACE_NAME, "prop2", value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(-888, prop2);
    alljoyn_msgarg_destroy(value);

    value = alljoyn_msgarg_create_and_set("u", 98);
    status = alljoyn_proxybusobject_setproperty(proxyObj, INTERFACE_NAME, "prop3", value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((uint32_t)98, prop3);
    alljoyn_msgarg_destroy(value);

    alljoyn_proxybusobject_destroy(proxyObj);
    TearDownBusObjectTestService();
}

TEST_F(BusObjectTest, getall_properties)
{
    SetUpBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg value = alljoyn_msgarg_create();
    status = alljoyn_proxybusobject_getallproperties(proxyObj, INTERFACE_NAME, value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg variant_arg;
    const char* str;
    status = alljoyn_msgarg_getdictelement(value, "{sv}", "prop1", &variant_arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(variant_arg, "s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(prop1, str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint32_t num;
    status = alljoyn_msgarg_getdictelement(value, "{sv}", "prop3", &variant_arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(variant_arg, "u", &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((uint32_t)42, num);
    alljoyn_msgarg_destroy(value);
    alljoyn_proxybusobject_destroy(proxyObj);
    TearDownBusObjectTestService();
}

TEST_F(BusObjectTest, property_changed_signal)
{
    /* create/start/connect alljoyn_busattachment */
    servicebus = alljoyn_busattachment_create("ProxyBusObjectTestservice", false);
    status = alljoyn_busattachment_start(servicebus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(servicebus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(servicebus, INTERFACE_NAME, &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop2", "i", ALLJOYN_PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addpropertyannotation(testIntf, "prop2", "org.freedesktop.DBus.Property.EmitsChangedSignal", "invalidates");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addproperty(testIntf, "prop3", "u", ALLJOYN_PROP_ACCESS_RW);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addpropertyannotation(testIntf, "prop3", "org.freedesktop.DBus.Property.EmitsChangedSignal", "true");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(testIntf);
    /* Initialize properties to a known value*/
    prop2 = -32;
    prop3 =  42;
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
        &get_property,
        &set_property,
        &busobject_registered,
        &busobject_unregistered
    };
    testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);

    status = alljoyn_busobject_addinterface(testObj, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_registerbusobject(servicebus, testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (object_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(object_registered_flag);

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

    prop_changed_flag = QCC_FALSE;

    const char* props[] = { "prop2", "prop3" };
    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_proxybusobject_registerpropertieschangedlistener(proxyObj, INTERFACE_NAME, props, sizeof(props) / sizeof(props[0]), obj_prop_changed, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg value;
    prop_changed_flag = QCC_FALSE;
    value = alljoyn_msgarg_create_and_set("i", -888);
    status = alljoyn_proxybusobject_setproperty(proxyObj, INTERFACE_NAME, "prop2", value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (prop_changed_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_FALSE(prop_changed_flag);
    EXPECT_EQ(-888, prop2);
    alljoyn_msgarg_destroy(value);

    prop_changed_flag = QCC_FALSE;
    value = alljoyn_msgarg_create_and_set("u", 98);
    status = alljoyn_proxybusobject_setproperty(proxyObj, INTERFACE_NAME, "prop3", value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (prop_changed_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_EQ((uint32_t)98, prop3);
    EXPECT_FALSE(prop_changed_flag);
    alljoyn_msgarg_destroy(value);

    prop_changed_flag = QCC_FALSE;
    const char* propNames[] = { "prop2", "prop3" };
    size_t numProps = sizeof(propNames) / sizeof(propNames[0]);
    alljoyn_busobject_emitpropertieschanged(testObj, INTERFACE_NAME, propNames, numProps, 0);
    for (size_t i = 0; i < 200; ++i) {
        if (prop_changed_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(prop_changed_flag);

    alljoyn_proxybusobject_destroy(proxyObj);
    TearDownBusObjectTestService();
}

TEST_F(BusObjectTest, addmethodhandler)
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
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmethod(testIntf, "ping", "s", "s", "in,out", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmethod(testIntf, "chirp", "s", "", "chirp", 0, 0);
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
    alljoyn_busobject testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
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

    status = alljoyn_busobject_addmethodhandler(testObj, ping_member, &ping_method, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busobject_addmethodhandler(testObj, chirp_member, &chirp_method, NULL);
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

    /* call methods */
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

    chirp_method_flag = QCC_FALSE;

    status = alljoyn_proxybusobject_methodcall(proxyObj, INTERFACE_NAME, "chirp", input, 1, reply, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
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

    /* cleanup */
    alljoyn_busattachment_destroy(servicebus);
    alljoyn_buslistener_destroy(buslistener);
    alljoyn_busobject_destroy(testObj);
}

TEST_F(BusObjectTest, addmethodhandlers)
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
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmethod(testIntf, "ping", "s", "s", "in,out", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmethod(testIntf, "chirp", "s", "", "chirp", 0, 0);
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
    alljoyn_busobject testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
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

    /* call methods */
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

    chirp_method_flag = QCC_FALSE;

    status = alljoyn_proxybusobject_methodcall(proxyObj, INTERFACE_NAME, "chirp", input, 1, reply, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
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

    /* cleanup */
    alljoyn_busattachment_destroy(servicebus);
    alljoyn_buslistener_destroy(buslistener);
    alljoyn_busobject_destroy(testObj);
}

TEST_F(BusObjectTest, addmethodhandler_addmethodhandlers_mix)
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
    ASSERT_TRUE(testIntf != NULL);
    status = alljoyn_interfacedescription_addmethod(testIntf, "ping", "s", "s", "in,out", 0, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_interfacedescription_addmethod(testIntf, "chirp", "s", "", "chirp", 0, 0);
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
    alljoyn_busobject testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
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
    };
    status = alljoyn_busobject_addmethodhandlers(testObj, methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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

    /* call methods */
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

    chirp_method_flag = QCC_FALSE;

    status = alljoyn_proxybusobject_methodcall(proxyObj, INTERFACE_NAME, "chirp", input, 1, reply, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
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

    /* cleanup */
    alljoyn_busattachment_destroy(servicebus);
    alljoyn_buslistener_destroy(buslistener);
    alljoyn_busobject_destroy(testObj);
}

static QCC_BOOL getpropertycb_flag = QCC_FALSE;

static void AJ_CALL getpropertycb_prop1(QStatus status, alljoyn_proxybusobject obj, const alljoyn_msgarg value, void* context) {
    const char*str;
    status = alljoyn_msgarg_get(value, "s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(prop1, str);
    EXPECT_STREQ("AllJoyn Test String.", (char*)context);
    getpropertycb_flag = QCC_TRUE;
}

static void AJ_CALL getpropertycb_prop2(QStatus status, alljoyn_proxybusobject obj, const alljoyn_msgarg value, void* context) {
    EXPECT_EQ(ER_BUS_PROPERTY_ACCESS_DENIED, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("AllJoyn Test String.", (char*)context);
    getpropertycb_flag = QCC_TRUE;
}

static void AJ_CALL getpropertycb_prop3(QStatus status, alljoyn_proxybusobject obj, const alljoyn_msgarg value, void* context) {
    uint32_t return_value;
    status = alljoyn_msgarg_get(value, "u", &return_value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((uint32_t)42, return_value);
    EXPECT_STREQ("AllJoyn Test String.", (char*)context);
    getpropertycb_flag = QCC_TRUE;
}
TEST_F(BusObjectTest, get_propertyasync_handler)
{
    SetUpBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    getpropertycb_flag = QCC_FALSE; //make sure the flag is in a known state.
    status = alljoyn_proxybusobject_getpropertyasync(proxyObj, INTERFACE_NAME, "prop1", &getpropertycb_prop1, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, (void*)"AllJoyn Test String.");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 2 seconds for the getproperty call to complete.
    for (int i = 0; i < 200; ++i) {

        if (getpropertycb_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(getpropertycb_flag);
    /* should fail to read a write only property*/
    getpropertycb_flag = QCC_FALSE; //make sure the flag is in a known state.
    status = alljoyn_proxybusobject_getpropertyasync(proxyObj, INTERFACE_NAME, "prop2", &getpropertycb_prop2, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, (void*)"AllJoyn Test String.");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 2 seconds for the getproperty call to complete.
    for (int i = 0; i < 200; ++i) {

        if (getpropertycb_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(getpropertycb_flag);

    getpropertycb_flag = QCC_FALSE; //make sure the flag is in a known state.
    status = alljoyn_proxybusobject_getpropertyasync(proxyObj, INTERFACE_NAME, "prop3", &getpropertycb_prop3, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, (void*)"AllJoyn Test String.");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 2 seconds for the getproperty call to complete.
    for (int i = 0; i < 200; ++i) {

        if (getpropertycb_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(getpropertycb_flag);

    alljoyn_proxybusobject_destroy(proxyObj);
    TearDownBusObjectTestService();
}

static QCC_BOOL setpropertycb_flag = QCC_FALSE;

static void AJ_CALL setpropertycb_prop1(QStatus status, alljoyn_proxybusobject obj, void* context)
{
    EXPECT_EQ(ER_BUS_PROPERTY_ACCESS_DENIED, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("AllJoyn Test String.", (char*)context);
    setpropertycb_flag = QCC_TRUE;
}

static void AJ_CALL setpropertycb_prop2(QStatus status, alljoyn_proxybusobject obj, void* context)
{
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("AllJoyn Test String.", (char*)context);
    setpropertycb_flag = QCC_TRUE;
}

TEST_F(BusObjectTest, set_propertyasync_handler)
{
    SetUpBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    setpropertycb_flag = QCC_FALSE;
    /* should fail to write a read only property*/
    alljoyn_msgarg value = alljoyn_msgarg_create_and_set("s", "This should not work.");
    status = alljoyn_proxybusobject_setpropertyasync(proxyObj, INTERFACE_NAME, "prop1", value, &setpropertycb_prop1, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, (void*)"AllJoyn Test String.");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 2 seconds for the getproperty call to complete.
    for (int i = 0; i < 200; ++i) {

        if (setpropertycb_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(setpropertycb_flag);

    alljoyn_msgarg_destroy(value);

    setpropertycb_flag = QCC_FALSE;
    value = alljoyn_msgarg_create_and_set("i", -888);
    status = alljoyn_proxybusobject_setpropertyasync(proxyObj, INTERFACE_NAME, "prop2", value, &setpropertycb_prop2, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, (void*)"AllJoyn Test String.");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 2 seconds for the getproperty call to complete.
    for (int i = 0; i < 200; ++i) {

        if (setpropertycb_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(setpropertycb_flag);

    EXPECT_EQ(-888, prop2);
    alljoyn_msgarg_destroy(value);

    setpropertycb_flag = QCC_FALSE;
    value = alljoyn_msgarg_create_and_set("u", 98);
    //reusing the setpropertycb_prop2 we expect this callback to have the exact same results.
    status = alljoyn_proxybusobject_setpropertyasync(proxyObj, INTERFACE_NAME, "prop3", value, &setpropertycb_prop2, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, (void*)"AllJoyn Test String.");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 2 seconds for the getproperty call to complete.
    for (int i = 0; i < 200; ++i) {

        if (setpropertycb_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(setpropertycb_flag);

    EXPECT_EQ((uint32_t)98, prop3);
    alljoyn_msgarg_destroy(value);

    alljoyn_proxybusobject_destroy(proxyObj);
    TearDownBusObjectTestService();
}

static QCC_BOOL getallpropertiescb_flag = QCC_FALSE;

static void AJ_CALL getallpropertiescb(QStatus status, alljoyn_proxybusobject obj, const alljoyn_msgarg values, void* context)
{
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg variant_arg;
    const char* str;
    status = alljoyn_msgarg_getdictelement(values, "{sv}", "prop1", &variant_arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(variant_arg, "s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(prop1, str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint32_t num;
    status = alljoyn_msgarg_getdictelement(values, "{sv}", "prop3", &variant_arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(variant_arg, "u", &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((uint32_t)42, num);
    EXPECT_STREQ("AllJoyn Test String.", (char*)context);

    getallpropertiescb_flag = QCC_TRUE;
}

TEST_F(BusObjectTest, getallpropertiesasync)
{
    SetUpBusObjectTestService();

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(bus, OBJECT_NAME, OBJECT_PATH, 0);
    EXPECT_TRUE(proxyObj);
    status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //alljoyn_msgarg value = alljoyn_msgarg_create();
    getallpropertiescb_flag = QCC_FALSE;
    status = alljoyn_proxybusobject_getallpropertiesasync(proxyObj, INTERFACE_NAME, &getallpropertiescb, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, (void*)"AllJoyn Test String.");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait upto 2 seconds for the getproperty call to complete.
    for (int i = 0; i < 200; ++i) {

        if (getallpropertiescb_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(getallpropertiescb_flag);

    alljoyn_proxybusobject_destroy(proxyObj);
    TearDownBusObjectTestService();
}

TEST_F(BusObjectTest, getbusattachment)
{
    SetUpBusObjectTestService();
    /* the testObj was registered with the servicebus. */
    alljoyn_busattachment returnbus = alljoyn_busobject_getbusattachment(testObj);
    EXPECT_STREQ(alljoyn_busattachment_getglobalguidstring(servicebus), alljoyn_busattachment_getglobalguidstring(returnbus));

    TearDownBusObjectTestService();
}
