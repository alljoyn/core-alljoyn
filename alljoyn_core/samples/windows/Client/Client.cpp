/**
 * @file
 * @brief  Sample implementation of an AllJoyn client.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/version.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Status.h>

using namespace std;
using namespace qcc;
using namespace ajn;

/** Static top level message bus object */
static BusAttachment* g_msgBus = NULL;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.Bus.method_sample";
static const char* SERVICE_NAME = "org.alljoyn.Bus.method_sample";
static const char* SERVICE_PATH = "/method_sample";
static const SessionPort SERVICE_PORT = 25;

static bool s_joinComplete = false;
static SessionId s_sessionId = 0;
static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        printf("FoundAdvertisedName(name=%s, prefix=%s)\n", name, namePrefix);
        if (0 == strcmp(name, SERVICE_NAME)) {
            /* We found a remote bus that is advertising basic sercice's  well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status = g_msgBus->JoinSession(name, SERVICE_PORT, this, s_sessionId, opts);
            if (ER_OK != status) {
                printf("JoinSession failed (status=%s)\n", QCC_StatusText(status));
            } else {
                printf("JoinSession SUCCESS (Session id=%d)\n", s_sessionId);
            }
        }
        s_joinComplete = true;
    }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }
};

/** Static bus listener */
static MyBusListener g_busListener;


/** Main entry point */
int main(int argc, char** argv, char** envArg)
{
    QStatus status = ER_OK;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    const char* connectArgs = getenv("BUS_ADDRESS");
    if (connectArgs == NULL) {
#ifdef _WIN32
        connectArgs = "tcp:addr=127.0.0.1,port=9956";
#else
        connectArgs = "unix:abstract=alljoyn";
#endif
    }

    /* Create message bus */
    g_msgBus = new BusAttachment("myApp", true);

    /* Add org.alljoyn.Bus.method_sample interface */
    InterfaceDescription* testIntf = NULL;
    status = g_msgBus->CreateInterface(INTERFACE_NAME, testIntf);
    if (status == ER_OK) {
        printf("Interface Created.\n");
        testIntf->AddMethod("cat", "ss",  "s", "inStr1,inStr2,outStr", 0);
        testIntf->Activate();
    } else {
        printf("Failed to create interface 'org.alljoyn.Bus.method_sample'\n");
    }


    /* Start the msg bus */
    if (ER_OK == status) {
        status = g_msgBus->Start();
        if (ER_OK != status) {
            printf("BusAttachment::Start failed\n");
        } else {
            printf("BusAttachment started.\n");
        }
    }

    /* Connect to the bus */
    if (ER_OK == status) {
        status = g_msgBus->Connect(connectArgs);
        if (ER_OK != status) {
            printf("BusAttachment::Connect(\"%s\") failed\n", connectArgs);
        } else {
            printf("BusAttchement connected to %s\n", connectArgs);
        }
    }

    /* Register a bus listener in order to get discovery indications */
    if (ER_OK == status) {
        g_msgBus->RegisterBusListener(g_busListener);
        printf("BusListener Registered.\n");
    }

    /* Begin discovery on the well-known name of the service to be called */
    if (ER_OK == status) {
        status = g_msgBus->FindAdvertisedName(SERVICE_NAME);
        if (status != ER_OK) {
            printf("org.alljoyn.Bus.FindAdvertisedName failed (%s))\n", QCC_StatusText(status));
        }
    }

    /* Wait for join session to complete */
    while (!s_joinComplete  && !g_interrupt) {
#ifdef _WIN32
        Sleep(1000);
#else
        usleep(100 * 1000);
#endif
    }

    if (status == ER_OK && g_interrupt == false) {
        ProxyBusObject remoteObj(*g_msgBus, SERVICE_NAME, SERVICE_PATH, s_sessionId);
        const InterfaceDescription* alljoynTestIntf = g_msgBus->GetInterface(INTERFACE_NAME);
        assert(alljoynTestIntf);
        remoteObj.AddInterface(*alljoynTestIntf);

        Message reply(*g_msgBus);
        MsgArg inputs[2];
        inputs[0].Set("s", "Hello ");
        inputs[1].Set("s", "World!");
        status = remoteObj.MethodCall(SERVICE_NAME, "cat", inputs, 2, reply, 5000);
        if (ER_OK == status) {
            printf("%s.%s ( path=%s) returned \"%s\"\n", SERVICE_NAME, "cat",
                   SERVICE_PATH, reply->GetArg(0)->v_string.str);
        } else {
            printf("MethodCall on %s.%s failed", SERVICE_NAME, "cat");
        }
    }

    /* Deallocate bus */
    if (g_msgBus) {
        BusAttachment* deleteMe = g_msgBus;
        g_msgBus = NULL;
        delete deleteMe;
    }

    printf("basic client exiting with status %d (%s)\n", status, QCC_StatusText(status));

    return (int) status;
}
