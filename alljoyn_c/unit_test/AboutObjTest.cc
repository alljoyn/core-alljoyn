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
#include <alljoyn_c/AboutObjectDescription.h>
#include <alljoyn_c/AboutProxy.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/AjAPI.h>
#include <alljoyn/MsgArg.h>
#include "ajTestCommon.h"
#include "alljoyn/DBusStd.h"
#include <qcc/Thread.h>
#include <qcc/GUID.h>


/*
 * This test uses the GUID128 in multiple places to generate a random string.
 * We are using random strings in many of the interface names to prevent multiple
 * tests interfering with one another. Some automated build systems could run this
 * same test on multiple platforms at one time.  Since the names announced could
 * be seen across platforms we want to make the names unique so we know we are
 * responding to an advertisement we have made.
 */

/*
 * The unit test use many busy wait loops.  The busy wait loops were chosen
 * over thread sleeps because of the ease of understanding the busy wait loops.
 * Also busy wait loops do not require any platform specific threading code.
 */
#define WAIT_TIME 5

using namespace ajn;

static QCC_BOOL my_sessionportlistener_acceptsessionjoiner(const void* context,
                                                           alljoyn_sessionport sessionPort,
                                                           const char* joiner,
                                                           const alljoyn_sessionopts opts)
{
    QCC_UNUSED(context);
    QCC_UNUSED(sessionPort);
    QCC_UNUSED(joiner);
    QCC_UNUSED(opts);
    return QCC_TRUE;
}

static void echo_aboutobject(alljoyn_busobject object,
                             const alljoyn_interfacedescription_member* member,
                             alljoyn_message message)
{
    QCC_UNUSED(member);
    alljoyn_msgarg arg = alljoyn_message_getarg(message, 0);
    QStatus status = alljoyn_busobject_methodreply_args(object, message, arg, 1);
    EXPECT_EQ(ER_OK, status) << "Echo: Error sending reply,  Actual Status: " << QCC_StatusText(status);
}

static alljoyn_busobject create_about_obj_test_bus_object(alljoyn_busattachment bus,
                                                          const char* path,
                                                          const char* interfaceName)
{
    QStatus status = ER_FAIL;
    alljoyn_busobject result = NULL;
    result = alljoyn_busobject_create(path, QCC_FALSE, NULL, NULL);

    alljoyn_interfacedescription iface = alljoyn_busattachment_getinterface(bus, interfaceName);
    EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceName;

    if (iface == NULL) {
        printf("The interfaceDescription pointer for %s was NULL when it should not have been.", interfaceName);
        return NULL;
    }
    alljoyn_busobject_addinterface(result, iface);
    alljoyn_busobject_setannounceflag(result, iface, ANNOUNCED);

    /* Register the method handlers with the object */
    alljoyn_interfacedescription_member echomember;
    alljoyn_interfacedescription_getmember(iface, "Echo", &echomember);

    const alljoyn_busobject_methodentry methodEntries[] = { { &echomember, echo_aboutobject } };
    status = alljoyn_busobject_addmethodhandlers(result, methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    return result;
}

static void destroy_about_obj_test_bus_object(alljoyn_busobject obj)
{
    if (obj) {
        alljoyn_busobject_destroy(obj);
        obj = NULL;
    }
}

typedef struct about_obj_test_about_listener_2_t {
    int announceListenerFlag;
    int aboutObjectPartOfAnnouncement;
    char* busName;
    alljoyn_sessionport port;
    alljoyn_aboutlistener listener;
    uint16_t version;
}about_obj_test_about_listener_2;

static void about_obj_test_about_listener_announced_cb(const void* context,
                                                       const char* busName,
                                                       uint16_t version,
                                                       alljoyn_sessionport port,
                                                       const alljoyn_msgarg objectDescriptionArg,
                                                       const alljoyn_msgarg aboutDataArg)
{
    QCC_UNUSED(aboutDataArg);
    about_obj_test_about_listener_2* listener = (about_obj_test_about_listener_2*)(context);
    EXPECT_FALSE(listener->announceListenerFlag == 1)
        << "We don't expect the flag to already be true when an AnnouceSignal is received.";
    size_t busNameLen = strlen(busName);
    if (listener->busName) {
        free(listener->busName);
        listener->busName = NULL;
    }
    listener->busName = (char*) malloc(sizeof(char) * (busNameLen + 1));
    strncpy(listener->busName, busName, busNameLen);
    listener->busName[busNameLen] = '\0';
    listener->port = port;
    listener->version = version;
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create();
    QStatus status = alljoyn_aboutobjectdescription_createfrommsgarg(aod, objectDescriptionArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    listener->aboutObjectPartOfAnnouncement =
        alljoyn_aboutobjectdescription_hasinterface(aod, "org.alljoyn.About");
    listener->announceListenerFlag = QCC_TRUE;
    alljoyn_aboutobjectdescription_destroy(aod);

}

static about_obj_test_about_listener_2* create_about_obj_test_about_listener_2()
{
    about_obj_test_about_listener_2* result =
        (about_obj_test_about_listener_2*) malloc(sizeof(about_obj_test_about_listener_2));
    alljoyn_aboutlistener_callback callbacks = {
        &about_obj_test_about_listener_announced_cb
    };
    result->listener = alljoyn_aboutlistener_create(&callbacks, result);
    result->port = 0;
    result->busName = NULL;
    result->announceListenerFlag = 0;
    result->aboutObjectPartOfAnnouncement = 0;
    return result;
}

static void destroy_about_obj_test_about_listener_2(about_obj_test_about_listener_2* listener)
{
    if (listener->busName) {
        free(listener->busName);
        listener->busName = NULL;
    }
    if (listener->listener) {
        alljoyn_aboutlistener_destroy(listener->listener);
        listener = NULL;
    }
}

class AboutObjTest : public testing::Test {
  public:
    AboutObjTest()
    {
        port = 25;
        aboutData = alljoyn_aboutdata_create("en");
        serviceBus = NULL;
    }

    virtual void SetUp()
    {
        QStatus status = ER_FAIL;
        serviceBus = alljoyn_busattachment_create("AboutObjTestServiceBus", QCC_TRUE);
        status = alljoyn_busattachment_start(serviceBus);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = alljoyn_busattachment_connect(serviceBus, NULL);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        /* Setup the about data */
        qcc::GUID128 appId;
        status = alljoyn_aboutdata_setappid(aboutData, appId.GetBytes(), qcc::GUID128::SIZE);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setdevicename(aboutData, "My Device Name", "en");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        qcc::GUID128 deviceId;
        status = alljoyn_aboutdata_setdeviceid(aboutData, deviceId.ToString().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setdescription(aboutData,
                                                  "A poetic description of this application",
                                                  "en");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setdateofmanufacture(aboutData, "2014-03-24");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_sethardwareversion(aboutData, "0.0.1");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_aboutdata_setsupporturl(aboutData, "http://www.example.com");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

        alljoyn_sessionportlistener_callbacks callbacks = {
            &my_sessionportlistener_acceptsessionjoiner
        };
        alljoyn_sessionopts opts =
            alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                       ALLJOYN_PROXIMITY_ANY,
                                       ALLJOYN_TRANSPORT_ANY);

        listener = alljoyn_sessionportlistener_create(&callbacks, NULL);
        status = alljoyn_busattachment_bindsessionport(serviceBus, &port, opts, listener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown()
    {
        if (serviceBus) {
            alljoyn_busattachment_stop(serviceBus);
            alljoyn_busattachment_join(serviceBus);
            alljoyn_busattachment_destroy(serviceBus);
            serviceBus = NULL;
        }
        if (aboutData) {
            alljoyn_aboutdata_destroy(aboutData);
            aboutData = NULL;
        }
        if (listener) {
            alljoyn_sessionportlistener_destroy(listener);
            listener = NULL;
        }
    }

    alljoyn_sessionportlistener listener;
    alljoyn_busattachment serviceBus;
    alljoyn_aboutdata aboutData;
    alljoyn_sessionport port;
};

TEST_F(AboutObjTest, AnnounceSessionPortNotBound) {
    QStatus status = ER_FAIL;
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);
    /* The SessionPort 5154 is not bound so should return ER_ABOUT_SESSIONPORT_NOT_BOUND error */
    status = alljoyn_aboutobj_announce(aboutObj, (alljoyn_sessionport)(5154), aboutData);
    EXPECT_EQ(ER_ABOUT_SESSIONPORT_NOT_BOUND, status)
        << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_aboutobj_destroy(aboutObj);
}

TEST_F(AboutObjTest, AnnounceMissingRequiredField) {
    QStatus status = ER_FAIL;
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);
    alljoyn_aboutdata badAboutData = alljoyn_aboutdata_create("en");

    /* DefaultLanguage and other required fields are missing */
    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setdefaultlanguage(badAboutData, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status)
        << "  Actual Status: " << QCC_StatusText(status);

    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(badAboutData, originalAppId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* DeviceId and other required fields are missing */
    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setdeviceid(badAboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* AppName and other required fields are missing */
    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setappname(badAboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Manufacturer and other required fields are missing */
    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setmanufacturer(badAboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* ModelNumber and other required fields are missing */
    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setmodelnumber(badAboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Description and other required fields are missing */
    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setdescription(badAboutData,
                                              "A poetic description of this application",
                                              "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* SoftwareVersion missing */
    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setsoftwareversion(badAboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Now all required fields are set for the default language */
    status = alljoyn_aboutobj_announce(aboutObj, port, badAboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutdata_destroy(badAboutData);
}

TEST_F(AboutObjTest, SetAnnounceFlag) {
    QStatus status = ER_FAIL;
    char interfaceName[] = "org.alljoyn.About";
    alljoyn_interfacedescription iface =
        alljoyn_busattachment_getinterface(serviceBus, interfaceName);

    EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceName;
    alljoyn_busobject busObj = alljoyn_busobject_create("/test/alljoyn/AboutObj",
                                                        QCC_FALSE, NULL, NULL);

    status = alljoyn_busobject_addinterface_announced(busObj, iface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busobject_setannounceflag(busObj, iface, UNANNOUNCED);

    size_t numIfaces = alljoyn_busobject_getannouncedinterfacenames(busObj, NULL, 0);
    ASSERT_EQ(static_cast<size_t>(0), numIfaces);

    status = alljoyn_busobject_setannounceflag(busObj, iface, ANNOUNCED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    numIfaces = alljoyn_busobject_getannouncedinterfacenames(busObj, NULL, 0);
    ASSERT_EQ(static_cast<size_t>(1), numIfaces);
    const char* interfaces[1];

    numIfaces = alljoyn_busobject_getannouncedinterfacenames(busObj, interfaces, numIfaces);
    EXPECT_STREQ("org.alljoyn.About", interfaces[0]);

    alljoyn_interfacedescription dbusIface =
        alljoyn_busattachment_getinterface(serviceBus,
                                           org::freedesktop::DBus::InterfaceName);
    status = alljoyn_busobject_setannounceflag(busObj, dbusIface, ANNOUNCED);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busobject_setannounceflag(busObj, iface, UNANNOUNCED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    numIfaces = alljoyn_busobject_getannouncedinterfacenames(busObj, NULL, 0);
    ASSERT_EQ(static_cast<size_t>(0), numIfaces);

    alljoyn_busobject_destroy(busObj);
}

TEST_F(AboutObjTest, CancelAnnouncement) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutObjTestClient", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_obj_test_about_listener_2* testAboutListener2 = create_about_obj_test_about_listener_2();
    alljoyn_busattachment_registeraboutlistener(clientBus, testAboutListener2->listener);
    status =  alljoyn_busattachment_whoimplements_interface(clientBus, "org.alljoyn.About");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, ANNOUNCED);

    status = alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (testAboutListener2->announceListenerFlag == QCC_TRUE) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    EXPECT_TRUE(testAboutListener2->announceListenerFlag)
        << "The announceListenerFlag must be true to continue this test.";
    EXPECT_TRUE(testAboutListener2->aboutObjectPartOfAnnouncement)
        << "The org.alljoyn.About interface was not part of the announced object description.";
    const char* serviceBusUniqueName = alljoyn_busattachment_getuniquename(serviceBus);
    EXPECT_STREQ(serviceBusUniqueName, testAboutListener2->busName);
    EXPECT_EQ(port, testAboutListener2->port);

    status = alljoyn_aboutobj_unannounce(aboutObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_stop(clientBus);
    alljoyn_busattachment_join(clientBus);

    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
    destroy_about_obj_test_about_listener_2(testAboutListener2);
}

TEST_F(AboutObjTest, AnnounceTheAboutObj) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutObjTestClient", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_obj_test_about_listener_2* testAboutListener2 = create_about_obj_test_about_listener_2();
    alljoyn_busattachment_registeraboutlistener(clientBus, testAboutListener2->listener);
    status =  alljoyn_busattachment_whoimplements_interface(clientBus, "org.alljoyn.About");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, ANNOUNCED);

    status = alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (testAboutListener2->announceListenerFlag == QCC_TRUE) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    EXPECT_TRUE(testAboutListener2->announceListenerFlag) << "The announceListenerFlag must be true to continue this test.";
    EXPECT_TRUE(testAboutListener2->aboutObjectPartOfAnnouncement) << "The org.alljoyn.About interface was not part of the announced object description.";
    const char* serviceBusUniqueName = alljoyn_busattachment_getuniquename(serviceBus);
    EXPECT_STREQ(serviceBusUniqueName, testAboutListener2->busName);
    EXPECT_EQ(port, testAboutListener2->port);

    alljoyn_busattachment_stop(clientBus);
    alljoyn_busattachment_join(clientBus);

    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
    destroy_about_obj_test_about_listener_2(testAboutListener2);
}

TEST_F(AboutObjTest, Announce) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceNameQcc = "test.about.a" + interface_rand_string.ToString();
    qcc::String interfaceQcc = "<node>"
                               "<interface name='" + ifaceNameQcc + "'>"
                               "  <method name='Echo'>"
                               "    <arg name='out_arg' type='s' direction='in' />"
                               "    <arg name='return_arg' type='s' direction='out' />"
                               "  </method>"
                               "</interface>"
                               "</node>";

    const char* ifaceName = ifaceNameQcc.c_str();
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject =
        create_about_obj_test_bus_object(serviceBus, "/test/alljoyn/AboutObj", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutObjTestClient", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_obj_test_about_listener_2* aboutListener = create_about_obj_test_about_listener_2();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    status =  alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);
    status = alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener->announceListenerFlag == QCC_TRUE) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(aboutListener->announceListenerFlag)
        << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(alljoyn_busattachment_getuniquename(serviceBus), aboutListener->busName);
    EXPECT_EQ(port, aboutListener->port);

    alljoyn_sessionid sessionId;
    alljoyn_sessionopts sessionOpts =
        alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                   ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    status = alljoyn_busattachment_joinsession(clientBus, aboutListener->busName,
                                               aboutListener->port, NULL,
                                               &sessionId, sessionOpts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_proxybusobject proxy =
        alljoyn_proxybusobject_create(clientBus, aboutListener->busName,
                                      "/test/alljoyn/AboutObj", sessionId);

    status = alljoyn_proxybusobject_parsexml(proxy, interface, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: "
                             << QCC_StatusText(status) << "\n" << interface;
    QCC_BOOL isImplementsIface =
        alljoyn_proxybusobject_implementsinterface(proxy, ifaceName);
    EXPECT_TRUE(isImplementsIface) << interface << "\n" << ifaceName;

    alljoyn_msgarg arg =
        alljoyn_msgarg_create_and_set("s", "String that should be Echoed back.");
    alljoyn_message replyMsg = alljoyn_message_create(clientBus);
    status = alljoyn_proxybusobject_methodcall(proxy, ifaceName, "Echo", arg,
                                               1, replyMsg, 25000, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: "
                             << QCC_StatusText(status) << " " << "Invalid arg";

    char* echoReply;
    alljoyn_msgarg arg0 = alljoyn_message_getarg(replyMsg, 0);
    status = alljoyn_msgarg_get(arg0, "s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    alljoyn_busattachment_stop(clientBus);
    alljoyn_busattachment_join(clientBus);

    destroy_about_obj_test_about_listener_2(aboutListener);
    destroy_about_obj_test_bus_object(busObject);
    alljoyn_proxybusobject_destroy(proxy);
    alljoyn_sessionopts_destroy(sessionOpts);
    alljoyn_msgarg_destroy(arg);
    alljoyn_message_destroy(replyMsg);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutObjTest, ProxyAccessToAboutObj) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceNameQcc = "test.about.a" + interface_rand_string.ToString();
    qcc::String interfaceQcc = "<node>"
                               "<interface name='" + ifaceNameQcc + "'>"
                               "  <method name='Echo'>"
                               "    <arg name='out_arg' type='s' direction='in' />"
                               "    <arg name='return_arg' type='s' direction='out' />"
                               "  </method>"
                               "</interface>"
                               "</node>";

    const char* ifaceName = ifaceNameQcc.c_str();
    const char* interface = interfaceQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject busObject =
        create_about_obj_test_bus_object(serviceBus, "/test/alljoyn/AboutObj", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("AboutObjTestClient", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_obj_test_about_listener_2* aboutListener = create_about_obj_test_about_listener_2();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    status =  alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);
    status = alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener->announceListenerFlag == QCC_TRUE) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(aboutListener->announceListenerFlag)
        << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(alljoyn_busattachment_getuniquename(serviceBus), aboutListener->busName);
    EXPECT_EQ(port, aboutListener->port);

    alljoyn_sessionid sessionId;
    alljoyn_sessionopts sessionOpts =
        alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                   ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);

    alljoyn_busattachment_enableconcurrentcallbacks(clientBus);
    status = alljoyn_busattachment_joinsession(clientBus, aboutListener->busName,
                                               aboutListener->port, NULL,
                                               &sessionId, sessionOpts);

    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutproxy aProxy = alljoyn_aboutproxy_create(clientBus, aboutListener->busName, sessionId);
    /*
     * Call each of the proxy methods
     * GetObjDescription
     * GetAboutData
     * GetVersion
     */
    uint16_t ver;
    status = alljoyn_aboutproxy_getversion(aProxy, &ver);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(aboutListener->version, ver);

    alljoyn_msgarg aboutArg = alljoyn_msgarg_create();
    status = alljoyn_aboutproxy_getaboutdata(aProxy, "en", aboutArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_aboutdata testAboutData = alljoyn_aboutdata_create_full(aboutArg, "en");

    char* appName;
    status = alljoyn_aboutdata_getappname(testAboutData, &appName, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Application", appName);

    char* manufacturer;
    status = alljoyn_aboutdata_getmanufacturer(testAboutData, &manufacturer, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Manufacturer", manufacturer);

    char* modelNum;
    status = alljoyn_aboutdata_getmodelnumber(testAboutData, &modelNum);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("123456", modelNum);

    char* desc;
    status = alljoyn_aboutdata_getdescription(testAboutData, &desc, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("A poetic description of this application", desc);

    char* dom;
    status = alljoyn_aboutdata_getdateofmanufacture(testAboutData, &dom);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("2014-03-24", dom);

    char* softVer;
    status = alljoyn_aboutdata_getsoftwareversion(testAboutData, &softVer);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("0.1.2", softVer);

    char* hwVer;
    status = alljoyn_aboutdata_gethardwareversion(testAboutData, &hwVer);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("0.0.1", hwVer);

    char* support;
    status = alljoyn_aboutdata_getsupporturl(testAboutData, &support);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("http://www.example.com", support);

    /*
     * French is specified language.
     * Expect an error.
     */
    alljoyn_msgarg aboutArg_fr = alljoyn_msgarg_create();
    status = alljoyn_aboutproxy_getaboutdata(aProxy, "fr", aboutArg_fr);
    EXPECT_EQ(ER_LANGUAGE_NOT_SUPPORTED, status)
        << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg objDesc = alljoyn_msgarg_create();
    alljoyn_aboutproxy_getobjectdescription(aProxy, objDesc);
    EXPECT_EQ(ER_LANGUAGE_NOT_SUPPORTED, status)
        << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobjectdescription aObjDesc =
        alljoyn_aboutobjectdescription_create_full(objDesc);

    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(aObjDesc, "/test/alljoyn/AboutObj"));
    EXPECT_TRUE(alljoyn_aboutobjectdescription_hasinterface(aObjDesc, ifaceName));

    alljoyn_busattachment_stop(clientBus);
    alljoyn_busattachment_join(clientBus);

    alljoyn_aboutobjectdescription_destroy(aObjDesc);
    alljoyn_msgarg_destroy(objDesc);
    alljoyn_msgarg_destroy(aboutArg);
    alljoyn_msgarg_destroy(aboutArg_fr);
    alljoyn_aboutdata_destroy(testAboutData);
    destroy_about_obj_test_about_listener_2(aboutListener);
    destroy_about_obj_test_bus_object(busObject);
    alljoyn_aboutproxy_destroy(aProxy);
    alljoyn_sessionopts_destroy(sessionOpts);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}
