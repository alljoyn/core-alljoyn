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
 * same test on multiple platforms as one time.  Since the names announced could
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

class AboutInterfaceTestSessionPortListener : public SessionPortListener {
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

/*
 * A PropertyStoreImplementation with all of the AboutData fields filled out.
 * The AppId and DeviceId are generated at random using the qcc::GUID function
 */
class AboutInterfaceTestPropertyStoreImpl {
  public:
    AboutInterfaceTestPropertyStoreImpl() {
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
    ~AboutInterfaceTestPropertyStoreImpl() {
        delete appId;
        delete deviceId;
    }
    AboutPropertyStoreImpl propertyStore;
    qcc::GUID128* appId;
    qcc::GUID128* deviceId;

};

class AboutInterfaceTest : public testing::Test {
  public:
    BusAttachment* g_msgBus;

    virtual void SetUp() {

        QStatus status;

        serviceBus = new BusAttachment("announceHandlerTest", true);
        status = serviceBus->Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus->Connect();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        propStore = new AboutInterfaceTestPropertyStoreImpl();
        AboutServiceApi::Init(*serviceBus, propStore->propertyStore);
        EXPECT_TRUE(AboutServiceApi::getInstance() != NULL);

        SessionPort port = 25;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        AboutInterfaceTestSessionPortListener listener;
        status = serviceBus->BindSessionPort(port, opts, listener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = AboutServiceApi::getInstance()->Register(port);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = serviceBus->RegisterBusObject(*AboutServiceApi::getInstance());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        AboutServiceApi::DestroyInstance();
        serviceBus->Stop();
        serviceBus->Join();
        if (serviceBus) {
            BusAttachment* deleteMe = serviceBus;
            serviceBus = NULL;
            delete deleteMe;
        }
        delete propStore;
    }

    BusAttachment* serviceBus;
    AboutInterfaceTestPropertyStoreImpl* propStore;
};

static bool announceHandlerFlag = false;

class JoinAnnounceHandler : public ajn::services::AnnounceHandler {
  public:
    virtual void Announce(unsigned short version, unsigned short port, const char* busName,
                          const ObjectDescriptions& objectDescs, const AboutData& aboutData)
    {
        QCC_UNUSED(objectDescs);
        EXPECT_EQ(1, version);
        EXPECT_EQ(25, port);
        char* deviceName;
        aboutData.find("DeviceName")->second.Get("s", &deviceName);
        EXPECT_STREQ("AnnounceHandler Unit Test framework", deviceName);
        char* defaultLanguage;
        aboutData.find("DefaultLanguage")->second.Get("s", &defaultLanguage);
        EXPECT_STREQ("en", defaultLanguage);
        char* appName;
        aboutData.find("AppName")->second.Get("s", &appName);
        EXPECT_STREQ("AnnounceHander Unit Test", appName);
        announcePort = port;
        announceBusName = busName;
        announceHandlerFlag = true;
    }

    unsigned short announcePort;
    qcc::String announceBusName;
};

TEST_F(AboutInterfaceTest, JoinAnnouncement)
{
    QStatus status;
    announceHandlerFlag = false;
    qcc::GUID128 guid;
    qcc::String ifaceName = "o" + guid.ToShortString() + ".test.AnnounceHandlerTest.Join";
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

    JoinAnnounceHandler announceHandler;

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
