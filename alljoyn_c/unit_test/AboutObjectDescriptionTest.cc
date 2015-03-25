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

#include <alljoyn_c/AboutData.h>
#include <alljoyn_c/AboutListener.h>
#include <alljoyn_c/AboutObj.h>
#include <alljoyn_c/AboutIcon.h>
#include <alljoyn_c/AboutIconObj.h>
#include <alljoyn_c/AboutObjectDescription.h>
#include <alljoyn_c/AboutProxy.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/MsgArg.h>
#include <alljoyn_c/AjAPI.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/MsgArg.h>
#include <BusInternal.h>
#include "ajTestCommon.h"
#include "alljoyn/DBusStd.h"
#include <qcc/Thread.h>
#include <qcc/String.h>
#include <qcc/GUID.h>
#include <stdlib.h>

using namespace ajn;
using namespace qcc;

static alljoyn_busobject my_alljoyn_busobject_create(alljoyn_busattachment bus, const char* path) {
    alljoyn_busobject result = alljoyn_busobject_create(path, 0, NULL, NULL);
    QStatus status = ER_FAIL;
    const alljoyn_interfacedescription test_iface =
        alljoyn_busattachment_getinterface(bus, "org.alljoyn.test");
    EXPECT_TRUE(test_iface != NULL) << "NULL InterfaceDescription* for org.alljoyn.test";
    if (test_iface == NULL) {
        printf("The interfaceDescription pointer for org.alljoyn.test was NULL \
                when it should not have been.\n");
        return NULL;
    }
    status = alljoyn_busobject_addinterface_announced(result, test_iface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const alljoyn_interfacedescription game_iface =
        alljoyn_busattachment_getinterface(bus, "org.alljoyn.game");
    EXPECT_TRUE(game_iface != NULL) << "NULL InterfaceDescription* for org.alljoyn.game";
    if (game_iface == NULL) {
        printf("The interfaceDescription pointer for org.alljoyn.game was NULL \
                when it should not have been.\n");
        return NULL;
    }
    status = alljoyn_busobject_addinterface_announced(result, game_iface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    const alljoyn_interfacedescription mediaplayer_iface =
        alljoyn_busattachment_getinterface(bus, "org.alljoyn.mediaplayer");
    EXPECT_TRUE(mediaplayer_iface != NULL)
        << "NULL InterfaceDescription* for org.alljoyn.mediaplayer";
    if (mediaplayer_iface == NULL) {
        printf("The interfaceDescription pointer for org.alljoyn.mediaplayer was NULL \
                when it should not have been.\n");
        return NULL;
    }
    status = alljoyn_busobject_addinterface_announced(result, mediaplayer_iface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    return result;
}

static alljoyn_busobject my_alljoyn_busobject_create_1(alljoyn_busattachment bus, const char* path)
{
    alljoyn_busobject result = alljoyn_busobject_create(path, 0, NULL, NULL);
    QStatus status = ER_FAIL;
    const alljoyn_interfacedescription iface =
        alljoyn_busattachment_getinterface(bus, "test.about.objectdescription.interface1");
    EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for org.alljoyn.test";
    if (iface == NULL) {
        printf("The interfaceDescription pointer for org.alljoyn.test was NULL \
                when it should not have been.\n");
        return NULL;
    }
    status = alljoyn_busobject_addinterface_announced(result, iface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    return result;
}

static alljoyn_busobject my_alljoyn_busobject_create_2(alljoyn_busattachment bus, const char* path) {

    alljoyn_busobject result = alljoyn_busobject_create(path, 0, NULL, NULL);
    QStatus status = ER_FAIL;
    const alljoyn_interfacedescription iface =
        alljoyn_busattachment_getinterface(bus, "test.about.objectdescription.interface2");
    EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for org.alljoyn.test";
    if (iface == NULL) {
        printf("The interfaceDescription pointer for org.alljoyn.test was NULL \
                when it should not have been.\n");
        return NULL;
    }
    status = alljoyn_busobject_addinterface_announced(result, iface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    return result;
}

TEST(AboutObjectDescriptionTest, Construct) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus =
        alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);
    alljoyn_abouticon aicon = alljoyn_abouticon_create();
    status = alljoyn_abouticon_seturl(aicon, "image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticonobj aboutIconObj = alljoyn_abouticonobj_create(bus, aicon);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='org.alljoyn.test'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.game'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.mediaplayer'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject = my_alljoyn_busobject_create(bus, "/org/alljoyn/test");
    status = alljoyn_busattachment_registerbusobject(bus, busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg arg = alljoyn_msgarg_create();
    ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)arg));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(arg);
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/About/DeviceIcon", "org.alljoyn.Icon"));

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.Icon"));

    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/About/DeviceIcon", "org.alljoyn.test"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/About/DeviceIcon", "org.alljoyn.game"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/About/DeviceIcon", "org.alljoyn.mediaplayer"));

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.Icon"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.test"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.game"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.mediaplayer"));

    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.IAmNotReal"));

    alljoyn_abouticonobj_destroy(aboutIconObj);
    alljoyn_busobject_destroy(busObject);
    alljoyn_msgarg_destroy(arg);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, GetMsgArg) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus =
        alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);
    alljoyn_abouticon aicon = alljoyn_abouticon_create();
    status = alljoyn_abouticon_seturl(aicon, "image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticonobj aboutIconObj = alljoyn_abouticonobj_create(bus, aicon);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='org.alljoyn.test'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.game'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.mediaplayer'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject = my_alljoyn_busobject_create(bus, "/org/alljoyn/test");
    status = alljoyn_busattachment_registerbusobject(bus, busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg argObj = alljoyn_msgarg_create();
    ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)argObj));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(argObj);

    alljoyn_msgarg arg = alljoyn_msgarg_create();
    alljoyn_aboutobjectdescription_getmsgarg(aod, arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg structarg;
    size_t s_size;
    status = alljoyn_msgarg_get(arg, "a(oas)", &s_size, &structarg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(2u, s_size);

    if (s_size > 0) {
        char** object_path = (char**) malloc(sizeof(char*) * s_size);
        size_t* number_itfs = (size_t*) malloc(sizeof(size_t) * s_size);
        alljoyn_msgarg* interfaces_arg =
            (alljoyn_msgarg*) malloc(sizeof(alljoyn_msgarg) * s_size);
        for (size_t i = 0; i < s_size; ++i) {
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(structarg, i),
                                        "(oas)", &(object_path[i]),
                                        &(number_itfs[i]),
                                        &(interfaces_arg[i]));
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }

        EXPECT_STREQ("/About/DeviceIcon", object_path[0]);
        ASSERT_EQ(1u, number_itfs[0]);
        char* intf;
        alljoyn_msgarg_get(alljoyn_msgarg_array_element(interfaces_arg[0], 0), "s", &intf);
        EXPECT_STREQ("org.alljoyn.Icon", intf);
        EXPECT_STREQ("/org/alljoyn/test", object_path[1]);
        ASSERT_EQ(3u, number_itfs[1]);

        /*
         * This test makes some assumptions about order that may not always be true
         * if we see failures that is a result of right values in the wrong order
         * then this test should be modified to account for that.
         */
        alljoyn_msgarg_get(alljoyn_msgarg_array_element(interfaces_arg[1], 0), "s", &intf);
        EXPECT_STREQ("org.alljoyn.game", intf);
        alljoyn_msgarg_get(alljoyn_msgarg_array_element(interfaces_arg[1], 1), "s", &intf);
        EXPECT_STREQ("org.alljoyn.mediaplayer", intf);
        alljoyn_msgarg_get(alljoyn_msgarg_array_element(interfaces_arg[1], 2), "s", &intf);
        EXPECT_STREQ("org.alljoyn.test", intf);

        free(object_path);
        free(number_itfs);
        free(interfaces_arg);
    }
    alljoyn_abouticon_destroy(aicon);
    alljoyn_abouticonobj_destroy(aboutIconObj);
    alljoyn_busobject_destroy(busObject);
    alljoyn_msgarg_destroy(arg);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, GetPaths) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus =
        alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);
    alljoyn_abouticon aicon = alljoyn_abouticon_create();
    status = alljoyn_abouticon_seturl(aicon, "image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticonobj aboutIconObj = alljoyn_abouticonobj_create(bus, aicon);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='org.alljoyn.test'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.game'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.mediaplayer'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject = my_alljoyn_busobject_create(bus, "/org/alljoyn/test");
    status = alljoyn_busattachment_registerbusobject(bus, busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg argObj = alljoyn_msgarg_create();
    ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)argObj));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(argObj);

    size_t numPaths = alljoyn_aboutobjectdescription_getpaths(aod, NULL, 0);
    ASSERT_EQ(2u, numPaths);
    const char** paths = (const char**) malloc(sizeof(const char*) * numPaths);
    alljoyn_aboutobjectdescription_getpaths(aod, paths, numPaths);

    EXPECT_TRUE(strcmp(paths[0], "/About/DeviceIcon") == 0
                || strcmp(paths[0], "/org/alljoyn/test") == 0) << paths[0];
    EXPECT_TRUE(strcmp(paths[1], "/About/DeviceIcon") == 0
                || strcmp(paths[1], "/org/alljoyn/test") == 0) << paths[1];

    free(paths);
    alljoyn_abouticon_destroy(aicon);
    alljoyn_abouticonobj_destroy(aboutIconObj);
    alljoyn_busobject_destroy(busObject);
    alljoyn_msgarg_destroy(argObj);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, GetInterfaces) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus =
        alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);
    alljoyn_abouticon aicon = alljoyn_abouticon_create();
    status = alljoyn_abouticon_seturl(aicon, "image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticonobj aboutIconObj = alljoyn_abouticonobj_create(bus, aicon);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='org.alljoyn.test'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.game'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.mediaplayer'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject = my_alljoyn_busobject_create(bus, "/org/alljoyn/test");
    status = alljoyn_busattachment_registerbusobject(bus, busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg argObj = alljoyn_msgarg_create();
    ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)argObj));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(argObj);

    size_t numPaths = alljoyn_aboutobjectdescription_getpaths(aod, NULL, 0);
    ASSERT_EQ(2u, numPaths);
    size_t numInterfaces =
        alljoyn_aboutobjectdescription_getinterfaces(aod,
                                                     "/About/DeviceIcon",
                                                     NULL, 0);
    ASSERT_EQ(1u, numInterfaces);
    const char** interfaces =
        (const char**) malloc(sizeof(const char*) * numInterfaces);
    alljoyn_aboutobjectdescription_getinterfaces(aod, "/About/DeviceIcon",
                                                 interfaces, numInterfaces);
    EXPECT_STREQ("org.alljoyn.Icon", interfaces[0]);
    free(interfaces);

    numInterfaces =
        alljoyn_aboutobjectdescription_getinterfaces(aod,
                                                     "/org/alljoyn/test",
                                                     NULL, 0);
    ASSERT_EQ(3u, numInterfaces);
    interfaces = (const char**) malloc(sizeof(const char*) * numInterfaces);
    alljoyn_aboutobjectdescription_getinterfaces(aod, "/org/alljoyn/test",
                                                 interfaces, numInterfaces);

    EXPECT_TRUE(strcmp(interfaces[0], "org.alljoyn.test") == 0 ||
                strcmp(interfaces[0], "org.alljoyn.game") == 0 ||
                strcmp(interfaces[0], "org.alljoyn.mediaplayer") == 0)
        << interfaces[0];
    EXPECT_TRUE(strcmp(interfaces[1], "org.alljoyn.test") == 0 ||
                strcmp(interfaces[1], "org.alljoyn.game") == 0 ||
                strcmp(interfaces[1], "org.alljoyn.mediaplayer") == 0)
        << interfaces[1];
    EXPECT_TRUE(strcmp(interfaces[2], "org.alljoyn.test") == 0 ||
                strcmp(interfaces[2], "org.alljoyn.game") == 0 ||
                strcmp(interfaces[2], "org.alljoyn.mediaplayer") == 0)
        << interfaces[2];
    free(interfaces);
    alljoyn_abouticon_destroy(aicon);
    alljoyn_abouticonobj_destroy(aboutIconObj);
    alljoyn_busobject_destroy(busObject);
    alljoyn_msgarg_destroy(argObj);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, Clear) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus =
        alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);
    alljoyn_abouticon aicon = alljoyn_abouticon_create();
    status = alljoyn_abouticon_seturl(aicon, "image/png", "http://www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_abouticonobj aboutIconObj = alljoyn_abouticonobj_create(bus, aicon);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='org.alljoyn.test'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.game'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='org.alljoyn.mediaplayer'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject = my_alljoyn_busobject_create(bus, "/org/alljoyn/test");
    status = alljoyn_busattachment_registerbusobject(bus, busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg argObj = alljoyn_msgarg_create();
    ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)argObj));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(argObj);
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/About/DeviceIcon", "org.alljoyn.Icon"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    alljoyn_aboutobjectdescription_clear(aod);
    EXPECT_FALSE(alljoyn_aboutobjectdescription_haspath(aod, "/About/DeviceIcon"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_haspath(aod, "/org/alljoyn/test"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/About/DeviceIcon", "org.alljoyn.Icon"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    alljoyn_abouticon_destroy(aicon);
    alljoyn_abouticonobj_destroy(aboutIconObj);
    alljoyn_busobject_destroy(busObject);
    alljoyn_msgarg_destroy(argObj);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, PopulateAutomaticallyFromBusObject) {
    QStatus status = ER_FAIL;
    static const char* interface = "<interface name='test.about.objectdescription.interface1'>"
                                   "  <method name='Ping'>"
                                   "    <arg name='out_arg' type='s' direction='in' />"
                                   "    <arg name='return_arg' type='s' direction='out' />"
                                   "  </method>"
                                   "  <signal name='Chirp'>"
                                   "    <arg name='sound' type='s' />"
                                   "  </signal>"
                                   "  <property name='volume' type='i' access='readwrite'/>"
                                   "</interface>";

    alljoyn_busattachment bus = alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);

    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject1 = my_alljoyn_busobject_create_1(bus, "/test/path1");

    status = alljoyn_busattachment_registerbusobject(bus, busObject1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg aodArg = alljoyn_msgarg_create();
    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)aodArg));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(aodArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "test.about.objectdescription.interface1"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(aod, "/test/path1"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/test/path1", "test.about.objectdescription.interface1"));

    alljoyn_msgarg_destroy(aodArg);
    alljoyn_aboutobjectdescription_destroy(aod);
    alljoyn_busobject_destroy(busObject1);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, PopulateAutomaticallyFromMultipleBusObjects) {
    QStatus status = ER_FAIL;
    static const char* interface1 = "<interface name='test.about.objectdescription.interface1'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    static const char* interface2 = "<interface name='test.about.objectdescription.interface2'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";

    alljoyn_busattachment bus = alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject1 = my_alljoyn_busobject_create_1(bus, "/test/path1");
    alljoyn_busobject busObject2 = my_alljoyn_busobject_create_2(bus, "/test/path2");

    status = alljoyn_busattachment_registerbusobject(bus, busObject1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_registerbusobject(bus, busObject2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg aodArg = alljoyn_msgarg_create();
    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)aodArg));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(aodArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "test.about.objectdescription.interface1"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(aod, "/test/path1"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/test/path1", "test.about.objectdescription.interface1"));

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "test.about.objectdescription.interface2"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(aod, "/test/path2"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/test/path2", "test.about.objectdescription.interface2"));

    alljoyn_busobject_destroy(busObject1);
    alljoyn_busobject_destroy(busObject2);
    alljoyn_msgarg_destroy(aodArg);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, PopulateAutomaticallyRemoveBusObject) {
    QStatus status = ER_FAIL;
    static const char* interface1 = "<interface name='test.about.objectdescription.interface1'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    static const char* interface2 = "<interface name='test.about.objectdescription.interface2'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    alljoyn_busattachment bus = alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busobject busObject1 = my_alljoyn_busobject_create_1(bus, "/test/path1");
    alljoyn_busobject busObject2 = my_alljoyn_busobject_create_2(bus, "/test/path2");
    status = alljoyn_busattachment_registerbusobject(bus, busObject1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_registerbusobject(bus, busObject2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg aodArg = alljoyn_msgarg_create();
    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)aodArg));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(aodArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "test.about.objectdescription.interface1"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(aod, "/test/path1"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/test/path1", "test.about.objectdescription.interface1"));

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "test.about.objectdescription.interface2"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(aod, "/test/path2"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/test/path2", "test.about.objectdescription.interface2"));

    alljoyn_busattachment_unregisterbusobject(bus, busObject1);

    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)aodArg));
    alljoyn_aboutobjectdescription_clear(aod);

    alljoyn_aboutobjectdescription_createfrommsgarg(aod, aodArg);
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod, "test.about.objectdescription.interface1"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_haspath(aod, "/test/path1"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/test/path1", "test.about.objectdescription.interface1"));

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "test.about.objectdescription.interface2"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(aod, "/test/path2"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/test/path2", "test.about.objectdescription.interface2"));

    alljoyn_busobject_destroy(busObject1);
    alljoyn_busobject_destroy(busObject2);
    alljoyn_msgarg_destroy(aodArg);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, GetInterfacePaths) {
    QStatus status = ER_FAIL;
    static const char* interface1 = "<interface name='test.about.objectdescription.interface1'>"
                                    "  <method name='Ping'>"
                                    "    <arg name='out_arg' type='s' direction='in' />"
                                    "    <arg name='return_arg' type='s' direction='out' />"
                                    "  </method>"
                                    "</interface>";
    alljoyn_busattachment bus = alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject1 = my_alljoyn_busobject_create_1(bus, "/test/path1");
    alljoyn_busobject busObject2 = my_alljoyn_busobject_create_1(bus, "/test/path2");
    alljoyn_busobject busObject3 = my_alljoyn_busobject_create_1(bus, "/test/path3");
    alljoyn_busobject busObject4 = my_alljoyn_busobject_create_1(bus, "/test/path4");
    alljoyn_busobject busObject5 = my_alljoyn_busobject_create_1(bus, "/test/path5");
    alljoyn_busobject busObject6 = my_alljoyn_busobject_create_1(bus, "/test/path6");

    status = alljoyn_busattachment_registerbusobject(bus, busObject1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg aodArg = alljoyn_msgarg_create();
    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)aodArg));
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(aodArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    size_t pathNum = alljoyn_aboutobjectdescription_getinterfacepaths(aod, "test.about.objectdescription.interface1", NULL, 0);
    EXPECT_EQ((size_t)1, pathNum);
    const char** paths = (const char**) malloc(sizeof(const char*) * pathNum);
    alljoyn_aboutobjectdescription_getinterfacepaths(aod,
                                                     "test.about.objectdescription.interface1",
                                                     paths, pathNum);
    EXPECT_STREQ("/test/path1", paths[0]);
    free(paths);
    paths = NULL;

    status = alljoyn_busattachment_registerbusobject(bus, busObject2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_registerbusobject(bus, busObject3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_registerbusobject(bus, busObject4);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_registerbusobject(bus, busObject5);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_registerbusobject(bus, busObject6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)aodArg));
    alljoyn_aboutobjectdescription_createfrommsgarg(aod, aodArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    pathNum = alljoyn_aboutobjectdescription_getinterfacepaths(aod,
                                                               "test.about.objectdescription.interface1",
                                                               NULL, 0);
    EXPECT_EQ((size_t)6, pathNum);
    paths = (const char**) malloc(sizeof(const char*) * pathNum);
    alljoyn_aboutobjectdescription_getinterfacepaths(aod,
                                                     "test.about.objectdescription.interface1",
                                                     paths, pathNum);

    EXPECT_STREQ("/test/path1", paths[0]);
    EXPECT_STREQ("/test/path2", paths[1]);
    EXPECT_STREQ("/test/path3", paths[2]);
    EXPECT_STREQ("/test/path4", paths[3]);
    EXPECT_STREQ("/test/path5", paths[4]);
    EXPECT_STREQ("/test/path6", paths[5]);
    free(paths);
    paths = NULL;

    alljoyn_busobject_destroy(busObject1);
    alljoyn_busobject_destroy(busObject2);
    alljoyn_busobject_destroy(busObject3);
    alljoyn_busobject_destroy(busObject4);
    alljoyn_busobject_destroy(busObject5);
    alljoyn_busobject_destroy(busObject6);
    alljoyn_msgarg_destroy(aodArg);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, Empty_Negative)
{
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus =
        alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);

    alljoyn_msgarg arg = alljoyn_msgarg_create();
    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)arg));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(arg);

    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.Icon"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.About"));

    const qcc::String interface = "<node>"
                                  "<interface name='org.alljoyn.test'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "<interface name='org.alljoyn.game'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "<interface name='org.alljoyn.mediaplayer'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";

    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)arg));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobjectdescription aod1 = alljoyn_aboutobjectdescription_create_full(arg);
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod1, "org.alljoyn.Icon"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod1, "org.alljoyn.About"));

    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod1, "/org/alljoyn/test", "org.alljoyn.test"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod1, "/org/alljoyn/test", "org.alljoyn.game"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod1, "/org/alljoyn/test", "org.alljoyn.mediaplayer"));

    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod1, "org.alljoyn.test"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod1, "org.alljoyn.game"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod1, "org.alljoyn.mediaplayer"));

    alljoyn_msgarg_destroy(arg);
    alljoyn_aboutobjectdescription_destroy(aod);
    alljoyn_aboutobjectdescription_destroy(aod1);
    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, AboutInterface) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus =
        alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(bus, ANNOUNCED);
    alljoyn_msgarg arg = alljoyn_msgarg_create();
    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)arg));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(arg);

    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/About", "org.alljoyn.About"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.About"));

    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_msgarg_destroy(arg);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(AboutObjectDescriptionTest, NoAboutInterface) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus =
        alljoyn_busattachment_create("AlljoynObjectDescriptionTest", QCC_TRUE);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(bus, UNANNOUNCED);
    alljoyn_msgarg arg = alljoyn_msgarg_create();
    status = ((BusAttachment*)bus)->GetInternal().GetAnnouncedObjectDescription(*((MsgArg*)arg));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create_full(arg);
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterfaceatpath(aod, "/About", "org.alljoyn.About"));
    EXPECT_FALSE(alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.About"));

    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_msgarg_destroy(arg);
    alljoyn_aboutobjectdescription_destroy(aod);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}
