/**
 * @file
 * @brief  Sample implementation of an AllJoyn client.
 */

/******************************************************************************
 *
 *
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
#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>

#include <qcc/String.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Init.h>
#include <alljoyn/Status.h>
#include <alljoyn/version.h>

using namespace std;
using namespace qcc;
using namespace ajn;

/** Static top level message bus object */
static BusAttachment* g_msgBus = NULL;

/*constants*/
static const char* INTERFACE_NAME = "com.example.sample";
static const char* SERVICE_NAME = "com.example.sample";
static const char* SERVICE_PATH = "/sample";
static const SessionPort SERVICE_PORT = 16;

static bool s_joinComplete = false;
static String s_sessionHost;
static SessionId s_sessionId = 0;

static volatile sig_atomic_t s_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    s_interrupt = true;
}

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        if (0 == strcmp(name, SERVICE_NAME) && s_sessionHost.empty()) {
            printf("FoundAdvertisedName(name='%s', transport = 0x%x, prefix='%s')\n", name, transport, namePrefix);

            /* We found a remote bus that is advertising basic service's well-known name so connect to it. */
            /* Since we are in a callback we must enable concurrent callbacks before calling a synchronous method. */
            s_sessionHost = name;
            g_msgBus->EnableConcurrentCallbacks();
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status = g_msgBus->JoinSession(name, SERVICE_PORT, this, s_sessionId, opts);
            if (ER_OK == status) {
                printf("JoinSession SUCCESS (Session id=%d).\n", s_sessionId);
            } else {
                printf("JoinSession failed (status=%s).\n", QCC_StatusText(status));
            }
            s_joinComplete = true;
        }
    }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name='%s', oldOwner='%s', newOwner='%s'.\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }
};

/** Create the interface, report the result to stdout, and return the result status. */
QStatus CreateInterface(void)
{
    /* Add org.alljoyn.Bus.method_sample interface */
    InterfaceDescription* testIntf = NULL;
    QStatus status = g_msgBus->CreateInterface(INTERFACE_NAME, testIntf);

    if (status == ER_OK) {
        printf("Interface '%s' created.\n", INTERFACE_NAME);
        testIntf->AddMethod("cat", "ss",  "s", "inStr1,inStr2,outStr", 0);
        testIntf->Activate();
    } else {
        printf("Failed to create interface '%s'.\n", INTERFACE_NAME);
    }

    return status;
}

/** Start the message bus, report the result to stdout, and return the result status. */
QStatus StartMessageBus(void)
{
    QStatus status = g_msgBus->Start();

    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("BusAttachment::Start failed.\n");
    }

    return status;
}

/** Handle the connection to the bus, report the result to stdout, and return the result status. */
QStatus ConnectToBus(void)
{
    QStatus status = g_msgBus->Connect();

    if (ER_OK == status) {
        printf("BusAttachment connected to '%s'.\n", g_msgBus->GetConnectSpec().c_str());
    } else {
        printf("BusAttachment::Connect('%s') failed.\n", g_msgBus->GetConnectSpec().c_str());
    }

    return status;
}

/** Register a bus listener in order to get discovery indications and report the event to stdout. */
void RegisterBusListener(void)
{
    /* Static bus listener */
    static MyBusListener s_busListener;

    g_msgBus->RegisterBusListener(s_busListener);
    printf("BusListener Registered.\n");
}
/*
 * Print out the fields found in the AboutData. Only fields with known signatures
 * are printed out.  All others will be treated as an unknown field.
 */
void printAboutData(AboutData& aboutData, const char* language, int tabCount)
{
    size_t fieldCount = aboutData.GetFields();

    const char** fields = new const char*[fieldCount];
    aboutData.GetFields(fields, fieldCount);

    for (size_t i = 0; i < fieldCount; ++i) {
        for (int j = 0; j < tabCount; ++j) {
            printf("\t");
        }
        printf("Key: %s", fields[i]);

        MsgArg* tmp;
        aboutData.GetField(fields[i], tmp, language);
        printf("\t");
        if (tmp->Signature() == "s") {
            const char* tmp_s;
            tmp->Get("s", &tmp_s);
            printf("%s", tmp_s);
        } else if (tmp->Signature() == "as") {
            size_t las;
            MsgArg* as_arg;
            tmp->Get("as", &las, &as_arg);
            for (size_t j = 0; j < las; ++j) {
                const char* tmp_s;
                as_arg[j].Get("s", &tmp_s);
                printf("%s ", tmp_s);
            }
        } else if (tmp->Signature() == "ay") {
            size_t lay;
            uint8_t* pay;
            tmp->Get("ay", &lay, &pay);
            for (size_t j = 0; j < lay; ++j) {
                printf("%02x ", pay[j]);
            }
        } else {
            printf("User Defined Value\tSignature: %s", tmp->Signature().c_str());
        }
        printf("\n");
    }
    delete [] fields;
}

class MyAboutListener : public AboutListener, public SessionListener {
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg)
    {
        AboutObjectDescription objectDescription;
        objectDescription.CreateFromMsgArg(objectDescriptionArg);
        printf("*********************************************************************************\n");
        printf("Announce signal discovered\n");
        printf("\tFrom bus %s\n", busName);
        printf("\tAbout version %hu\n", version);
        printf("\tSessionPort %hu\n", port);
        printf("\tObjectDescription:\n");
        AboutObjectDescription aod(objectDescriptionArg);
        size_t pathCount = aod.GetPaths(NULL, 0);
        const char** paths = new const char*[pathCount];
        aod.GetPaths(paths, pathCount);
        for (size_t i = 0; i < pathCount; ++i) {
            printf("\t\t%s\n", paths[i]);
            size_t interfaceCount = aod.GetInterfaces(paths[i], NULL, 0);
            const char** interfaces = new const char*[interfaceCount];
            aod.GetInterfaces(paths[i], interfaces, interfaceCount);
            for (size_t j = 0; j < interfaceCount; ++j) {
                printf("\t\t\t%s\n", interfaces[j]);
            }
            delete [] interfaces;
        }
        delete [] paths;
        printf("\tAboutData:\n");
        AboutData aboutData(aboutDataArg);
        printAboutData(aboutData, NULL, 2);
        printf("*********************************************************************************\n");
        if (g_msgBus != NULL) {
            SessionId sessionId;
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            g_msgBus->EnableConcurrentCallbacks();
            s_sessionHost = busName;
            QStatus status = g_msgBus->JoinSession(busName, port, this, sessionId, opts);
            printf("SessionJoined sessionId = %u, status = %s\n", sessionId, QCC_StatusText(status));
            s_joinComplete = true;
        }
    }

};

/** Begin discovery on the well-known name of the service to be called, report the result to
   stdout, and return the result status. */
QStatus FindAbout(void)
{

    MyAboutListener* aboutListener = new MyAboutListener();
    g_msgBus->RegisterAboutListener(*aboutListener);

    /* Passing NULL into WhoImplements will listen for all About announcements */
    QStatus status = g_msgBus->WhoImplements(NULL);
    if (ER_OK == status) {
        printf("WhoImplements NULL called.\n");
    } else {
        printf("WhoImplements call FAILED with status %s\n", QCC_StatusText(status));
        exit(1);
    }

    return status;
}

/** Wait for join session to complete, report the event to stdout, and return the result status. */
QStatus WaitForJoinSessionCompletion(void)
{
    unsigned int count = 0;

    while (!s_joinComplete && !s_interrupt) {
        if (0 == (count++ % 10)) {
            printf("Waited %u seconds for JoinSession completion.\n", count / 10);
        }

#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
    return s_joinComplete && !s_interrupt ? ER_OK : ER_ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED;
}

/** Do a method call, report the result to stdout, and return the result status. */
QStatus MakeMethodCall(void)
{
    ProxyBusObject remoteObj(*g_msgBus, s_sessionHost.c_str(), SERVICE_PATH, s_sessionId);
    const InterfaceDescription* alljoynTestIntf = g_msgBus->GetInterface(INTERFACE_NAME);

    assert(alljoynTestIntf);
    remoteObj.AddInterface(*alljoynTestIntf);

    Message reply(*g_msgBus);
    MsgArg inputs[2];

    inputs[0].Set("s", "Hello ");
    inputs[1].Set("s", "World!");

    QStatus status = remoteObj.MethodCall(INTERFACE_NAME, "cat", inputs, 2, reply, 25000);

    if (ER_OK == status) {
        printf("'%s.%s' (path='%s') returned '%s'.\n", SERVICE_NAME, "cat",
               SERVICE_PATH, reply->GetArg(0)->v_string.str);
    } else {
        printf("MethodCall on '%s.%s' failed.", SERVICE_NAME, "cat");
    }

    return status;
}

/** Main entry point */
int CDECL_CALL main(int argc, char** argv, char** envArg)
{
    QCC_UNUSED(argc);
    QCC_UNUSED(argv);
    QCC_UNUSED(envArg);

    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }
#endif

    printf("AllJoyn Library version: %s.\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s.\n", ajn::GetBuildInfo());

    /* Install SIGINT handler. */
    signal(SIGINT, SigIntHandler);

    QStatus status = ER_OK;

    /* Create message bus. */
    g_msgBus = new BusAttachment("myApp", true);

    /* This test for NULL is only required if new() behavior is to return NULL
     * instead of throwing an exception upon an out of memory failure.
     */
    if (!g_msgBus) {
        status = ER_OUT_OF_MEMORY;
    }

    if (ER_OK == status) {
        status = CreateInterface();
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    if (ER_OK == status) {
        status = ConnectToBus();
    }

    if (ER_OK == status) {
        RegisterBusListener();
        status = FindAbout();
    }

    if (ER_OK == status) {
        status = WaitForJoinSessionCompletion();
    }

    if (ER_OK == status) {
        status = MakeMethodCall();
    }

    /* Deallocate bus */
    delete g_msgBus;
    g_msgBus = NULL;

    printf("Basic client exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
