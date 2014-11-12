/**
 * @file
 * Sample implementation of an AllJoyn client.
 */

/******************************************************************************
 * Copyright (c) 2009-2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/Log.h>
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

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

#define METHODCALL_TIMEOUT 30000

using namespace std;
using namespace qcc;
using namespace ajn;

/** Sample constants */
namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* InterfaceName = "org.alljoyn.alljoyn_test";
const char* DefaultWellKnownName = "org.alljoyn.alljoyn_test";
const char* ObjectPath = "/org/alljoyn/alljoyn_test";
const SessionPort SessionPort = 24;   /**< Well-known session port value for bbclient/bbservice */
namespace values {
const char* InterfaceName = "org.alljoyn.alljoyn_test.values";
}
}
}
}
/** static interrupt flag */
static volatile sig_atomic_t g_interrupt = false;

/** Static data */
static BusAttachment* g_msgBus = NULL;
static Event g_discoverEvent;
static String g_remoteBusName = ::org::alljoyn::alljoyn_test::DefaultWellKnownName;
static TransportMask allowedTransports = TRANSPORT_ANY;
static uint32_t findStartTime = 0;
static uint32_t findEndTime = 0;
static uint32_t joinStartTime = 0;
static uint32_t joinEndTime = 0;
static uint32_t keyExpiration = 0xFFFFFFFF;
static String g_testAboutApplicationName = "bbservice";
static bool g_useAboutFeatureDiscovery = false;

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:

    MyBusListener(bool stopDiscover) : BusListener(), sessionId(0), stopDiscover(stopDiscover) { }

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

        if (0 == ::strcmp(name, g_remoteBusName.c_str())) {
            /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, transport);
            QStatus status;

            if (stopDiscover) {
                status = g_msgBus->CancelFindAdvertisedName(g_remoteBusName.c_str());
                if (ER_OK != status) {
                    QCC_LogError(status, ("CancelFindAdvertisedName(%s) failed", name));
                }
            }

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
        QCC_SyncPrintf("SessionLost(%08x) was called. Reason=%u.\n", sessionId, reason);
        g_interrupt = true;
    }

    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
    bool stopDiscover;
};

/** Static bus listener */
static MyBusListener* g_busListener;

class MyAboutListener : public AboutListener {
  public:
    MyAboutListener(bool stopDiscover) : sessionId(0), stopDiscover(stopDiscover) { }
    void Announced(const char* busName, uint16_t version, SessionPort port,
                   const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {

        AboutData ad;
        ad.CreatefromMsgArg(aboutDataArg);

        char* appName;
        ad.GetAppName(&appName);

        if (appName != NULL && strcmp(g_testAboutApplicationName.c_str(), appName) == 0) {
            findEndTime = GetTimestamp();

            g_remoteBusName = busName;

            /* We must enable concurrent callbacks since some of the calls below are blocking */
            g_msgBus->EnableConcurrentCallbacks();

            /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status;

            if (stopDiscover) {
                const char* interfaces[] = { ::org::alljoyn::alljoyn_test::InterfaceName,
                                             ::org::alljoyn::alljoyn_test::values::InterfaceName };
                status = g_msgBus->CancelWhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
                if (ER_OK != status) {
                    QCC_LogError(status, ("CancelWhoImplements(%s) failed { %s, %s }",
                                          ::org::alljoyn::alljoyn_test::InterfaceName,
                                          ::org::alljoyn::alljoyn_test::values::InterfaceName));
                }
            }

            joinStartTime = GetTimestamp();

            status = g_msgBus->JoinSession(busName, port, g_busListener, sessionId, opts);
            if (ER_OK != status) {
                QCC_LogError(status, ("JoinSession(%s) failed", busName));
            }

            /* Release the main thread */
            if (ER_OK == status) {
                joinEndTime = GetTimestamp();
                QCC_SyncPrintf("JoinSession 0x%x takes %d ms \n", TRANSPORT_ANY, (joinEndTime - joinStartTime));

                g_discoverEvent.SetEvent();
            }
        }
    }
  private:
    SessionId sessionId;
    bool stopDiscover;
};

static MyAboutListener* g_aboutListener;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

static void usage(void)
{
    printf("Usage: bbclient [-h] [-c <count>] [-i] [-e] [-r #] [-l | -la | -d[s]] [-n <well-known name>] [-t[a] <delay> [<interval>] | -rt]\n\n");
    printf("Options:\n");
    printf("   -h                        = Print this help message\n");
    printf("   -k <key store name>       = The key store file name\n");
    printf("   -c <count>                = Number of pings to send to the server\n");
    printf("   -i                        = Use introspection to discover remote interfaces\n");
    printf("   -e[k] [RSA|SRP|PIN|LOGON|ECDHE_NULL|ECDHE_PSK|ECDHE_ECDSA] = Encrypt the test interface using specified auth mechanism, -ek means clear keys\n");
    printf("   -en                       = Interface security is N/A\n");
    printf("   -eo                       = Enable object security\n");
    printf("   -a #                      = Max authentication attempts\n");
    printf("   -kx #                     = Authentication key expiration (seconds)\n");
    printf("   -r #                      = AllJoyn attachment restart count\n");
    printf("   -l                        = launch bbservice if not already running\n");
    printf("   -n <well-known name>      = Well-known bus name advertised by bbservice\n");
    printf("   -d                        = discover remote bus with test service\n");
    printf("   -ds                       = discover remote bus with test service and cancel discover when found\n");
    printf("   -t                        = Call delayed_ping with <delay> and repeat at <interval> if -c given\n");
    printf("   -ta                       = Like -t except calls asynchronously\n");
    printf("   -rt [run time]            = Round trip timer (optional run time in ms)\n");
    printf("   -u                        = Set allowed transports to TRANSPORT_UDP\n");
    printf("   -w                        = Don't wait for service\n");
    printf("   -s                        = Wait for SIGINT (Control-C) at the end of the tests\n");
    printf("   -be                       = Send messages as big endian\n");
    printf("   -le                       = Send messages as little endian\n");
    printf("   -m <trans_mask>           = Transports allowed to connect to service\n");
    printf("   -about [name]             = use the about feature for discovery (optional application name to join).\n");
    printf("\n");
}


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

/* these keys were generated by the unit test common/unit_test/CertificateECCTest.SUCCESS_GenCertForBBservice */
static const char ecdsaPrivateKeyPEM[] = {
    "-----BEGIN PRIVATE KEY-----\n"
    "CkzgQdvZSOQMmqOnddsw0BRneCNZhioNMyUoJwec9rMAAAAA"
    "-----END PRIVATE KEY-----"
};

static const char ecdsaCertChainType2PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAp1LKGlnpVVtV4Sa1TULsxGJR9C53Uq5AH3fxqxJjNdYAAAAAAobbdvBKaw9\n"
    "eHox7o9fNbN5usuZw8XkSPSmipikYCPJAAAAAAAAAABiToQ8L3KZLwSCetlNJwfd\n"
    "bbxbo2x/uooeYwmvXbH2uwAAAABFQGcdlcsvhdRxgI4SVziI4hbg2d2xAMI47qVB\n"
    "ZZsqJAAAAAAAAAAAAAAAAAABYGEAAAAAAAFhjQCJ9dkuY0Z6jjx+a8azIQh4UF0h\n"
    "8plX3uAhOlF2vT2jfxe5U06zaWSXcs9kBEQvfOeMM4sUtoXPArUA+TNahfOS9Bbf\n"
    "0Hh08SvDJSDgM2OetQAAAAAYUr2pw2kb90fWblBWVKnrddtrI5Zs8BYx/EodpMrS\n"
    "twAAAAA=\n"
    "-----END CERTIFICATE-----"
};
/* -- end keys generated by CertificateECCTest.SUCCESS_GenCertForBBservice */

class MyAuthListener : public AuthListener {
  public:

    MyAuthListener(const qcc::String& userName, unsigned long maxAuth) : AuthListener(), userName(userName), maxAuth(maxAuth) { }

  private:

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {

        if (authCount > maxAuth) {
            return false;
        }

        printf("RequestCredentials for authenticating %s using mechanism %s\n", authPeer, authMechanism);

        if (keyExpiration != 0xFFFFFFFF) {
            creds.SetExpiration(keyExpiration);
        }

        if (strcmp(authMechanism, "ALLJOYN_PIN_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("ABCDEFGH");
            }
            return authCount == 1;
        }

        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                if (authCount == 3) {
                    creds.SetPassword("123456");
                } else {
                    creds.SetPassword("xxxxxx");
                }
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
                if (authCount == 1) {
                    creds.SetUserName("Mr Bogus");
                } else {
                    creds.SetUserName(userName);
                }
            }
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return true;
        }
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
            printf("AuthListener::RequestCredentials for key exchange %s\n", authMechanism);
            return true;
        }
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
            if ((credMask & AuthListener::CRED_USER_NAME) == AuthListener::CRED_USER_NAME) {
                printf("AuthListener::RequestCredentials for key exchange %s received psk ID %s\n", authMechanism, creds.GetUserName().c_str());
            }
            String psk("123456");
            creds.SetPassword(psk);
            return true;
        }
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            bool usePrivateKey = true;  /* use to toggle the test */
            if (usePrivateKey) {
                if ((credMask & AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                    String pk(ecdsaPrivateKeyPEM, strlen(ecdsaPrivateKeyPEM));
                    creds.SetPrivateKey(pk);
                    printf("AuthListener::RequestCredentials for key exchange %s sends DSA private key %s\n", authMechanism, pk.c_str());
                }
                if ((credMask & AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                    String cert(ecdsaCertChainType2PEM, strlen(ecdsaCertChainType2PEM));
                    creds.SetCertChain(cert);
                    printf("AuthListener::RequestCredentials for key exchange %s sends DSA public cert %s\n", authMechanism, cert.c_str());
                }
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
        } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
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


class MyMessageReceiver : public MessageReceiver {
  public:
    void PingResponseHandler(Message& message, void* context)
    {
        const InterfaceDescription::Member* pingMethod = static_cast<const InterfaceDescription::Member*>(context);
        if (message->GetType() == MESSAGE_METHOD_RET) {
            QCC_SyncPrintf("%s.%s returned \"%s\"\n",
                           g_remoteBusName.c_str(),
                           pingMethod->name.c_str(),
                           message->GetArg(0)->v_string.str);

        } else {
            // must be an error
            qcc::String errMsg;
            const char* errName = message->GetErrorName(&errMsg);
            QCC_SyncPrintf("%s.%s returned error %s: %s\n",
                           g_remoteBusName.c_str(),
                           pingMethod->name.c_str(),
                           errName,
                           errMsg.c_str());
        }
    }
};



/** Main entry point */
int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    bool useIntrospection = false;
    InterfaceSecurityPolicy secPolicy = AJ_IFC_SECURITY_INHERIT;
    bool clearKeys = false;
    qcc::String authMechs;
    qcc::String pbusConnect;
    qcc::String userId;
    const char* keyStore = NULL;
    unsigned long pingCount = 1;
    unsigned long repCount = 1;
    unsigned long authCount = 1000;
    uint64_t runTime = 0;
    Environ* env;
    bool startService = false;
    bool discoverRemote = false;
    bool stopDiscover = false;
    bool waitForService = true;
    bool asyncPing = false;
    uint32_t pingDelay = 0;
    uint32_t pingInterval = 0;
    bool waitForSigint = false;
    bool roundtrip = false;
    bool objSecure = false;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-i", argv[i])) {
            useIntrospection = true;
        } else if (0 == strcmp("-le", argv[i])) {
            _Message::SetEndianess(ALLJOYN_LITTLE_ENDIAN);
        } else if (0 == strcmp("-be", argv[i])) {
            _Message::SetEndianess(ALLJOYN_BIG_ENDIAN);
        } else if (0 == ::strcmp("-m", argv[i])) {
            ++i;
            allowedTransports = StringToU32(argv[i], 0, 0);
            if (allowedTransports == 0) {
                printf("Invalid value \"%s\" for option -m\n", argv[i]);
                usage();
                exit(1);
            }
        } else if ((0 == strcmp("-eo", argv[i]))) {
            objSecure = true;
        } else if ((0 == strcmp("-en", argv[i]))) {
            secPolicy = AJ_IFC_SECURITY_OFF;
        } else if ((0 == strcmp("-e", argv[i])) || (0 == strcmp("-ek", argv[i]))) {
            if (!authMechs.empty()) {
                authMechs += " ";
            }
            bool ok = false;
            secPolicy = AJ_IFC_SECURITY_REQUIRED;
            clearKeys |= (argv[i][2] == 'k');
            ++i;
            if (i != argc) {

                if (strcmp(argv[i], "RSA") == 0) {
                    authMechs += "ALLJOYN_RSA_KEYX";
                    ok = true;
                } else if (strcmp(argv[i], "PIN") == 0) {
                    authMechs += "ALLJOYN_PIN_KEYX";
                    ok = true;
                } else if (strcmp(argv[i], "SRP") == 0) {
                    authMechs += "ALLJOYN_SRP_KEYX";
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
                } else if (strcmp(argv[i], "ECDHE_NULL") == 0) {
                    authMechs += "ALLJOYN_ECDHE_NULL";
                    ok = true;
                } else if (strcmp(argv[i], "ECDHE_PSK") == 0) {
                    authMechs += "ALLJOYN_ECDHE_PSK";
                    ok = true;
                } else if (strcmp(argv[i], "ECDHE_ECDSA") == 0) {
                    authMechs += "ALLJOYN_ECDHE_ECDSA";
                    ok = true;
                }
            }
            if (!ok) {
                printf("option %s requires an auth mechanism \n", argv[i - 1]);
                usage();
                exit(1);
            }
        } else if (0 == strcmp("-k", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                keyStore = argv[i];
            }
        } else if (0 == strcmp("-kx", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                keyExpiration = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-a", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                authCount = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-c", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                pingCount = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-r", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                repCount = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_remoteBusName = argv[i];
            }
        } else if (0 == strcmp("-h", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-l", argv[i])) {
            startService = true;
        } else if (0 == strcmp("-d", argv[i])) {
            discoverRemote = true;
        } else if (0 == strcmp("-ds", argv[i])) {
            discoverRemote = true;
            stopDiscover = true;
        } else if (0 == strcmp("-u", argv[i])) {
            allowedTransports = TRANSPORT_UDP;
        } else if (0 == strcmp("-w", argv[i])) {
            waitForService = false;
        } else if ((0 == strcmp("-t", argv[i])) || (0 == strcmp("-ta", argv[i]))) {
            if (argv[i][2] == 'a') {
                asyncPing = true;
            }
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                pingDelay = strtoul(argv[i], NULL, 10);
                ++i;
                if ((i == argc) || (argv[i][0] == '-')) {
                    --i;
                } else {
                    pingInterval = strtoul(argv[i], NULL, 10);
                }
            }
        } else if (0 == strcmp("-rt", argv[i])) {
            roundtrip = true;
            if ((argc > (i + 1)) && argv[i + 1][0] != '-') {
                String runTimeStr(argv[i + 1]);
                runTime = StringToU64(runTimeStr);
                pingCount = 1;
                ++i;
            } else if (pingCount == 1) {
                pingCount = 1000;
            }
        } else if (0 == strcmp("-s", argv[i])) {
            waitForSigint = true;
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

    /* Get env vars */
    env = Environ::GetAppEnviron();
    qcc::String connectArgs = env->Find("BUS_ADDRESS");

    for (unsigned long i = 0; i < repCount && !g_interrupt; i++) {
        unsigned long pings;
        if (runTime > 0) {
            pings = 1;
            pingCount = 0;
        } else {
            pings = pingCount;
        }

        /* Create message bus */
        g_msgBus = new BusAttachment("bbclient", true);

        if (!useIntrospection) {
            /* Add org.alljoyn.alljoyn_test interface */
            InterfaceDescription* testIntf = NULL;
            status = g_msgBus->CreateInterface(::org::alljoyn::alljoyn_test::InterfaceName, testIntf, secPolicy);
            if ((ER_OK == status) && testIntf) {
                testIntf->AddSignal("my_signal", NULL, NULL, 0);
                testIntf->AddMethod("my_ping", "s", "s", "outStr,inStr", 0);
                testIntf->AddMethod("delayed_ping", "su", "s", "outStr,delay,inStr", 0);
                testIntf->AddMethod("time_ping", "uq", "uq", NULL, 0);
                testIntf->Activate();
            } else {
                if (ER_OK == status) {
                    status = ER_FAIL;
                }
                QCC_LogError(status, ("Failed to create interface \"%s\"", ::org::alljoyn::alljoyn_test::InterfaceName));
            }

            if (ER_OK == status) {
                /* Add org.alljoyn.alljoyn_test.values interface */
                InterfaceDescription* valuesIntf = NULL;
                status = g_msgBus->CreateInterface(::org::alljoyn::alljoyn_test::values::InterfaceName, valuesIntf, secPolicy);
                if ((ER_OK == status) && valuesIntf) {
                    valuesIntf->AddProperty("int_val", "i", PROP_ACCESS_RW);
                    valuesIntf->AddProperty("str_val", "s", PROP_ACCESS_RW);
                    valuesIntf->AddProperty("ro_str", "s", PROP_ACCESS_READ);
                    valuesIntf->Activate();
                } else {
                    if (ER_OK == status) {
                        status = ER_FAIL;
                    }
                    QCC_LogError(status, ("Failed to create interface \"%s\"", ::org::alljoyn::alljoyn_test::values::InterfaceName));
                }
            }
        }

        /* Register a bus listener in order to get discovery indications */
        if (ER_OK == status) {
            g_busListener = new MyBusListener(stopDiscover);
            g_msgBus->RegisterBusListener(*g_busListener);
        }

        /* Start the msg bus */
        if (ER_OK == status) {
            status = g_msgBus->Start();
            if (ER_OK == status) {
                if (secPolicy != AJ_IFC_SECURITY_INHERIT) {
                    g_msgBus->EnablePeerSecurity(authMechs.c_str(), new MyAuthListener(userId, authCount), keyStore, keyStore != NULL);
                    if (clearKeys) {
                        g_msgBus->ClearKeyStore();
                    }
                }
            } else {
                QCC_LogError(status, ("BusAttachment::Start failed"));
            }
        }

        /* Connect to the bus */
        if (ER_OK == status) {
            if (connectArgs.empty()) {
                status = g_msgBus->Connect();
            } else {
                status = g_msgBus->Connect(connectArgs.c_str());
            }
            if (ER_OK != status) {
                QCC_LogError(status, ("BusAttachment::Connect(\"%s\") failed", connectArgs.c_str()));
            }
        }

        if (ER_OK == status) {
            if (startService) {
                /* Start the org.alljoyn.alljoyn_test service. */
                MsgArg args[2];
                Message reply(*g_msgBus);
                args[0].Set("s", g_remoteBusName.c_str());
                args[1].Set("u", 0);
                const ProxyBusObject& dbusObj = g_msgBus->GetDBusProxyObj();
                status = dbusObj.MethodCall(ajn::org::freedesktop::DBus::InterfaceName,
                                            "StartServiceByName",
                                            args,
                                            ArraySize(args),
                                            reply);
            } else if (discoverRemote) {
                /* Begin discovery on the well-known name of the service to be called */
                findStartTime = GetTimestamp();
                /*
                 * Make sure the g_discoverEvent flag has been set to the
                 * name-not-found state before trying to find the well-known name.
                 */
                g_discoverEvent.ResetEvent();
                status = g_msgBus->FindAdvertisedName(g_remoteBusName.c_str());
                if (status != ER_OK) {
                    QCC_LogError(status, ("FindAdvertisedName failed"));
                }
            }
            if (g_useAboutFeatureDiscovery) {
                /* Begin discovery on the well-known name of the service to be called */
                findStartTime = GetTimestamp();
                /*
                 * Make sure the g_discoverEvent flag has been set to the
                 * name-not-found state before trying to find the well-known name.
                 */
                g_discoverEvent.ResetEvent();
                g_aboutListener = new MyAboutListener(stopDiscover);
                g_msgBus->RegisterAboutListener(*g_aboutListener);
                const char* interfaces[] = { ::org::alljoyn::alljoyn_test::InterfaceName,
                                             ::org::alljoyn::alljoyn_test::values::InterfaceName };
                status = g_msgBus->WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
                if (status != ER_OK) {
                    QCC_LogError(status, ("WhoImplements failed"));
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
        } else if (waitForService && (ER_OK == status)) {
            /* If bbservice's well-known name is not currently on the bus yet, then wait for it to appear */
            bool hasOwner = false;
            g_discoverEvent.ResetEvent();
            status = g_msgBus->NameHasOwner(g_remoteBusName.c_str(), hasOwner);
            if ((ER_OK == status) && !hasOwner) {
                QCC_SyncPrintf("Waiting for name %s to appear on the bus\n", g_remoteBusName.c_str());
                status = Event::Wait(g_discoverEvent);
                if (ER_OK != status) {
                    QCC_LogError(status, ("Event::Wait failed"));
                }
            }
        }

        if (ER_OK == status) {
            /* Create the remote object that will be called */
            ProxyBusObject remoteObj;
            if (ER_OK == status) {
                remoteObj = ProxyBusObject(*g_msgBus, g_remoteBusName.c_str(), ::org::alljoyn::alljoyn_test::ObjectPath, g_busListener->GetSessionId(), objSecure);
                if (useIntrospection) {
                    status = remoteObj.IntrospectRemoteObject();
                    if (ER_OK != status) {
                        QCC_LogError(status, ("Introspection of %s (path=%s) failed",
                                              g_remoteBusName.c_str(),
                                              ::org::alljoyn::alljoyn_test::ObjectPath));
                    }
                } else {
                    const InterfaceDescription* alljoynTestIntf = g_msgBus->GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
                    assert(alljoynTestIntf);
                    remoteObj.AddInterface(*alljoynTestIntf);

                    const InterfaceDescription* alljoynTestValuesIntf = g_msgBus->GetInterface(::org::alljoyn::alljoyn_test::values::InterfaceName);
                    assert(alljoynTestValuesIntf);
                    remoteObj.AddInterface(*alljoynTestValuesIntf);
                }
                /* Enable security if it is needed */
                if ((remoteObj.IsSecure() || (secPolicy == AJ_IFC_SECURITY_REQUIRED)) && !g_msgBus->IsPeerSecurityEnabled()) {
                    QCC_SyncPrintf("Enabling peer security\n");
                    g_msgBus->EnablePeerSecurity("ALLJOYN_SRP_KEYX ALLJOYN_PIN_KEYX ALLJOYN_RSA_KEYX ALLJOYN_SRP_LOGON",
                                                 new MyAuthListener(userId, authCount),
                                                 keyStore,
                                                 keyStore != NULL);
                }
            }

            MyMessageReceiver msgReceiver;
            size_t cnt = 0;
            uint64_t sample = 0;
            uint64_t timeSum = 0;
            uint64_t max_delta = 0;
            uint64_t min_delta = ~0;

            /* Call the remote method */
            while ((ER_OK == status) && pings--) {
                Message reply(*g_msgBus);
                MsgArg pingArgs[2];
                const InterfaceDescription::Member* pingMethod;
                const InterfaceDescription* ifc = remoteObj.GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
                if (ifc == NULL) {
                    status = ER_BUS_NO_SUCH_INTERFACE;
                    QCC_SyncPrintf("Unable to Get InterfaceDecription for the %s interface\n",
                                   ::org::alljoyn::alljoyn_test::InterfaceName);
                    break;
                }
                char buf[80];

                if (roundtrip) {
                    Timespec now;
                    GetTimeNow(&now);
                    pingMethod = ifc->GetMember("time_ping");
                    pingArgs[0].Set("u", now.seconds);
                    pingArgs[1].Set("q", now.mseconds);
                } else {
                    snprintf(buf, 80, "Ping String %u", static_cast<unsigned int>(++cnt));
                    pingArgs[0].Set("s", buf);

                    if (pingDelay > 0) {
                        pingMethod = ifc->GetMember("delayed_ping");
                        pingArgs[1].Set("u", pingDelay);
                    } else {
                        pingMethod = ifc->GetMember("my_ping");
                    }
                }

                if (!roundtrip && asyncPing) {
                    QCC_SyncPrintf("Sending \"%s\" to %s.%s asynchronously\n",
                                   buf, ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str());
                    status = remoteObj.MethodCallAsync(*pingMethod,
                                                       &msgReceiver,
                                                       static_cast<MessageReceiver::ReplyHandler>(&MyMessageReceiver::PingResponseHandler),
                                                       pingArgs, (pingDelay > 0) ? 2 : 1,
                                                       const_cast<void*>(static_cast<const void*>(pingMethod)),
                                                       pingDelay + 10000);
                    if (status != ER_OK) {
                        QCC_LogError(status, ("MethodCallAsync on %s.%s failed", ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str()));
                    }
                } else {
                    if (!roundtrip) {
                        QCC_SyncPrintf("Sending \"%s\" to %s.%s synchronously\n",
                                       buf, ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str());
                    }
                    status = remoteObj.MethodCall(*pingMethod, pingArgs, (roundtrip || (pingDelay > 0)) ? 2 : 1, reply, pingDelay + 50000);
                    if (ER_OK == status) {
                        if (roundtrip) {
                            Timespec now;
                            GetTimeNow(&now);
                            uint64_t delta = ((static_cast<uint64_t>(now.seconds) * 1000ULL + now.mseconds) -
                                              (static_cast<uint64_t>(reply->GetArg(0)->v_uint32) * 1000ULL + reply->GetArg(1)->v_uint16));
                            if (delta > max_delta) {
                                max_delta = delta;
                                QCC_SyncPrintf("New Max time: %llu ms\n", max_delta);
                            }
                            if (delta < min_delta) {
                                min_delta = delta;
                                QCC_SyncPrintf("New Min time: %llu ms\n", min_delta);
                            }
                            if ((runTime == 0) && (delta > ((~0ULL) / pingCount))) {
                                QCC_SyncPrintf("Round trip time &llu ms will overflow average calculation; dropping...\n", delta);
                            } else {
                                timeSum += delta;
                            }
                            QCC_SyncPrintf("DELTA: %llu %llu %llu\n", sample, timeSum, delta);
                            ++sample;
                            if (runTime > 0) {
                                ++pingCount;
                                if (timeSum >= runTime) {
                                    pings = 0;
                                } else {
                                    pings = 1;
                                }
                            }
                        } else {
                            QCC_SyncPrintf("%s.%s ( path=%s ) returned \"%s\"\n",
                                           g_remoteBusName.c_str(),
                                           pingMethod->name.c_str(),
                                           ::org::alljoyn::alljoyn_test::ObjectPath,
                                           reply->GetArg(0)->v_string.str);
                        }
                    } else if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
                        qcc::String errDescription;
                        const char* errName = reply->GetErrorName(&errDescription);
                        QCC_SyncPrintf("MethodCall on %s.%s reply was error %s %s\n", ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str(), errName, errDescription.c_str());
                        status = ER_OK;
                    } else {
                        QCC_LogError(status, ("MethodCall on %s.%s failed", ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str()));
                    }
                }
                if (pingInterval > 0) {
                    qcc::Sleep(pingInterval);
                }
            }

            if (roundtrip) {
                QCC_SyncPrintf("Round trip time MIN/AVG/MAX: %llu/%llu.%03u/%llu ms\n",
                               min_delta,
                               timeSum / pingCount, ((timeSum % pingCount) * 1000) / pingCount,
                               max_delta);
            }

            /* Get the test property */
            if (!roundtrip && (ER_OK == status)) {
                MsgArg val;
                status = remoteObj.GetProperty(::org::alljoyn::alljoyn_test::values::InterfaceName, "int_val", val);
                if (ER_OK == status) {
                    int iVal = 0;
                    val.Get("i", &iVal);
                    QCC_SyncPrintf("%s.%s ( path=%s) returned \"%d\"\n",
                                   g_remoteBusName.c_str(),
                                   "GetProperty",
                                   ::org::alljoyn::alljoyn_test::ObjectPath,
                                   iVal);
                } else {
                    QCC_LogError(status, ("GetProperty on %s failed", g_remoteBusName.c_str()));
                }
            }
        }

        if (status == ER_OK && waitForSigint) {
            while (g_interrupt == false) {
                qcc::Sleep(100);
            }
        }

        /* Deallocate bus */
        delete g_msgBus;
        g_msgBus = NULL;

        delete g_busListener;
        g_busListener = NULL;

        delete g_aboutListener;
        g_aboutListener = NULL;

        if (status != ER_OK) {
            break;
        }
    }

    printf("bbclient exiting with status %d (%s)\n", status, QCC_StatusText(status));

    return (int) status;
}
