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
        status = aboutData.SetSupportUrl("http://www.alljoyn.org");
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
    AboutObjTestAboutListener() : announceListenerFlag(false), busName(), port(0) { }

    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const AboutObjectDescription& objectDescription, const AboutData& aboutData) {
        EXPECT_FALSE(announceListenerFlag) << "We don't expect the flag to already be true when an AnnouceSignal is received.";
        this->busName = qcc::String(busName);
        this->port = port;
        announceListenerFlag = true;
    }
    bool announceListenerFlag;
    qcc::String busName;
    SessionPort port;
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
    clientBus.RegisterAboutListener(aboutListener, ifaceName.c_str());

    AboutObj aboutObj(*serviceBus);
    aboutObj.Announce(port, aboutData);

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
