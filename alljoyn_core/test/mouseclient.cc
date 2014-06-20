/**
 * @file
 * Mouse receiver AJ client
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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

#ifndef _WIN32
#error mouseclient.cc can only be built for Windows
#endif

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <vector>

#include <qcc/Environ.h>
#include <qcc/Event.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/time.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>

#include <Status.h>

#include <Windows.h>

#define QCC_MODULE "ALLJOYN"

#define METHODCALL_TIMEOUT 30000

using namespace std;
using namespace qcc;
using namespace ajn;

/** Sample constants */
namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* InterfaceName = "org.alljoyn.ajlite_test";
const char* DefaultWellKnownName = "org.alljoyn.ajlite";
const char* ObjectPath = "/org/alljoyn/ajlite_test";
const SessionPort SessionPort = 24;   /**< Well-known session port value for bbclient/bbservice */
}
}
}

/** Static data */
static volatile sig_atomic_t g_interrupt = false;
static BusAttachment* g_msgBus = NULL;
static Event g_discoverEvent;
static String g_wellKnownName = ::org::alljoyn::alljoyn_test::DefaultWellKnownName;
static TransportMask allowedTransports = TRANSPORT_ANY;
static uint32_t findStartTime = 0;
static uint32_t findEndTime = 0;
static uint32_t joinStartTime = 0;
static uint32_t joinEndTime = 0;

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:

    MyBusListener() : BusListener(), sessionId(0) { }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        findEndTime = GetTimestamp();
        QCC_SyncPrintf("FindAdvertisedName 0x%x takes %d ms \n", transport, (findEndTime - findStartTime));
        QCC_SyncPrintf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);

        if (0 == (transport & allowedTransports)) {
            QCC_SyncPrintf("Ignoring FoundAdvertised name from transport 0x%x\n", transport);
            return;
        }

        /* We must enable concurrent callbacks since some of the calls below are blocking */
        g_msgBus->EnableConcurrentCallbacks();

        if (0 == ::strcmp(name, g_wellKnownName.c_str())) {
            /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, transport);
            QStatus status;

            joinStartTime = GetTimestamp();

            status = g_msgBus->JoinSession(name, ::org::alljoyn::alljoyn_test::SessionPort, this, sessionId, opts);
            if (ER_OK != status) {
                QCC_LogError(status, ("JoinSession(%s) failed", name));
            }

            /* Release the main thread */
            if (ER_OK == status) {
                joinEndTime = GetTimestamp();
                QCC_SyncPrintf("JoinSession 0x%x takes %d ms \n", transport, (joinEndTime - joinStartTime));

                g_discoverEvent.SetEvent();
            }
        }
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* prefix)
    {
        QCC_SyncPrintf("LostAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, prefix);
    }

    void NameOwnerChanged(const char* name, const char* previousOwner, const char* newOwner)
    {
        QCC_SyncPrintf("NameOwnerChanged(%s, %s, %s)\n",
                       name,
                       previousOwner ? previousOwner : "null",
                       newOwner ? newOwner : "null");
    }

    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        QCC_SyncPrintf("SessionLost(%08x) was called. Reason = %u.\n", sessionId, reason);
        g_interrupt = true;
    }

    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
};

/** Static bus listener */
static MyBusListener* g_busListener;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

static void usage(void)
{
    printf("Usage: bbclient [-h] [-d] [-n <well-known name>]\n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -n <well-known name>  = Well-known bus name advertised by bbservice\n");
    printf("   -d                    = discover remote bus with test service\n");
    printf("\n");
}


class LocalTestObject : public BusObject {
  public:

    void ObjectRegistered() {
        QStatus status = bus->AddMatch("type='signal',interface='org.alljoyn.ajlite_test',member='ADC_Update'");
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register Match rule for 'org.alljoyn.ajlite_test.ADC_Update'"));
        }

        status = bus->AddMatch("type='signal',interface='org.alljoyn.ajlite_test',member='Gyro_Update'");
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register Match rule for 'org.alljoyn.ajlite_test.Gyro_Update'"));
        }

        status = bus->AddMatch("type='signal',interface='org.alljoyn.ajlite_test',member='Button_Down'");
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register Match rule for 'org.alljoyn.ajlite_test.Button_Down'"));
        }
    }

    LocalTestObject(BusAttachment& bus, const char* path) :
        BusObject(bus, path),
        sensitivity(0)
    {
        const InterfaceDescription* regTestIntf = bus.GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
        assert(regTestIntf);
        AddInterface(*regTestIntf);

        printf("Registering ADC_Update\n");
        const InterfaceDescription::Member* member = regTestIntf->GetMember("ADC_Update");
        assert(member);
        bus.RegisterSignalHandler(this,
                                  static_cast<MessageReceiver::SignalHandler>(&LocalTestObject::ADC_Update),
                                  member,
                                  NULL);

        member = regTestIntf->GetMember("Gyro_Update");
        assert(member);
        bus.RegisterSignalHandler(this,
                                  static_cast<MessageReceiver::SignalHandler>(&LocalTestObject::Gyro_Update),
                                  member,
                                  NULL);

        member = regTestIntf->GetMember("Button_Down");
        assert(member);
        bus.RegisterSignalHandler(this,
                                  static_cast<MessageReceiver::SignalHandler>(&LocalTestObject::Button_Down),
                                  member,
                                  NULL);
    }


    // return the acceleration; integer in range: [-100, 100]
    int GetXDelta(int x)
    {
        return (x - 2000) / 10;
    }

    int GetYDelta(int y)
    {
        return (2000 - y) / 10;
    }

    void Gyro_Update(const InterfaceDescription::Member* member,
                     const char* sourcePath,
                     Message& msg)
    {
        const static int CX = (65536 / GetSystemMetrics(SM_CXSCREEN));
        const static int CY = (65536 / GetSystemMetrics(SM_CYSCREEN));

        int32_t x, y;
        msg->GetArgs("ii", &x, &y);

        POINT p;
        GetCursorPos(&p);

        // get scaling factors for x and y coordinates
        x = GetXDelta(x);
        y = GetYDelta(y);

        INPUT input;
        memset(&input, 0, sizeof(input));
        input.type = INPUT_MOUSE;
        input.mi.mouseData = 0;
        input.mi.dx = (p.x + x) * CX;
        input.mi.dy = (p.y + y) * CY;
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        SendInput(1, &input, sizeof(input));
    }

    void Button_Down(
        const InterfaceDescription::Member* member,
        const char* sourcePath,
        Message& msg)
    {
        int32_t clicks;
        msg->GetArgs("i", &clicks);

        POINT p;
        GetCursorPos(&p);

        INPUT input;
        input.type = INPUT_MOUSE;
        input.mi.mouseData = 0;
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;

        for (int i = 0; i < clicks; ++i) {
            SendInput(1, &input, sizeof(INPUT));
        }
    }

    void ADC_Update(const InterfaceDescription::Member* member,
                    const char* sourcePath,
                    Message& msg)
    {
        int32_t i;
        msg->GetArgs("i", &i);
        printf("ADC_Update: %d\n", i);
        sensitivity = i;
    }

    int32_t sensitivity;
};



/** Main entry point */
int main(int argc, char** argv)
{
    QStatus status = ER_OK;


    Environ* env;
    bool discoverRemote = false;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_wellKnownName = argv[i];
            }
        } else if (0 == strcmp("-h", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-d", argv[i])) {
            discoverRemote = true;
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    /* Get env vars */
    env = Environ::GetAppEnviron();
    qcc::String connectArgs = env->Find("BUS_ADDRESS", "tcp:addr=127.0.0.1,port=9956");


    /* Create message bus */
    g_msgBus = new BusAttachment("bbclient", true);


    /* Add org.alljoyn.alljoyn_test interface */
    InterfaceDescription* testIntf = NULL;
    status = g_msgBus->CreateInterface(::org::alljoyn::alljoyn_test::InterfaceName, testIntf, false);
    if ((ER_OK == status) && testIntf) {
        testIntf->AddSignal("ADC_Update", "i", "value");
        testIntf->AddSignal("Gyro_Update", "ii", "x,y");
        testIntf->AddSignal("Button_Down", "i", "dummy");
        testIntf->Activate();
    } else {
        if (ER_OK == status) {
            status = ER_FAIL;
        }
        QCC_LogError(status, ("Failed to create interface \"%s\"", ::org::alljoyn::alljoyn_test::InterfaceName));
    }


    /* Register a bus listener in order to get discovery indications */
    if (ER_OK == status) {
        g_busListener = new MyBusListener();
        g_msgBus->RegisterBusListener(*g_busListener);
    }

    /* Start the msg bus */
    if (ER_OK == status) {
        status = g_msgBus->Start();
        if (ER_OK == status) {
            printf("Registering BusObject");
            LocalTestObject* testObj = new LocalTestObject(*g_msgBus, ::org::alljoyn::alljoyn_test::ObjectPath);
            g_msgBus->RegisterBusObject(*testObj);

        } else {
            QCC_LogError(status, ("BusAttachment::Start failed"));
        }
    }




    /* Connect to the bus */
    if (ER_OK == status) {
        status = g_msgBus->Connect(connectArgs.c_str());
        if (ER_OK != status) {
            QCC_LogError(status, ("BusAttachment::Connect(\"%s\") failed", connectArgs.c_str()));
        }
    }

    if (ER_OK == status) {
        if (discoverRemote) {
            /* Begin discovery on the well-known name of the service to be called */
            findStartTime = GetTimestamp();
            /*
             * Make sure the g_discoverEvent flag has been set to the
             * name-not-found state before trying to find the well-known name.
             */
            g_discoverEvent.ResetEvent();
            status = g_msgBus->FindAdvertisedName(g_wellKnownName.c_str());
            if (status != ER_OK) {
                QCC_LogError(status, ("FindAdvertisedName failed"));
            }
        }
    }

    /*
     * If discovering, wait for the "FoundAdvertisedName" signal that tells us that we are connected to a
     * remote bus that is advertising bbservice's well-known name.
     */
    if (discoverRemote && (ER_OK == status)) {
        for (bool discovered = false; !discovered;) {
            /*
             * We want to wait for the discover event, but we also want to
             * be able to interrupt discovery with a control-C.  The AllJoyn
             * idiom for waiting for more than one thing this is to create a
             * vector of things to wait on.  To provide quick response we
             * poll the g_interrupt bit every 100 ms using a 100 ms timer
             * event.
             */
            qcc::Event timerEvent(100, 100);
            vector<qcc::Event*> checkEvents, signaledEvents;
            checkEvents.push_back(&g_discoverEvent);
            checkEvents.push_back(&timerEvent);
            status = qcc::Event::Wait(checkEvents, signaledEvents);
            if (status != ER_OK && status != ER_TIMEOUT) {
                break;
            }

            /*
             * If it was the discover event that popped, we're done.
             */
            for (vector<qcc::Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
                if (*i == &g_discoverEvent) {
                    discovered = true;
                    break;
                }
            }
            /*
             * If we see the g_interrupt bit, we're also done.  Set an error
             * condition so we don't do anything else.
             */
            if (g_interrupt) {
                status = ER_FAIL;
                break;
            }
        }
    }

    if (ER_OK == status) {
        /* Create the remote object that will be called */
        ProxyBusObject remoteObj;
        if (ER_OK == status) {
            remoteObj = ProxyBusObject(*g_msgBus, g_wellKnownName.c_str(), ::org::alljoyn::alljoyn_test::ObjectPath, g_busListener->GetSessionId());
            const InterfaceDescription* alljoynTestIntf = g_msgBus->GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
            assert(alljoynTestIntf);
            remoteObj.AddInterface(*alljoynTestIntf);
        }
    }


    if (status == ER_OK) {
        while (g_interrupt == false) {
            qcc::Sleep(100);
        }
    }

    /* Deallocate bus */
    delete g_msgBus;
    g_msgBus = NULL;

    delete g_busListener;
    g_busListener = NULL;



    printf("mouseclient exiting with status %d (%s)\n", status, QCC_StatusText(status));
    return (int) status;
}
