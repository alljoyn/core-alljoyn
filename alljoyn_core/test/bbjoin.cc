/* bbjoin - will join any names on multipoint session port 26.*/

/******************************************************************************
 * Copyright (c) 2009-2011, 2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/StringUtil.h>
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

namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* DefaultWellKnownName = "org.alljoyn.signals";
}
}
}

/** Static top level message bus object */
static BusAttachment* g_msgBus = NULL;
static String g_wellKnownName = ::org::alljoyn::alljoyn_test::DefaultWellKnownName;
static bool g_acceptSession = true;
static bool g_stressTest = false;
static char* g_findPrefix = NULL;
static int g_sleepBeforeRejoin = 0;
static int g_sleepBeforeLeave = 0;
static int g_useCount = 0;
static bool g_useMultipoint = true;
static bool g_suppressNameOwnerChanged = false;
static bool g_keep_retrying_in_failure = false;
static uint32_t g_concurrent_threads = 4;

SessionPort SESSION_PORT = 26;

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

class MyBusListener : public BusListener, public SessionPortListener, public SessionListener, public BusAttachment::JoinSessionAsyncCB {

  public:

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        return g_acceptSession;
    }

    void SessionJoined(SessionPort sessionPort, SessionId sessionId, const char* joiner)
    {
        printf("=============> Session Established: joiner=%s, sessionId=%u\n", joiner, sessionId);
        QStatus status = g_msgBus->SetSessionListener(sessionId, this);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to SetSessionListener(%u)", sessionId));
        }
    }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        printf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);
        if (strcmp(name, g_wellKnownName.c_str()) != 0) {
            SessionOpts::TrafficType traffic = SessionOpts::TRAFFIC_MESSAGES;
            SessionOpts opts(traffic, g_useMultipoint, SessionOpts::PROXIMITY_ANY, transport);

            QStatus status = g_msgBus->JoinSessionAsync(name, SESSION_PORT, this, opts, this, ::strdup(name));
            if (ER_OK != status) {
                QCC_LogError(status, ("JoinSessionAsync(%s) failed \n", name));
                g_interrupt = true;
            }
        }
    }

    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context)
    {
        const char* name = reinterpret_cast<const char*>(context);

        if (status == ER_OK) {
            printf("JoinSessionAsync succeeded. SessionId=%u ===========================>  %s\n", sessionId, name);
        } else {
            QCC_LogError(status, ("JoinSessionCB failure "));
            if (g_keep_retrying_in_failure) {
                /* Keep retrying inspite of failure. */
                char* retryContext = ::strdup(name);
                SessionOpts::TrafficType traffic = SessionOpts::TRAFFIC_MESSAGES;
                SessionOpts opts1(traffic, g_useMultipoint, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
                QStatus status1 = g_msgBus->JoinSessionAsync(name, SESSION_PORT, this, opts1, this, retryContext);
                if (status1 != ER_OK) {
                    QCC_LogError(status1, ("JoinSession retry failure"));
                    free(retryContext);
                }
            } else {
                QCC_LogError(status, ("JoinSessionAsyncCB: JoinSession failure"));
            }
        }

        /* Start over if we are in stress mode */
        if ((status == ER_OK) && g_stressTest) {
            if (g_sleepBeforeLeave) {
                qcc::Sleep(g_sleepBeforeLeave);
            }

            g_msgBus->EnableConcurrentCallbacks();

            QStatus status = g_msgBus->LeaveSession(sessionId);

            if (status == ER_OK) {
                if (g_sleepBeforeRejoin) {
                    qcc::Sleep(g_sleepBeforeRejoin);
                }
                char* retryContext = ::strdup(name);
                status = g_msgBus->JoinSessionAsync(name, SESSION_PORT, this, opts, this, retryContext);
                if (status != ER_OK) {
                    QCC_LogError(status, ("JoinSessionAsync failed"));
                    free(retryContext);
                }
            } else {
                QCC_LogError(status, ("LeaveSession failed"));
            }
        }
        free(context);
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* prefix)
    {
        printf("LostAdvertisedName(name=%s, transport=0x%x,  prefix=%s)\n", name, transport, prefix);
    }

    void NameOwnerChanged(const char* name, const char* previousOwner, const char* newOwner)
    {
        if (!g_suppressNameOwnerChanged) {
            printf("NameOwnerChanged(%s, %s, %s)\n",
                   name,
                   previousOwner ? previousOwner : "null",
                   newOwner ? newOwner : "null");
        }
    }

    void SessionLost(SessionId sessid)
    {
        printf("Session Lost  %u\n", sessid);
    }
};

static void usage(void)
{
    printf("Usage: bbjoin \n\n");
    printf("Options:\n");
    printf("   -?           = Print this help message\n");
    printf("   -h           = Print this help message\n");
    printf("   -n <name>    = Well-known name to advertise\n");
    printf("   -r           = Reject incoming joinSession attempts\n");
    printf("   -s           = Stress test. Continous leave/join\n");
    printf("   -f <prefix>  = FindAdvertisedName prefix\n");
    printf("   -b           = Advertise/Discover over Bluetooth\n");
    printf("   -t           = Advertise/Discover over TCP\n");
    printf("   -u           = Advertise/Discover over UDP\n");
    printf("   -w           = Advertise/Discover over Wi-Fi Direct\n");
    printf("   -l           = Advertise/Discover over LOCAL\n");
    printf("   -dj <ms>     = Number of ms to delay between leaving and re-joining\n");
    printf("   -dl <ms>     = Number of ms to delay before leaving the session\n");
    printf("   -p           = Use point-to-point sessions rather than multi-point\n");
    printf("   -qnoc        = Suppress NameOwnerChanged printing\n");
    printf("   -fa          = Retryjoin session even during failure\n");
    printf("   -ct  #       = Set concurrency level\n");
    printf("   -sp  #       = Session port\n");
    printf("\n");
}

/** Main entry point */
int main(int argc, char** argv)
{
    const uint64_t startTime = GetTimestamp64(); // timestamp in milliseconds
    QStatus status = ER_OK;
    uint32_t transportOpts = TRANSPORT_TCP;

    // echo command line to provide distinguishing information within multipoint session
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i]) || 0 == strcmp("-?", argv[i])) {
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
        } else if (0 == strcmp("-r", argv[i])) {
            g_acceptSession = false;
        } else if (0 == strcmp("-s", argv[i])) {
            g_stressTest = true;
        } else if (0 == strcmp("-f", argv[i])) {
            g_findPrefix = argv[++i];
        } else if (0 == strcmp("-b", argv[i])) {
            transportOpts |= TRANSPORT_BLUETOOTH;
        } else if (0 == strcmp("-t", argv[i])) {
            transportOpts = TRANSPORT_TCP;
        } else if (0 == strcmp("-u", argv[i])) {
            transportOpts = TRANSPORT_UDP;
        } else if (0 == strcmp("-w", argv[i])) {
            transportOpts = TRANSPORT_WFD;
        } else if (0 == strcmp("-l", argv[i])) {
            transportOpts |= TRANSPORT_LOCAL;
        } else if (0 == strcmp("-dj", argv[i])) {
            g_sleepBeforeRejoin = qcc::StringToU32(argv[++i], 0);
        } else if (0 == strcmp("-dl", argv[i])) {
            g_sleepBeforeLeave = qcc::StringToU32(argv[++i], 0);
        } else if (0 == strcmp("-p", argv[i])) {
            g_useMultipoint = false;
        } else if (0 == strcmp("-qnoc", argv[i])) {
            g_suppressNameOwnerChanged = true;
        } else if (0 == strcmp("-fa", argv[i])) {
            g_keep_retrying_in_failure = true;
        } else if (0 == strcmp("-ct", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_concurrent_threads = qcc::StringToU32(argv[i], 0);;
            }
        } else if (0 == strcmp("-sp", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                SESSION_PORT = (SessionPort) qcc::StringToU32(argv[i], 0);;
            }
        }  else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }


    /* Get env vars */
    Environ* env = Environ::GetAppEnviron();
    qcc::String clientArgs = env->Find("BUS_ADDRESS");

    /* Create message bus */
    g_msgBus = new BusAttachment("bbjoin", true, g_concurrent_threads);
    if (g_msgBus != NULL) {
        status = g_msgBus->Start();
        if (ER_OK != status) {
            QCC_LogError(status, ("BusAttachment::Start failed"));
            exit(-1);
        }

        /* Connect to the daemon */
        if (clientArgs.empty()) {
            status = g_msgBus->Connect();
        } else {
            status = g_msgBus->Connect(clientArgs.c_str());
        }
        if (ER_OK != status) {
            QCC_LogError(status, ("BusAttachment::Connect failed"));
            exit(-1);
        }

        MyBusListener myBusListener;
        g_msgBus->RegisterBusListener(myBusListener);

        /* Register local objects and connect to the daemon */
        if (ER_OK == status) {

            /* Create session opts */
            SessionOpts optsmp(SessionOpts::TRAFFIC_MESSAGES, g_useMultipoint,  SessionOpts::PROXIMITY_ANY, transportOpts);

            /* Create a session for incoming client connections */
            status = g_msgBus->BindSessionPort(SESSION_PORT, optsmp, myBusListener);
            if (status != ER_OK) {
                QCC_LogError(status, ("BindSessionPort failed"));
                exit(-1);
            }

            /* Request a well-known name */
            QStatus status = g_msgBus->RequestName(g_wellKnownName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
            if (status != ER_OK) {
                QCC_LogError(status, ("RequestName(%s) failed. ", g_wellKnownName.c_str()));
                exit(-1);
            }

            /* Begin Advertising the well-known name */
            status = g_msgBus->AdvertiseName(g_wellKnownName.c_str(), transportOpts);
            if (ER_OK != status) {
                QCC_LogError(status, ("Advertise name(%s) failed ", g_wellKnownName.c_str()));
                exit(-1);
            }

            status = g_msgBus->FindAdvertisedNameByTransport(g_findPrefix ? g_findPrefix : "com", transportOpts);
            if (status != ER_OK) {
                QCC_LogError(status, ("FindAdvertisedName failed "));
                exit(-1);
            }
        }

        while ((g_interrupt == false) || (g_useCount > 0)) {
            qcc::Sleep(100);
        }

        /* Clean up msg bus */
        g_msgBus->Stop();
        g_msgBus->Join();
        delete g_msgBus;
    }

    printf("Elapsed time is %" PRIi64 " seconds\n", (GetTimestamp64() - startTime) / 1000);

    return (int) status;
}
