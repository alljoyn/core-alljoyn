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

#include <signal.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/Session.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/Init.h>
#include <alljoyn/Status.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/Translator.h>
#include <alljoyn/TransportMask.h>

#include <qcc/Log.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>

#define QCC_MODULE "ALLJOYN"

using namespace ajn;
using namespace qcc;

class TestBusListener;
static BusAttachment* s_msgBus = NULL;
static TestBusListener* s_busListener = NULL;

static const char* INTERFACE_NAME = "test.alljoyn.example.eventsactionsservice";
static const char* SERVICE_NAME = "test.alljoyn.example.eventsactionsservice";
static const char* SERVICE_PATH = "/example/path";
static SessionPort SERVICE_PORT = 24;
static const char* testAction = "Test Action";
static volatile sig_atomic_t g_interrupt = false;

static const char* xmlWithDescription =
    "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.0//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.0.dtd\">\n"
    "<node>\n"
    "  <node name=\"org\"/>\n"
    "  <interface name=\"test.alljoyn.example.eventsactionsservice\">\n"
    "    <description>This is the interface</description>\n"
    "    <method name='TestAction'>"
    "      <description>This is the test action</description>\n"
    "      <arg name='in_arg' type='s' direction='in' />"
    "      <arg name='out_arg' type='s' direction='out' />"
    "    </method>"
    "    <signal name=\"TestEvent\">"
    "      <description>This is the test event</description>\n"
    "      <arg name=\"str\" type=\"s\"/>"
    "    </signal>"
    "  </interface>\n"
    "</node>\n";

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    g_interrupt = true;
}

class TestBusListener : public BusListener, public SessionPortListener {
  public:
    TestBusListener() { }

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != SERVICE_PORT) {
            printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
            return false;
        }
        printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
               joiner, opts.proximity, opts.traffic, opts.transports);
        return true;
    }
};

class TestBusObject : public BusObject {

  public:
    TestBusObject(BusAttachment& bus, const char* path) : BusObject(path)
    {
        QStatus status = s_msgBus->CreateInterfacesFromXml(xmlWithDescription);
        if (ER_OK != status) {
            QCC_LogError(ER_OK, ("Error while creating the interface"));
        }

        const InterfaceDescription* intf = bus.GetInterface(INTERFACE_NAME);
        AddInterface(*intf, ANNOUNCED);

        SetDescription("en", testAction);

    }
};

static void usage(void)
{
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -?                    = Print this help message\n");
    printf("   -t                    = Advertise over TCP (enables selective advertising)\n");
    printf("   -l                    = Advertise locally (enables selective advertising)\n");
    printf("   -u                    = Advertise over UDP-based ARDP (enables selective advertising)\n");
    printf("\n");
}

int CDECL_CALL main(int argc, char** argv)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_NONE);

    /*Adding Transport Flags*/
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i]) || 0 == strcmp("-?", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-t", argv[i])) {
            opts.transports |= TRANSPORT_TCP;
        } else if (0 == strcmp("-l", argv[i])) {
            opts.transports |= TRANSPORT_LOCAL;
        } else if (0 == strcmp("-u", argv[i])) {
            opts.transports |= TRANSPORT_UDP;
        }
    }

    /* If no transport option was specifie, then make session options very open */
    if (opts.transports == 0) {
        opts.transports = TRANSPORT_ANY;
    }

    QStatus status = AllJoynInit();
    if (ER_OK != status) {
        return 1;
    }
#ifdef ROUTER
    status = AllJoynRouterInit();
    if (ER_OK != status) {
        AllJoynShutdown();
        return 1;
    }
#endif

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    s_msgBus = new BusAttachment("easervice", true);
    if (!s_msgBus) {
        QCC_LogError(ER_FAIL, ("Error while creating BusAttachment"));
    }

    status = s_msgBus->Start();
    if (ER_OK != status) {
        QCC_LogError(status, ("Error while starting the Bus"));
        return 1;
    }

    s_busListener = new TestBusListener();
    s_msgBus->RegisterBusListener(*s_busListener);

    TestBusObject* testBusObject = new TestBusObject(*s_msgBus, SERVICE_PATH);
    status = s_msgBus->RegisterBusObject(*testBusObject);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error while registering the bus object TestBusObject with the bus"));
        return 1;
    }

    const InterfaceDescription* introspectIntf = s_msgBus->GetInterface("org.allseen.Introspectable");
    status = testBusObject->SetAnnounceFlag(introspectIntf);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error while setting the annouce flag on the interface"));
    }

    status = s_msgBus->Connect();
    if (ER_OK != status) {
        QCC_LogError(status, ("Error while connecting to the Bus"));
        return 1;
    }

    const uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
    status = s_msgBus->RequestName(SERVICE_NAME, flags);
    if (ER_OK != status) {
        QCC_LogError(status, ("RequestName('%s') failed (status=%s).\n", SERVICE_NAME, QCC_StatusText(status)));
        return 1;
    }


    status = s_msgBus->BindSessionPort(SERVICE_PORT, opts, *s_busListener);
    if (ER_OK != status) {
        QCC_LogError(status, ("BindSessionPort failed (%s).\n", QCC_StatusText(status)));
    }

    //Set up About Data
    AboutData aboutData("en");
    uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                        0x1E, 0x82, 0x11, 0xE4,
                        0x86, 0x51, 0xD1, 0x56,
                        0x1D, 0x5D, 0x46, 0xB0 };
    aboutData.SetAppId(appId, 16);
    aboutData.SetDeviceName("My Device Name");
    aboutData.SetDeviceId("93c06771-c725-48c2-b1ff-6a2a59d445b8");
    aboutData.SetAppName("Application");
    aboutData.SetManufacturer("Manufacturer");
    aboutData.SetModelNumber("123456");
    aboutData.SetDescription("A poetic description of this application");
    aboutData.SetDateOfManufacture("2014-03-24");
    aboutData.SetSoftwareVersion("0.1.2");
    aboutData.SetHardwareVersion("0.0.1");
    aboutData.SetSupportUrl("http://www.example.org");
    if (!aboutData.IsValid()) {
        QCC_LogError(ER_FAIL, ("Failed to setup about data"));
    }

    // Announce about signal
    AboutObj* aboutObj = new AboutObj(*s_msgBus);
    status = aboutObj->Announce(SERVICE_PORT, aboutData);
    if (ER_OK != status) {
        QCC_LogError(status, ("AboutObj Announce failed (%s)", QCC_StatusText(status)));
        return 1;
    }

    printf("About Announced. Waiting for incoming connection \n");
    while (g_interrupt == false) {
        qcc::Sleep(100);
    }

    s_msgBus->UnregisterBusObject(*testBusObject);

    delete aboutObj;
    aboutObj = NULL;

    delete s_msgBus;
    s_msgBus = NULL;

    delete s_busListener;
    s_busListener = NULL;

    delete testBusObject;
    testBusObject = NULL;

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();

    return 0;

}

