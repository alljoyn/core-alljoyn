/**
 * @file
 * Sample implementation of an AllJoyn client the uses raw sockets.
 */

/******************************************************************************
 * Copyright (c) 2009-2011,2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/Debug.h>
#include <qcc/Thread.h>

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <errno.h>

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
#include <alljoyn/TransportMask.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;

/** Sample constants */
static const SessionPort SESSION_PORT = 33;

/** Static data */
static BusAttachment* g_msgBus = NULL;
static Event g_discoverEvent;
static String g_wellKnownName = "org.alljoyn.raw_test";

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener {
  public:

    MyBusListener() : BusListener(), sessionId(0), transportMask(TRANSPORT_ANY) { }

    void SetTransportMask(TransportMask transportMask)
    {
        this->transportMask = transportMask;
    }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_SyncPrintf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);

        if (0 == strcmp(name, g_wellKnownName.c_str()) && ((transport & transportMask) != 0)) {
            /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_RAW_RELIABLE, false, SessionOpts::PROXIMITY_ANY, transport);
            g_msgBus->EnableConcurrentCallbacks();
            QStatus status = g_msgBus->JoinSession(name, SESSION_PORT, NULL, sessionId, opts);
            if (ER_OK != status) {
                QCC_LogError(status, ("JoinSession(%s) failed", name));
            } else {
                /* Release the main thread */
                QCC_SyncPrintf("Session Joined with session id = %u\n", sessionId);
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

    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
    TransportMask transportMask;
};

/** Static bus listener */
static MyBusListener g_busListener;

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

static void usage(void)
{
    printf("Usage: rawclient [-h] [-n <well-known name>] [-t <transport_mask>]\n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -n <well-known name>  = Well-known bus name advertised by bbservice\n");
    printf("   -t <transport_mask>   = Set the transports that will attempt a joinSession\n");
    printf("\n");
}

/** Main entry point */
int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    Environ* env;

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
        } else if (0 == strcmp("-t", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a paramter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                TransportMask transportMask = (TransportMask) StringToU32(argv[i], 16, 0);
                if (transportMask == 0) {
                    printf("Invalid transport mask 0x%x\n", transportMask);
                    usage();
                    exit(1);
                } else {
                    g_busListener.SetTransportMask(transportMask);
                }
            }
        } else if (0 == strcmp("-h", argv[i])) {
            usage();
            exit(0);
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    /* Get env vars */
    env = Environ::GetAppEnviron();
    qcc::String connectArgs = env->Find("BUS_ADDRESS");

    /* Create message bus */
    g_msgBus = new BusAttachment("rawclient", true);

    /* Register a bus listener in order to get discovery indications */
    g_msgBus->RegisterBusListener(g_busListener);

    /* Start the msg bus */
    status = g_msgBus->Start();
    if (ER_OK != status) {
        QCC_LogError(status, ("BusAttachment::Start failed"));
    }

    /* Connect to the bus */
    if (ER_OK == status) {
        if (connectArgs.empty()) {
            status = g_msgBus->Connect();
        } else {
            status = g_msgBus->Connect(connectArgs.c_str());
        }
        if (ER_OK != status) {
            QCC_LogError(status, ("BusAttachment::Connect(\"%s\") failed", g_msgBus->GetConnectSpec().c_str()));
        }
    }

    /* Begin discovery for the well-known name of the service */
    if (ER_OK == status) {
        status = g_msgBus->FindAdvertisedName(g_wellKnownName.c_str());
        if (status != ER_OK) {
            QCC_LogError(status, ("org.alljoyn.raw_test.FindAdvertisedName failed"));
        }
    }

    /* Wait for the "FoundAdvertisedName" signal */
    if (ER_OK == status) {
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

    /* Check the session */
    SessionId ssId = g_busListener.GetSessionId();
    if (ssId == 0) {
        status = ER_FAIL;
        QCC_LogError(status, ("Raw session id is invalid"));
    } else {
        /* Get the descriptor */
        SocketFd sockFd;
        QStatus status = g_msgBus->GetSessionFd(ssId, sockFd);
        if (status == ER_OK) {
            /* Attempt to read test string from fd */
            char buf[256];
            size_t recvd;
            while ((status == ER_OK) || (status == ER_WOULDBLOCK)) {
                status = qcc::Recv(sockFd, buf, sizeof(buf) - 1, recvd);
                if (status == ER_OK) {
                    QCC_SyncPrintf("Read %d bytes from fd\n", recvd);
                    buf[recvd] = '\0';
                    QCC_SyncPrintf("Bytes: %s\n", buf);
                    break;
                } else if (status == ER_WOULDBLOCK) {
                    qcc::Sleep(200);
                } else {
                    QCC_LogError(status, ("Read from raw fd failed"));
                }
            }
        } else {
            QCC_LogError(status, ("GetSessionFd failed"));
        }
    }

    /* Stop the bus */
    delete g_msgBus;

    printf("rawclient exiting with status 0x%x (%s)\n", status, QCC_StatusText(status));

    return (int) status;
}
