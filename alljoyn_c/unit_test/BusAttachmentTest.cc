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
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/InterfaceDescription.h>
#include <alljoyn_c/ProxyBusObject.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include "ajTestCommon.h"

TEST(BusAttachmentTest, createinterface) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_FALSE);
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.BusAttachment", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, deleteinterface) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_FALSE);
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.BusAttachment", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_deleteinterface(bus, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, start_stop_join) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_FALSE);
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, isstarted_isstopping) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_FALSE);
    EXPECT_EQ(QCC_FALSE, alljoyn_busattachment_isstarted(bus));
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(QCC_TRUE, alljoyn_busattachment_isstarted(bus));
    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /*
     * Assumption made that the isstopping function will be called before all of
     * the BusAttachement threads have completed so it will return QCC_TRUE it is
     * possible, but unlikely, that this could return QCC_FALSE.
     */

    EXPECT_EQ(QCC_TRUE, alljoyn_busattachment_isstopping(bus));
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(QCC_FALSE, alljoyn_busattachment_isstarted(bus));
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getconcurrency) {
    alljoyn_busattachment bus = NULL;
    unsigned int concurrency = -1;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_TRUE);

    concurrency = alljoyn_busattachment_getconcurrency(bus);
    //The default value for getconcurrency is 4
    EXPECT_EQ(4u, concurrency) << "  Expected a concurrency of 4 got " << concurrency;

    alljoyn_busattachment_destroy(bus);

    bus = NULL;
    concurrency = -1;

    bus = alljoyn_busattachment_create_concurrency("BusAttachmentTest", QCC_TRUE, 8);

    concurrency = alljoyn_busattachment_getconcurrency(bus);
    //The default value for getconcurrency is 4
    EXPECT_EQ(8u, concurrency) << "  Expected a concurrency of 8 got " << concurrency;

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, isconnected)
{
    QStatus status;
    alljoyn_busattachment bus;
    size_t i;

    QCC_BOOL allow_remote[2] = { QCC_FALSE, QCC_TRUE };

    for (i = 0; i < ArraySize(allow_remote); i++) {
        status = ER_FAIL;
        bus = NULL;

        bus = alljoyn_busattachment_create("BusAttachmentTest", allow_remote[i]);

        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));

        status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_disconnect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_stop(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_join(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_busattachment_destroy(bus);
    }
}

TEST(BusAttachmentTest, disconnect)
{
    QStatus status;
    alljoyn_busattachment bus;
    size_t i;

    QCC_BOOL allow_remote[2] = { QCC_FALSE, QCC_TRUE };

    for (i = 0; i < ArraySize(allow_remote); i++) {
        status = ER_FAIL;
        bus = NULL;

        bus = alljoyn_busattachment_create("BusAttachmentTest", allow_remote[i]);

        status = alljoyn_busattachment_disconnect(bus, NULL);
        EXPECT_EQ(ER_BUS_BUS_NOT_STARTED, status);

        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));

        status = alljoyn_busattachment_disconnect(bus, NULL);
        EXPECT_EQ(ER_BUS_NOT_CONNECTED, status);

        status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_disconnect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_stop(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_join(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_busattachment_destroy(bus);
    }
}

TEST(BusAttachmentTest, connect_null)
{
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_TRUE);

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));

    const char* connectspec = alljoyn_busattachment_getconnectspec(bus);

    /*
     * The BusAttachment has joined either a separate daemon or it is using
     * the in process name service.  If the internal name service is used
     * the connect spec will be 'null:' otherwise it will match the ConnectArg.
     */
    EXPECT_TRUE(strcmp(ajn::getConnectArg().c_str(), connectspec) == 0 ||
                strcmp("null:", connectspec) == 0);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getconnectspec)
{
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_TRUE);

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char* connectspec = alljoyn_busattachment_getconnectspec(bus);

    /*
     * The BusAttachment has joined either a separate daemon or it is using
     * the in process name service.  If the internal name service is used
     * the connect spec will be 'null:' otherwise it will match the ConnectArg.
     */
    EXPECT_TRUE(strcmp(ajn::getConnectArg().c_str(), connectspec) == 0 ||
                strcmp("null:", connectspec) == 0);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getdbusobject) {
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_TRUE);

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject dBusProxyObject = alljoyn_busattachment_getdbusproxyobj(bus);

    alljoyn_msgarg msgArgs = alljoyn_msgarg_array_create(2);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(msgArgs, 0), "s", "org.alljoyn.test.BusAttachment");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(msgArgs, 1), "u", 7u);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_message replyMsg = alljoyn_message_create(bus);

    status = alljoyn_proxybusobject_methodcall(dBusProxyObject, "org.freedesktop.DBus", "RequestName", msgArgs, 2, replyMsg, 25000, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    unsigned int requestNameReply;
    alljoyn_msgarg reply = alljoyn_message_getarg(replyMsg, 0);
    alljoyn_msgarg_get(reply, "u", &requestNameReply);

    EXPECT_EQ(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER, requestNameReply);

    alljoyn_msgarg_destroy(msgArgs);
    alljoyn_message_destroy(replyMsg);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, ping_self) {
    QStatus status;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_TRUE);

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_ping(bus, alljoyn_busattachment_getuniquename(bus), 1000));

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, ping_other_on_same_bus) {
    QStatus status;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create("BusAttachmentTest", QCC_TRUE);

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment otherbus = NULL;
    otherbus = alljoyn_busattachment_create("BusAttachment OtherBus", QCC_TRUE);

    status = alljoyn_busattachment_start(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(otherbus, ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_ping(bus, alljoyn_busattachment_getuniquename(otherbus), 1000));

    status = alljoyn_busattachment_stop(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(otherbus);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}
