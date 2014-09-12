/**
 * @file
 * A test program that can send/receive large signals (64KB)
 */
/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#include <iostream>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/Mutex.h>
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

static const char* DefaultWellKnownName = "org.alljoyn.LargeSignals";
static const char* ObjectPath = "/org/alljoyn/LargeSignals";
static const char* InterfaceName = "org.alljoyn.LargeSignals";
static const SessionPort g_SessionPort = 42;

static SessionId g_sessionId = 0;
static String g_joiner;

class MyBusListener;
static MyBusListener* g_myBusListener = NULL;
static ProxyBusObject* g_proxyObj = NULL;

/* Static top level globals */
static BusAttachment* g_msgBus = NULL;
static String g_wellKnownName = DefaultWellKnownName;
static bool g_echoBack = false;

static volatile sig_atomic_t g_interrupt = false;
static volatile sig_atomic_t g_session_joined = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

class MyBusListener : public SessionPortListener, public SessionListener {

  public:
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        return true;
    }

    void SessionJoined(SessionPort sessionPort, SessionId sessionId, const char* joiner)
    {
        QCC_SyncPrintf("Session Established: joiner=%s, sessionId=%08x\n", joiner, sessionId);
        g_session_joined = true;
        /* Enable concurrent callbacks since some of the calls below could block */
        g_msgBus->EnableConcurrentCallbacks();

        QStatus status = g_msgBus->SetSessionListener(sessionId, this);
        if (status != ER_OK) {
            QCC_LogError(status, ("SetSessionListener failed"));
            return;
        }

        /* Set the link timeout */
        uint32_t timeout = 10;
        status = g_msgBus->SetLinkTimeout(sessionId, timeout);
        if (status == ER_OK) {
            QCC_SyncPrintf("Link timeout was successfully set to %d\n", timeout);
        } else {
            QCC_LogError(status, ("SetLinkTimeout failed"));
        }

        g_sessionId = sessionId;
        g_joiner = joiner;
    }

    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        QCC_SyncPrintf("SessionLost(%08x) was called. Reason = %u.\n", sessionId, reason);
    }

};

class LocalTestObject : public BusObject {
  public:

    LocalTestObject(const char*path) : BusObject(path)
    {
        const InterfaceDescription* Intf = g_msgBus->GetInterface(InterfaceName);
        assert(Intf);
        AddInterface(*Intf);

        my_signal_member = Intf->GetMember("large_signal");
        assert(my_signal_member);
    }

    QStatus SendSignal(SessionId sessionId) {
        static uint32_t u = 0;
        MsgArg arg[2];
        uint16_t rand_len =  1 + qcc::Rand16() % (65534);
        uint8_t* buf = new uint8_t[rand_len];

        //Fill in the first and the last byte of the array
        uint8_t randData = 1 + qcc::Rand16() % 255;
        buf[0] = randData;
        buf[rand_len - 1] = randData;

        arg[1].Set("ay", rand_len, buf);
        arg[0].Set("u", u);
        QStatus status = Signal(NULL, sessionId, *my_signal_member, arg, 2, 0, 0);
        if (ER_OK != status) {
            QCC_LogError(status, ("Error sending signal."));
        }
        std::cout << "<=== Sending signal with " << rand_len << "bytes" << std::endl;
        delete buf;
        u++;
        qcc::Sleep(1000);
        return status;
    }

    void SignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg) {
        uint32_t u(msg->GetArg(0)->v_uint32);
        uint32_t length = msg->GetArg(1)->v_scalarArray.numElements;
        uint8_t firstByte = msg->GetArg(1)->v_scalarArray.v_byte[0];
        uint8_t lastByte  = msg->GetArg(1)->v_scalarArray.v_byte[length - 1];
        QCC_SyncPrintf("======> Signal Received. #- %u  Bytes= %u  firstByte= %u  lastByte= %u \n", u, length, firstByte, lastByte);

        if (firstByte != lastByte) {
            QCC_SyncPrintf("*****************  INTEGRITY ERROR - first element != last element \n");
            _exit(-1);
        }

        //To echo a signal back, apply concurrent callbacks.
        g_msgBus->EnableConcurrentCallbacks();

        //SetProperty to continue the loop.
        MsgArg val;
        val.Set("b", true);
        QStatus status = g_proxyObj->SetProperty(InterfaceName, "ok_to_send", val);
        if (status != ER_OK) {
            QCC_LogError(status, ("Set property failed."));
            _exit(-1);
        }

        if (g_echoBack) {
            QStatus status = this->SendSignal(g_sessionId);
            if (status != ER_OK) {
                QCC_LogError(status, ("Error sending echo signal. "));
            }
        }
    }


  private:
    const InterfaceDescription::Member* my_signal_member;
};

static void usage(void)
{
    std::cout << "Usage: bbsigtest\n"
              << "\t-n <well-known name> \n"
              << "\t-c <signal count> (useful only in sender mode) \n"
              << "\t-s sender mode \n"
              << "\t-e echo back \n"
              << "\t-h/-? display usage \n";
}

int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    uint32_t signalCount = 1000;
    bool signalSender = false;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i]) || 0 == strcmp("-?", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                std::cout << "option " << argv[i - 1] << " requires a parameter" << std::endl;
                usage();
                exit(1);
            } else {
                g_wellKnownName = argv[i];
            }
        } else if (0 == strcmp("-c", argv[i])) {
            ++i;
            if (i == argc) {
                std::cout << "option " << argv[i - 1] << " requires a parameter" << std::endl;
                usage();
                exit(1);
            } else {
                signalCount = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-s", argv[i])) {
            signalSender = true;
        } else if (0 == strcmp("-e", argv[i])) {
            g_echoBack = true;
        } else {
            status = ER_FAIL;
            std::cout << "Unknown option: " << argv[i] << std::endl;
            usage();
            exit(1);
        }
    }

    /* Create message bus */
    g_msgBus = new BusAttachment("bbsigtest", true);

    /* Add org.alljoyn.alljoyn_test interface */
    InterfaceDescription* testIntf = NULL;
    status = g_msgBus->CreateInterface(InterfaceName, testIntf);
    if (ER_OK == status) {
        testIntf->AddSignal("large_signal", "uay", NULL, 0);
        testIntf->AddProperty("ok_to_send", "b", PROP_ACCESS_RW);
        testIntf->Activate();
    } else {
        QCC_LogError(status, ("Failed to create interface %s", InterfaceName));
        return status;
    }

    status = g_msgBus->Start();
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to start bus attachment."));
        return status;
    }

    g_myBusListener = new MyBusListener();

    Environ* env = Environ::GetAppEnviron();
    qcc::String clientArgs = env->Find("BUS_ADDRESS");
    /* Connect to the daemon */
    status = clientArgs.empty() ? g_msgBus->Connect() : g_msgBus->Connect(clientArgs.c_str());
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to connect to \"%s\"", clientArgs.c_str()));
        return status;
    }

    LocalTestObject testObj(ObjectPath);
    g_msgBus->RegisterBusObject(testObj);
    /* Register the signal handler with the bus */
    const InterfaceDescription::Member* signal_member = testIntf->GetMember("large_signal");
    assert(signal_member);
    status = g_msgBus->RegisterSignalHandler(&testObj,
                                             static_cast<MessageReceiver::SignalHandler>(&LocalTestObject::SignalHandler),
                                             signal_member,
                                             NULL);

    SessionPort sessionPort = g_SessionPort;
    status = g_msgBus->BindSessionPort(sessionPort, opts, *g_myBusListener);
    if (status != ER_OK) {
        QCC_LogError(status, ("BindSessionPort failed"));
        return status;
    }

    /* Request a well-known name */
    status = g_msgBus->RequestName(g_wellKnownName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (status != ER_OK) {
        QCC_LogError(status, ("RequestName(%s) failed.", g_wellKnownName.c_str()));
        return status;
    }

    /* Begin Advertising the well-known name */
    status = g_msgBus->AdvertiseName(g_wellKnownName.c_str(), opts.transports);
    if (ER_OK != status) {
        QCC_LogError(status, ("AdvertiseName(%s) failed.", g_wellKnownName.c_str()));
        return status;
    }

    QCC_SyncPrintf("Waiting for session to be established.. \n");
    while (!g_session_joined) {
        qcc::Sleep(100);
    }
    QCC_SyncPrintf("Session established.. \n");

    g_proxyObj = new ProxyBusObject(*g_msgBus, g_joiner.c_str(), ObjectPath, g_sessionId);
    status = g_proxyObj->IntrospectRemoteObject();
    if (status != ER_OK) {
        QCC_LogError(status, ("Introspection of proxy bus object failed."));
        return status;
    }

    MsgArg val;
    val.Set("b", true);
    status = g_proxyObj->SetProperty(InterfaceName, "ok_to_send", val);
    if (status != ER_OK) {
        QCC_LogError(status, ("Set property failed."));
        return status;
    }

    if (signalSender) {
        for (uint32_t i = 0; i < signalCount && !g_interrupt; i++) {
            status = testObj.SendSignal(g_sessionId);
            if (status != ER_OK) {
                QCC_LogError(status, ("Error while sending signal (# %u of %u).", i, signalCount));
                break;
            }
        }
    }

    while (g_interrupt == false) {
        qcc::Sleep(100);
    }

    /* Clean up msg bus */
    delete g_msgBus;
    delete g_myBusListener;

    std::cout << argv[0] << " exiting with status " << QCC_StatusText(status) << std::endl;

    return (int) status;
}
