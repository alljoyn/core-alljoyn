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
#include <alljoyn/AboutObj.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutProxy.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AboutProxy.h>
#include <alljoyn/BusAttachment.h>

#include <qcc/Thread.h>
#include <qcc/GUID.h>
#include <map>
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
using namespace std;
using namespace qcc;

static const char* ENGLISH_TAG = "en";
static const char* SPANISH_TAG = "es";
static const char* FRENCH_TAG = "fr";

std::pair<const char*, const char*> ENGLISH_DATA[] = {
    std::make_pair(AboutData::DEVICE_NAME, "Dish Washer"),
    std::make_pair(AboutData::APP_NAME, "Controller"),
    std::make_pair(AboutData::MANUFACTURER, "Alliance"),
    std::make_pair(AboutData::DESCRIPTION, "Smart dish washer"),
    std::make_pair(AboutData::MODEL_NUMBER, "HDW-1111"),
    std::make_pair(AboutData::DATE_OF_MANUFACTURE, "2014-20-24"),
    std::make_pair(AboutData::SOFTWARE_VERSION, "0.2.2")
};

std::pair<const char*, const char*> SPANISH_DATA[] = {
    std::make_pair(AboutData::DEVICE_NAME, "dispositivo"),
    std::make_pair(AboutData::APP_NAME, "aplicacion"),
    std::make_pair(AboutData::MANUFACTURER, "manufactura"),
    std::make_pair(AboutData::DESCRIPTION, "Una descripcion poetica de esta aplicacion"),
    std::make_pair(AboutData::MODEL_NUMBER, "HDW-1111"),
    std::make_pair(AboutData::DATE_OF_MANUFACTURE, "2014-20-24"),
    std::make_pair(AboutData::SOFTWARE_VERSION, "0.2.2")
};

// About English data which is fixed,
// appId and deviceId are randomly generated
static map<const char*, const char*> FixedEnglishData(ENGLISH_DATA,
                                                      ENGLISH_DATA + sizeof(ENGLISH_DATA) / sizeof(ENGLISH_DATA[0]));

// About Spanish data is fixed,
static map<const char*, const char*> FixedSpanishData(SPANISH_DATA,
                                                      SPANISH_DATA + sizeof(SPANISH_DATA) / sizeof(SPANISH_DATA[0]));

class AboutProxyTestSessionPortListener : public SessionPortListener {
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
};


class AboutProxyTest : public testing::Test {
  public:
    AboutProxyTest() :
        listener(),
        serviceBus(NULL),
        aboutEnglishData(ENGLISH_TAG),
        aboutSpanishData(SPANISH_TAG),
        port(25)
    {
    }

    virtual void SetUp() {

        QStatus status;

        serviceBus = new BusAttachment("AboutProxyTestServiceBus", true);
        status = serviceBus->Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus->Connect();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


        // Initialize english data
        setUpEnglishData();
        aboutEnglishData.SetSupportedLanguage(SPANISH_TAG);

        // Initialize spanish data
        setUpSpanishData();
        aboutSpanishData.SetSupportedLanguage(ENGLISH_TAG);

        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
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

    AboutProxyTestSessionPortListener listener;
    BusAttachment* serviceBus;
    AboutData aboutEnglishData;
    AboutData aboutSpanishData;
    SessionPort port;

  private:
    void setUpEnglishData() {
        QStatus status = ER_OK;

        // Setup the about English data
        qcc::GUID128 appId;
        status = aboutEnglishData.SetAppId(appId.GetBytes(), qcc::GUID128::SIZE);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        qcc::GUID128 deviceId;
        status = aboutEnglishData.SetDeviceId(deviceId.ToString().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = aboutEnglishData.SetDeviceName(FixedEnglishData[AboutData::DEVICE_NAME]);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = aboutEnglishData.SetAppName(FixedEnglishData[AboutData::APP_NAME], ENGLISH_TAG);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = aboutEnglishData.SetManufacturer(FixedEnglishData[AboutData::MANUFACTURER], ENGLISH_TAG);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = aboutEnglishData.SetModelNumber(FixedEnglishData[AboutData::MODEL_NUMBER]);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = aboutEnglishData.SetDescription(FixedEnglishData[AboutData::DESCRIPTION], ENGLISH_TAG);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = aboutEnglishData.SetDateOfManufacture(FixedEnglishData[AboutData::DATE_OF_MANUFACTURE]);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = aboutEnglishData.SetSoftwareVersion(FixedEnglishData[AboutData::SOFTWARE_VERSION]);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        ASSERT_TRUE(aboutEnglishData.IsValid()) << " Failed to setup about English data!";
    }

    void setUpSpanishData() {
        QStatus status;
        // Setup the about Spanish data
        uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        status = aboutSpanishData.SetAppId(appId, 16);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutSpanishData.SetDeviceId("fakeID");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        EXPECT_FALSE(aboutSpanishData.IsValid());

        status = aboutSpanishData.SetAppName(FixedSpanishData[AboutData::APP_NAME], "es");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutSpanishData.SetDeviceName(FixedSpanishData[AboutData::DEVICE_NAME], "es");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutSpanishData.SetManufacturer(FixedSpanishData[AboutData::MANUFACTURER], "es");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutSpanishData.SetDescription(FixedSpanishData[AboutData::DESCRIPTION], "es");
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(aboutSpanishData.IsValid());

        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutSpanishData.SetModelNumber(FixedSpanishData[AboutData::MODEL_NUMBER]);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = aboutSpanishData.SetSoftwareVersion(FixedSpanishData[AboutData::SOFTWARE_VERSION]);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(aboutSpanishData.IsValid());
    }
};

class AboutProxyTestBusObject : public BusObject {
  public:
    AboutProxyTestBusObject(BusAttachment& bus, const char* path, qcc::String& interfaceName, bool announce = true)
        : BusObject(path), isAnnounce(announce) {
        const InterfaceDescription* iface = bus.GetInterface(interfaceName.c_str());
        EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceName.c_str();
        if (iface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL when it should not have been.\n", interfaceName.c_str());
            return;
        }

        if (isAnnounce) {
            printf("Interface %s is announced. \n", interfaceName.c_str());
            AddInterface(*iface, ANNOUNCED);
        } else {
            printf("Interface %s is NOT announced. \n", interfaceName.c_str());
            AddInterface(*iface, UNANNOUNCED);
        }
    }

  private:
    bool isAnnounce;
};

class AboutProxyTestAboutListener : public AboutListener {
  public:
    AboutProxyTestAboutListener() : announceListenerFlag(false), busName(), port(0) { }

    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData) {
        EXPECT_FALSE(announceListenerFlag) << "We don't expect the flag to already be true when an AnnouceSignal is received.";
        this->busName = qcc::String(busName);
        this->version = version;
        this->port = port;
        announceListenerFlag = true;
    }
    bool announceListenerFlag;
    qcc::String busName;
    SessionPort port;
    uint16_t version;
};

TEST_F(AboutProxyTest, GetObjectDescription) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceName = "test.about.a" + interface_rand_string.ToString();
    qcc::String interface = "<node>"
                            "<interface name='" + ifaceName + "'>"
                            "</interface>"
                            "</node>";

    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutProxyTestBusObject busObject(*serviceBus, "/test/alljoyn/AboutProxy", ifaceName, true);
    status = serviceBus->RegisterBusObject(busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    BusAttachment clientBus("AboutProxyTestClient", true);

    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutProxyTestAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);
    aboutObj.Announce(port, aboutEnglishData);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerFlag == true) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(aboutListener.announceListenerFlag) << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(serviceBus->GetUniqueName().c_str(), aboutListener.busName.c_str());
    EXPECT_EQ(port, aboutListener.port);

    SessionId sessionId;
    SessionOpts opts;
    status = clientBus.JoinSession(aboutListener.busName.c_str(), aboutListener.port, NULL, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutProxy proxy(clientBus, aboutListener.busName.c_str(), sessionId);

    uint16_t aboutVersion;
    status = proxy.GetVersion(aboutVersion);
    EXPECT_EQ(ER_OK, status) << "  GetVersion Status: " << QCC_StatusText(status);

    EXPECT_EQ(aboutListener.version, aboutVersion) << "Version mismatch!";

    //
    MsgArg objDescriptionArg;
    status = proxy.GetObjectDescription(objDescriptionArg);
    EXPECT_EQ(ER_OK, status) << "  GetObjectDescription Status: " << QCC_StatusText(status);

    // Parse object descriptions
    AboutObjectDescription aod;
    aod.CreateFromMsgArg(objDescriptionArg);

    size_t numPaths = aod.GetPaths(NULL, 0);

    ASSERT_EQ(1u, numPaths);
    const char** paths = new const char*[numPaths];
    aod.GetPaths(paths, numPaths);
    // Object path must match sender
    EXPECT_TRUE(strcmp(paths[0], "/test/alljoyn/AboutProxy") == 0) << paths[0];

    size_t numInterfaces = aod.GetInterfaces(paths[0], NULL, 0);

    EXPECT_EQ(1u, numInterfaces);
    if (numInterfaces == 1u) {
        const char** supportedInterfaces = new const char*[numInterfaces];
        aod.GetInterfaces(paths[0], supportedInterfaces, numInterfaces);
        EXPECT_STREQ(ifaceName.c_str(), supportedInterfaces[0]) << "Interface mismatch!";
        delete [] supportedInterfaces;
    }
    delete [] paths;

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutProxyTest, GetAboutdata_English) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceName = "test.about.b" + interface_rand_string.ToString();
    qcc::String interface = "<node>"
                            "<interface name='" + ifaceName + "'>"
                            "</interface>"
                            "</node>";

    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutProxyTestBusObject busObject(*serviceBus, "/test/alljoyn/English", ifaceName, true);
    status = serviceBus->RegisterBusObject(busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    BusAttachment clientBus("AboutProxyTestClient", true);

    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutProxyTestAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);
    aboutObj.Announce(port, aboutEnglishData);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerFlag == true) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(aboutListener.announceListenerFlag) << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(serviceBus->GetUniqueName().c_str(), aboutListener.busName.c_str());
    EXPECT_EQ(port, aboutListener.port);

    SessionId sessionId;
    SessionOpts opts;
    status = clientBus.JoinSession(aboutListener.busName.c_str(), aboutListener.port, NULL, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutProxy proxy(clientBus, aboutListener.busName.c_str(), sessionId);

    uint16_t aboutVersion;
    status = proxy.GetVersion(aboutVersion);
    EXPECT_EQ(ER_OK, status) << "  GetVersion Status: " << QCC_StatusText(status);

    EXPECT_EQ(aboutListener.version, aboutVersion) << "Version mismatch!";

    // AboutData for ENGLISH
    MsgArg dataArg;
    status = proxy.GetAboutData(ENGLISH_TAG, dataArg);

    EXPECT_EQ(ER_OK, status) << "  GetAboutData Status: " << QCC_StatusText(status);

    AboutData aboutData(ENGLISH_TAG);

    // about data for English
    aboutData.CreatefromMsgArg(dataArg);

    char* appName;
    status = aboutData.GetAppName(&appName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedEnglishData[AboutData::APP_NAME], appName);

    char* deviceName;
    status = aboutData.GetDeviceName(&deviceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedEnglishData[AboutData::DEVICE_NAME], deviceName);

    char* dateOfManufacture;
    status = aboutData.GetDateOfManufacture(&dateOfManufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedEnglishData[AboutData::DATE_OF_MANUFACTURE], dateOfManufacture);

    char* manufacture;
    status = aboutData.GetManufacturer(&manufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedEnglishData[AboutData::MANUFACTURER], manufacture);

    char* description;
    status = aboutData.GetDescription(&description);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedEnglishData[AboutData::DESCRIPTION], description);

    char* modelNumber;
    status = aboutData.GetModelNumber(&modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedEnglishData[AboutData::MODEL_NUMBER], modelNumber);

    char* softwareVersion;
    status = aboutData.GetSoftwareVersion(&softwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedEnglishData[AboutData::SOFTWARE_VERSION], softwareVersion);

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutProxyTest, GetAboutdata_Spanish) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceName = "test.about.c" + interface_rand_string.ToString();
    qcc::String interface = "<node>"
                            "<interface name='" + ifaceName + "'>"
                            "</interface>"
                            "</node>";

    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutProxyTestBusObject busObject(*serviceBus, "/test/alljoyn/Spanish", ifaceName, true);
    status = serviceBus->RegisterBusObject(busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    BusAttachment clientBus("AboutProxyTestClient", true);

    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutProxyTestAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);
    aboutObj.Announce(port, aboutSpanishData);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerFlag == true) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(aboutListener.announceListenerFlag) << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(serviceBus->GetUniqueName().c_str(), aboutListener.busName.c_str());
    EXPECT_EQ(port, aboutListener.port);

    SessionId sessionId;
    SessionOpts opts;
    status = clientBus.JoinSession(aboutListener.busName.c_str(), aboutListener.port, NULL, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutProxy proxy(clientBus, aboutListener.busName.c_str(), sessionId);

    uint16_t aboutVersion;
    status = proxy.GetVersion(aboutVersion);
    EXPECT_EQ(ER_OK, status) << "  GetVersion Status: " << QCC_StatusText(status);

    EXPECT_EQ(aboutListener.version, aboutVersion) << "Version mismatch!";

    // AboutData for Spanish
    MsgArg dataArg;
    status = proxy.GetAboutData(SPANISH_TAG, dataArg);

    EXPECT_EQ(ER_OK, status) << "  GetAboutData Status: " << QCC_StatusText(status);

    AboutData aboutData(SPANISH_TAG);

    // about data for Spanish
    aboutData.CreatefromMsgArg(dataArg);

    char* appName;
    status = aboutData.GetAppName(&appName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedSpanishData[AboutData::APP_NAME], appName);

    char* deviceName;
    status = aboutData.GetDeviceName(&deviceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedSpanishData[AboutData::DEVICE_NAME], deviceName);

    char* manufacture;
    status = aboutData.GetManufacturer(&manufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedSpanishData[AboutData::MANUFACTURER], manufacture);

    char* description;
    status = aboutData.GetDescription(&description);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(FixedSpanishData[AboutData::DESCRIPTION], description);

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

//ASACORE-958
TEST_F(AboutProxyTest, GetAboutdata_UnsupportedLanguage) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceName = "test.about.d" + interface_rand_string.ToString();
    qcc::String interface = "<node>"
                            "<interface name='" + ifaceName + "'>"
                            "</interface>"
                            "</node>";

    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutProxyTestBusObject busObject(*serviceBus, "/test/alljoyn/Unsupported", ifaceName, true);
    status = serviceBus->RegisterBusObject(busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    BusAttachment clientBus("AboutProxyTestClient", true);

    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutProxyTestAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);
    aboutObj.Announce(port, aboutEnglishData);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerFlag == true) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    ASSERT_TRUE(aboutListener.announceListenerFlag) << "The announceListenerFlag must be true to continue this test.";
    EXPECT_STREQ(serviceBus->GetUniqueName().c_str(), aboutListener.busName.c_str());
    EXPECT_EQ(port, aboutListener.port);

    SessionId sessionId;
    SessionOpts opts;
    status = clientBus.JoinSession(aboutListener.busName.c_str(), aboutListener.port, NULL, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutProxy proxy(clientBus, aboutListener.busName.c_str(), sessionId);

    uint16_t aboutVersion;
    status = proxy.GetVersion(aboutVersion);
    EXPECT_EQ(ER_OK, status) << "  GetVersion Status: " << QCC_StatusText(status);

    EXPECT_EQ(aboutListener.version, aboutVersion) << "Version mismatch!";

    // AboutData for unsupported language- French
    MsgArg dataArg;
    status = proxy.GetAboutData(FRENCH_TAG, dataArg);
    EXPECT_EQ(ER_LANGUAGE_NOT_SUPPORTED, status) << "  GetAboutData Status: " << QCC_StatusText(status);

    status = clientBus.CancelWhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.UnregisterAboutListener(aboutListener);

    status = clientBus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
