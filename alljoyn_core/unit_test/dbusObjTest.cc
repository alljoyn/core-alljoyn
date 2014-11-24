/******************************************************************************
 * Copyright (c) 2011,2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>

#include <qcc/String.h>
#include <qcc/Util.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "ajTestCommon.h"
//#include <qcc/time.h>

using namespace std;
using namespace qcc;
using namespace ajn;
class DBusObjTest : public testing::Test {
  public:
    BusAttachment bus;

    DBusObjTest() : bus("testDBusObj", false) { };

    virtual void SetUp() {
        QStatus status = ER_OK;
        status = bus.Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = bus.Connect(getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        bus.Stop();
        bus.Join();
    }

};

TEST_F(DBusObjTest, RequestName_CorrectName_Success)
{
    QStatus status = ER_OK;

    const char* requestedName = "org.alljoyn.myService";

    /* flag indicates that Fail if name cannot be immediatly obtained */
    status = bus.RequestName(requestedName, DBUS_NAME_FLAG_DO_NOT_QUEUE);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Cleanup */
    bus.ReleaseName(requestedName);
}

TEST_F(DBusObjTest, RequestName_TwoNames_Success)
{
    QStatus status = ER_OK;

    const char* requestedName1 = "org.alljoyn.myService1";
    const char* requestedName2 = "org.alljoyn.myService2";

    /* flag indicates - Fail if name cannot be immediatly obtained */
    status = bus.RequestName(requestedName1, DBUS_NAME_FLAG_DO_NOT_QUEUE);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.RequestName(requestedName2, DBUS_NAME_FLAG_DO_NOT_QUEUE);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Cleanup */
    bus.ReleaseName(requestedName1);
    bus.ReleaseName(requestedName2);
}

TEST_F(DBusObjTest, RequestName_DuplicateName_Fail)
{
    QStatus status = ER_OK;

    const char* requestedName1 = "org.alljoyn.myService2";
    const char* requestedName2 = "org.alljoyn.myService2";

    /* flag indicates - Fail if name cannot be immediatly obtained */
    status = bus.RequestName(requestedName1, DBUS_NAME_FLAG_DO_NOT_QUEUE);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.RequestName(requestedName2, DBUS_NAME_FLAG_DO_NOT_QUEUE);
    ASSERT_EQ(ER_DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER, status);

    /* Cleanup */
    bus.ReleaseName(requestedName1);
}

TEST_F(DBusObjTest, RequestName_BogusUniqueName_Fail)
{
    QStatus status = ER_OK;

    const char* requestedName = ":xyz.1";

    /* flag indicates that Fail if name cannot be immediatly obtained */
    status = bus.RequestName(requestedName, DBUS_NAME_FLAG_DO_NOT_QUEUE);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);

}
TEST_F(DBusObjTest, ListQueuedOwners) {
    QStatus status = ER_OK;

    /* Create message bus */
    BusAttachment bus2("testDBusObj2", false);
    BusAttachment bus3("testDBusObj3", false);
    BusAttachment bus4("testDBusObj4", false);

    /* Start the msg bus2 */
    status = bus2.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Create the client-side endpoint */
    status = bus2.Connect(getConnectArg().c_str());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Start the msg bus */
    status = bus3.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Create the client-side endpoint */
    status = bus3.Connect(getConnectArg().c_str());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Start the msg bus */
    status = bus4.Start();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Create the client-side endpoint */
    status = bus4.Connect(getConnectArg().c_str());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


    ProxyBusObject dbusObj1(bus.GetDBusProxyObj());

    //Initialize variables used to make ListQueuedOwners method call
    MsgArg arg;
    status = arg.Set("s", "com.test.foo");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    Message reply(bus);

    //Initialize variables that will hold the results of ListQueuedOwners method call
    MsgArg* asArray;
    size_t las;
    std::vector<char*> queuedNames;
    /*-----Calling ListQueuedOwners for name that is not on queue------*/
    /*
     * Check that No names are returned even when the name does not yet exist on
     * the bus
     */
    status = dbusObj1.MethodCall("org.freedesktop.DBus", "ListQueuedOwners", &arg, 1,  reply);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    reply->GetArg(0)->Get("as", &las, &asArray);
    ASSERT_EQ(static_cast<size_t>(0), las);
    queuedNames.clear();
    queuedNames.reserve(las);
    for (unsigned int i = 0; i < las; ++i) {
        status = asArray[i].Get("s", &queuedNames[i]);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    /* Request name */
    uint32_t flags = DBUS_NAME_FLAG_ALLOW_REPLACEMENT;
    status = bus.RequestName("com.test.foo", flags);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


    /*-----Calling ListQueuedOwners when a single name is on queue------*/
    /*
     * check that no names are returned when only the primary name exist on the bus
     */
    status = dbusObj1.MethodCall("org.freedesktop.DBus", "ListQueuedOwners", &arg, 1,  reply);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    reply->GetArg(0)->Get("as", &las, &asArray);
    ASSERT_EQ(static_cast<size_t>(0), las);
    queuedNames.clear();
    queuedNames.reserve(las);
    for (unsigned int i = 0; i < las; ++i) {
        status = asArray[i].Get("s", &queuedNames[i]);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    /*--Calling ListQueuedOwners when multiple names are on the queue---*/
    /* Request name (queue Name) */
    flags = 0;
    status = bus2.RequestName("com.test.foo", flags);
    ASSERT_EQ(ER_DBUS_REQUEST_NAME_REPLY_IN_QUEUE, status);

    /* Request name (queue Name)*/
    status = bus3.RequestName("com.test.foo", flags);
    ASSERT_EQ(ER_DBUS_REQUEST_NAME_REPLY_IN_QUEUE, status);

    /*
     * Check that there are now two unique names in queue
     */
    status = dbusObj1.MethodCall("org.freedesktop.DBus", "ListQueuedOwners", &arg, 1,  reply);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = reply->GetArg(0)->Get("as", &las, &asArray);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(static_cast<size_t>(2), las);
    queuedNames.clear();
    queuedNames.resize(las);
    for (unsigned int i = 0; i < las; ++i) {
        status = asArray[i].Get("s", &queuedNames[i]);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    EXPECT_STREQ(bus2.GetUniqueName().c_str(), queuedNames[0]);
    EXPECT_STREQ(bus3.GetUniqueName().c_str(), queuedNames[1]);

    /*----Calling ListQueuedOwners after adding name as primary owner---*/
    /* Request name (make primary owner)*/
    flags = DBUS_NAME_FLAG_REPLACE_EXISTING;
    status = bus4.RequestName("com.test.foo", flags);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = dbusObj1.MethodCall("org.freedesktop.DBus", "ListQueuedOwners", &arg, 1,  reply);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    reply->GetArg(0)->Get("as", &las, &asArray);
    ASSERT_EQ(static_cast<size_t>(3), las);
    queuedNames.clear();
    queuedNames.resize(las);
    for (unsigned int i = 0; i < las; ++i) {
        status = asArray[i].Get("s", &queuedNames[i]);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    EXPECT_STREQ(bus.GetUniqueName().c_str(), queuedNames[0]);
    EXPECT_STREQ(bus2.GetUniqueName().c_str(), queuedNames[1]);
    EXPECT_STREQ(bus3.GetUniqueName().c_str(), queuedNames[2]);

    bus.ReleaseName("com.test.foo");
    bus2.ReleaseName("com.test.foo");
    bus3.ReleaseName("com.test.foo");
    bus4.ReleaseName("com.test.foo");

    bus2.Stop();
    bus3.Stop();
    bus4.Stop();
}

TEST_F(DBusObjTest, GetConnectionUnixUser) {
    QStatus status = ER_FAIL;
    const char* name = "org.alljoyn.bus.ifaces.testGetConnctionUnixUser";
    uint32_t flags = 0;
    status = bus.RequestName(name, flags);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject dbusObj(bus.GetDBusProxyObj());

    MsgArg arg("s", name);
    Message reply(bus);
    status = dbusObj.MethodCall("org.freedesktop.DBus", "GetConnectionUnixUser", &arg, 1, reply);
#if defined(QCC_OS_GROUP_WINDOWS)
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
#else
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    uint32_t uid = 0;
    const MsgArg* uidArg = reply->GetArg(0);
    uidArg->Get("u", &uid);
    EXPECT_EQ(qcc::GetUid(), uid);
#endif
    status = bus.ReleaseName(name);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(DBusObjTest, GetConnectionUnixProcessID) {
    QStatus status = ER_FAIL;
    const char* name = "org.alljoyn.bus.ifaces.testGetConnectionUnixProcessID";
    uint32_t flags = 0;
    status = bus.RequestName(name, flags);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status)  << "\n\t" << name;;

    ProxyBusObject dbusObj(bus.GetDBusProxyObj());
    MsgArg arg("s", name);
    Message reply(bus);
    status = dbusObj.MethodCall("org.freedesktop.DBus", "GetConnectionUnixProcessID", &arg, 1, reply);
#if defined(QCC_OS_GROUP_WINDOWS)
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
#else
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    uint32_t pid = 0;
    reply->GetArg(0)->Get("u", &pid);
    EXPECT_EQ(qcc::GetPid(), pid);
#endif
    status = bus.ReleaseName(name);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
