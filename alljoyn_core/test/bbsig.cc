/**
 * @file
 * Message Bus Client
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <signal.h>
#include <stdio.h>
#include <assert.h>

#include <vector>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Environ.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;

/** Constants */
namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* InterfaceName = "org.alljoyn.alljoyn_test";
const char* DefaultWellKnownName = "org.alljoyn.alljoyn_test";
const char* DefaultAdvertiseName = "org.alljoyn.alljoyn_sig";
const char* ObjectPath = "/org/alljoyn/alljoyn_test";
const SessionPort SessionPort = 24;   /**< Well-known session port value for bbsig */
}
}
}

/** Static top level message bus object */
static BusAttachment* g_msgBus = NULL;
static String g_wellKnownName = ::org::alljoyn::alljoyn_test::DefaultWellKnownName;
static String g_advertiseName = ::org::alljoyn::alljoyn_test::DefaultAdvertiseName;
static Event g_discoverEvent;
static bool g_selfjoin;

static TransportMask g_preferredTransport = 0;

static bool compress = false;
static bool encryption = false;
static bool broacast = false;
static unsigned long timeToLive = 0;
static String g_testAboutApplicationName = "bbservice";
static bool g_useAboutFeatureDiscovery = false;

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_SyncPrintf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);

        if ((transport & g_preferredTransport) == 0) {
            QCC_SyncPrintf("FoundAdvertisedName(): not interested in transport=0x%x\n", transport);
            return;
        }

        /* We must enable concurrent callbacks since some of the calls below are blocking */
        g_msgBus->EnableConcurrentCallbacks();

        if (0 == strcmp(name, g_wellKnownName.c_str())) {
            /* We found a remote bus that is advertising the well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, transport);
            QStatus status = g_msgBus->JoinSession(name, ::org::alljoyn::alljoyn_test::SessionPort, this, sessionId, opts);
            if (ER_OK == status) {
                if (encryption) {
                    ProxyBusObject remotePeerObj(*g_msgBus, name, "/", 0);
                    status = remotePeerObj.SecureConnection();
                    if (status != ER_OK) {
                        QCC_LogError(status, ("Failed to authenticate remote peer (status=%s)", QCC_StatusText(status)));
                    }
                }
                /* Release main thread */
                g_discoverEvent.SetEvent();
            } else {
                QCC_LogError(status, ("JoinSession failed (status=%s)", QCC_StatusText(status)));
            }
        }
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_SyncPrintf("LostAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);
        if (0 == strcmp(name, g_wellKnownName.c_str())) {
            g_discoverEvent.ResetEvent();
        }
    }

    void NameOwnerChanged(const char* name, const char* previousOwner, const char* newOwner)
    {
        QCC_SyncPrintf("NameOwnerChanged(%s, %s, %s)\n",
                       name,
                       previousOwner ? previousOwner : "null",
                       newOwner ? newOwner : "null");
    }

    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        QCC_SyncPrintf("SessionLost(%u) was called. Reason = %u.\n", sessionId, reason);
    }

    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
};

/** Static bus listener */
static MyBusListener g_busListener;

class MyAboutListener : public AboutListener {
  public:
    MyAboutListener() : sessionId(0) { }
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        AboutData ad;
        ad.CreatefromMsgArg(aboutDataArg);

        char* appName;
        ad.GetAppName(&appName);

        if (appName != NULL && strcmp(g_testAboutApplicationName.c_str(), appName) == 0) {
            QCC_SyncPrintf("Found Announced interface name=%s\n", busName);

            /* We must enable concurrent callbacks since some of the calls below are blocking */
            g_msgBus->EnableConcurrentCallbacks();

            /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

            QStatus status = g_msgBus->JoinSession(busName, ::org::alljoyn::alljoyn_test::SessionPort, &g_busListener, sessionId, opts);
            if (ER_OK == status) {
                if (encryption) {
                    ProxyBusObject remotePeerObj(*g_msgBus, busName, "/", 0);
                    status = remotePeerObj.SecureConnection();
                    if (status != ER_OK) {
                        QCC_LogError(status, ("Failed to authenticate remote peer (status=%s)", QCC_StatusText(status)));
                    }
                }
                /* Release main thread */
                g_discoverEvent.SetEvent();
            } else {
                QCC_LogError(status, ("JoinSession failed (status=%s)", QCC_StatusText(status)));
            }
        }
    }

    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
};

static MyAboutListener g_aboutListener;

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

class LocalTestObject : public BusObject {

  public:

    LocalTestObject(BusAttachment& bus, const char* path, unsigned long signalDelay, unsigned long disconnectDelay,
                    unsigned long reportInterval, unsigned long maxSignals) :
        BusObject(path),
        signalDelay(signalDelay),
        disconnectDelay(disconnectDelay),
        reportInterval(reportInterval),
        maxSignals(maxSignals),
        my_signal_member(NULL)
    {
        QStatus status;

        /* Add org.alljoyn.alljoyn_test interface */
        InterfaceDescription* testIntf = NULL;
        status = bus.CreateInterface(::org::alljoyn::alljoyn_test::InterfaceName, testIntf);
        if ((ER_OK == status) && testIntf) {
            testIntf->AddSignal("my_signal", "a{ys}", NULL, 0);
            testIntf->AddMethod("my_ping", "s", "s", "outStr,inStr", 0);
            testIntf->Activate();
        } else {
            QCC_LogError(status, ("Failed to create interface %s", ::org::alljoyn::alljoyn_test::InterfaceName));
            return;
        }

        if (ER_OK == status) {
            if (g_useAboutFeatureDiscovery) {
                AddInterface(*testIntf, ANNOUNCED);
            } else {
                AddInterface(*testIntf);
            }
        }


        /* Get my_signal member */
        if (ER_OK == status) {
            my_signal_member = testIntf->GetMember("my_signal");
            assert(my_signal_member);

            status =  bus.RegisterSignalHandler(this,
                                                static_cast<MessageReceiver::SignalHandler>(&LocalTestObject::SignalHandler),
                                                my_signal_member,
                                                NULL);

            if (status != ER_OK) {
                QCC_SyncPrintf("Failed to register signal handler for 'org.alljoyn.alljoyn_test.my_signal': %s\n", QCC_StatusText(status));
            }
        }

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { testIntf->GetMember("my_ping"), static_cast<MessageReceiver::MethodHandler>(&LocalTestObject::Ping) },
        };
        status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to register method handlers for LocalTestObject"));
        }
    }

    QStatus SendSignal() {
        uint8_t flags = ALLJOYN_FLAG_GLOBAL_BROADCAST;
        assert(my_signal_member);
        MsgArg arg("a{ys}", 0, NULL);
        if (compress) {
            flags |= ALLJOYN_FLAG_COMPRESSED;
        }
        if (encryption) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        if (broacast) {
            return Signal(NULL, 0, *my_signal_member, &arg, 1, timeToLive, flags);
        } else {
            if (g_useAboutFeatureDiscovery) {
                return Signal(NULL, g_aboutListener.GetSessionId(), *my_signal_member, &arg, 1, timeToLive, flags);
            } else {
                return Signal(NULL, g_busListener.GetSessionId(), *my_signal_member, &arg, 1, timeToLive, flags);
            }
        }
    }

    void SignalHandler(const InterfaceDescription::Member* member,
                       const char* sourcePath,
                       Message& msg)
    {
        if ((++rxCounts[sourcePath] % reportInterval) == 0) {
            QCC_SyncPrintf("RxSignal: %s - %u\n", sourcePath, rxCounts[sourcePath]);
            if (msg->IsEncrypted()) {
                QCC_SyncPrintf("Authenticated using %s\n", msg->GetAuthMechanism().c_str());
            }
        }
    }

    void RegisterSignalHandler()
    {
        assert(bus);
        QStatus status = bus->AddMatch("type='signal',interface='org.alljoyn.alljoyn_test',member='my_signal'");
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register Match rule for 'org.alljoyn.alljoyn_test.my_signal'"));
        }
    }


    void ObjectRegistered(void)
    {
        assert(bus);
        BusObject::ObjectRegistered();

        /* Request a well-known name */
        /* Note that you cannot make a blocking method call here */
        const ProxyBusObject& dbusObj = bus->GetDBusProxyObj();
        MsgArg args[2];
        args[0].Set("s", g_advertiseName.c_str());
        args[1].Set("u", DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        QStatus status = dbusObj.MethodCallAsync(ajn::org::freedesktop::DBus::InterfaceName,
                                                 "RequestName",
                                                 this,
                                                 static_cast<MessageReceiver::ReplyHandler>(&LocalTestObject::NameAcquiredCB),
                                                 args,
                                                 ArraySize(args));
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to request name %s", g_advertiseName.c_str()));
        }
    }

    void NameAcquiredCB(Message& msg, void* context)
    {
        assert(bus);
        /* Check name acquired result */
        size_t numArgs;
        const MsgArg* args;
        msg->GetArgs(numArgs, args);

        if (args[0].v_uint32 == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
            /* Begin Advertising the well known name to remote busses */
            const ProxyBusObject& alljoynObj = bus->GetAllJoynProxyObj();
            MsgArg args[2];
            size_t numArgs = ArraySize(args);
            MsgArg::Set(args, numArgs, "sq", g_advertiseName.c_str(), g_preferredTransport);
            QStatus status = alljoynObj.MethodCallAsync(ajn::org::alljoyn::Bus::InterfaceName,
                                                        "AdvertiseName",
                                                        this,
                                                        static_cast<MessageReceiver::ReplyHandler>(&LocalTestObject::AdvertiseRequestCB),
                                                        args,
                                                        numArgs);
            if (ER_OK != status) {
                QCC_LogError(status, ("Sending org.alljoyn.Bus.Advertise failed"));
            }
        } else {
            QCC_LogError(ER_FAIL, ("Failed to obtain name \"%s\". RequestName returned %d",
                                   g_advertiseName.c_str(), args[0].v_uint32));
        }
    }

    void AdvertiseRequestCB(Message& msg, void* context)
    {
        /* Make sure request was processed */
        size_t numArgs;
        const MsgArg* args;
        msg->GetArgs(numArgs, args);

        if ((MESSAGE_METHOD_RET != msg->GetType()) || (ALLJOYN_ADVERTISENAME_REPLY_SUCCESS != args[0].v_uint32)) {
            QCC_LogError(ER_FAIL, ("Failed to advertise name \"%s\". org.alljoyn.Bus.Advertise returned %d",
                                   g_advertiseName.c_str(),
                                   args[0].v_uint32));
        }
    }

    void Ping(const InterfaceDescription::Member* member, Message& msg)
    {
        /* Reply with same string that was sent to us */
        MsgArg arg(*(msg->GetArg(0)));
        printf("Pinged with: %s\n", msg->GetArg(0)->ToString().c_str());
        QStatus status = MethodReply(msg, &arg, 1);
        if (ER_OK != status) {
            QCC_LogError(status, ("Ping: Error sending reply"));
        }
    }

    map<qcc::String, size_t> rxCounts;

    unsigned long signalDelay;
    unsigned long disconnectDelay;
    unsigned long reportInterval;
    unsigned long maxSignals;
    const InterfaceDescription::Member* my_signal_member;
};


static const char x509cert[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n"
    "QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n"
    "N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n"
    "AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n"
    "h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n"
    "xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n"
    "AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n"
    "viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n"
    "PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n"
    "7THIAV79Lg==\n"
    "-----END CERTIFICATE-----"
};

static const char privKey[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: AES-128-CBC,0AE4BAB94CEAA7829273DD861B067DBA\n"
    "\n"
    "LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n"
    "jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n"
    "XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n"
    "w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n"
    "9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n"
    "YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n"
    "wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n"
    "Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n"
    "3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n"
    "AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n"
    "pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n"
    "DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n"
    "bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n"
    "-----END RSA PRIVATE KEY-----"
};

class MyAuthListener : public AuthListener {
  public:

    MyAuthListener(const qcc::String& userName, unsigned long maxAuth) : AuthListener(), userName(userName), maxAuth(maxAuth) { }

  private:

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {

        if (authCount > maxAuth) {
            return false;
        }

        if (strcmp(authMechanism, "ALLJOYN_PIN_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("ABCDEFGH");
            }
            return true;
        }

        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
                printf("AuthListener returning fixed pin \"%s\" for %s\n", creds.GetPassword().c_str(), authMechanism);
            }
            return true;
        }

        if (strcmp(authMechanism, "ALLJOYN_RSA_KEYX") == 0) {
            if (credMask & AuthListener::CRED_CERT_CHAIN) {
                creds.SetCertChain(x509cert);
            }
            if (credMask & AuthListener::CRED_PRIVATE_KEY) {
                creds.SetPrivateKey(privKey);
            }
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return true;
        }

        if (strcmp(authMechanism, "ALLJOYN_SRP_LOGON") == 0) {
            if (credMask & AuthListener::CRED_USER_NAME) {
                creds.SetUserName(userName);
            }
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return true;
        }

        return false;
    }

    bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds) {
        if (strcmp(authMechanism, "ALLJOYN_RSA_KEYX") == 0) {
            if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                printf("Verify\n%s\n", creds.GetCertChain().c_str());
                return true;
            }
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        printf("Authentication %s %s\n", authMechanism, success ? "succesful" : "failed");
    }

    void SecurityViolation(QStatus status, const Message& msg) {
        printf("Security violation %s\n", QCC_StatusText(status));
    }

    qcc::String userName;
    unsigned long maxAuth;
};

class MySessionPortListener : public SessionPortListener {

  public:
    MySessionPortListener() { }
    ~MySessionPortListener() { }
  private:

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner) {  }
};

static MySessionPortListener g_portListener;

static void usage(void)
{
    printf("Usage: bbsig [-n <name> ] [-a <name> ] [-h] [-l] [-s] [-r #] [-i #] [-c #] [-t #] [-x] [--tcp] [--udp] [--wfd] [-e[k] <mech>]\n\n");
    printf("Options:\n");
    printf("   -h                          = Print this help message\n");
    printf("   -?                          = Print this help message\n");
    printf("   -a <name>                   = Well-known name to advertise\n");
    printf("   -n <name>                   = Well-known name to find\n");
    printf("   -s                          = Enable stress mode (connect/disconnect w/ server between runs non-stop)\n");
    printf("   -l                          = Register signal handler for loopback\n");
    printf("   -r #                        = Signal rate (delay in ms between signals sent; default = 0)\n");
    printf("   -y #                        = Delay (in ms) between sending last signal and disconnecting (stress mode only)\n");
    printf("   -i #                        = Signal report interval (number of signals tx/rx per update; default = 1000)\n");
    printf("   -c #                        = Max number of signals to send, default = 1000000)\n");
    printf("   -t #                        = TTL for the signals\n");
    printf("   --tcp                       = Advertise and discover using the TCP transport\n");
    printf("   --udp                       = Advertise and discover using the UDP transport\n");
    printf("   --wfd                       = Advertise and discover using the Wi-Fi Direct transport\n");
    printf("   -x                          = Compress headers\n");
    printf("   -e[k] [RSA|SRP|LOGON|PINX]   = Encrypt the test interface using specified auth mechanism, -ek means clear keys\n");
    printf("   -d                          = discover remote bus with test service\n");
    printf("   -b                          = Signal is broadcast rather than multicast\n");
    printf("   --ls                        = Call LeaveSession before tearing down the Bus Attachment\n");
    printf("   --self-join                 = Test self-join \n");
    printf("   -about [name]   = use the about feature for discovery (optional application name to join).\n");
    printf("\n");
}

/** Main entry point */
int main(int argc, char** argv)
{
    QStatus status = ER_OK;

    bool clearKeys = false;
    qcc::String userId;
    qcc::String authMechs;
    unsigned long authCount = 1000;

    bool doStress = false;
    bool useSignalHandler = false;
    bool discoverRemote = false;
    bool transportSpecific = false;
    bool ls = false;

    unsigned long signalDelay = 0;
    unsigned long disconnectDelay = 0;
    unsigned long reportInterval = 1000;
    unsigned long maxSignals = 1000000;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i]) || 0 == strcmp("-?", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-s", argv[i])) {
            doStress = true;
        } else if (0 == strcmp("--tcp", argv[i])) {
            g_preferredTransport = TRANSPORT_UDP;
            transportSpecific = true;
        } else if (0 == strcmp("--udp", argv[i])) {
            g_preferredTransport = TRANSPORT_UDP;
            transportSpecific = true;
        } else if (0 == strcmp("--wfd", argv[i])) {
            g_preferredTransport = TRANSPORT_WFD;
            transportSpecific = true;
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_wellKnownName = argv[i];
            }

        } else if (0 == strcmp("-a", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_advertiseName = argv[i];
            }

        } else if (0 == strcmp("-l", argv[i])) {
            useSignalHandler = true;
        } else if (0 == strcmp("-d", argv[i])) {
            discoverRemote = true;
        } else if (0 == strcmp("--ls", argv[i])) {
            ls = true;
        } else if (0 == strcmp("-r", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                signalDelay = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-y", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                disconnectDelay = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-i", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                reportInterval = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-c", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                maxSignals = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-t", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                timeToLive = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-x", argv[i])) {
            compress = true;
        } else if (0 == strcmp("-b", argv[i])) {
            broacast = true;
        } else if ((0 == strcmp("-e", argv[i])) || (0 == strcmp("-ek", argv[i]))) {
            if (!authMechs.empty()) {
                authMechs += " ";
            }
            bool ok = false;
            encryption = true;
            clearKeys |= (argv[i][2] == 'k');
            ++i;
            if (i != argc) {
                if (strcmp(argv[i], "RSA") == 0) {
                    authMechs += "ALLJOYN_RSA_KEYX";
                    ok = true;
                } else if (strcmp(argv[i], "SRP") == 0) {
                    authMechs += "ALLJOYN_SRP_KEYX";
                    ok = true;
                } else if (strcmp(argv[i], "PINX") == 0) {
                    authMechs += "ALLJOYN_PIN_KEYX";
                    ok = true;
                } else if (strcmp(argv[i], "LOGON") == 0) {
                    if (++i == argc) {
                        printf("option %s LOGON requires a user id\n", argv[i - 2]);
                        usage();
                        exit(1);
                    }
                    authMechs += "ALLJOYN_SRP_LOGON";
                    userId = argv[i];
                    ok = true;
                }
            }
            if (!ok) {
                printf("option %s requires an auth mechanism \n", argv[i - 1]);
                usage();
                exit(1);
            }
        } else if (0 == strcmp("--self-join", argv[i])) {
            g_selfjoin = true;
            g_wellKnownName = g_advertiseName;
        } else if (0 == strcmp("-about", argv[i])) {
            g_useAboutFeatureDiscovery = true;
            if ((i + 1) < argc && argv[i + 1][0] != '-') {
                ++i;
                g_testAboutApplicationName = argv[i];
            } else {
                g_testAboutApplicationName = "bbservice";
            }
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    if (transportSpecific == false) {
        printf("default to TRANSPORT_ANY\n");
        g_preferredTransport = TRANSPORT_ANY;
    }

    do {
        /* Create message bus */
        g_msgBus = new BusAttachment("bbsig", true);

        /* Get env vars */
        Environ* env = Environ::GetAppEnviron();
        qcc::String connectArgs = env->Find("BUS_ADDRESS");

        /* Start the msg bus */
        status = g_msgBus->Start();
        if (ER_OK != status) {
            QCC_LogError(status, ("Bus::Start failed"));
            break;
        }

        if (encryption) {
            g_msgBus->EnablePeerSecurity(authMechs.c_str(), new MyAuthListener(userId, authCount));
            if (clearKeys) {
                g_msgBus->ClearKeyStore();
            }
        }

        /* Register a bus listener in order to get discovery indications */
        if (discoverRemote || g_selfjoin) {
            g_msgBus->RegisterBusListener(g_busListener);
        }
        /* Register an AboutListener in order to get interface discovery indications */
        if (g_useAboutFeatureDiscovery) {
            g_msgBus->RegisterAboutListener(g_aboutListener);
        }

        /* Register object and start the bus */
        LocalTestObject* testObj = new LocalTestObject(*g_msgBus, ::org::alljoyn::alljoyn_test::ObjectPath, signalDelay, disconnectDelay, reportInterval, maxSignals);
        g_msgBus->RegisterBusObject(*testObj);

        /* Connect to the bus */
        if (connectArgs.empty()) {
            status = g_msgBus->Connect();
        } else {
            status = g_msgBus->Connect(connectArgs.c_str());
        }
        if (ER_OK != status) {
            QCC_LogError(status, ("Connect to \"%s\" failed", connectArgs.c_str()));
            break;
        }

        if (g_selfjoin) {
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ALL);
            SessionPort sessionPort = ::org::alljoyn::alljoyn_test::SessionPort;
            QStatus status = g_msgBus->BindSessionPort(sessionPort, opts, g_portListener);
            if (ER_OK != status) {
                QCC_LogError(status, ("Could not bind to session"));
                break;
            }

        }

        if (discoverRemote || g_selfjoin) {
            /*
             * Make sure the event is cleared so we don't pick up a stale event from the previous
             * iteration when running the stress test.
             */
            g_discoverEvent.ResetEvent();

            /* Begin discovery on the well-known name of the service to be called */
            Message reply(*g_msgBus);
            const ProxyBusObject& alljoynObj = g_msgBus->GetAllJoynProxyObj();

            MsgArg serviceName("s", g_wellKnownName.c_str());
            status = alljoynObj.MethodCall(::ajn::org::alljoyn::Bus::InterfaceName,
                                           "FindAdvertisedName",
                                           &serviceName,
                                           1,
                                           reply,
                                           5000);
            if (ER_OK != status) {
                QCC_LogError(status, ("%s.FindAdvertisedName failed", ::ajn::org::alljoyn::Bus::InterfaceName));
            }
        } else if (g_useAboutFeatureDiscovery) {
            /*
             * Make sure the event is cleared so we don't pick up a stale event from the previous
             * iteration when running the stress test.
             */
            g_discoverEvent.ResetEvent();
            const char* interfaces[] = { ::org::alljoyn::alljoyn_test::InterfaceName };
            status = g_msgBus->WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
        }

        /*
         * If discovering, wait for the "FoundName" signal that tells us that we are connected to a
         * remote bus that is advertising bbservice's well-known name.
         */
        if ((discoverRemote || g_selfjoin || g_useAboutFeatureDiscovery) && (ER_OK == status)) {
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

        /* Register the signal handler and start sending signals */
        if (ER_OK == status) {
            if (useSignalHandler) {
                testObj->RegisterSignalHandler();
            }
            for (uint32_t i = 0; i < testObj->maxSignals; i++) {
                if (((i + 1) % testObj->reportInterval) == 0) {
                    QCC_SyncPrintf("SendSignal: %u\n", i + 1);
                }
                QStatus status = testObj->SendSignal();
                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed to send signal"));
                    break;
                }
                if (testObj->signalDelay > 0) {
                    qcc::Sleep(testObj->signalDelay);
                }
                if (g_interrupt) {
                    break;
                }
            }
            /* If we are not sending signals we wait for signals to be sent to us */
            if (useSignalHandler && (testObj->maxSignals == 0)) {
                qcc::Sleep(0xFFFFFFFF);
            }

        }

        if (testObj->disconnectDelay > 0) {
            qcc::Sleep(testObj->disconnectDelay);
        }

        if (ls) {
            if (g_useAboutFeatureDiscovery) {
                g_msgBus->LeaveSession(g_aboutListener.GetSessionId());
            } else {
                g_msgBus->LeaveSession(g_busListener.GetSessionId());
            }
        }

        /* Clean up msg bus for next stress loop iteration*/
        delete g_msgBus;
        g_msgBus = NULL;

        /* Clean up the test object for the next stress loop iteration */
        delete testObj;
        testObj = NULL;

    } while ((ER_OK == status) && doStress && !g_interrupt);


    if (g_msgBus) {
        delete g_msgBus;
        g_msgBus = NULL;
    }

    QCC_SyncPrintf("bbsig exiting with %d (%s)\n", status, QCC_StatusText(status));

    return (int) status;
}
