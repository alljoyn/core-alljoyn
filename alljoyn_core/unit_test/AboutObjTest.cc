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
#include <qcc/String.h>
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

class AboutObjTestSessionPortListener : public SessionPortListener {
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
};


class AboutObjTest : public testing::Test {
  public:
    AboutObjTest() : listener(), serviceBus(NULL), aboutData("en"), port(25) { }
    virtual void SetUp() {

        QStatus status;

        serviceBus = new BusAttachment("AboutObjTestServiceBus", true);
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
        status = aboutData.SetSupportUrl("http://www.example.com");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (!aboutData.IsValid()) {
            printf("failed to setup about data.\n");
        }

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

    AboutObjTestSessionPortListener listener;
    BusAttachment* serviceBus;
    AboutData aboutData;
    SessionPort port;
};

class AboutObjTestBusObject : public BusObject {
  public:
    AboutObjTestBusObject(BusAttachment& bus, const char* path, qcc::String& interfaceName)
        : BusObject(path) {
        const InterfaceDescription* iface = bus.GetInterface(interfaceName.c_str());
        EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceName;
        if (iface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL when it should not have been.", interfaceName.c_str());
            return;
        }
        AddInterface(*iface, ANNOUNCED);

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { iface->GetMember("Echo"), static_cast<MessageReceiver::MethodHandler>(&AboutObjTestBusObject::Echo) }
        };
        QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
//        QStatus status = AddMethodHandler(iface->GetMember("Echo"), static_cast<MessageReceiver::MethodHandler>(&AboutObjTestBusObject::Echo));
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    }
    void Echo(const InterfaceDescription::Member* member, Message& msg) {
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "Echo: Error sending reply,  Actual Status: " << QCC_StatusText(status);
    }
};

class AboutObjTestAboutListener : public AboutListener {
  public:
    AboutObjTestAboutListener() : announceListenerFlag(false), busName(), port(0), version(0) { }

    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData) {
        EXPECT_FALSE(announceListenerFlag) << "We don't expect the flag to already be true when an AnnouceSignal is received.";
        this->busName = qcc::String(busName);
        this->port = port;
        this->version = version;
        announceListenerFlag = true;
    }
    bool announceListenerFlag;
    qcc::String busName;
    SessionPort port;
    uint16_t version;
};

TEST_F(AboutObjTest, Announce) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceName = "test.about.a" + interface_rand_string.ToString();
    qcc::String interface = "<node>"
                            "<interface name='" + ifaceName + "'>"
                            "  <method name='Echo'>"
                            "    <arg name='out_arg' type='s' direction='in' />"
                            "    <arg name='return_arg' type='s' direction='out' />"
                            "  </method>"
                            "</interface>"
                            "</node>";

    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutObjTestBusObject busObject(*serviceBus, "/test/alljoyn/AboutObj", ifaceName);
    status = serviceBus->RegisterBusObject(busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    BusAttachment clientBus("AboutObjTestClient", true);

    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjTestAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);
    status = aboutObj.Announce(port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
    ProxyBusObject proxy(clientBus, aboutListener.busName.c_str(), "/test/alljoyn/AboutObj", sessionId, false);
    status = proxy.ParseXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << "\n" << interface;
    EXPECT_TRUE(proxy.ImplementsInterface(ifaceName.c_str())) << interface << "\n" << ifaceName;

    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(clientBus);
    status = proxy.MethodCall(ifaceName.c_str(), "Echo", &arg, static_cast<size_t>(1), replyMsg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " " << replyMsg->GetArg(0)->v_string.str;

    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    clientBus.Stop();
    clientBus.Join();
}

TEST_F(AboutObjTest, AnnounceSessionPortNotBound) {
    QStatus status = ER_FAIL;
    AboutObj aboutObj(*serviceBus);
    //the SessionPort 5154 is not bound so should return ER_ABOUT_SESSIONPORT_NOT_BOUND error.
    status = aboutObj.Announce(static_cast<SessionPort>(5154), aboutData);
    EXPECT_EQ(ER_ABOUT_SESSIONPORT_NOT_BOUND, status) << "  Actual Status: " << QCC_StatusText(status);
}

// ASACORE-1006
TEST_F(AboutObjTest, AnnounceMissingRequiredField) {
    QStatus status = ER_FAIL;
    AboutObj aboutObj(*serviceBus);

    AboutData badAboutData;
    //DefaultLanguage and other required fields are missing
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status) << "  Actual Status: " << QCC_StatusText(status);

    status = badAboutData.SetDefaultLanguage("en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status) << "  Actual Status: " << QCC_StatusText(status);

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = badAboutData.SetAppId(appId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //DeviceId and other required fields are missing
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status) << "  Actual Status: " << QCC_StatusText(status);

    status = badAboutData.SetDeviceId("fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //AppName and other required fields are missing
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status) << "  Actual Status: " << QCC_StatusText(status);

    status = badAboutData.SetAppName("Application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Manufacturer and other required fields are missing
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status) << "  Actual Status: " << QCC_StatusText(status);

    status = badAboutData.SetManufacturer("Manufacturer");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //ModelNumber and other required fields are missing
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status) << "  Actual Status: " << QCC_StatusText(status);

    status = badAboutData.SetModelNumber("123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Description and other required fields are missing
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status) << "  Actual Status: " << QCC_StatusText(status);

    status = badAboutData.SetDescription("A poetic description of this application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //SoftwareVersion missing
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, status) << "  Actual Status: " << QCC_StatusText(status);

    status = badAboutData.SetSoftwareVersion("0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // Now all required fields are set for the default language
    status = aboutObj.Announce(port, badAboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(AboutObjTest, ProxyAccessToAboutObj) {
    QStatus status = ER_FAIL;
    qcc::GUID128 interface_rand_string;
    qcc::String ifaceName = "test.about.a" + interface_rand_string.ToString();
    qcc::String interface = "<node>"
                            "<interface name='" + ifaceName + "'>"
                            "  <method name='Echo'>"
                            "    <arg name='out_arg' type='s' direction='in' />"
                            "    <arg name='return_arg' type='s' direction='out' />"
                            "  </method>"
                            "</interface>"
                            "</node>";

    status = serviceBus->CreateInterfacesFromXml(interface.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutObjTestBusObject busObject(*serviceBus, "/test/alljoyn/AboutObj", ifaceName);
    status = serviceBus->RegisterBusObject(busObject);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    BusAttachment clientBus("AboutObjTestClient", true);

    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjTestAboutListener aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements(ifaceName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus);
    status = aboutObj.Announce(port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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

    AboutProxy aProxy(clientBus, aboutListener.busName.c_str(), sessionId);

    // Call each of the proxy methods
    // GetObjDescription
    // GetAboutData
    // GetVersion
    uint16_t ver;
    status = aProxy.GetVersion(ver);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(aboutListener.version, ver);

    MsgArg aboutArg;
    status = aProxy.GetAboutData("en", aboutArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutData aboutData(aboutArg);

    char* appName;
    status = aboutData.GetAppName(&appName, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Application", appName);

    char* manufacturer;
    status = aboutData.GetManufacturer(&manufacturer, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Manufacturer", manufacturer);

    char* modelNum;
    status = aboutData.GetModelNumber(&modelNum);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("123456", modelNum);

    char* desc;
    status = aboutData.GetDescription(&desc);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("A poetic description of this application", desc);

    char* dom;
    status = aboutData.GetDateOfManufacture(&dom);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("2014-03-24", dom);

    char* softVer;
    status = aboutData.GetSoftwareVersion(&softVer);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("0.1.2", softVer);

    char* hwVer;
    status = aboutData.GetHardwareVersion(&hwVer);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("0.0.1", hwVer);

    char* support;
    status = aboutData.GetSupportUrl(&support);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("http://www.example.com", support);

    // French is an specified language. expect an error
    MsgArg aboutArg_fr;
    status = aProxy.GetAboutData("fr", aboutArg_fr);
    EXPECT_EQ(ER_LANGUAGE_NOT_SUPPORTED, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg objDesc;
    status = aProxy.GetObjectDescription(objDesc);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    AboutObjectDescription aObjDesc(objDesc);
    EXPECT_TRUE(aObjDesc.HasPath("/test/alljoyn/AboutObj"));
    EXPECT_TRUE(aObjDesc.HasInterface(ifaceName.c_str()));

    clientBus.Stop();
    clientBus.Join();
}

class AboutObjTestAboutListener2 : public AboutListener {
  public:
    AboutObjTestAboutListener2() : announceListenerFlag(false),
        aboutObjectPartOfAnnouncement(false), busName(), port(0) { }

    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescription, const MsgArg& aboutData) {
        EXPECT_FALSE(announceListenerFlag) << "We don't expect the flag to already be true when an AnnouceSignal is received.";
        this->busName = qcc::String(busName);
        this->port = port;
        AboutObjectDescription aod;
        QStatus status = aod.CreateFromMsgArg(objectDescription);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        aboutObjectPartOfAnnouncement = aod.HasInterface("org.alljoyn.About");
        announceListenerFlag = true;
    }
    bool announceListenerFlag;
    bool aboutObjectPartOfAnnouncement;
    qcc::String busName;
    SessionPort port;
};

TEST_F(AboutObjTest, AnnounceTheAboutObj) {
    QStatus status = ER_FAIL;

    BusAttachment clientBus("AboutObjTestClient", true);

    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjTestAboutListener2 aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements("org.alljoyn.About");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus, BusObject::ANNOUNCED);
    status = aboutObj.Announce(port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerFlag == true) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_TRUE(aboutListener.announceListenerFlag) << "The announceListenerFlag must be true to continue this test.";
    EXPECT_TRUE(aboutListener.aboutObjectPartOfAnnouncement) << "The org.alljoyn.About interface was not part of the announced object description.";
    EXPECT_STREQ(serviceBus->GetUniqueName().c_str(), aboutListener.busName.c_str());
    EXPECT_EQ(port, aboutListener.port);

    clientBus.Stop();
    clientBus.Join();
}

// The About interface is used for this test however this could be done with
// any valid interface description.
TEST_F(AboutObjTest, SetAnnounceFlag) {
    QStatus status = ER_FAIL;
    AboutObj aboutObj(*serviceBus);

    const InterfaceDescription* aboutIface = serviceBus->GetInterface("org.alljoyn.About");
    ASSERT_TRUE(aboutIface != NULL);


    size_t numIfaces = aboutObj.GetAnnouncedInterfaceNames();
    ASSERT_EQ(static_cast<size_t>(0), numIfaces);

    status = aboutObj.SetAnnounceFlag(aboutIface);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    numIfaces = aboutObj.GetAnnouncedInterfaceNames();
    ASSERT_EQ(static_cast<size_t>(1), numIfaces);
    const char* interfaces[1];

    aboutObj.GetAnnouncedInterfaceNames(interfaces, numIfaces);
    EXPECT_STREQ("org.alljoyn.About", interfaces[0]);

    const InterfaceDescription* dbusIface = serviceBus->GetInterface(org::freedesktop::DBus::InterfaceName);
    status = aboutObj.SetAnnounceFlag(dbusIface);
    EXPECT_EQ(ER_BUS_OBJECT_NO_SUCH_INTERFACE, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutObj.SetAnnounceFlag(aboutIface, BusObject::UNANNOUNCED);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    numIfaces = aboutObj.GetAnnouncedInterfaceNames();
    ASSERT_EQ(static_cast<size_t>(0), numIfaces);
}

TEST_F(AboutObjTest, Unannounce) {
    QStatus status = ER_FAIL;

    BusAttachment clientBus("AboutObjTestClient", true);

    status = clientBus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = clientBus.Connect();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObjTestAboutListener2 aboutListener;
    clientBus.RegisterAboutListener(aboutListener);

    status = clientBus.WhoImplements("org.alljoyn.About");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AboutObj aboutObj(*serviceBus, BusObject::ANNOUNCED);
    status = aboutObj.Announce(port, aboutData);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (uint32_t msec = 0; msec < 5000; msec += WAIT_TIME) {
        if (aboutListener.announceListenerFlag == true) {
            break;
        }
        qcc::Sleep(WAIT_TIME);
    }

    EXPECT_TRUE(aboutListener.announceListenerFlag) << "The announceListenerFlag must be true to continue this test.";
    EXPECT_TRUE(aboutListener.aboutObjectPartOfAnnouncement) << "The org.alljoyn.About interface was not part of the announced object description.";
    EXPECT_STREQ(serviceBus->GetUniqueName().c_str(), aboutListener.busName.c_str());
    EXPECT_EQ(port, aboutListener.port);

    status = aboutObj.Unannounce();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    clientBus.Stop();
    clientBus.Join();
}
