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
        status = aboutData.AddAppId(appId.GetBytes(), qcc::GUID128::SIZE);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddDeviceName("My Device Name");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        qcc::GUID128 deviceId;
        status = aboutData.AddDeviceId(deviceId.ToString().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddAppName("Application");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddManufacture("Manufacture");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddModelNumber("123456");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddDescription("A poetic description of this application");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddDateOfManufacture("2014-03-24");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddSoftwareVersion("0.1.2");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddHardwareVersion("0.0.1");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutData.AddSupportUrl("http://www.alljoyn.org");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (!aboutData.IsValid()) {
            printf("failed to setup about data.\n");
        }

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
                   AboutObjectDescription& objectDescription, AboutData& aboutData) {
        announceListenerFlag = true;
    }
};


TEST_F(AboutListenerTest, ReceiverAnnouncement) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest";
    AboutObjectDescription aod;
    aod.Add("/org/test/about", ifaceName);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    status = clientBus.RegisterAboutListener(aboutListener, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.UnregisterAboutListener(aboutListener, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    qcc::String ifaceName = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest";

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    status = clientBus.RegisterAboutListener(aboutListener, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjectDescription aod;
    aod.Add("/org/test/about", ifaceName);
    AboutObj aboutObj(*serviceBus);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.UnregisterAboutListener(aboutListener, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, ReAnnounceAnnouncement) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest";

    AboutObjectDescription aod;
    aod.Add("/org/test/about", ifaceName);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    status = clientBus.RegisterAboutListener(aboutListener, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    announceListenerFlag = false;

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    status = clientBus.UnregisterAboutListener(aboutListener, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
                   AboutObjectDescription& objectDescription, AboutData& aboutData) {
        announceListenerFlag1 = true;
    }
};

class AboutTestAboutListener2 : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   AboutObjectDescription& objectDescription, AboutData& aboutData) {
        announceListenerFlag2 = true;
    }
};

class AboutTestAboutListener3 : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   AboutObjectDescription& objectDescription, AboutData& aboutData) {
        announceListenerFlag3 = true;
    }
};

TEST_F(AboutListenerTest, MultipleAnnounceListeners) {
    QStatus status;
    announceListenerFlag1 = false;
    announceListenerFlag2 = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest";
    AboutObjectDescription aod;
    aod.Add("/org/test/about", ifaceName);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener1 aboutListener1;

    status = clientBus.RegisterAboutListener(aboutListener1, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener2 aboutListener2;

    status = clientBus.RegisterAboutListener(aboutListener2, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

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

    status = clientBus.UnregisterAboutListener(aboutListener1, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.UnregisterAboutListener(aboutListener2, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    qcc::String ifaceName = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest";
    AboutObjectDescription aod;
    aod.Add("/org/test/about", ifaceName);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener1 aboutListener1;

    status = clientBus.RegisterAboutListener(aboutListener1, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener2 aboutListener2;

    status = clientBus.RegisterAboutListener(aboutListener2, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

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

    status = clientBus.UnregisterAboutListener(aboutListener1, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 5 sec for the second Announce Signal.
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceListenerFlag2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_FALSE(announceListenerFlag1);
    ASSERT_TRUE(announceListenerFlag2);

    status = clientBus.UnregisterAboutListener(aboutListener2, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    qcc::String ifaceName = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest";
    AboutObjectDescription aod;
    aod.Add("/org/test/about", ifaceName);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener1 aboutListener1;

    status = clientBus.RegisterAboutListener(aboutListener1, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener2 aboutListener2;

    status = clientBus.RegisterAboutListener(aboutListener2, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener3 aboutListener3;

    status = clientBus.RegisterAboutListener(aboutListener3, ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

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

    status = clientBus.UnregisterAllAboutListeners();

    announceListenerFlag1 = false;
    announceListenerFlag2 = false;
    announceListenerFlag3 = false;

    status = clientBus.RegisterAboutListener(aboutListener2, ifaceName.c_str());
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

    status = clientBus.UnregisterAllAboutListeners();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, MatchMultipleInterfaces) {
    QStatus status;
    announceListenerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.c";

    AboutObjectDescription aod;
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();

    aod.Add("/org/test/about", ifaces, 3);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestAboutListener aboutListener;

    status = clientBus.RegisterAboutListener(aboutListener, ifaces, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.UnregisterAboutListener(aboutListener, ifaces, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    ifaceNames[0] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.c";
    ifaceNames[3] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.d";
    ifaceNames[4] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.e";
    ifaceNames[5] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.f";

    AboutObjectDescription aod;
    const char* ifaces[6];
    for (size_t i = 0; i < 6; ++i) {
        ifaces[i] = ifaceNames[i].c_str();
    }

    aod.Add("/org/test/about", ifaces, 6);
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
    status = clientBus.RegisterAboutListener(aboutListener, ifacesSubSet, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.UnregisterAboutListener(aboutListener, ifacesSubSet, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    ifaceNames[0] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.c";
    ifaceNames[3] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.d";
    ifaceNames[4] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.e";
    ifaceNames[5] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.f";

    AboutObjectDescription aod;
    const char* ifaces[6];
    for (size_t i = 0; i < 6; ++i) {
        ifaces[i] = ifaceNames[i].c_str();
    }

    aod.Add("/org/test/about", ifaces, 6);
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

    status = clientBus.RegisterAboutListener(aboutListener, ifaceslist, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceListenerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceListenerFlag);

    status = clientBus.UnregisterAboutListener(aboutListener, ifaceslist, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

class AboutTestWildCardAboutListener : public AboutListener {
  public:
    AboutTestWildCardAboutListener() : announceListenerCount(0) { }
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   AboutObjectDescription& objectDescription, AboutData& aboutData) {
        announceListenerCount++;
    }
    uint32_t announceListenerCount;
};

TEST_F(AboutListenerTest, WildCardInterfaceMatching) {
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.c";

    AboutObjectDescription aod;
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();

    aod.Add("/org/test/about", ifaces, 3);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestWildCardAboutListener aboutListener;

    qcc::String wildCard = "org.test.a" + guid.ToShortString() + ".*";
    status = clientBus.RegisterAboutListener(aboutListener, wildCard.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1, aboutListener.announceListenerCount);;

    status = clientBus.UnregisterAboutListener(aboutListener, wildCard.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    ifaceNames[0] = "org.test.a" + guid.ToShortString() + ".a.AnnounceHandlerTest";
    ifaceNames[1] = "org.test.a" + guid.ToShortString() + ".b.AnnounceHandlerTest";
    ifaceNames[2] = "org.test.a" + guid.ToShortString() + ".c.AnnounceHandlerTest";

    AboutObjectDescription aod;
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();

    aod.Add("/org/test/about", ifaces, 3);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestWildCardAboutListener aboutListener;

    qcc::String wildCard = "org.test.a" + guid.ToShortString() + ".*.AnnounceHandlerTest";
    status = clientBus.RegisterAboutListener(aboutListener, wildCard.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1, aboutListener.announceListenerCount);;

    status = clientBus.UnregisterAboutListener(aboutListener, wildCard.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, MultipleWildCardInterfaceMatching) {
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.foo.a" + guid.ToShortString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.foo.a" + guid.ToShortString() + ".AnnounceHandlerTest.c";

    AboutObjectDescription aod;
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();

    aod.Add("/org/test/about", ifaces, 3);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestWildCardAboutListener aboutListener;

    qcc::String wildCard = "org.test.a" + guid.ToShortString() + ".*";
    qcc::String wildCard2 = "org.test.foo.a" + guid.ToShortString() + ".*";
    const char* interfacelist[2];
    interfacelist[0] = wildCard.c_str();
    interfacelist[1] = wildCard2.c_str();
    status = clientBus.RegisterAboutListener(aboutListener, interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1, aboutListener.announceListenerCount);;

    status = clientBus.UnregisterAboutListener(aboutListener, interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutListenerTest, MixedWildCardNonWildCardInterfaceMatching) {
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.foo.a" + guid.ToShortString() + ".AnnounceHandlerTest.b";
    ifaceNames[2] = "org.test.foo.a" + guid.ToShortString() + ".AnnounceHandlerTest.c";

    AboutObjectDescription aod;
    const char* ifaces[3];
    ifaces[0] = ifaceNames[0].c_str();
    ifaces[1] = ifaceNames[1].c_str();
    ifaces[2] = ifaceNames[2].c_str();

    aod.Add("/org/test/about", ifaces, 3);
    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestWildCardAboutListener aboutListener;

    qcc::String wildCard = "org.test.foo.a" + guid.ToShortString() + ".*";
    const char* interfacelist[2];
    interfacelist[0] = ifaceNames[0].c_str();
    interfacelist[1] = wildCard.c_str();
    status = clientBus.RegisterAboutListener(aboutListener, interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1, aboutListener.announceListenerCount);;

    status = clientBus.UnregisterAboutListener(aboutListener, interfacelist, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

class AboutTestRemoveObjectDescriptionAboutListener : public AboutListener {
  public:
    AboutTestRemoveObjectDescriptionAboutListener() : announceListenerCount(0) { }
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   AboutObjectDescription& objectDescription, AboutData& aboutData) {
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
    ifaceNames[0] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.a";
    ifaceNames[1] = "org.test.a" + guid.ToShortString() + ".AnnounceHandlerTest.b";

    AboutObjectDescription aod;
    aod.Add("/org/test/about/a", ifaceNames[0]);
    aod.Add("/org/test/about/b", ifaceNames[1]);

    AboutObj aboutObj(*serviceBus);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutTestRemoveObjectDescriptionAboutListener aboutListener;

    status = clientBus.RegisterAboutListener(aboutListener, ifaceNames[0].c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_EQ(1, aboutListener.announceListenerCount);

    aod.Remove("/org/test/about/b", ifaceNames[1]);

    aboutObj.Announce(port, aod, aboutData);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerCount == 2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_EQ(2, aboutListener.announceListenerCount);

    status = clientBus.UnregisterAboutListener(aboutListener, ifaceNames[0].c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
