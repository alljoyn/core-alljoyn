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
#ifndef _WIN32
#define _BSD_SOURCE /* usleep */
#endif
#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/version.h>
#include <alljoyn_c/Status.h>

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.alljoyn_test";
static const char* INTERFACE_VALUE_NAME = "org.alljoyn.alljoyn_test.values";
static const char* DEFAULT_WELLKNOWN_NAME = "org.alljoyn.alljoyn_test";
static const char* OBJECT_PATH = "/org/alljoyn/alljoyn_test";
static alljoyn_sessionport SESSION_PORT = 24;


/* Static top level globals */
static alljoyn_busattachment g_msgBus = NULL;
static alljoyn_sessionlistener g_sessionListener = NULL;
static char* g_wellKnownName = NULL;
//TODO - use ALLJOYN_TRANSPORT_ANY
//static alljoyn_transportmask g_allowed_transport = (alljoyn_transport_mask)ALLJOYN_TRANSPORT_ANY;
static alljoyn_transportmask g_allowed_transport = 65535;
static uint32_t g_keyExpiration = 0xFFFFFFFF;
static uint8_t g_maxAuth = 3;
static QCC_BOOL g_stopDiscover = QCC_FALSE;
static QCC_BOOL g_discovered = QCC_FALSE;

static volatile sig_atomic_t g_interrupt = QCC_FALSE;

static void SigIntHandler(int sig)
{
    g_interrupt = QCC_TRUE;
}

void AJ_CALL found_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix)
{
    alljoyn_sessionopts session_opts;
    QStatus status = ER_OK;
    alljoyn_sessionid sessionid = 0;

    printf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);

    if (0 == (transport & g_allowed_transport)) {
        printf("Ignoring FoundAdvertised name from transport 0x%x\n", transport);
        return;
    }

    g_discovered = QCC_TRUE;

    /* Enable concurrent callbacks. */
    alljoyn_busattachment_enableconcurrentcallbacks(g_msgBus);

    if (0 == strcmp(name, g_wellKnownName)) {
        /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
        session_opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, transport);

        if (g_stopDiscover) {
            alljoyn_busattachment_canceladvertisename(g_msgBus, g_wellKnownName, transport);
            if (status != ER_OK) {
                printf("CancelAdvertiseName(%s) failed with %s", g_wellKnownName, QCC_StatusText(status));
            }
        }

        status = alljoyn_busattachment_joinsession(g_msgBus, name, SESSION_PORT, g_sessionListener, &sessionid, session_opts);
        if (ER_OK != status) {
            printf("JoinSession(%s) failed because of %s", name, QCC_StatusText(status));
        } else {
            /* set the session id in context. */
            *((alljoyn_sessionid*)context) = sessionid;
        }

        //destroy session opts
        alljoyn_sessionopts_destroy(session_opts);
    }

}

void AJ_CALL lost_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix)
{
    printf("LostAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);
}

void AJ_CALL name_owner_changed(const void* context, const char* name, const char* previousOwner, const char* newOwner)
{
    printf("NameOwnerChanged(%s, %s, %s)\n", name,  (previousOwner ? previousOwner : "null"), (newOwner ? newOwner : "null"));
}

void AJ_CALL session_lost(const void* context, alljoyn_sessionid sessionId, alljoyn_sessionlostreason reason) {
    printf("SessionLost(%u) was called\n", sessionId);
    _exit(1);
}

static void usage(void)
{
    printf("Usage: bbcclient [-h] [-c <count>] [-i] [-e] [-r #] [-l | -la | -d[s]] [-n <well-known name>] [-t[a] <delay> [<interval>] | -rt]\n\n");
    printf("Options:\n");
    printf("   -h                        = Print this help message\n");
    printf("   -k <key store name>       = The key store file name\n");
    printf("   -c <count>                = Number of pings to send to the server\n");
    printf("   -i                        = Use introspection to discover remote interfaces\n");
    printf("   -e[k] [RSA|SRP|PIN|LOGON] = Encrypt the test interface using specified auth mechanism, -ek means clear keys\n");
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
    printf("   -w                        = Don't wait for service\n");
    printf("   -s                        = Wait for SIGINT (Control-C) at the end of the tests\n");
    printf("   -be                       = Send messages as big endian\n");
    printf("   -le                       = Send messages as little endian\n");
    printf("   -m <trans_mask>           = Transports allowed to connect to service\n");
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


QCC_BOOL AJ_CALL request_credentials(const void* context, const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, alljoyn_credentials credentials)
{
    char guid[100];
    size_t size_of_guid = 100;

    if (authCount > g_maxAuth) {
        return QCC_FALSE;
    }
    /* fetch the user id from the context. */
    if (context) {
        userId =  (char*)context;
    }

    printf("RequestCredentials for authenticating %s using mechanism %s\n", authPeer, authMechanism);

    alljoyn_busattachment_getpeerguid(g_msgBus, authPeer, guid, &size_of_guid);
    printf("Peer guid %s   %zu\n", guid, size_of_guid);

    if (g_keyExpiration != 0xFFFFFFFF) {
        alljoyn_busattachment_setkeyexpiration(g_msgBus, guid, g_keyExpiration);
    }

    if (strcmp(authMechanism, "ALLJOYN_PIN_KEYX") == 0) {
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
            printf("AuthListener returning fixed pin \"%s\" for %s\n", alljoyn_credentials_getpassword(credentials), authMechanism);
        }
        return (authCount == 1);
    }

    if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            if (authCount == 3) {
                alljoyn_credentials_setpassword(credentials, "123456");
            } else {
                alljoyn_credentials_setpassword(credentials, "yyyyyy");
            }
            printf("AuthListener returning fixed pin \"%s\" for %s\n", alljoyn_credentials_getpassword(credentials), authMechanism);
        }
        return QCC_TRUE;
    }

    if (strcmp(authMechanism, "ALLJOYN_RSA_KEYX") == 0) {

        if (credMask & ALLJOYN_CRED_CERT_CHAIN) {
            alljoyn_credentials_setcertchain(credentials, x509cert);
        }

        if (credMask & ALLJOYN_CRED_PRIVATE_KEY) {
            alljoyn_credentials_setprivatekey(credentials, privKey);
        }

        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(credentials, "123456");
        }

        printf("AuthListener returning fixed pin \"%s\" for %s\n", alljoyn_credentials_getpassword(credentials), authMechanism);
        return QCC_TRUE;
    }


    if (strcmp(authMechanism, "ALLJOYN_SRP_LOGON") == 0) {
        if (credMask & ALLJOYN_CRED_USER_NAME) {
            if (authCount == 1) {
                alljoyn_credentials_setusername(credentials, "Mr Bogus");
            } else {
                alljoyn_credentials_setusername(credentials, userId);
            }
        }

        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(credentials, "123456");
        }
        return QCC_TRUE;
    }

    return QCC_FALSE;
}

QCC_BOOL AJ_CALL verify_credentials(const void*context, const char* authMechanism, const char* authPeer, const alljoyn_credentials credentials) {

    if (strcmp(authMechanism, "ALLJOYN_RSA_KEYX") == 0) {
        if (alljoyn_credentials_isset(credentials, ALLJOYN_CRED_CERT_CHAIN)) {
            printf("Verify\n%s\n", alljoyn_credentials_getcertchain(credentials));
            return QCC_TRUE;
        }
    }
    return QCC_FALSE;
}

void AJ_CALL authentication_complete(const void*context, const char* authMechanism, const char* authPeer, QCC_BOOL success) {
    printf("Authentication %s %s\n", authMechanism, success ? "succesful" : "failed");
}

void AJ_CALL security_violation(const void*context, QStatus status, const alljoyn_message msg) {
    printf("Security violation %s\n", QCC_StatusText(status));
}

void AJ_CALL ping_response_handler(alljoyn_message message, void* context)
{
    alljoyn_messagetype msg_type;
    const char* ret_string;
    char* interface_name = (char*)context;

    msg_type = alljoyn_message_gettype(message);

    if (ALLJOYN_MESSAGE_METHOD_RET == msg_type) {
        alljoyn_msgarg msg_arg = alljoyn_message_getarg(message, 0);
        alljoyn_msgarg_get(msg_arg, "s", &ret_string);
        printf("%s.%s returned \"%s\"\n", g_wellKnownName, interface_name, ret_string);
    } else {
        //ERROR
        char errormessage[100];
        size_t size = 100;
        ret_string = alljoyn_message_geterrorname(message, errormessage, &size);
        printf("%s.%s returned error %s: %s\n", g_wellKnownName, interface_name, (ret_string != NULL) ? ret_string : "NULL", errormessage);
    }
}

int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    QCC_BOOL useIntrospection = QCC_FALSE;
    QCC_BOOL encryptIfc = QCC_FALSE;
    QCC_BOOL clearKeys = QCC_FALSE;
    char authMechs[100] = "";
    char*userId = NULL;
    const char* keyStore = NULL;
    unsigned long pingCount = 1;
    unsigned long repCount = 1;
    uint64_t runTime = 0;
    QCC_BOOL discoverRemote = QCC_FALSE;
    QCC_BOOL asyncPing = QCC_FALSE;
    uint32_t pingDelay = 0;
    uint32_t pingInterval = 0;
    QCC_BOOL waitForSigint = QCC_FALSE;
    QCC_BOOL roundtrip = QCC_FALSE;
    QCC_BOOL hasOwner = QCC_FALSE;

    alljoyn_sessionid sessionid = 0;
    alljoyn_interfacedescription intf = NULL;
    alljoyn_interfacedescription valIntf = NULL;
    alljoyn_buslistener busListener = NULL;
    alljoyn_authlistener authListener = NULL;
    alljoyn_proxybusobject remoteObj = NULL;
    int i = 0;
    unsigned long j = 0;
    QCC_BOOL ok = QCC_FALSE;

    alljoyn_buslistener_callbacks bl_cbs = {
        NULL,
        NULL,
        &found_advertised_name,
        &lost_advertised_name,
        &name_owner_changed,
        NULL,
        NULL,
        NULL
    };

    alljoyn_sessionlistener_callbacks sl_cbs = {
        &session_lost,
        NULL,
        NULL
    };

    /* Auth listener callbacks. */
    alljoyn_authlistener_callbacks authcbs = {
        &request_credentials,
        &verify_credentials,
        &security_violation,
        &authentication_complete
    };

    size_t cnt = 0;

    printf("AllJoyn Library version: %s\n", alljoyn_getversion());
    printf("AllJoyn Library build info: %s\n", alljoyn_getbuildinfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    g_wellKnownName = (char*)DEFAULT_WELLKNOWN_NAME;

    /* Parse command line args */
    for (i = 1; i < argc; ++i) {
        if (0 == strcmp("-i", argv[i])) {
            useIntrospection = QCC_TRUE;
        } else if (0 == strcmp("-le", argv[i])) {
            alljoyn_message_setendianess(ALLJOYN_LITTLE_ENDIAN);
        } else if (0 == strcmp("-be", argv[i])) {
            alljoyn_message_setendianess(ALLJOYN_BIG_ENDIAN);
        } else if (0 == strcmp("-m", argv[i])) {
            ++i;
            g_allowed_transport = strtoul(argv[i], NULL, 10);
            if (g_allowed_transport == 0) {
                printf("Invalid value \"%s\" for option -m\n", argv[i]);
                usage();
                exit(1);
            }
        } else if ((0 == strcmp("-e", argv[i])) || (0 == strcmp("-ek", argv[i]))) {
            if (strcmp(authMechs, "") != 0) {
                strcat(authMechs, " ");
            }
            encryptIfc = QCC_TRUE;
            clearKeys |= (argv[i][2] == 'k');
            ++i;
            if (i != argc) {
                if (strcmp(argv[i], "RSA") == 0) {
                    strcat(authMechs, "ALLJOYN_RSA_KEYX");
                    ok = QCC_TRUE;
                } else if (strcmp(argv[i], "PIN") == 0) {
                    strcat(authMechs, "ALLJOYN_PIN_KEYX");
                    ok = QCC_TRUE;
                } else if (strcmp(argv[i], "SRP") == 0) {
                    strcat(authMechs, "ALLJOYN_SRP_KEYX");
                    ok = QCC_TRUE;
                } else if (strcmp(argv[i], "LOGON") == 0) {
                    if (++i == argc) {
                        printf("option %s LOGON requires a user id\n", argv[i - 2]);
                        usage();
                        exit(1);
                    }
                    strcat(authMechs, "ALLJOYN_SRP_LOGON");
                    userId = argv[i];
                    ok = QCC_TRUE;
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
                g_keyExpiration = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-a", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_maxAuth = strtoul(argv[i], NULL, 10);
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
                g_wellKnownName = argv[i];
            }
        } else if (0 == strcmp("-h", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-d", argv[i])) {
            discoverRemote = QCC_TRUE;
        } else if (0 == strcmp("-ds", argv[i])) {
            discoverRemote = QCC_TRUE;
            g_stopDiscover = QCC_TRUE;
        } else if ((0 == strcmp("-t", argv[i])) || (0 == strcmp("-ta", argv[i]))) {
            if (argv[i][2] == 'a') {
                asyncPing = QCC_TRUE;
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
            roundtrip = QCC_TRUE;
            if ((argc > (i + 1)) && argv[i + 1][0] != '-') {
                runTime = strtoul(argv[i], NULL, 10);
                pingCount = 1;
                ++i;
            } else if (pingCount == 1) {
                pingCount = 1000;
            }
        } else if (0 == strcmp("-s", argv[i])) {
            waitForSigint = QCC_TRUE;
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    for (j = 0; j < repCount && !g_interrupt; j++) {
        unsigned long pings;
        if (runTime > 0) {
            pings = 1;
            pingCount = 0;
        } else {
            pings = pingCount;
        }

        //Create bus attachment
        g_msgBus = alljoyn_busattachment_create("bbcclient", QCC_TRUE);

        //If not using introspection, add the interfaces manually to the bus.
        if (!useIntrospection) {
            /* Add org.alljoyn.alljoyn_test interface */
            status = alljoyn_busattachment_createinterface_secure(g_msgBus, INTERFACE_NAME, &intf,
                                                                  (encryptIfc) ? AJ_IFC_SECURITY_REQUIRED : AJ_IFC_SECURITY_INHERIT);
            if (status != ER_OK) {
                printf("Could not create %s interface because of %s. \n", INTERFACE_NAME, QCC_StatusText(status));
                return status;
            }

            status = alljoyn_busattachment_createinterface_secure(g_msgBus, INTERFACE_VALUE_NAME, &valIntf,
                                                                  (encryptIfc) ? AJ_IFC_SECURITY_REQUIRED : AJ_IFC_SECURITY_INHERIT);
            if (status != ER_OK) {
                printf("Could not create %s interface because of %s. \n", INTERFACE_VALUE_NAME, QCC_StatusText(status));
                return status;
            }

            status = alljoyn_interfacedescription_addmethod(intf, "my_ping", "s", "s", "i,i", 0, NULL);
            if (status != ER_OK) {
                printf("Could not add method %s to interface %s because of %s. \n", "my_ping", INTERFACE_NAME, QCC_StatusText(status));
                return status;
            }

            status = alljoyn_interfacedescription_addmember(intf, ALLJOYN_MESSAGE_SIGNAL, "my_signal", "a{ys}",  NULL, "inStr", 0);
            if (status != ER_OK) {
                printf("Could not add signal %s to interface %s because of %s. \n", "my_signal", INTERFACE_NAME, QCC_StatusText(status));
                return status;
            }

            status = alljoyn_interfacedescription_addmethod(intf, "delayed_ping", "su", "s", "ii,i", 0, NULL);
            if (status != ER_OK) {
                printf("Could not add method %s to interface %s because of %s. \n", "delayed_ping", INTERFACE_NAME, QCC_StatusText(status));
                return status;
            }

            status = alljoyn_interfacedescription_addmethod(intf, "time_ping", "uq", "uq", "i,i", 0, NULL);
            if (status != ER_OK) {
                printf("Could not add method %s to interface %s because of %s. \n", "time_ping", INTERFACE_NAME, QCC_StatusText(status));
                return status;
            }

            alljoyn_interfacedescription_activate(intf);

            /*Activate org.alljoyn.alljoyn_test.values */
            status = alljoyn_interfacedescription_addproperty(valIntf, "int_val", "i", ALLJOYN_PROP_ACCESS_WRITE);
            if (status != ER_OK) {
                printf("Could not add property %s to interface %s because of %s. \n", "int_val", INTERFACE_VALUE_NAME, QCC_StatusText(status));
                return status;
            }

            status = alljoyn_interfacedescription_addproperty(valIntf, "str_val", "s", ALLJOYN_PROP_ACCESS_RW);
            if (status != ER_OK) {
                printf("Could not add method %s to interface %s because of %s. \n", "str_val", INTERFACE_VALUE_NAME, QCC_StatusText(status));
                return status;
            }

            status = alljoyn_interfacedescription_addproperty(valIntf, "ro_str", "s", ALLJOYN_PROP_ACCESS_READ);
            if (status != ER_OK) {
                printf("Could not add method %s to interface %s because of %s. \n", "ro_str", INTERFACE_VALUE_NAME, QCC_StatusText(status));
                return status;
            }

            alljoyn_interfacedescription_activate(valIntf);
        }

        /* Register the bus listener. */
        /* TODO - Add other callbacks. */
        busListener = alljoyn_buslistener_create(&bl_cbs, &sessionid);
        alljoyn_busattachment_registerbuslistener(g_msgBus, busListener);

        g_sessionListener = alljoyn_sessionlistener_create(&sl_cbs, NULL);

        //Start the bus
        status = alljoyn_busattachment_start(g_msgBus);
        if (status != ER_OK) {
            printf("Could not start the bus because of %s. \n", QCC_StatusText(status));
            return status;
        }

        //Connect to the bus TODO - change the connect spec
        status = alljoyn_busattachment_connect(g_msgBus, "null:");
        if (status != ER_OK) {
            printf("Could not connect to the bus because of %s. \n", QCC_StatusText(status));
            return status;
        }

        if (encryptIfc) {
            authListener = alljoyn_authlistener_create(&authcbs, userId);

            status = alljoyn_busattachment_enablepeersecurity(g_msgBus, authMechs, authListener, keyStore, keyStore != NULL);
            if (ER_OK != status) {
                printf("enablePeerSecurity failed (%s)\n", QCC_StatusText(status));
                return status;
            }
            if (clearKeys) {
                alljoyn_busattachment_clearkeystore(g_msgBus);
            }
        }

        /* Discovery. */
        if (discoverRemote) {
            status = alljoyn_busattachment_findadvertisedname(g_msgBus, g_wellKnownName);
            if (status != ER_OK) {
                printf("FindAdvertisedName (%s) failed due to %s", g_wellKnownName, QCC_StatusText(status));
                return status;
            }
        }

        if (discoverRemote) {
            /* Wait till you discover something. */
            while (!g_interrupt && !g_discovered) {
#ifdef _WIN32
                Sleep(100);
#else
                usleep(100 * 1000);
#endif
            }
        }

        /* Joinsession is called in FoundAdvertised Name. You need to block till the wkn of service appears on the bus. */
        while (!g_interrupt && !hasOwner) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100 * 1000);
#endif
            alljoyn_busattachment_namehasowner(g_msgBus, g_wellKnownName, &hasOwner);
        }

        /* Create ProxyBusObject. */
        remoteObj = alljoyn_proxybusobject_create(g_msgBus, g_wellKnownName, OBJECT_PATH, sessionid);

        if (useIntrospection) {
            status = alljoyn_proxybusobject_introspectremoteobject(remoteObj);
            if (ER_OK != status) {
                printf("Introspection of %s (path=%s), (session=%u) failed due to %s. \n", g_wellKnownName, OBJECT_PATH, sessionid, QCC_StatusText(status));
            }
        } else {
            /* Manually  add the interfaces to the proxyBusObject.*/
            alljoyn_interfacedescription intf = NULL;
            alljoyn_interfacedescription valintf = NULL;

            intf = alljoyn_busattachment_getinterface(g_msgBus, INTERFACE_NAME);
            valintf = alljoyn_busattachment_getinterface(g_msgBus, INTERFACE_VALUE_NAME);

            alljoyn_proxybusobject_addinterface(remoteObj, intf);
            alljoyn_proxybusobject_addinterface(remoteObj, valintf);
        }

        /* Call the remote method */
        while ((ER_OK == status) && pings--) {

            alljoyn_message reply = alljoyn_message_create(g_msgBus);
            alljoyn_msgarg pingArgs = alljoyn_msgarg_array_create(2);
            size_t numArgs = 2;

            char buf[80];

            if (roundtrip) {
                //TODO - Implement later. Get time of day.
            } else {
                snprintf(buf, 80, "Ping String %u", (unsigned int)(++cnt));
                status = alljoyn_msgarg_array_set(pingArgs, &numArgs, "su", buf, pingDelay);
                if (status != ER_OK) {
                    printf("Could not set arguments because of %s. \n", QCC_StatusText(status));
                }
            }

            if (status == ER_OK) {
                /* Make a method call. */
                if (!roundtrip && asyncPing) {
                    printf("Sending \"%s\" to %s.%s asynchronously\n",
                           buf, INTERFACE_NAME, ((pingDelay == 0) ? "my_ping" : "delayed_ping"));
                    status = alljoyn_proxybusobject_methodcallasync(remoteObj, INTERFACE_NAME, ((pingDelay == 0) ? "my_ping" : "delayed_ping"),
                                                                    &ping_response_handler,
                                                                    pingArgs, (pingDelay > 0) ? 2 : 1,
                                                                    ((pingDelay == 0) ? "my_ping" : "delayed_ping"),
                                                                    pingDelay + 50000, 0);
                    if (status != ER_OK) {
                        printf("MethodCallAsync on %s.%s failed because of %s", INTERFACE_NAME, ((pingDelay == 0) ? "my_ping" : "delayed_ping"), QCC_StatusText(status));
                    }

                } else {

                    if (!roundtrip) {
                        printf("Sending \"%s\" to %s.%s synchronously\n", buf, INTERFACE_NAME, ((pingDelay == 0) ? "my_ping" : "delayed_ping"));
                    }

                    status = alljoyn_proxybusobject_methodcall(remoteObj, INTERFACE_NAME, ((pingDelay == 0) ? "my_ping" : "delayed_ping"),
                                                               pingArgs, (pingDelay > 0) ? 2 : 1, reply, pingDelay + 5000, 0);

                    if (ER_OK == status) {
                        if (roundtrip) {
                            printf("Round trip not implemented. \n");
                        } else {
                            char*value = NULL;
                            status = alljoyn_msgarg_get(alljoyn_message_getarg(reply, 0), "s", &value);
                            if (pingDelay == 0) {
                                printf("%s.%s ( path=%s ) returned \"%s\"\n", g_wellKnownName, ((pingDelay == 0) ? "my_ping" : "delayed_ping"), OBJECT_PATH, value);
                            } else {
                                printf("%s.%s ( path=%s ) returned \"%s\"\n", g_wellKnownName, "delayed_ping", OBJECT_PATH, value);
                            }
                        }
                    } else if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
                        char errorMessage[100];
                        size_t size = 100;
                        const char* errName = alljoyn_message_geterrorname(reply, errorMessage, &size);
                        printf("MethodCall on %s.%s reply was error %s %s\n", INTERFACE_NAME, ((pingDelay == 0) ? "my_ping" : "delayed_ping"), (errName != NULL) ? errName : "NULL", errorMessage);
                        status = ER_OK;
                    } else {
                        printf("MethodCall on %s.%s failed due to %s. \n", INTERFACE_NAME, ((pingDelay == 0) ? "my_ping" : "delayed_ping"), QCC_StatusText(status));
                    }

                }

                if (pingInterval > 0) {
                    //TODO - Add sleep
                }

                alljoyn_message_destroy(reply);
                alljoyn_msgarg_destroy(pingArgs);

            } else {
                alljoyn_message_destroy(reply);
                alljoyn_msgarg_destroy(pingArgs);
                break;
            }
        }


        if (roundtrip) {
            printf("No round trip data. To be implemented later. \n");
        }

        /* Get the test property */
        if (!roundtrip && (ER_OK == status)) {
            alljoyn_msgarg arg = alljoyn_msgarg_create();
            status = alljoyn_proxybusobject_getproperty(remoteObj, INTERFACE_VALUE_NAME, "int_val", arg);
            if (ER_OK == status) {
                int iVal = 0;
                status = alljoyn_msgarg_get(arg, "i", &iVal);
                if (status != ER_OK) {
                    printf("Could not get arg from getproperty because of %s. \n", QCC_StatusText(status));
                } else {
                    printf("%s.%s ( path=%s) returned \"%d\"\n", g_wellKnownName, "GetProperty", OBJECT_PATH, iVal);
                }
            } else {
                printf("GetProperty on %s failed because of %s. \n", g_wellKnownName, QCC_StatusText(status));
            }
            //destroy msgarg
            alljoyn_msgarg_destroy(arg);
        }

        //Wait for Ctrl-C
        if (status == ER_OK && waitForSigint) {
            while (g_interrupt == QCC_FALSE) {
#ifdef _WIN32
                Sleep(100);
#else
                usleep(100 * 1000);
#endif
            }
        }

        alljoyn_busattachment_unregisterbuslistener(g_msgBus, busListener);

        /* Delete all the creations. Destroy busattachment at last.*/
        if (remoteObj) {
            alljoyn_proxybusobject_destroy(remoteObj);
        }
        if (g_msgBus) {
            alljoyn_busattachment_destroy(g_msgBus);
        }

        if (busListener) {
            alljoyn_buslistener_destroy(busListener);
        }
        if (authListener) {
            alljoyn_authlistener_destroy(authListener);
        }
        if (g_sessionListener) {
            alljoyn_sessionlistener_destroy(g_sessionListener);
        }

        /* Break out of the inner for loop. */
        if (status != ER_OK) {
            break;
        }

    }

    printf("bbcclient exiting with status %d (%s)\n", status, QCC_StatusText(status));

    return (int) status;
}
