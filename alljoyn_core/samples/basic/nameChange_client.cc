/**
 * @file
 * @brief  Sample implementation of an AllJoyn client.
 *
 * This is a simple client that can change the 'name' property of the
 * 'org.alljoyn.Bus.signal_sample' service then exit. Can also be used
 * to run introspect on 'org.alljoyn.Bus.signal_sample'.
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <qcc/platform.h>

#include <signal.h>
#include <stdio.h>
#include <vector>
#include <memory>

#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Debug.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Init.h>
#include <alljoyn/Status.h>
#include <alljoyn/version.h>

using namespace std;
using namespace qcc;
using namespace ajn;

/** Static top level message bus object */
static BusAttachment* s_msgBus = nullptr;

static const char* INTERFACE_NAME = "org.alljoyn.Bus.signal_sample";
static const char* SERVICE_NAME = "org.alljoyn.Bus.signal_sample";
static const char* SERVICE_PATH = "/";
static const SessionPort SERVICE_PORT = 25;

static bool s_joinComplete = false;
static String s_sessionHost;
static SessionId s_sessionId = 0;
static qcc::Mutex* s_sessionLock = nullptr;

static volatile sig_atomic_t s_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    s_interrupt = true;
}

/** Inform the app thread that JoinSession is complete, store the session ID. */
class MyJoinCallback : public BusAttachment::JoinSessionAsyncCB {
    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) {
        QCC_UNUSED(opts);
        QCC_UNUSED(context);

        if (ER_OK == status) {
            printf("JoinSession SUCCESS (Session id=%u).\n", sessionId);
            s_sessionLock->Lock(MUTEX_CONTEXT);
            s_sessionId = sessionId;
            s_joinComplete = true;
            s_sessionLock->Unlock(MUTEX_CONTEXT);
        } else {
            printf("JoinSession failed (status=%s).\n", QCC_StatusText(status));
        }
    }
};

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener {
  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        s_sessionLock->Lock(MUTEX_CONTEXT);
        if (0 == strcmp(name, SERVICE_NAME) && s_sessionHost.empty()) {
            s_sessionHost = name;
            s_sessionLock->Unlock(MUTEX_CONTEXT);
            printf("FoundAdvertisedName(name='%s', transport = 0x%x, prefix='%s')\n", name, transport, namePrefix);

            /* We found a remote bus that is advertising basic service's well-known name so connect to it. */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status = s_msgBus->JoinSessionAsync(name, SERVICE_PORT, nullptr, opts, &joinCb);
            if (ER_OK != status) {
                printf("JoinSessionAsync failed (status=%s)", QCC_StatusText(status));
            }
        } else {
            s_sessionLock->Unlock(MUTEX_CONTEXT);
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

  private:
    MyJoinCallback joinCb;
};

/** Start the message bus, report the result to stdout, and return the result status. */
QStatus StartMessageBus(void)
{
    QStatus status = s_msgBus->Start();

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
    QStatus status = s_msgBus->Connect();

    if (ER_OK == status) {
        printf("BusAttachment connected to '%s'.\n", s_msgBus->GetConnectSpec().c_str());
    } else {
        printf("BusAttachment::Connect('%s') failed.\n", s_msgBus->GetConnectSpec().c_str());
    }

    return status;
}

class ServiceSignalReceiver : public MessageReceiver {
  public:
    ServiceSignalReceiver() : m_signalReceivedFlag(false), m_msg(nullptr) { }
    void SignalHandler(const InterfaceDescription::Member* member,
                       const char* sourcePath, Message& msg) {
        QCC_UNUSED(member);
        QCC_UNUSED(sourcePath);
        m_signalReceivedFlag = true;
        m_msg.reset(new Message(msg));
    }
    void WaitForSignal() {
        size_t count = 0;
        while (!m_signalReceivedFlag && (count <= 100)) {
            if (0 == (count++ % 10)) {
                printf("Waited %zu seconds for signal.\n", count / 10);
            }

    #ifdef _WIN32
            Sleep(100);
    #else
            usleep(100 * 1000);
    #endif
        }
    }
    bool m_signalReceivedFlag;
    unique_ptr<Message> m_msg;
};

/** Begin discovery on the well-known name of the service to be called, report the result to
 *  stdout, and return the result status.
 */
QStatus FindAdvertisedName(void)
{
    /* Begin discovery on the well-known name of the service to be called */
    QStatus status = s_msgBus->FindAdvertisedName(SERVICE_NAME);

    if (status == ER_OK) {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') succeeded.\n", SERVICE_NAME);
    } else {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') failed (%s).\n", SERVICE_NAME, QCC_StatusText(status));
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

    return (s_joinComplete && !s_interrupt) ? ER_OK : ER_ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED;
}

void DoCleanup(void)
{
    fflush(stdout);
    if (s_sessionId != 0) {
        s_msgBus->LeaveJoinedSession(s_sessionId);
    }
    s_msgBus->CancelFindAdvertisedName(SERVICE_NAME);
    if (s_msgBus->IsConnected()) {
        s_msgBus->Disconnect();
    }
    if (ER_OK == s_msgBus->Stop()) {
        s_msgBus->Join();
    }
}

QStatus DoNameChange(ProxyBusObject& remoteObj, const String& newName)
{
    QStatus status = remoteObj.SetProperty(INTERFACE_NAME, "name", newName.c_str());
    if (ER_OK == status) {
        printf("SetProperty to change the 'name' property to '%s' was successful.\n", newName.c_str());
    } else {
        printf("Error calling SetProperty to change the 'name' property (%s).\n", QCC_StatusText(status));
    }

    return status;
}

QStatus DoIntrospect(ProxyBusObject& remoteObj, const String& lang)
{
    QStatus status = remoteObj.IntrospectRemoteObject();
    if (ER_OK != status) {
        printf("Introspection of '%s' (path='%s') failed (%s).\n", SERVICE_NAME, SERVICE_PATH, QCC_StatusText(status));
        return status;
    }
    uint32_t timeout = 30000;

    Message replyMsg(*s_msgBus);
    if (!lang.empty()) {
        MsgArg msgArg("s", lang.c_str());
        printf("Calling %s.IntrospectWithDescription.\n", org::allseen::Introspectable::InterfaceName);
        status = remoteObj.MethodCall(org::allseen::Introspectable::InterfaceName, "IntrospectWithDescription", &msgArg, 1, replyMsg, timeout);
    } else {
        printf("Calling %s.Introspect.\n", org::freedesktop::DBus::Introspectable::InterfaceName);
        status = remoteObj.MethodCall(org::freedesktop::DBus::Introspectable::InterfaceName, "Introspect", nullptr, 0, replyMsg, timeout);
    }

    /* Parse the XML reply */
    if (ER_OK == status) {
        printf("Introspection XML in sample:\n%s\n\n", replyMsg->GetArg(0)->v_string.str);
        qcc::String ident = replyMsg->GetSender();
        ident += " : ";
        ident += replyMsg->GetObjectPath();
        status = remoteObj.ParseXml(replyMsg->GetArg(0)->v_string.str, ident.c_str());
    } else {
        printf("Introspection failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

static void usage(void)
{
    printf("Usage: nameChange_Client [-h] [-i [lang]] [-n <nameToChangeTo>] \n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -i [lang]             = Call introspect. If language is provided calls %s.IntrospectWithDescription.\n", org::allseen::Introspectable::InterfaceName);
    printf("                           Supported languages are: en, de, hi. When no language is provided call %s.Introspect.\n", org::freedesktop::DBus::Introspectable::InterfaceName);
    printf("   -n <nameToChangeTo>   = Change name to \"nameToChangeTo\" and wait for sessionless signal nameChanged in response.\n");
    printf("\n");
}

/** Main entry point */
int CDECL_CALL main(int argc, char** argv, char** envArg)
{
    QCC_UNUSED(envArg);
    MyBusListener s_busListener;
    bool callIntrospect = false;
    String lang;
    String newName;

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

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                cout << "option " << argv[i - 1] << " requires a parameter" << endl;
                usage();
                exit(1);
            } else {
                newName = argv[i];
            }
        } else if (0 == strcmp("-i", argv[i])) {
            ++i;
            if ((i != argc) && (0 != strcmp("-h", argv[i])) && (0 != strcmp("-n", argv[i]))) {
                lang = argv[i];
            } else {
                --i;
            }
            callIntrospect = true;
        } else {
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    QStatus status = ER_OK;

    /* Create message bus */
    s_msgBus = new BusAttachment("nameChange_client", true);
    s_sessionLock = new qcc::Mutex();
    /* This test for nullptr is only required if new() behavior is to return nullptr
     * instead of throwing an exception upon an out of memory failure.
     */
    if ((s_msgBus == nullptr) || (s_sessionLock == nullptr)) {
        status = ER_OUT_OF_MEMORY;
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    if (ER_OK == status) {
        status = ConnectToBus();
    }

    s_msgBus->RegisterBusListener(s_busListener);

    if (ER_OK == status) {
        status = FindAdvertisedName();
    }

    if (ER_OK == status) {
        status = WaitForJoinSessionCompletion();
    }

    if ((ER_OK == status) && callIntrospect) {
        s_sessionLock->Lock(MUTEX_CONTEXT);
        ProxyBusObject remoteObj(*s_msgBus, SERVICE_NAME, SERVICE_PATH, s_sessionId);
        s_sessionLock->Unlock(MUTEX_CONTEXT);
        status = DoIntrospect(remoteObj, lang);
    }

    if ((ER_OK == status) && !newName.empty()) {
        s_sessionLock->Lock(MUTEX_CONTEXT);
        ProxyBusObject remoteObj(*s_msgBus, SERVICE_NAME, SERVICE_PATH, s_sessionId);
        s_sessionLock->Unlock(MUTEX_CONTEXT);
        ServiceSignalReceiver signalReceiver;
        status = remoteObj.IntrospectRemoteObject();
        if (ER_OK == status) {
            s_msgBus->AddMatch("sessionless='t'");
            status = s_msgBus->RegisterSignalHandler(&signalReceiver,
                                                     static_cast<MessageReceiver::SignalHandler>(&ServiceSignalReceiver::SignalHandler),
                                                     remoteObj.GetInterface(INTERFACE_NAME)->GetMember("nameChanged"),
                                                     nullptr
                                                     );
            if (ER_OK != status) {
                printf("RegisterSignalHandler failed (%s).\n", QCC_StatusText(status));
            } else {
                status = DoNameChange(remoteObj, newName);
                signalReceiver.WaitForSignal();
                if (signalReceiver.m_signalReceivedFlag) {
                    printf("Received sessionless signal echoing new name '%s'.\n", (*signalReceiver.m_msg.get())->GetArg(0)->v_string.str);
                }
                s_msgBus->UnregisterSignalHandler(&signalReceiver,
                                                  static_cast<MessageReceiver::SignalHandler>(&ServiceSignalReceiver::SignalHandler),
                                                  remoteObj.GetInterface(INTERFACE_NAME)->GetMember("nameChanged"),
                                                  nullptr
                                                  );
            }
        } else {
            printf("Introspection of '%s' (path='%s') failed (%s).\n", SERVICE_NAME, SERVICE_PATH, QCC_StatusText(status));
            printf("Make sure the service is running before launching the client.\n");
        }
    }

    /* Clean up */
    DoCleanup();
    if (s_msgBus != nullptr) {
        delete s_msgBus;
    }
    if (s_sessionLock != nullptr) {
        delete s_sessionLock;
    }

    printf("Name change client exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
