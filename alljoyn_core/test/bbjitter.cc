/* bbjitter - tests roundtrip times and computes  jitter */

/******************************************************************************
 * Copyright (c) 2009-2012,2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>
#include <map>

#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/Util.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>


#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;

/** Sample constants */
namespace org {
namespace alljoyn {
namespace jitter_test {
const char* Interface = "org.alljoyn.jitter_test";
const char* Path = "/org/alljoyn/jitter_test";
}
}
}

static const char ifcXML[] =
    "<node name=\"/org/alljoyn/jitter_test\">"
    "  <interface name=\"org.alljoyn.jitter_test\">"
    "    <method name=\"TimedPing\">"
    "      <arg name=\"timestampIn\" type=\"u\" direction=\"in\"/>"
    "      <arg name=\"timestampOut\" type=\"u\" direction=\"out\"/>"
    "    </method>"
    "  </interface>"
    "</node>";

/** Static top level message bus object */
static BusAttachment* g_msgBus = NULL;
static Event g_discoverEvent;
static String g_wellKnownName;
static String g_findPrefix("org.alljoyn.jitter");
SessionPort SESSION_PORT_MESSAGES_MP1 = 26;

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

class PingObject : public BusObject {
  public:
    PingObject() : BusObject(::org::alljoyn::jitter_test::Path) { }

    void TimedPing(const InterfaceDescription::Member* member, Message& msg)
    {
        MsgArg replyArg = *msg->GetArg();
        MethodReply(msg, &replyArg, 1);
    }

    QStatus Init()
    {
        /* Initialize bus object */
        QStatus status = g_msgBus->CreateInterfacesFromXml(ifcXML);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to parse XML"));
            return status;
        }
        const InterfaceDescription* ifc = g_msgBus->GetInterface(::org::alljoyn::jitter_test::Interface);
        if (ifc) {
            AddInterface(*ifc);
            AddMethodHandler(ifc->GetMember("TimedPing"), static_cast<MessageReceiver::MethodHandler>(&PingObject::TimedPing));
        }
        g_msgBus->RegisterBusObject(*this);
        return status;
    }

};

class PingThread : public qcc::Thread, BusObject {

  public:
    PingThread(uint32_t iterations, uint32_t delay) :
        qcc::Thread("PingThread"),
        BusObject(::org::alljoyn::jitter_test::Path),
        sessionId(0),
        iterations(iterations),
        delay(delay)
    {
        Start(NULL);
    }


    SessionId sessionId;
    qcc::String remoteName;

  private:

    uint32_t iterations;
    uint32_t delay;

    qcc::ThreadReturn STDCALL Run(void* arg)
    {
        if (iterations == 0) {
            return (qcc::ThreadReturn)0;
        }

        QCC_SyncPrintf("Start ping thread\n");

        while (!IsStopping()) {

            // Wait until alerted
            QStatus status = Event::Wait(Event::neverSet);

            // Clear stop event if we were just alerted
            if (status == ER_ALERTED_THREAD) {
                GetStopEvent().ResetEvent();
                status = ER_OK;
            }
            if (status != ER_OK) {
                break;
            }

            QCC_SyncPrintf("Ping thread alerted\n");

            /* Create the proxy object */
            ProxyBusObject remoteObj(*g_msgBus, remoteName.c_str(), ::org::alljoyn::jitter_test::Path, sessionId);
            status = remoteObj.ParseXml(ifcXML, "jitter_test");
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to parse XML"));
                return (qcc::ThreadReturn)status;
            }

            const uint32_t bucketSize = 5;
            const uint32_t numBuckets = 200;
            uint32_t histogram[numBuckets];
            uint32_t avg = 0;

            memset(histogram, 0, sizeof(histogram));

            /* Let all the joining etc. settle down before we start */
            qcc::Sleep(2000);

            for (size_t i = 0; i < iterations; ++i) {
                Message reply(*g_msgBus);
                MsgArg arg("u", g_msgBus->GetTimestamp());
                status = remoteObj.MethodCall(::org::alljoyn::jitter_test::Interface, "TimedPing", &arg, 1, reply);
                if (status != ER_OK) {
                    String errMsg;
                    const char* errName = reply->GetErrorName(&errMsg);
                    QCC_LogError(status, ("TimedPing returned ERROR_MESSAGE (error=%s, \"%s\")", errName, errMsg.c_str()));
                    QCC_UNUSED(errName); /* avoid unused variable warning in release build */
                    break;
                }
                uint32_t timestamp;
                reply->GetArgs("u", &timestamp);
                uint32_t rt = g_msgBus->GetTimestamp() - timestamp;
                avg += rt;
                uint32_t bucket = rt / bucketSize;
                if (bucket >= numBuckets) {
                    bucket = numBuckets - 1;
                }
                histogram[bucket] += 1;
                qcc::Sleep(delay);
            }
            avg /= iterations;
            QCC_SyncPrintf("Round trip avg=%u\n", avg);
            QCC_SyncPrintf("\n=================================\n");
            size_t last = numBuckets - 1;
            while ((last > 0) && (histogram[last] == 0)) {
                --last;
            }
            for (size_t i = 0; i <= last; ++i) {
                QCC_SyncPrintf("%3u ", (i + 1) * bucketSize);
            }
            QCC_SyncPrintf("\n");
            for (size_t i = 0; i <= last; ++i) {
                QCC_SyncPrintf("%3u ", histogram[i]);
            }
            QCC_SyncPrintf("\n=================================\n");

        }

        QCC_SyncPrintf("Exit ping thread\n");
        return 0;
    }
};

class MyBusListener : public BusListener, public SessionPortListener, public SessionListener, public BusAttachment::JoinSessionAsyncCB {

  public:
    MyBusListener(uint32_t iterations, uint32_t delay) : BusListener(), pingThread(iterations, delay) { }

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        return true;
    }

    void SessionJoined(SessionPort sessionPort, SessionId sessionId, const char* joiner)
    {
        QCC_SyncPrintf("Session Established: joiner=%s, sessionId=%u\n", joiner, sessionId);
        QStatus status = g_msgBus->SetSessionListener(sessionId, this);
        if (ER_OK == status) {
            pingThread.sessionId = sessionId;
            pingThread.remoteName = joiner;
            pingThread.Alert();
        } else {
            QCC_LogError(status, ("Failed to SetSessionListener(%u)", sessionId));
        }
    }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_SyncPrintf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);
        if (strcmp(name, g_wellKnownName.c_str()) != 0) {
            SessionOpts::TrafficType traffic = SessionOpts::TRAFFIC_MESSAGES;
            SessionOpts opts(traffic, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

            QStatus status = g_msgBus->JoinSessionAsync(name, 26, this, opts, this, ::strdup(name));
            if (ER_OK != status) {
                QCC_LogError(status, ("JoinSessionAsync(%s) failed \n", name));
            }
        }
    }

    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context)
    {
        if (status == ER_OK) {
            QCC_SyncPrintf("JoinSessionAsync succeeded. SessionId=%u\n", sessionId);
        } else {
            QCC_LogError(status, ("JoinSessionAsych failed"));
            QCC_SyncPrintf("JoinSession failed with %s\n", QCC_StatusText(status));
        }
        free(context);
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* prefix)
    {
        QCC_SyncPrintf("LostAdvertisedName(name=%s, transport=0x%x,  prefix=%s)\n", name, transport, prefix);
    }

    void NameOwnerChanged(const char* name, const char* previousOwner, const char* newOwner)
    {
        QCC_SyncPrintf("NameOwnerChanged(%s, %s, %s)\n",
                       name,
                       previousOwner ? previousOwner : "null",
                       newOwner ? newOwner : "null");
    }

    void SessionLost(SessionId sessid, SessionLostReason reason)
    {
        QCC_SyncPrintf("Session Lost  %u. Reason=%u.\n", sessid, reason);
    }

    ~MyBusListener()
    {
        pingThread.Stop();
        pingThread.Join();
    }

    PingThread pingThread;

};

static MyBusListener* myBusListener;

QStatus CreateSession(SessionPort sessport, SessionOpts& options)
{
    /* Create a session for incoming client connections */
    QStatus status = ER_OK;
    status = g_msgBus->BindSessionPort(sessport, options, *myBusListener);
    if (status != ER_OK) {
        QCC_LogError(status, ("BindSessionPort failed"));
        return status;
    }
    return status;
}

static void usage(void)
{
    printf("Usage: bbjitter \n\n");
    printf("Options:\n");
    printf("   -n <well-known-name> = Well-known bus name to advertise\n");
    printf("   -h                   = Print this help message\n");
    printf("   -c <calls>           = Number of roundtrip calls to make\n");
    printf("   -d <delay>           = Delay between each rountdtrip call in milliseconds\n");
    printf("   -f <prefix>          = FindAdvertisedName prefix\n");
    printf("   -t                   = Advertise over TCP (enables selective advertising)\n");
}

/** Main entry point */
int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    uint32_t transportOpts = 0;
    uint32_t iterations = 500;
    uint32_t delay = 100;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_wellKnownName = argv[i];
            }
        } else if (0 == strcmp("-c", argv[i])) {
            if (++i == argc) {
                iterations = -1;
            } else {
                iterations = strtoul(argv[i], NULL, 0);
            }
            if ((int32_t)iterations < 0) {
                printf("option %s requires an integer parameter >= 0\n", argv[i - 1]);
                usage();
                exit(1);
            }
        } else if (0 == strcmp("-d", argv[i])) {
            if (++i == argc) {
                delay = -1;
            } else {
                delay = strtoul(argv[i], NULL, 0);
            }
            if ((int32_t)delay < 0) {
                printf("option %s requires an integer parameter >= 0\n", argv[i - 1]);
                usage();
                exit(1);
            }
        } else if (0 == strcmp("-f", argv[i])) {
            g_findPrefix = argv[++i];
        } else if (0 == strcmp("-t", argv[i])) {
            transportOpts |= TRANSPORT_WLAN;
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    /* If no transport option was specifie, then make session options very open */
    if (transportOpts == 0) {
        transportOpts = TRANSPORT_ANY;
    }

    /* Get env vars */
    Environ* env = Environ::GetAppEnviron();
    qcc::String clientArgs = env->Find("DBUS_STARTER_ADDRESS");

    if (clientArgs.empty()) {
        clientArgs = env->Find("BUS_ADDRESS");
    }

    /* Create message bus */
    g_msgBus = new BusAttachment("bbjitter", true);

    /* Start the msg bus */
    if (ER_OK == status) {
        status = g_msgBus->Start();
    } else {
        QCC_LogError(status, ("BusAttachment::Start failed"));
    }


    /* Create an initialize the ping bus object */
    PingObject pingObj;
    status = pingObj.Init();
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to initialize ping object"));
        exit(0);
    }
    g_msgBus->RegisterBusObject(pingObj);

    /* Connect to the daemon */
    if (clientArgs.empty()) {
        status = g_msgBus->Connect();
    } else {
        status = g_msgBus->Connect(clientArgs.c_str());
    }
    if (ER_OK != status) {
        QCC_LogError(status, ("BusAttachment::Connect failed"));
        exit(0);
    }

    g_wellKnownName = g_findPrefix + ".U" + g_msgBus->GetGlobalGUIDString();

    myBusListener = new MyBusListener(iterations, delay);
    g_msgBus->RegisterBusListener(*myBusListener);

    /* Register local objects and connect to the daemon */
    if (ER_OK == status) {

        /* Create session opts */
        SessionOpts optsmp(SessionOpts::TRAFFIC_MESSAGES, true,  SessionOpts::PROXIMITY_ANY, transportOpts);

        /* Create a session for incoming client connections */
        status = CreateSession(SESSION_PORT_MESSAGES_MP1, optsmp);

        /* Request a well-known name */
        QStatus status = g_msgBus->RequestName(g_wellKnownName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        if (status != ER_OK) {
            status = (status == ER_OK) ? ER_FAIL : status;
            QCC_LogError(status, ("RequestName(%s) failed. ", g_wellKnownName.c_str()));
            return status;
        }

        /* Begin Advertising the well-known name */
        status = g_msgBus->AdvertiseName(g_wellKnownName.c_str(), transportOpts);
        if (ER_OK != status) {
            status = (status == ER_OK) ? ER_FAIL : status;
            QCC_LogError(status, ("Sending org.alljoyn.Bus.Advertise failed "));
            return status;
        }

        status = g_msgBus->FindAdvertisedName(g_findPrefix.c_str());
        if (status != ER_OK) {
            status = (status == ER_OK) ? ER_FAIL : status;
            QCC_LogError(status, ("FindAdvertisedName failed "));
        }
    }

    while (g_interrupt == false) {
        qcc::Sleep(100);
    }

    g_msgBus->UnregisterBusListener(*myBusListener);

    delete myBusListener;

    /* Clean up msg bus */
    if (g_msgBus) {
        delete g_msgBus;
        g_msgBus = NULL;
    }

    printf("\n %s exiting with status %d (%s)\n", argv[0], status, QCC_StatusText(status));

    return (int) status;
}
