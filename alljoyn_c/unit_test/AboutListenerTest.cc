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

#include <qcc/Thread.h>
#include <qcc/GUID.h>

#include "AboutListenerTestApi.h"

#define WAIT_TIME 5

extern QCC_BOOL announceListenerFlags[];

static
QCC_BOOL my_sessionportlistener_acceptsessionjoiner(const void* context,
                                                    alljoyn_sessionport sessionPort,
                                                    const char* joiner,
                                                    const alljoyn_sessionopts opts)
{
    return QCC_TRUE;
}

class AboutListenerTest : public testing::Test {
  public:
    AboutListenerTest() {
        serviceBus = NULL;
        port = 25;
        aboutData = alljoyn_aboutdata_create("en");
    }

    virtual void SetUp() {
        QStatus status;

        serviceBus = alljoyn_busattachment_create("AnnounceListenerTest", QCC_TRUE);
        status = alljoyn_busattachment_start(serviceBus);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(serviceBus, NULL);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Setup the about data */
        qcc::GUID128 appId;
        status = alljoyn_aboutdata_setappid(aboutData,
                                            appId.GetBytes(),
                                            qcc::GUID128::SIZE);
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
        status = alljoyn_aboutdata_setsupporturl(aboutData, "http://www.alljoyn.org");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

        alljoyn_sessionportlistener_callbacks callbacks = {
            &my_sessionportlistener_acceptsessionjoiner
        };
        alljoyn_sessionopts opts =
            alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                       ALLJOYN_PROXIMITY_ANY,
                                       ALLJOYN_TRANSPORT_ANY);
        alljoyn_sessionportlistener listener =
            alljoyn_sessionportlistener_create(&callbacks, NULL);
        status =
            alljoyn_busattachment_bindsessionport(serviceBus, &port, opts, listener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
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
    }

    alljoyn_busattachment serviceBus;
    alljoyn_aboutdata aboutData;
    alljoyn_sessionport port;
};

TEST_F(AboutListenerTest, ReceiverAnnouncement) {
    QStatus status;
    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    zero_announceListenerFlags();
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNameQcc + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    const char* ifaceName = ifaceNameQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_about_listener* aboutListener = create_about_test_about_listener(0);

    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);
    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, ReceiveAnnouncementNullWhoImplementsValue) {
    QStatus status;
    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    zero_announceListenerFlags();
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNameQcc + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    const char* ifaceName = ifaceNameQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_about_listener* aboutListener = create_about_test_about_listener(0);

    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    status =  alljoyn_busattachment_whoimplements_interface(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);
    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, NULL);
    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, ReAnnounceAnnouncement) {
    QStatus status;
    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    zero_announceListenerFlags();
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNameQcc + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    const char* ifaceName = ifaceNameQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_about_listener* aboutListener = create_about_test_about_listener(0);

    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    status =  alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);
    announceListenerFlags[0] = QCC_FALSE;
    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);
    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, ReceiveAnnouncementRegisterThenAddInterface) {
    QStatus status;
    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    zero_announceListenerFlags();
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNameQcc + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    const char* ifaceName = ifaceNameQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_about_listener* aboutListener = create_about_test_about_listener(0);

    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    status =  alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);
    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, MultipleAnnounceListeners) {
    QStatus status;
    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    zero_announceListenerFlags();
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNameQcc + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    const char* ifaceName = ifaceNameQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_about_listener* aboutListener1 = create_about_test_about_listener(1);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener1->listener);
    about_test_about_listener* aboutListener2 = create_about_test_about_listener(2);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener2->listener);

    status =  alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[1]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    /* Wait for a maximum of 5 sec for the second Announce Signal */
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlags[2]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[1]);
    ASSERT_TRUE(announceListenerFlags[2]);

    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener1->listener);
    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener2->listener);
    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener1);
    destroy_about_test_about_listener(aboutListener2);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, MultipleAnnounceListenersUnregister) {
    QStatus status;
    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    zero_announceListenerFlags();
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNameQcc + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    const char* ifaceName = ifaceNameQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_about_listener* aboutListener1 = create_about_test_about_listener(1);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener1->listener);
    about_test_about_listener* aboutListener2 = create_about_test_about_listener(2);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener2->listener);

    status =  alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[1]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    /* Wait for a maximum of 5 sec for the second Announce Signal */
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlags[2]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[1]);
    ASSERT_TRUE(announceListenerFlags[2]);

    announceListenerFlags[1] = QCC_FALSE;
    announceListenerFlags[2] = QCC_FALSE;

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener1->listener);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 5 sec for the second Announce Signal */
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlags[2]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_FALSE(announceListenerFlags[1]);
    ASSERT_TRUE(announceListenerFlags[2]);
    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener2->listener);
    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener1);
    destroy_about_test_about_listener(aboutListener2);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, MultipleAnnounceListenersUnregisterAll) {
    QStatus status;
    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    zero_announceListenerFlags();
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNameQcc + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();
    const char* ifaceName = ifaceNameQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_about_listener* aboutListener1 = create_about_test_about_listener(1);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener1->listener);
    about_test_about_listener* aboutListener2 = create_about_test_about_listener(2);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener2->listener);
    about_test_about_listener* aboutListener3 = create_about_test_about_listener(3);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener3->listener);

    status =  alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[1]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    /* Wait for a maximum of 5 sec for the second Announce Signal */
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlags[2]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    /* Wait for a maximum of 5 sec for the third Announce Signal */
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlags[3]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[1]);
    ASSERT_TRUE(announceListenerFlags[2]);
    ASSERT_TRUE(announceListenerFlags[3]);

    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_unregisterallaboutlisteners(clientBus);

    announceListenerFlags[1] = QCC_FALSE;
    announceListenerFlags[2] = QCC_FALSE;
    announceListenerFlags[3] = QCC_FALSE;

    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener2->listener);
    status =  alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Wait for a maximum of 5 sec for the second Announce Signal */
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlags[2]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    ASSERT_FALSE(announceListenerFlags[1]);
    ASSERT_TRUE(announceListenerFlags[2]);
    ASSERT_FALSE(announceListenerFlags[3]);

    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisterallaboutlisteners(clientBus);
    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener1);
    destroy_about_test_about_listener(aboutListener2);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, MatchMultipleInterfaces) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();
    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNames[0] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[1] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[2] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, "/org/test/about", ifaces, 3);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    about_test_about_listener* aboutListener = create_about_test_about_listener(0);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    alljoyn_busattachment_whoimplements_interfaces(clientBus, ifaces, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);
    alljoyn_busattachment_cancelwhoimplements_interfaces(clientBus, ifaces, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, MatchMultipleInterfacesSubSet) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNames[6];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";
    ifaceNames[3] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.d";
    ifaceNames[4] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.e";
    ifaceNames[5] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.f";

    const char* ifaces[6];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();
    ifaces[3] = ifaceNames[3].c_str();
    ifaces[4] = ifaceNames[4].c_str();
    ifaces[5] = ifaceNames[5].c_str();

    const char* ifacesSubSet[2];
    ifacesSubSet[0] = ifaceNames[1].c_str();
    ifacesSubSet[1] = ifaceNames[2].c_str();

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNames[0] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[1] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[2] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[3] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[4] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[5] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, "/org/test/about", ifaces, 6);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    about_test_about_listener* aboutListener = create_about_test_about_listener(0);

    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    alljoyn_busattachment_whoimplements_interfaces(clientBus, ifacesSubSet, 2);

    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);

    alljoyn_busattachment_cancelwhoimplements_interfaces(clientBus, ifacesSubSet, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, MatchMultipleInterfacesRegisterInDifferentOrder) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNames[6];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";
    ifaceNames[3] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.d";
    ifaceNames[4] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.e";
    ifaceNames[5] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.f";

    const char* ifaces[6];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();
    ifaces[3] = ifaceNames[3].c_str();
    ifaces[4] = ifaceNames[4].c_str();
    ifaces[5] = ifaceNames[5].c_str();

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNames[0] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[1] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[2] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[3] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[4] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[5] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, "/org/test/about", ifaces, 6);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    about_test_about_listener* aboutListener = create_about_test_about_listener(0);

    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    const char* ifaceslist[6];
    ifaceslist[0] = ifaceNames[3].c_str();
    ifaceslist[1] = ifaceNames[0].c_str();
    ifaceslist[2] = ifaceNames[5].c_str();
    ifaceslist[3] = ifaceNames[2].c_str();
    ifaceslist[4] = ifaceNames[1].c_str();
    ifaceslist[5] = ifaceNames[4].c_str();

    alljoyn_busattachment_whoimplements_interfaces(clientBus, ifaceslist, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);

    alljoyn_busattachment_cancelwhoimplements_interfaces(clientBus, ifaceslist, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    destroy_about_test_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, WildCardInterfaceMatching) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();
    qcc::String wildCardQcc = "org.test.a" + guid.ToString() + ".*";
    const char* wildCard = wildCardQcc.c_str();

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNames[0] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[1] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[2] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, "/org/test/about", ifaces, 3);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_wildcard_about_listener* aboutListener = create_about_test_wildcard_about_listener();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    alljoyn_busattachment_whoimplements_interface(clientBus, wildCard);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener->announcelistenercount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(aboutListener->announcelistenercount, 1u);

    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, wildCard);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    destroy_about_test_wildcard_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, WildCardInterfaceMatching2) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".a.AnnounceHandlerTest";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".b.AnnounceHandlerTest";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".c.AnnounceHandlerTest";
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();
    qcc::String wildCardQcc = "org.test.a" + guid.ToString() + ".*.AnnounceHandlerTest";
    const char* wildCard = wildCardQcc.c_str();

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNames[0] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[1] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[2] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, "/org/test/about", ifaces, 3);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_wildcard_about_listener* aboutListener = create_about_test_wildcard_about_listener();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    alljoyn_busattachment_whoimplements_interface(clientBus, wildCard);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener->announcelistenercount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(aboutListener->announcelistenercount, 1u);

    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, wildCard);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    destroy_about_test_wildcard_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, MultipleWildCardInterfaceMatching) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.foo.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.foo.a" + guid.ToString() + ".AnnounceHandlerTest.c";
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();
    qcc::String wildCard = "org.test.a" + guid.ToString() + ".*";
    qcc::String wildCard2 = "org.test.foo.a" + guid.ToString() + ".*";
    const char* interfacelist[2];
    interfacelist[0] = wildCard.c_str();
    interfacelist[1] = wildCard2.c_str();

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNames[0] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[1] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[2] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, "/org/test/about", ifaces, 3);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);
    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_wildcard_about_listener* aboutListener = create_about_test_wildcard_about_listener();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);
    alljoyn_busattachment_whoimplements_interfaces(clientBus, interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener->announcelistenercount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(aboutListener->announcelistenercount, 1u);

    alljoyn_busattachment_cancelwhoimplements_interfaces(clientBus, interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    destroy_about_test_wildcard_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, MixedWildCardNonWildCardInterfaceMatching) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.foo.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.foo.a" + guid.ToString() + ".AnnounceHandlerTest.c";
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();
    qcc::String wildCard = "org.test.a" + guid.ToString() + ".*";
    const char* interfacelist[2];
    interfacelist[0] = ifaceNames[0].c_str();
    interfacelist[1] = wildCard.c_str();

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNames[0] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[1] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNames[2] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, "/org/test/about", ifaces, 3);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_wildcard_about_listener* aboutListener = create_about_test_wildcard_about_listener();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    alljoyn_busattachment_whoimplements_interfaces(clientBus, interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener->announcelistenercount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(aboutListener->announcelistenercount, 1u);

    alljoyn_busattachment_cancelwhoimplements_interfaces(clientBus, interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    destroy_about_test_wildcard_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, RemoveObjectDescriptionAnnouncement) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNamesQcc[2];
    ifaceNamesQcc[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNamesQcc[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";

    const char* ifaceNames[2];
    ifaceNames[0] = ifaceNamesQcc[0].c_str();
    ifaceNames[1] = ifaceNamesQcc[1].c_str();

    const qcc::String interfaceQcc0 = "<node>"
                                      "<interface name='" + ifaceNamesQcc[0] + "'>"
                                      "  <method name='Foo'>"
                                      "  </method>"
                                      "</interface>"
                                      "</node>";
    const qcc::String interfaceQcc1 = "<node>"
                                      "<interface name='" + ifaceNamesQcc[1] + "'>"
                                      "  <method name='Foo'>"
                                      "  </method>"
                                      "</interface>"
                                      "</node>";

    const char* interface0 = interfaceQcc0.c_str();
    const char* interface1 = interfaceQcc1.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj0 =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about/a", ifaceNames[0]);
    alljoyn_busobject altObj1 =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about/b", ifaceNames[1]);

    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    remove_object_description_about_listener* aboutListener =
        create_remove_object_description_about_listener();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    alljoyn_busattachment_whoimplements_interface(clientBus, ifaceNames[0]);

    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener->announcelistenercount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(aboutListener->announcelistenercount, 1u);

    alljoyn_busattachment_unregisterbusobject(serviceBus, altObj1);
    status = alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener->announcelistenercount == 2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }
    ASSERT_EQ(2u, aboutListener->announcelistenercount);
    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceNames[0]);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    destroy_remove_object_description_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj0);
    destroy_about_obj_test_bus_object(altObj1);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, StressInterfaces) {
    QStatus status;
    zero_announceListenerFlags();
    qcc::GUID128 guid;

    /* Use a randomly generated prefix to avoid unexpected conflicts */
    const qcc::String INTERFACE_PREFIX = "a" + guid.ToString() + ".";

    /* Max interface name length is 255 chars */
    const size_t MAX_INTERFACE_BODY_LEN = 255 - INTERFACE_PREFIX.length();
    /* 100 interfaces. */
    const size_t INTERFACE_COUNT = 100;

    qcc::String ifaceNamesQcc[INTERFACE_COUNT];
    qcc::String interfaceXmlQcc = "<node>";

    /*
     *  Test can't support more than 221 interfaces since max interface length is 255
     *  each test interface name has a prefix and variable body
     */
    ASSERT_TRUE(INTERFACE_COUNT < MAX_INTERFACE_BODY_LEN) << " Too many interfaces";

    char randChar = 'a';
    for (size_t i = 0; i < INTERFACE_COUNT; i++) {
        /* Generate a pseudo-random char (loop from a-z). */
        randChar = 'a' + (i % 26);

        /* Generate string with different length per loop like a/bb/ccc/dddd... */
        qcc::String randStr((i + 1), (randChar));

        ifaceNamesQcc[i] = INTERFACE_PREFIX + randStr;
        interfaceXmlQcc += "<interface name='" + ifaceNamesQcc[i] + "'>"
                           "  <method name='Foo'>"
                           "  </method>"
                           "</interface>";
    }
    interfaceXmlQcc += "</node>";
    const char* ifaceNames[INTERFACE_COUNT];
    for (size_t i = 0; i < INTERFACE_COUNT; ++i) {
        ifaceNames[i] = ifaceNamesQcc[i].c_str();
    }
    const char* interfaceXml = interfaceXmlQcc.c_str();
    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interfaceXml);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, "/org/test/stress",
                                           ifaceNames, INTERFACE_COUNT);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    about_test_about_listener* aboutListener = create_about_test_about_listener(3);
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    alljoyn_busattachment_whoimplements_interfaces(clientBus, ifaceNames, INTERFACE_COUNT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[3]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[3]);
    alljoyn_busattachment_cancelwhoimplements_interfaces(clientBus,
                                                         ifaceNames,
                                                         INTERFACE_COUNT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    destroy_about_test_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, CancelWhoImplementsMisMatch) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    const char* ifaceName = ifaceNameQcc.c_str();
    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_BUS_MATCH_RULE_NOT_FOUND, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, AnnounceAppIdWithNon128BitLength) {
    QStatus status;
    zero_announceListenerFlags();

    qcc::GUID128 guid;
    qcc::String ifaceNameQcc = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNameQcc + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* ifaceName = ifaceNameQcc.c_str();
    const char* interface = interfaceQcc.c_str();

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object(serviceBus, "/org/test/about", ifaceName);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    announce_non_128_bit_app_id_about_listener* aboutListener =
        create_announce_non_128_bit_app_id_about_listener();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_whoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* 64-bit AppId */
    uint8_t appid_64[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    status = alljoyn_aboutdata_setappid(aboutData, appid_64, 8);
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status)
        << "  Actual Status: " << QCC_StatusText(status);

    /*
     * Wait for a maximum of 10 sec for the Announce Signal. Even if we get an
     * ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE error we expect to get the
     * Announce signal
     */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);

    alljoyn_aboutdata listenerAboutData = alljoyn_aboutdata_create_empty();
    alljoyn_aboutdata_createfrommsgarg(listenerAboutData, aboutListener->aboutData, "en");

    uint8_t* appId;
    size_t num;
    status = alljoyn_aboutdata_getappid(listenerAboutData, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(8u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(appid_64[i], appId[i]);
    }

    zero_announceListenerFlags();
    /* 192-bit AppId */
    uint8_t appid_192[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23 };
    status = alljoyn_aboutdata_setappid(aboutData, appid_192, 24);
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status)
        << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutobj_announce(aboutObj, port, aboutData);
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status)
        << "  Actual Status: " << QCC_StatusText(status);

    /*
     * Wait for a maximum of 10 sec for the Announce Signal. Even if we get an
     * ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE error we expect to get the
     * Announce signal.
     */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlags[0]) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlags[0]);

    alljoyn_aboutdata_destroy(listenerAboutData);
    listenerAboutData = alljoyn_aboutdata_create_empty();
    alljoyn_aboutdata_createfrommsgarg(listenerAboutData, aboutListener->aboutData, "en");

    status = alljoyn_aboutdata_getappid(listenerAboutData, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(24u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(appid_192[i], appId[i]);
    }

    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, ifaceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_aboutdata_destroy(listenerAboutData);
    destroy_announce_non_128_bit_app_id_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AboutListenerTest, WhoImplementsNull) {
    QStatus status;

    qcc::GUID128 guid;
    zero_announceListenerFlags();
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(serviceBus, UNANNOUNCED);

    qcc::String ifaceNamesQcc[3];
    ifaceNamesQcc[0] = "null.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNamesQcc[1] = "null.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNamesQcc[2] = "null.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";

    const qcc::String interfaceQcc = "<node>"
                                     "<interface name='" + ifaceNamesQcc[0] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNamesQcc[1] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "<interface name='" + ifaceNamesQcc[2] + "'>"
                                     "  <method name='Foo'>"
                                     "  </method>"
                                     "</interface>"
                                     "</node>";
    const char* ifaceNames[3];
    ifaceNames[0] = ifaceNamesQcc[0].c_str();
    ifaceNames[1] = ifaceNamesQcc[1].c_str();
    ifaceNames[2] = ifaceNamesQcc[2].c_str();
    const char* interface = interfaceQcc.c_str();
    const char path[] = "/org/test/null";

    status = alljoyn_busattachment_createinterfacesfromxml(serviceBus, interface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busobject altObj =
        create_about_obj_test_bus_object_2(serviceBus, path, ifaceNames, 3);
    status = alljoyn_busattachment_registerbusobject(serviceBus, altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment clientBus =
        alljoyn_busattachment_create("Receive Announcement client Test", QCC_TRUE);

    status = alljoyn_busattachment_start(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_connect(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    filtered_about_listener* aboutListener = create_filtered_about_listener();
    alljoyn_busattachment_registeraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_whoimplements_interface(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    setExpectInterfaces(aboutListener, path, ifaceNames, 3);
    status = alljoyn_aboutobj_announce(aboutObj, port, aboutData);

    /* Wait for a maximum of 10 sec for the Announce Signal */
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener->announcelistenercount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1u, aboutListener->announcelistenercount);

    alljoyn_busattachment_cancelwhoimplements_interface(clientBus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_unregisteraboutlistener(clientBus, aboutListener->listener);

    status = alljoyn_busattachment_stop(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status =  alljoyn_busattachment_join(clientBus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    destroy_filtered_about_listener(aboutListener);
    destroy_about_obj_test_bus_object(altObj);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(clientBus);
}
