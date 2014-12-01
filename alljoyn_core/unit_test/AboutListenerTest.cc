/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/AboutData.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AboutProxy.h>
#include <alljoyn/BusAttachment.h>

#include <qcc/Thread.h>
#include <qcc/GUID.h>
#include <set>

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
using namespace qcc;
using namespace std;

class AnnounceListenerTestSessionPortListener : public SessionPortListener {
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
};


class AboutListenerTest : public testing::Test {
  public:
    AboutListenerTest() : serviceBus(NULL), aboutData("en"), port(25) { }
    virtual void SetUp() {

        QStatus status;

        serviceBus = new BusAttachment("AnnounceListenerTest", true);
        status = serviceBus->Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus->Connect();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        // Setup the about data
        qcc::GUID128 appId;
        status = aboutData.SetAppId(appId.GetBytes(), qcc::GUID128::SIZE);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetDeviceName("My Device Name");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        qcc::GUID128 deviceId;
        status = aboutData.SetDeviceId(deviceId.ToString().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetAppName("Application");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetManufacturer("Manufacturer");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetModelNumber("123456");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetDescription("A poetic description of this application");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetDateOfManufacture("2014-03-24");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetSoftwareVersion("0.1.2");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetHardwareVersion("0.0.1");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.SetSupportUrl("http://www.alljoyn.org");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(aboutData.IsValid()) << "failed to setup about data.\n";

        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        AnnounceListenerTestSessionPortListener listener;
        status = serviceBus->BindSessionPort(port, opts, listener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        if (serviceBus) {
            serviceBus->Stop();
            serviceBus->Join();
            BusAttachment* deleteMe = serviceBus;
            serviceBus = NULL;
            delete deleteMe;
        }
    }

    BusAttachment* serviceBus;
    AboutData aboutData;
    SessionPort port;
};

static bool announceListenerFlag = false;

class AboutTestAboutListener : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData) {
        announceListenerFlag = true;
    }
};

class AboutListenerTestObject : public BusObject {
  public:
    AboutListenerTestObject(BusAttachment& bus, const char* path, const char* ifaceName)
        : BusObject(path) {

        const InterfaceDescription* iface = bus.GetInterface(ifaceName);
        EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << ifaceName;
        if (iface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL when it should not have been.\n", ifaceName);
            return;
        }
        QStatus status = AddInterface(*iface, ANNOUNCED);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { iface->GetMember("Foo"), static_cast<MessageReceiver::MethodHandler>(&AboutListenerTestObject::Foo) }
        };
        status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    void Foo(const InterfaceDescription::Member* member, Message& msg) {
        MethodReply(msg, (const MsgArg*)NULL, (size_t)0);
    }
};

TEST_F(AboutListenerTest, ReceiverAnnouncement) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";
    AboutObj aboutObj(*serviceBus);

    const qcc::String interface = "<node>"
                                  "<interface name='" + ifaceName + "'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj(*serviceBus, "/org/test/about", ifaceName.c_str());
    serviceBus->RegisterBusObject(altObj);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.CancelWhoImplements(ifaceName.c_str());

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, ReceiveAnnouncementNullWhoImplementsValue) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";
    AboutObj aboutObj(*serviceBus);

    const qcc::String interface = "<node>"
                                  "<interface name='" + ifaceName + "'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj(*serviceBus, "/org/test/about", ifaceName.c_str());
    serviceBus->RegisterBusObject(altObj);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.CancelWhoImplements(NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    announceListenerFlag = false;
    status = clientBus.WhoImplements(NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.CancelWhoImplements(NULL, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
/*
 * for most of the tests the interfaces are all added then the listener is
 * registered for this test we will register the listener before adding the
 * interfaces.  This should still work.
 */
TEST_F(AboutListenerTest, ReceiveAnnouncementRegisterThenAddInterface)
{
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    const qcc::String interface = "<node>"
                                  "<interface name='" + ifaceName + "'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj(*serviceBus, "/org/test/about", ifaceName.c_str());
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);
    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, ReAnnounceAnnouncement) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    const qcc::String interface = "<node>"
                                  "<interface name='" + ifaceName + "'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj(*serviceBus, "/org/test/about", ifaceName.c_str());
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    announceListenerFlag = false;

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

static bool announceListenerFlag1 = false;
static bool announceListenerFlag2 = false;
static bool announceListenerFlag3 = false;

class AboutTestAboutListener1 : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData) {
        announceListenerFlag1 = true;
    }
};

class AboutTestAboutListener2 : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData) {
        announceListenerFlag2 = true;
    }
};

class AboutTestAboutListener3 : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData) {
        announceListenerFlag3 = true;
    }
};

TEST_F(AboutListenerTest, MultipleAnnounceListeners) {
    QStatus status;
    announceListenerFlag1 = false;
    announceListenerFlag2 = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    const qcc::String interface = "<node>"
                                  "<interface name='" + ifaceName + "'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj(*serviceBus, "/org/test/about", ifaceName.c_str());
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener1 aboutListener1;

    clientBus.RegisterAboutListener(aboutListener1);

    AboutTestAboutListener2 aboutListener2;

    clientBus.RegisterAboutListener(aboutListener2);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    //Wait for a maximum of 5 sec for the second Announce Signal.
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlag2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag1);
    ASSERT_TRUE(announceListenerFlag2);

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener1);
    clientBus.UnregisterAboutListener(aboutListener2);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, MultipleAnnounceListenersUnregister) {
    QStatus status;
    announceListenerFlag1 = false;
    announceListenerFlag2 = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";
    const qcc::String interface = "<node>"
                                  "<interface name='" + ifaceName + "'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj(*serviceBus, "/org/test/about", ifaceName.c_str());
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener1 aboutListener1;
    clientBus.RegisterAboutListener(aboutListener1);

    AboutTestAboutListener2 aboutListener2;
    clientBus.RegisterAboutListener(aboutListener2);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    //Wait for a maximum of 5 sec for the second Announce Signal.
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlag2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag1);
    ASSERT_TRUE(announceListenerFlag2);

    announceListenerFlag1 = false;
    announceListenerFlag2 = false;

    clientBus.UnregisterAboutListener(aboutListener1);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 5 sec for the second Announce Signal.
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlag2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_FALSE(announceListenerFlag1);
    ASSERT_TRUE(announceListenerFlag2);

    clientBus.CancelWhoImplements(ifaceName.c_str());

    clientBus.UnregisterAboutListener(aboutListener2);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, MultipleAnnounceListenersUnregisterAll) {
    QStatus status;
    announceListenerFlag1 = false;
    announceListenerFlag2 = false;
    announceListenerFlag3 = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";

    const qcc::String interface = "<node>"
                                  "<interface name='" + ifaceName + "'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj(*serviceBus, "/org/test/about", ifaceName.c_str());
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener1 aboutListener1;
    clientBus.RegisterAboutListener(aboutListener1);

    AboutTestAboutListener2 aboutListener2;
    clientBus.RegisterAboutListener(aboutListener2);

    AboutTestAboutListener3 aboutListener3;
    clientBus.RegisterAboutListener(aboutListener3);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    //Wait for a maximum of 5 sec for the second Announce Signal.
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlag2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    //Wait for a maximum of 5 sec for the 3rd Announce Signal.
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlag3) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag1);
    ASSERT_TRUE(announceListenerFlag2);
    ASSERT_TRUE(announceListenerFlag3);

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAllAboutListeners();

    announceListenerFlag1 = false;
    announceListenerFlag2 = false;
    announceListenerFlag3 = false;

    clientBus.RegisterAboutListener(aboutListener2);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 5 sec for the second Announce Signal.
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlag2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_FALSE(announceListenerFlag1);
    ASSERT_TRUE(announceListenerFlag2);
    ASSERT_FALSE(announceListenerFlag3);

    clientBus.UnregisterAllAboutListeners();

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

class AboutListenerTestObject2 : public BusObject {
  public:
    AboutListenerTestObject2(BusAttachment& bus, const char* path, qcc::String* ifaceName, size_t ifaceNameSize)
        : BusObject(path) {
        for (size_t i = 0; i < ifaceNameSize; ++i) {
            const InterfaceDescription* iface = bus.GetInterface(ifaceName[i].c_str());
            EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << ifaceName;
            if (iface == NULL) {
                printf("The interfaceDescription pointer for %s was NULL when it should not have been.\n", ifaceName->c_str());
                return;
            }
            QStatus status = AddInterface(*iface, ANNOUNCED);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

            /* Register the method handlers with the object */
            const MethodEntry methodEntries[] = {
                { iface->GetMember("Foo"), static_cast<MessageReceiver::MethodHandler>(&AboutListenerTestObject2::Foo) }
            };
            status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    void Foo(const InterfaceDescription::Member* member, Message& msg) {
        MethodReply(msg, (const MsgArg*)NULL, (size_t)0);
    }
};

TEST_F(AboutListenerTest, MatchMultipleInterfaces) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";

    const qcc::String interface = "<node>"
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
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, "/org/test/about", ifaceNames, 3);
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaces, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.CancelWhoImplements(ifaces, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
TEST_F(AboutListenerTest, MatchMultipleInterfacesSubSet) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceNames[6];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";
    ifaceNames[3] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.d";
    ifaceNames[4] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.e";
    ifaceNames[5] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.f";

    const qcc::String interface = "<node>"
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
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, "/org/test/about", ifaceNames, 6);
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;


    const char* ifacesSubSet[2];
    ifacesSubSet[0] = ifaceNames[1].c_str();
    ifacesSubSet[1] = ifaceNames[2].c_str();
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifacesSubSet, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.CancelWhoImplements(ifacesSubSet, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, MatchMultipleInterfacesRegisterInDifferentOrder) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceNames[6];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";
    ifaceNames[3] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.d";
    ifaceNames[4] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.e";
    ifaceNames[5] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.f";

    const qcc::String interface = "<node>"
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
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, "/org/test/about", ifaceNames, 6);
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;


    const char* ifaceslist[6];
    ifaceslist[0] = ifaceNames[3].c_str();
    ifaceslist[1] = ifaceNames[0].c_str();
    ifaceslist[2] = ifaceNames[5].c_str();
    ifaceslist[3] = ifaceNames[2].c_str();
    ifaceslist[4] = ifaceNames[1].c_str();
    ifaceslist[5] = ifaceNames[4].c_str();

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceslist, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.CancelWhoImplements(ifaceslist, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

class AboutTestWildCardAboutListener : public AboutListener {
  public:
    AboutTestWildCardAboutListener() : announceListenerCount(0) { }
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData) {
        announceListenerCount++;
    }
    uint32_t announceListenerCount;
};

TEST_F(AboutListenerTest, WildCardInterfaceMatching) {
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";

    const qcc::String interface = "<node>"
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
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, "/org/test/about", ifaceNames, 3);
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestWildCardAboutListener aboutListener;

    qcc::String wildCard = "org.test.a" + guid.ToString() + ".*";
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(wildCard.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1u, aboutListener.announceListenerCount);

    status = clientBus.CancelWhoImplements(wildCard.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

/*
 * this tests using a mid string wildcard.  Its unknown if this is an expected
 * use case or not.
 */
TEST_F(AboutListenerTest, WildCardInterfaceMatching2) {
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".a.AnnounceHandlerTest";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".b.AnnounceHandlerTest";
    ifaceNames[2] = "org.test.a" + guid.ToString() + ".c.AnnounceHandlerTest";

    const qcc::String interface = "<node>"
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
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, "/org/test/about", ifaceNames, 3);
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestWildCardAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    qcc::String wildCard = "org.test.a" + guid.ToString() + ".*.AnnounceHandlerTest";
    status = clientBus.WhoImplements(wildCard.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1u, aboutListener.announceListenerCount);

    status = clientBus.CancelWhoImplements(wildCard.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, MultipleWildCardInterfaceMatching) {
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.foo.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.foo.a" + guid.ToString() + ".AnnounceHandlerTest.c";

    const qcc::String interface = "<node>"
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
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, "/org/test/about", ifaceNames, 3);
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestWildCardAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    qcc::String wildCard = "org.test.a" + guid.ToString() + ".*";
    qcc::String wildCard2 = "org.test.foo.a" + guid.ToString() + ".*";
    const char* interfacelist[2];
    interfacelist[0] = wildCard.c_str();
    interfacelist[1] = wildCard2.c_str();
    status = clientBus.WhoImplements(interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1u, aboutListener.announceListenerCount);;

    status = clientBus.CancelWhoImplements(interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, MixedWildCardNonWildCardInterfaceMatching) {
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.foo.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.foo.a" + guid.ToString() + ".AnnounceHandlerTest.c";

    const qcc::String interface = "<node>"
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
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, "/org/test/about", ifaceNames, 3);
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestWildCardAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    qcc::String wildCard = "org.test.foo.a" + guid.ToString() + ".*";
    const char* interfacelist[2];
    interfacelist[0] = ifaceNames[0].c_str();
    interfacelist[1] = wildCard.c_str();
    status = clientBus.WhoImplements(interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1u, aboutListener.announceListenerCount);;

    status = clientBus.CancelWhoImplements(interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

class AboutTestRemoveObjectDescriptionAboutListener : public AboutListener {
  public:
    AboutTestRemoveObjectDescriptionAboutListener() : announceListenerCount(0) { }
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        AboutObjectDescription objectDescription;
        objectDescription.CreateFromMsgArg(objectDescriptionArg);
        if (announceListenerCount == 0) {
            EXPECT_TRUE(objectDescription.HasPath("/org/test/about/a"));
            EXPECT_TRUE(objectDescription.HasPath("/org/test/about/b"));
        } else {
            EXPECT_TRUE(objectDescription.HasPath("/org/test/about/a"));
            EXPECT_FALSE(objectDescription.HasPath("/org/test/about/b"));
        }
        announceListenerCount++;
    }
    uint32_t announceListenerCount;
};

//ASACORE-651
TEST_F(AboutListenerTest, RemoveObjectDescriptionAnnouncement) {
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[2];
    ifaceNames[0] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";

    const qcc::String interface0 = "<node>"
                                   "<interface name='" + ifaceNames[0] + "'>"
                                   "  <method name='Foo'>"
                                   "  </method>"
                                   "</interface>"
                                   "</node>";
    const qcc::String interface1 = "<node>"
                                   "<interface name='" + ifaceNames[1] + "'>"
                                   "  <method name='Foo'>"
                                   "  </method>"
                                   "</interface>"
                                   "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface0.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus->CreateInterfacesFromXml(interface1.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj0(*serviceBus, "/org/test/about/a", ifaceNames[0].c_str());
    AboutListenerTestObject altObj1(*serviceBus, "/org/test/about/b", ifaceNames[1].c_str());
    status = serviceBus->RegisterBusObject(altObj0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = serviceBus->RegisterBusObject(altObj1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestRemoveObjectDescriptionAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceNames[0].c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutObj.Announce(port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_EQ(1u, aboutListener.announceListenerCount);

    serviceBus->UnregisterBusObject(altObj1);

    status = aboutObj.Announce(port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_EQ(2u, aboutListener.announceListenerCount);

    status = clientBus.CancelWhoImplements(ifaceNames[0].c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
/*
 * Stress test:
 *    Service announce 100 interfaces, client interested in 100 interfaces.
 *    Client get announcement.
 */
TEST_F(AboutListenerTest, StressInterfaces) {
    QStatus status;

    qcc::GUID128 guid;

    // Use a randomly generated prefix to avoid unexpected conflicts
    const qcc::String INTERFACE_PREFIX = "a" + guid.ToString() + ".";

    // Max interface name length is 255 chars
    const size_t MAX_INTERFACE_BODY_LEN = 255 - INTERFACE_PREFIX.length();
    // 100 interfaces
    const size_t INTERFACE_COUNT = 100;

    qcc::String ifaceNames[INTERFACE_COUNT];
    qcc::String interfaceXml = "<node>";

    // Test can't support more than 221 interfaces since max interface length is 255
    // each test interface name has a prefix and variable body
    ASSERT_TRUE(INTERFACE_COUNT < MAX_INTERFACE_BODY_LEN) << " Too many interfaces";

    char randChar = 'a';
    for (size_t i = 0; i < INTERFACE_COUNT; i++) {
        // Generate a pseudo-random char (loop from a-z)
        randChar = 'a' + (i % 26);

        // Generate string with different length per loop like a/bb/ccc/dddd...
        qcc::String randStr((i + 1), (randChar));

        ifaceNames[i] = INTERFACE_PREFIX + randStr;
        interfaceXml += "<interface name='" + ifaceNames[i] + "'>"
                        "  <method name='Foo'>"
                        "  </method>"
                        "</interface>";
    }
    interfaceXml += "</node>";

    status = serviceBus->CreateInterfacesFromXml(interfaceXml.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, "/org/test/stress", ifaceNames, INTERFACE_COUNT);
    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener3 aboutListener;

    const char* ifaces[INTERFACE_COUNT];
    for (size_t i = 0; i < INTERFACE_COUNT; i++) {
        ifaces[i] = ifaceNames[i].c_str();
    }

    announceListenerFlag3 = false;

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaces, INTERFACE_COUNT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag3) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag3);

    status = clientBus.CancelWhoImplements(ifaces, INTERFACE_COUNT);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

/* If whoImplements use * or null, we may get unwanted announcements
 * This listener will filter out unwanted announcements
 */
class FilteredAboutListener : public AboutListener {
  public:
    FilteredAboutListener() :
        objPath(),
        expectedInterfaceSet(),
        announcedInterfaceSet(),
        interfaceCnt(0),
        announceListenerCount(0)
    { }

    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData)
    {
        QStatus status = ER_OK;
        AboutObjectDescription aod;

        status = aod.CreateFromMsgArg(objectDescription);
        EXPECT_EQ(ER_OK, status);

        // Check if this announcement is what we wanted
        if (objPath.c_str() != NULL) {
            if (aod.HasPath(objPath.c_str())) {
                size_t numInterfaces = aod.GetInterfaces(objPath.c_str(), NULL, 0);
                EXPECT_EQ(interfaceCnt, numInterfaces);

                const char** announcedInterfaces = new const char*[numInterfaces];
                aod.GetInterfaces(objPath.c_str(), announcedInterfaces, numInterfaces);

                for (size_t j = 0; j < numInterfaces; j++) {
                    announcedInterfaceSet.insert(announcedInterfaces[j]);
                }

                delete [] announcedInterfaces;

                if (announcedInterfaceSet == expectedInterfaceSet) {
                    announceListenerCount++;
                }
            }
        }
    }

    void expectInterfaces(String& path, String* interfaces, size_t infCount)
    {
        objPath = path;
        for (size_t i = 0; i < infCount; i++) {
            expectedInterfaceSet.insert(interfaces[i]);
        }

        interfaceCnt = infCount;
    }

    String objPath;
    set<String> expectedInterfaceSet;
    set<String> announcedInterfaceSet;
    size_t interfaceCnt;
    uint32_t announceListenerCount;
};

/* positive test
 *  ASACORE1007 - NULL should match all interfaces just like *
 */
TEST_F(AboutListenerTest, WhoImplementsNull) {
    QStatus status;

    qcc::GUID128 guid;
    String path("/org/test/null");

    qcc::String ifaceNames[3];
    ifaceNames[0] = "null.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "null.test.a" + guid.ToString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "null.test.a" + guid.ToString() + ".AnnounceHandlerTest.c";

    const qcc::String interface = "<node>"
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
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject2 altObj(*serviceBus, path.c_str(), ifaceNames, 3);

    status = serviceBus->RegisterBusObject(altObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    FilteredAboutListener aboutListener;

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutListener.expectInterfaces(path, ifaceNames, 3);

    aboutObj.Announce(port, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1u, aboutListener.announceListenerCount);

    status = clientBus.CancelWhoImplements(NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, CancelWhoImplementsMisMatch) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest.a";

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_BUS_MATCH_RULE_NOT_FOUND, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

class AnnounceAppIdWithNon128BitLengthAboutListener : public AboutListener {
  public:
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        aboutData = aboutDataArg;
        aboutData.Stabilize();
        announceListenerFlag = true;
    }
    MsgArg aboutData;
};

TEST_F(AboutListenerTest, AnnounceAppIdWithNon128BitLength) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToString() + ".AnnounceHandlerTest";
    AboutObj aboutObj(*serviceBus);

    const qcc::String interface = "<node>"
                                  "<interface name='" + ifaceName + "'>"
                                  "  <method name='Foo'>"
                                  "  </method>"
                                  "</interface>"
                                  "</node>";
    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutListenerTestObject altObj(*serviceBus, "/org/test/about", ifaceName.c_str());
    serviceBus->RegisterBusObject(altObj);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AnnounceAppIdWithNon128BitLengthAboutListener aboutListener;

    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // 64-bit AppId
    uint8_t appid_64[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    status = aboutData.SetAppId(appid_64, 8);
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutObj.Announce(port, aboutData);
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal. Even if we get an
    // ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE error we expect to get the
    // Announce signal
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    AboutData listenerAboutData(aboutListener.aboutData);
    uint8_t* appId;
    size_t num;
    status = listenerAboutData.GetAppId(&appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(8u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(appid_64[i], appId[i]);
    }

    announceListenerFlag = false;
    // 192-bit AppId
    uint8_t appid_192[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23 };
    status = aboutData.SetAppId(appid_192, 24);
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status) << "  Actual Status: " << QCC_StatusText(status);


    status = aboutObj.Announce(port, aboutData);
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal. Even if we get an
    // ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE error we expect to get the
    // Announce signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    listenerAboutData.CreatefromMsgArg(aboutListener.aboutData);
    status = aboutData.GetAppId(&appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(24u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(appid_192[i], appId[i]);
    }

    status = clientBus.CancelWhoImplements(ifaceName.c_str());

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
