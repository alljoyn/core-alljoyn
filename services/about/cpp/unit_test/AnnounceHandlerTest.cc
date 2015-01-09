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

#include <alljoyn/version.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/about/AboutIconService.h>
#include <alljoyn/about/AboutServiceApi.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>

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
using namespace services;


class AnnounceHandlerTestSessionPortListener : public SessionPortListener {
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
};

/*
 * A PropertyStoreImplementation with all of the AboutData fields filled out.
 * The AppId and DeviceId are generated at random using the qcc::GUID function
 */
class AnnounceHandlerTestPropertyStoreImpl {
  public:
    AnnounceHandlerTestPropertyStoreImpl() {
        QStatus status = ER_FAIL;
        //setup the property store
        appId = new qcc::GUID128();
        deviceId = new qcc::GUID128();
        status = propertyStore.setAppId(appId->ToString());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        std::vector<qcc::String> languages(1);
        languages[0] = "en";
        status = propertyStore.setSupportedLangs(languages);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setDefaultLang(qcc::String("en"));
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setDeviceName("AnnounceHandler Unit Test framework");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setDeviceId(deviceId->ToString());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setAppName("AnnounceHander Unit Test");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setManufacturer("AllSeen Alliance");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setModelNumber("abc123");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setDescription("A test of the Announce Handler code");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setDateOfManufacture("2014-05-29");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setSoftwareVersion("1.0.0");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setAjSoftwareVersion(ajn::GetVersion());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setHardwareVersion("0.0.1");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = propertyStore.setSupportUrl("www.allseen.org");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }
    ~AnnounceHandlerTestPropertyStoreImpl() {
        delete appId;
        delete deviceId;
    }
    AnnounceHandlerTestPropertyStoreImpl(const AnnounceHandlerTestPropertyStoreImpl& src) {
        appId = new qcc::GUID128();
        *appId = *src.appId;
        deviceId = new qcc::GUID128();
        *deviceId = *src.deviceId;
    }
    AnnounceHandlerTestPropertyStoreImpl& operator=(const AnnounceHandlerTestPropertyStoreImpl& src) {
        if (&src == this) {
            return *this;
        }
        *appId = *src.appId;
        *deviceId = *src.deviceId;
        return *this;
    }
    AboutPropertyStoreImpl propertyStore;
    qcc::GUID128* appId;
    qcc::GUID128* deviceId;
  private:


};

class AnnounceHandlerTest : public testing::Test {
  public:
    BusAttachment* g_msgBus;

    virtual void SetUp() {

        QStatus status;

        serviceBus = new BusAttachment("announceHandlerTest", true);
        status = serviceBus->Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus->Connect();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        propStore = new AnnounceHandlerTestPropertyStoreImpl();
        AboutServiceApi::Init(*serviceBus, propStore->propertyStore);
        EXPECT_TRUE(AboutServiceApi::getInstance() != NULL);

        SessionPort port = 25;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        AnnounceHandlerTestSessionPortListener listener;
        status = serviceBus->BindSessionPort(port, opts, listener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = AboutServiceApi::getInstance()->Register(port);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = serviceBus->RegisterBusObject(*AboutServiceApi::getInstance());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        AboutServiceApi::DestroyInstance();
        if (serviceBus) {
            serviceBus->Stop();
            serviceBus->Join();
            BusAttachment* deleteMe = serviceBus;
            serviceBus = NULL;
            delete deleteMe;
        }
        delete propStore;
    }

    BusAttachment* serviceBus;
    AnnounceHandlerTestPropertyStoreImpl* propStore;
};

static bool announceHandlerFlag = false;

class MyAnnounceHandler : public ajn::services::AnnounceHandler {

  public:

    virtual void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData) {
        announceHandlerFlag = true;
    }
};

TEST_F(AnnounceHandlerTest, ReceiveAnnouncement)
{
    QStatus status;
    announceHandlerFlag = false;
    qcc::GUID128 guid;
    qcc::String ifaceName = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest";
    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceName);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MyAnnounceHandler announceHandler;

    const char* interfaces[1];
    interfaces[0] = ifaceName.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandlerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandlerFlag);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

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
TEST_F(AnnounceHandlerTest, ReceiveAnnouncementRegisterThenAddInterface)
{
    QStatus status;
    announceHandlerFlag = false;

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    qcc::GUID128 guid;
    qcc::String ifaceName = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest";

    MyAnnounceHandler announceHandler;

    const char* interfaces[1];
    interfaces[0] = ifaceName.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceName);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandlerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandlerFlag);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AnnounceHandlerTest, ReAnnounceAnnouncement)
{
    QStatus status;
    announceHandlerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceName);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MyAnnounceHandler announceHandler;

    const char* interfaces[1];
    interfaces[0] = ifaceName.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandlerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandlerFlag);

    announceHandlerFlag = false;


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 30 sec for the Announce Signal.
    for (int i = 0; i < 3000; ++i) {
        if (announceHandlerFlag) {
            break;
        }
        qcc::Sleep(10);
    }

    ASSERT_TRUE(announceHandlerFlag);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

static bool announceHandler1Flag = false;
static bool announceHandler2Flag = false;
static bool announceHandler3Flag = false;

class AnnounceHandlerTestAnnounceHandler1 : public ajn::services::AnnounceHandler {

  public:

    virtual void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData) {
        announceHandler1Flag = true;
    }
};

class AnnounceHandlerTestAnnounceHandler2 : public ajn::services::AnnounceHandler {

  public:

    virtual void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData) {
        announceHandler2Flag = true;
    }
};

class AnnounceHandlerTestAnnounceHandler3 : public ajn::services::AnnounceHandler {

  public:

    virtual void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData) {
        announceHandler3Flag = true;
    }
};

TEST_F(AnnounceHandlerTest, MultipleAnnounceHandlers)
{
    QStatus status;
    announceHandler1Flag = false;
    announceHandler2Flag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceName);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AnnounceHandlerTestAnnounceHandler1 announceHandler1;

    const char* interfaces[1];
    interfaces[0] = ifaceName.c_str();

    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler1,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    AnnounceHandlerTestAnnounceHandler2 announceHandler2;

    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler2,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for first Announce Signal
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler1Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    //Wait for a maximum of 10 additional sec for second Announce Signal
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler2Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandler1Flag);
    ASSERT_TRUE(announceHandler2Flag);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler1,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler2,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AnnounceHandlerTest, MultipleAnnounceHandlersUnregister)
{
    QStatus status;
    announceHandler1Flag = false;
    announceHandler2Flag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceName);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AnnounceHandlerTestAnnounceHandler1 announceHandler1;

    const char* interfaces[1];
    interfaces[0] = ifaceName.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler1,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    AnnounceHandlerTestAnnounceHandler2 announceHandler2;

    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler2,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for first Announce Signal
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler1Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    //Wait for a maximum of 10 additional sec for second Announce Signal
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler2Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandler1Flag);
    ASSERT_TRUE(announceHandler2Flag);

    announceHandler1Flag = false;
    announceHandler2Flag = false;

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler1,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for second Announce Signal
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler2Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_FALSE(announceHandler1Flag);
    ASSERT_TRUE(announceHandler2Flag);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler2,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AnnounceHandlerTest, MultipleAnnounceHandlersUnregisterAll)
{
    QStatus status;
    announceHandler1Flag = false;
    announceHandler2Flag = false;
    announceHandler3Flag = false;

    qcc::GUID128 guid;
    qcc::String ifaceName = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceName);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AnnounceHandlerTestAnnounceHandler1 announceHandler1;

    const char* interfaces[1];
    interfaces[0] = ifaceName.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler1,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    AnnounceHandlerTestAnnounceHandler2 announceHandler2;

    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler2,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    AnnounceHandlerTestAnnounceHandler3 announceHandler3;

    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler3,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for first Announce Signal
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler1Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    //Wait for a maximum of 5 additional sec for second Announce Signal
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceHandler2Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    //Wait for a maximum of 5 additional sec for third Announce Signal
    for (int msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (announceHandler3Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandler1Flag);
    ASSERT_TRUE(announceHandler2Flag);
    ASSERT_TRUE(announceHandler3Flag);

    announceHandler1Flag = false;
    announceHandler2Flag = false;
    announceHandler3Flag = false;

    // Unregister all of the AnnounceHandlers
    AnnouncementRegistrar::UnRegisterAllAnnounceHandlers(clientBus);

    // Reregister the second announceHandler using this so we can verify that the
    // AnnounceHandlers are still called
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler2,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for second Announce Signal
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler2Flag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_FALSE(announceHandler1Flag);
    ASSERT_TRUE(announceHandler2Flag);
    ASSERT_FALSE(announceHandler3Flag);

    AnnouncementRegistrar::UnRegisterAllAnnounceHandlers(clientBus);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AnnounceHandlerTest, MatchMultiplInterfaces)
{
    QStatus status;
    announceHandlerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.a";
    ifaceNames[1] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.b";
    ifaceNames[2] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.c";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceNames[0]);
    object_interfaces.push_back(ifaceNames[1]);
    object_interfaces.push_back(ifaceNames[2]);

    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MyAnnounceHandler announceHandler;

    const char* interfaces[3];
    interfaces[0] = ifaceNames[0].c_str();
    interfaces[1] = ifaceNames[1].c_str();
    interfaces[2] = ifaceNames[2].c_str();

    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandlerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandlerFlag);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

/*
 * we add multiple interfaces but we are only interested in a subset of the
 * interfaces that are registered.
 */
TEST_F(AnnounceHandlerTest, MatchMultiplInterfacesSubSetHandler)
{
    QStatus status;
    announceHandlerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceNames[6];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.a";
    ifaceNames[1] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.b";
    ifaceNames[2] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.c";
    ifaceNames[3] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.d";
    ifaceNames[4] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.e";
    ifaceNames[5] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.f";

    std::vector<qcc::String> object_interfaces;
    for (size_t i = 0; i < 6; ++i) {
        object_interfaces.push_back(ifaceNames[i]);
    }


    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MyAnnounceHandler announceHandler;

    const char* interfaces[2];
    interfaces[0] = ifaceNames[1].c_str();
    interfaces[1] = ifaceNames[2].c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandlerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandlerFlag);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

/*
 * The order of the interfaces as they are listed in the object description do
 * not have to match the order of the interfaces that are given when calling
 * RegisterAnnounceHandler.
 */
TEST_F(AnnounceHandlerTest, MatchMultiplInterfacesRegisterInDifferentOrder)
{
    QStatus status;
    announceHandlerFlag = false;

    qcc::GUID128 guid;
    qcc::String ifaceNames[6];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.a";
    ifaceNames[1] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.b";
    ifaceNames[2] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.c";
    ifaceNames[3] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.d";
    ifaceNames[4] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.e";
    ifaceNames[5] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.f";

    std::vector<qcc::String> object_interfaces;
    for (size_t i = 0; i < 6; ++i) {
        object_interfaces.push_back(ifaceNames[i]);
    }

    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MyAnnounceHandler announceHandler;

    const char* interfaces[6];
    interfaces[0] = ifaceNames[3].c_str();
    interfaces[1] = ifaceNames[0].c_str();
    interfaces[2] = ifaceNames[5].c_str();
    interfaces[3] = ifaceNames[2].c_str();
    interfaces[4] = ifaceNames[1].c_str();
    interfaces[5] = ifaceNames[4].c_str();

    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandlerFlag) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(announceHandlerFlag);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

class WildCardAnnounceHandler : public ajn::services::AnnounceHandler {

  public:

    WildCardAnnounceHandler() : announceHandlerCount(0) { }
    virtual void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData) {
        announceHandlerCount++;
    }
    uint32_t announceHandlerCount;
};

TEST_F(AnnounceHandlerTest, WildCardInterfaceMatching)
{
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.a";
    ifaceNames[1] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.b";
    ifaceNames[2] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.c";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceNames[0]);
    object_interfaces.push_back(ifaceNames[1]);
    object_interfaces.push_back(ifaceNames[2]);

    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    WildCardAnnounceHandler announceHandler;

    qcc::String wildCard = "o" + guid.ToShortString() + ".test.*";
    const char* interfaces[1];
    interfaces[0] = wildCard.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler.announceHandlerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1U, announceHandler.announceHandlerCount);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

/*
 * its unknown at this time if this is a a usage of the wildcard matching that
 * is intended.  However, we know that placing the * in the middle of the match
 * rule works.
 */
TEST_F(AnnounceHandlerTest, WildCardInterfaceMatching2)
{
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.a.AnnounceHandlerTest";
    ifaceNames[1] = "o" + guid.ToShortString() + ".test.b.AnnounceHandlerTest";
    ifaceNames[2] = "o" + guid.ToShortString() + ".test.c.AnnounceHandlerTest";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceNames[0]);
    object_interfaces.push_back(ifaceNames[1]);
    object_interfaces.push_back(ifaceNames[2]);

    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    WildCardAnnounceHandler announceHandler;

    qcc::String wildCard = "o" + guid.ToShortString() + ".test.*.AnnounceHandlerTest";
    const char* interfaces[1];
    interfaces[0] = wildCard.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler.announceHandlerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1U, announceHandler.announceHandlerCount);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AnnounceHandlerTest, MultipleWildCardInterfaceMatching)
{
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.a";
    ifaceNames[1] = "o" + guid.ToShortString() + ".foo.AnnounceHandlerTest.b";
    ifaceNames[2] = "o" + guid.ToShortString() + ".foo.AnnounceHandlerTest.c";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceNames[0]);
    object_interfaces.push_back(ifaceNames[1]);
    object_interfaces.push_back(ifaceNames[2]);

    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    WildCardAnnounceHandler announceHandler;

    qcc::String wildCard = "o" + guid.ToShortString() + ".test.*";
    qcc::String wildCard2 = "o" + guid.ToShortString() + ".foo.*";
    const char* interfaces[2];
    interfaces[0] = wildCard.c_str();
    interfaces[1] = wildCard2.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler.announceHandlerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1U, announceHandler.announceHandlerCount);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AnnounceHandlerTest, MixedWildCardNonWildCardInterfaceMatching)
{
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[3];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.a";
    ifaceNames[1] = "o" + guid.ToShortString() + ".foo.AnnounceHandlerTest.b";
    ifaceNames[2] = "o" + guid.ToShortString() + ".foo.AnnounceHandlerTest.c";

    std::vector<qcc::String> object_interfaces;
    object_interfaces.push_back(ifaceNames[0]);
    object_interfaces.push_back(ifaceNames[1]);
    object_interfaces.push_back(ifaceNames[2]);

    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test", object_interfaces);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    WildCardAnnounceHandler announceHandler;

    qcc::String wildCard = "o" + guid.ToShortString() + ".foo.*";
    const char* interfaces[2];
    interfaces[0] = ifaceNames[0].c_str();
    interfaces[1] = wildCard.c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler.announceHandlerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_EQ(1U, announceHandler.announceHandlerCount);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

//ASACORE-651
class RemoveObjectDescriptionAnnounceHandler : public ajn::services::AnnounceHandler {

  public:
    RemoveObjectDescriptionAnnounceHandler(const char* objToBeRemoved) : announceHandlerCount(0), toRemove(objToBeRemoved) { }
    void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                  const AboutData& aboutData) {
        if (announceHandlerCount == 0) {
            EXPECT_NE(objectDescs.end(), objectDescs.find("/org/alljoyn/test/a"));
            EXPECT_NE(objectDescs.end(), objectDescs.find("/org/alljoyn/test/b"));
        } else {
            if (toRemove == "/org/alljoyn/test/b") {
                EXPECT_NE(objectDescs.end(), objectDescs.find("/org/alljoyn/test/a"));
                EXPECT_EQ(objectDescs.end(), objectDescs.find("/org/alljoyn/test/b"));
            } else {
                EXPECT_EQ(objectDescs.end(), objectDescs.find("/org/alljoyn/test/a"));
                EXPECT_NE(objectDescs.end(), objectDescs.find("/org/alljoyn/test/b"));
            }
        }
        announceHandlerCount++;
    }

    uint32_t announceHandlerCount;
    qcc::String toRemove;
};

TEST_F(AnnounceHandlerTest, RemoveObjectDescriptionAnnouncement)
{
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[2];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.a";
    ifaceNames[1] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.b";

    std::vector<qcc::String> object_interfaces1;
    object_interfaces1.push_back(ifaceNames[0]);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test/a", object_interfaces1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    std::vector<qcc::String> object_interfaces2;
    object_interfaces2.push_back(ifaceNames[1]);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test/b", object_interfaces2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    RemoveObjectDescriptionAnnounceHandler announceHandler("/org/alljoyn/test/b");

    const char* interfaces[1];
    interfaces[0] = ifaceNames[0].c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler.announceHandlerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_EQ(1U, announceHandler.announceHandlerCount);

    status = AboutServiceApi::getInstance()->RemoveObjectDescription("/org/alljoyn/test/b", object_interfaces2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler.announceHandlerCount == 2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_EQ(2U, announceHandler.announceHandlerCount);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AnnounceHandlerTest, RemoveLastInterestingObject)
{
    QStatus status;

    qcc::GUID128 guid;
    qcc::String ifaceNames[2];
    ifaceNames[0] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.a";
    ifaceNames[1] = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.b";

    std::vector<qcc::String> object_interfaces1;
    object_interfaces1.push_back(ifaceNames[0]);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test/a", object_interfaces1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    std::vector<qcc::String> object_interfaces2;
    object_interfaces2.push_back(ifaceNames[1]);
    status = AboutServiceApi::getInstance()->AddObjectDescription("/org/alljoyn/test/b", object_interfaces2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // receive
    BusAttachment clientBus("Receive Announcement client Test", true);
    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    RemoveObjectDescriptionAnnounceHandler announceHandler("/org/alljoyn/test/a");

    const char* interfaces[1];
    interfaces[0] = ifaceNames[0].c_str();
    AnnouncementRegistrar::RegisterAnnounceHandler(clientBus, announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));


    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler.announceHandlerCount == 1) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_EQ(1U, announceHandler.announceHandlerCount);

    status = AboutServiceApi::getInstance()->RemoveObjectDescription("/org/alljoyn/test/a", object_interfaces1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = AboutServiceApi::getInstance()->Announce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Wait for a maximum of 10 sec for the Announce Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_TIME) {
        if (announceHandler.announceHandlerCount == 2) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_EQ(2U, announceHandler.announceHandlerCount);

    AnnouncementRegistrar::UnRegisterAnnounceHandler(clientBus, announceHandler,
                                                     interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
