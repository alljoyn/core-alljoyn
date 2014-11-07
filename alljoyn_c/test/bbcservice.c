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
static char*  g_prop_str_val = "hello world";
static const char*  g_prop_ro_str  = "I cannot be written";
static int g_prop_int_val = 100;


/* Static top level globals */
static alljoyn_busattachment g_msgBus = NULL;
static alljoyn_sessionlistener g_sessionListener = NULL;
static alljoyn_sessionportlistener g_sessionPortListener = NULL;
static alljoyn_busobject g_testObj = NULL;
static alljoyn_sessionopts g_sessionOpts = NULL;
static char* g_wellKnownName = NULL;;
static QCC_BOOL g_echo_signal = QCC_FALSE;
static QCC_BOOL g_compress = QCC_FALSE;
static uint32_t g_keyExpiration = 0xFFFFFFFF;
static QCC_BOOL g_cancelAdvertise = QCC_FALSE;
static QCC_BOOL g_ping_back = QCC_FALSE;
static uint32_t g_reportInterval = 1000;

static volatile sig_atomic_t g_interrupt = QCC_FALSE;

static void SigIntHandler(int sig)
{
    g_interrupt = QCC_TRUE;
}

static const char x509certChain[] = {
    // User certificate
    "-----BEGIN CERTIFICATE-----\n"
    "MIICxzCCAjCgAwIBAgIJALZkSW0TWinQMA0GCSqGSIb3DQEBBQUAME8xCzAJBgNV\n"
    "BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMQ0wCwYDVQQKEwRRdUlDMQ0wCwYD\n"
    "VQQLEwRNQnVzMQ0wCwYDVQQDEwRHcmVnMB4XDTEwMDgyNTIzMTYwNVoXDTExMDgy\n"
    "NTIzMTYwNVowfzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAO\n"
    "BgNVBAcTB1NlYXR0bGUxIzAhBgNVBAoTGlF1YWxjb21tIElubm92YXRpb24gQ2Vu\n"
    "dGVyMREwDwYDVQQLEwhNQnVzIGRldjERMA8GA1UEAxMIU2VhIEtpbmcwgZ8wDQYJ\n"
    "KoZIhvcNAQEBBQADgY0AMIGJAoGBALz+YZcH0DZn91sjOA5vaTwjQVBnbR9ZRpCA\n"
    "kGD2am0F91juEPFvj/PAlvVLPd5nwGKSPiycN3l3ECxNerTrwIG2XxzBWantFn5n\n"
    "7dDzlRm3aerFr78EJmcCiImwgqsuhUT4eo5/jn457vANO9B5k/1ddc6zJ67Jvuh6\n"
    "0p4YAW4NAgMBAAGjezB5MAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5T\n"
    "U0wgR2VuZXJhdGVkIENlcnRpZmljYXRlMB0GA1UdDgQWBBTXau+rH64d658efvkF\n"
    "jkaEZJ+5BTAfBgNVHSMEGDAWgBTu5FqZL5ShsNq4KJjOo8IPZ70MBTANBgkqhkiG\n"
    "9w0BAQUFAAOBgQBNBt7+/IaqGUSOpYAgHun87c86J+R38P2dmOm+wk8CNvKExdzx\n"
    "Hp08aA51d5YtGrkDJdKXfC+Ly0CuE2SCiMU4RbK9Pc2H/MRQdmn7ZOygisrJNgRK\n"
    "Gerh1OQGuc1/USAFpfD2rd+xqndp1WZz7iJh+ezF44VMUlo2fTKjYr5jMQ==\n"
    "-----END CERTIFICATE-----\n"
    // Root certificate
    "-----BEGIN CERTIFICATE-----\n"
    "MIICzjCCAjegAwIBAgIJALZkSW0TWinPMA0GCSqGSIb3DQEBBQUAME8xCzAJBgNV\n"
    "BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMQ0wCwYDVQQKEwRRdUlDMQ0wCwYD\n"
    "VQQLEwRNQnVzMQ0wCwYDVQQDEwRHcmVnMB4XDTEwMDgyNTIzMTQwNloXDTEzMDgy\n"
    "NDIzMTQwNlowTzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xDTAL\n"
    "BgNVBAoTBFF1SUMxDTALBgNVBAsTBE1CdXMxDTALBgNVBAMTBEdyZWcwgZ8wDQYJ\n"
    "KoZIhvcNAQEBBQADgY0AMIGJAoGBANc1GTPfvD347zk1NlZbDhTf5txn3AcSG//I\n"
    "gdgdZOY7ubXkNMGEVBMyZDXe7K36MEmj5hfXRiqfZwpZjjzJeJBoPJvXkETzatjX\n"
    "vs4d5k1m0UjzANXp01T7EK1ZdIP7AjLg4QMk+uj8y7x3nElmSpNvPf3tBe3JUe6t\n"
    "Io22NI/VAgMBAAGjgbEwga4wHQYDVR0OBBYEFO7kWpkvlKGw2rgomM6jwg9nvQwF\n"
    "MH8GA1UdIwR4MHaAFO7kWpkvlKGw2rgomM6jwg9nvQwFoVOkUTBPMQswCQYDVQQG\n"
    "EwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjENMAsGA1UEChMEUXVJQzENMAsGA1UE\n"
    "CxMETUJ1czENMAsGA1UEAxMER3JlZ4IJALZkSW0TWinPMAwGA1UdEwQFMAMBAf8w\n"
    "DQYJKoZIhvcNAQEFBQADgYEAg3pDFX0270jUTf8mFJHJ1P+CeultB+w4EMByTBfA\n"
    "ZPNOKzFeoZiGe2AcMg41VXvaKJA0rNH+5z8zvVAY98x1lLKsJ4fb4aIFGQ46UZ35\n"
    "DMrqZYmULjjSXWMxiphVRf1svKGU4WHR+VSvtUNLXzQyvg2yUb6PKDPUQwGi9kDx\n"
    "tCI=\n"
    "-----END CERTIFICATE-----\n"
};

static const char privKey[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: DES-EDE3-CBC,86B9DBED35AEBAB3\n"
    "\n"
    "f28sibgVCkDz3VNoC/MzazG2tFj+KGf6xm9LQki/GsxpMhJsEEvT9dUluT1T4Ypr\n"
    "NjG+nBleLcfdHxOl5XHnusn8r/JVaQQGVSnDaeP/27KiirtB472p+8Wc2wfXexRz\n"
    "uSUv0DJT+Fb52zYGiGzwgaOinQEBskeO9AwRyG34sFKqyyapyJtSZDjh+wUAIMZb\n"
    "wKifvl1KHSCbXEhjDVlxBw4Rt7I36uKzTY5oax2L6W6gzxfHuOtzfVelAaM46j+n\n"
    "KANZgx6KGW2DKk27aad2HEZUYeDwznpwU5Duw9b0DeMTkez6CuayiZHb5qEod+0m\n"
    "pCCMwpqxFCJ/vg1VJjmxM7wpCQTc5z5cjX8saV5jMUJXp09NuoU/v8TvhOcXOE1T\n"
    "ENukIWYBT1HC9MJArroLwl+fMezKCu+F/JC3M0RfI0dlQqS4UWH+Uv+Ujqa2yr9y\n"
    "20zYS52Z4kyq2WnqwBk1//PLBl/bH/awWXPUI2yMnIILbuCisRYLyK52Ge/rS51P\n"
    "vUgUCZ7uoEJGTX6EGh0yQhp+5jGYVdHHZB840AyxzBQx7pW4MtTwqkw1NZuQcdSN\n"
    "IU9y/PferHhMKZeGfVRVEkAOcjeXOqvSi6NKDvYn7osCkvj9h7K388o37VMPSacR\n"
    "jDwDTT0HH/UcM+5v/74NgE/OebaK3YfxBVyMmBzi0WVFXgxHJir4xpj9c20YQVw9\n"
    "hE3kYepW8gGz/JPQmRszwLQpwQNEP60CgQveqtH7tZVXzDkElvSyveOdjJf1lw4B\n"
    "uCz54678UNNeIe7YB4yV1dMVhhcoitn7G/+jC9Qk3FTnuP+Ws5c/0g==\n"
    "-----END RSA PRIVATE KEY-----"
};

QStatus AJ_CALL request_credentials_async(const void* context, alljoyn_authlistener listener, const char* authMechanism,
                                          const char* authPeer, uint16_t authCount, const char* userId,
                                          uint16_t credMask, void* authContext)
{
    char guid[100];
    size_t size_of_guid = 100;
    alljoyn_credentials creds;
    QStatus status = ER_OK;

    creds = alljoyn_credentials_create();

    printf("RequestCredentials for authenticating %s using mechanism %s\n", authPeer, authMechanism);

    /* Random delay TODO*/

    alljoyn_busattachment_getpeerguid(g_msgBus, authPeer, guid, &size_of_guid);
    printf("Peer guid %s   %zu\n", guid, size_of_guid);

    if (g_keyExpiration != 0xFFFFFFFF) {
        alljoyn_busattachment_setkeyexpiration(g_msgBus, guid, g_keyExpiration);
    }

    if (strcmp(authMechanism, "ALLJOYN_PIN_KEYX") == 0) {
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(creds, "ABCDEFGH");
            printf("AuthListener returning fixed pin \"%s\" for %s\n", alljoyn_credentials_getpassword(creds), authMechanism);
        }

        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
        alljoyn_credentials_destroy(creds);
        return status;
    }

    if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            if (authCount == 1) {
                alljoyn_credentials_setpassword(creds, "yyyyyy");
            } else {
                alljoyn_credentials_setpassword(creds, "123456");
            }
            printf("AuthListener returning fixed pin \"%s\" for %s\n", alljoyn_credentials_getpassword(creds), authMechanism);
        }
        status =  alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
        alljoyn_credentials_destroy(creds);
        return status;
    }

    if (strcmp(authMechanism, "ALLJOYN_RSA_KEYX") == 0) {
        if (credMask & ALLJOYN_CRED_CERT_CHAIN) {
            alljoyn_credentials_setcertchain(creds, x509certChain);
        }
        if (credMask & ALLJOYN_CRED_PRIVATE_KEY) {
            alljoyn_credentials_setprivatekey(creds, privKey);
        }
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            if (authCount == 2) {
                alljoyn_credentials_setpassword(creds, "12345X");
            }
            if (authCount == 3) {
                alljoyn_credentials_setpassword(creds, "123456");
            }
        }
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
        alljoyn_credentials_destroy(creds);
        return status;
    }

    if (strcmp(authMechanism, "ALLJOYN_SRP_LOGON") == 0) {
        if (!userId) {
            status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_FALSE, creds);
            alljoyn_credentials_destroy(creds);
            return status;
        }
        printf("Attempting to logon user %s\n", userId);
        if (strcmp(userId, "happy") == 0) {
            if (credMask & ALLJOYN_CRED_PASSWORD) {
                alljoyn_credentials_setpassword(creds, "123456");
                status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
                alljoyn_credentials_destroy(creds);
                return status;
            }
        }
        if (strcmp(userId, "sneezy") == 0) {
            if (credMask & ALLJOYN_CRED_PASSWORD) {
                alljoyn_credentials_setpassword(creds, "123456");
                status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
                alljoyn_credentials_destroy(creds);
                return status;
            }
        }
        /*
         * Allow 3 logon attempts.
         */
        if (authCount <= 3) {
            status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
            alljoyn_credentials_destroy(creds);
            return status;
        }
    }

    status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_FALSE, creds);
    alljoyn_credentials_destroy(creds);
    return status;

}

QStatus AJ_CALL verify_credentials_async(const void*context, alljoyn_authlistener listener, const char* authMechanism, const char* authPeer,  const alljoyn_credentials creds, void*authContext) {

    QStatus status = ER_OK;


    if (strcmp(authMechanism, "ALLJOYN_RSA_KEYX") == 0) {
        if (alljoyn_credentials_isset(creds, ALLJOYN_CRED_CERT_CHAIN)) {
            printf("Verify\n%s\n", alljoyn_credentials_getcertchain(creds));
            status = alljoyn_authlistener_verifycredentialsresponse(listener, authContext, QCC_TRUE);
            return status;
        }
    }
    status = alljoyn_authlistener_verifycredentialsresponse(listener, authContext, QCC_FALSE);
    return status;
}

void AJ_CALL authentication_complete(const void*context, const char* authMechanism, const char* authPeer, QCC_BOOL success) {
    printf("Authentication %s %s\n", authMechanism, success ? "succesful" : "failed");
}

void AJ_CALL security_violation(const void*context, QStatus status, const alljoyn_message msg) {
    printf("Security violation %s\n", QCC_StatusText(status));
}


QCC_BOOL AJ_CALL accept_session_joiner(const void* context, alljoyn_sessionport sessionPort, const char* joiner,  const alljoyn_sessionopts opts)
{
    if (sessionPort != SESSION_PORT) {
        printf("Received JoinSession request for non-bound port. \n");
        return QCC_FALSE;
    }

    if (alljoyn_sessionopts_iscompatible(opts, g_sessionOpts)) {
        printf("Accepting JoinSession request from %s\n", joiner);
        return QCC_TRUE;
    } else {
        /* Reject incompatible transports */
        printf("Rejecting joiner %s with incompatible session options\n", joiner);
        return QCC_TRUE;
    }
}

void AJ_CALL session_joined(const void* context, alljoyn_sessionport sessionPort, alljoyn_sessionid sessionId, const char* joiner)
{
    uint32_t timeout = 10;
    QStatus status = ER_OK;

    printf("Session Established: joiner=%s, sessionId=%08x\n", joiner, sessionId);

    /* Enable concurrent callbacks since some of the calls below could block */
    alljoyn_busattachment_enableconcurrentcallbacks(g_msgBus);

    status = alljoyn_busattachment_setsessionlistener(g_msgBus, sessionId, g_sessionListener);
    if (status != ER_OK) {
        printf("SetSessionListener failed with %s \n", QCC_StatusText(status));
        return;
    }

    /* Set the link timeout */
    status = alljoyn_busattachment_setlinktimeout(g_msgBus, sessionId, &timeout);
    if (status == ER_OK) {
        printf("Link timeout was successfully set to %d\n", timeout);
    } else {
        printf("SetLinkTimeout failed with %s \n", QCC_StatusText(status));
    }

    /* cancel advertisment */
    if (g_cancelAdvertise) {
        alljoyn_busattachment_canceladvertisename(g_msgBus, g_wellKnownName, alljoyn_sessionopts_get_transports(g_sessionOpts));
        if (status != ER_OK) {
            printf("CancelAdvertiseName(%s) failed with %s", g_wellKnownName, QCC_StatusText(status));
        }
    }
}

void AJ_CALL session_lost(const void* context, alljoyn_sessionid sessionId, alljoyn_sessionlostreason reason) {

    QStatus status = ER_OK;
    printf("SessionLost(%08x) was called\n", sessionId);

    /* Enable concurrent callbacks since some of the calls below could block */
    alljoyn_busattachment_enableconcurrentcallbacks(g_msgBus);

    /* cancel advertisment */
    if (g_cancelAdvertise) {
        alljoyn_busattachment_canceladvertisename(g_msgBus, g_wellKnownName, alljoyn_sessionopts_get_transports(g_sessionOpts));
        if (status != ER_OK) {
            printf("CancelAdvertiseName(%s) failed with %s", g_wellKnownName, QCC_StatusText(status));
        }
    }
}

void AJ_CALL ping(alljoyn_busobject busobject, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{

    QStatus status = ER_OK;
    char*value = NULL;
    alljoyn_msgarg outArg;

    status = alljoyn_msgarg_get(alljoyn_message_getarg(msg, 0), "s", &value);
    if (ER_OK != status) {
        printf("Ping: Error reading alljoyn_message %s\n", QCC_StatusText(status));
    } else {
        printf("Pinged with: %s\n", value);
    }

    if (alljoyn_message_isencrypted(msg) == QCC_TRUE) {
        printf("Authenticated using %s\n", alljoyn_message_getauthmechanism(msg));
    }

    outArg = alljoyn_msgarg_create_and_set("s", value);
    status = alljoyn_busobject_methodreply_args(busobject, msg, outArg, 1);
    if (ER_OK != status) {
        printf("Ping: Error sending reply %s\n", QCC_StatusText(status));
    }
    /* Destroy the msgarg */
    alljoyn_msgarg_destroy(outArg);
}

void AJ_CALL delayed_ping(alljoyn_busobject busobject, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{
    QStatus status = ER_OK;
    char*value = NULL;
    alljoyn_msgarg outArg;
    uint32_t delay = 0;

    /* Enable concurrent callbacks since some of the calls below could block */
    alljoyn_busattachment_enableconcurrentcallbacks(g_msgBus);

    status = alljoyn_msgarg_get(alljoyn_message_getarg(msg, 0), "s", &value);
    status = alljoyn_msgarg_get(alljoyn_message_getarg(msg, 1), "u", &delay);

    printf("Pinged (response delayed %ums) with: \"%s\"\n", delay, value);

    if (alljoyn_message_isencrypted(msg) == QCC_TRUE) {
        printf("Authenticated using %s\n", alljoyn_message_getauthmechanism(msg));
    }
#ifdef _WIN32
    Sleep(delay);
#else
    usleep(100 * delay);
#endif

    outArg = alljoyn_msgarg_create_and_set("s", value);
    status = alljoyn_busobject_methodreply_args(busobject, msg, outArg, 1);
    if (ER_OK != status) {
        printf("DelayedPing: Error sending reply %s\n", QCC_StatusText(status));
    }
    /* Destroy the msgarg */
    alljoyn_msgarg_destroy(outArg);
}

void AJ_CALL time_ping(alljoyn_busobject busobject, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{
    QStatus status = ER_OK;
    size_t numArgs = 0;
    alljoyn_msgarg args[2];

    /* Reply with same data that was sent to us */
    alljoyn_message_getargs(msg, &numArgs, args);

    status = alljoyn_busobject_methodreply_args(busobject, msg, *args, numArgs);
    if (ER_OK != status) {
        printf("TimePing: Error sending reply %s\n", QCC_StatusText(status));
    }
}


/* Signal handler. */
void AJ_CALL signal_handler(const alljoyn_interfacedescription_member* member, const char* srcPath, alljoyn_message msg)
{
    static uint32_t rxCounts = 0;
    QStatus status = ER_OK;

    /* Enable concurrent callbacks since some of the calls below could block */
    alljoyn_busattachment_enableconcurrentcallbacks(g_msgBus);

    if ((++rxCounts % g_reportInterval) == 0) {
        printf("RxSignal: %s - %u\n", srcPath, rxCounts);

        if (alljoyn_message_isencrypted(msg) == QCC_TRUE) {
            printf("Authenticated using %s\n", alljoyn_message_getauthmechanism(msg));
        }
    }

    if (g_echo_signal) {
        alljoyn_msgarg arg;
        uint8_t flags = 0;
        alljoyn_interfacedescription intf = NULL;
        alljoyn_interfacedescription_member my_signal_member;
        QCC_BOOL foundMember = QCC_FALSE;

        arg = alljoyn_msgarg_create_and_set("a{ys}", 0);

        if (g_compress) {
            flags |= ALLJOYN_MESSAGE_FLAG_COMPRESSED;
        }

        intf = alljoyn_busattachment_getinterface(g_msgBus, alljoyn_message_getinterface(msg));
        if (intf != NULL) {
            foundMember = alljoyn_interfacedescription_getmember(intf, "my_signal", &my_signal_member);
        }

        if (foundMember) {
            status = alljoyn_busobject_signal(g_testObj, alljoyn_message_getsender(msg), alljoyn_message_getsessionid(msg), my_signal_member, arg, 1, 0, 0, msg);
            if (status != ER_OK) {
                printf("Failed to send Signal because of %s. \n", QCC_StatusText(status));
            }
        } else {
            printf("Not able to send signal as could not find signal member. \n");
        }

        /* Destroy the msgarg */
        alljoyn_msgarg_destroy(arg);
    }

    /* ping back means make a method call when you receive a signal. */
    if (g_ping_back) {

        alljoyn_msgarg arg;
        alljoyn_interfacedescription intf = NULL;
        alljoyn_interfacedescription_member my_ping_member;
        QCC_BOOL foundMember = QCC_FALSE;

        arg = alljoyn_msgarg_create_and_set("s", "Ping back");

        intf = alljoyn_busattachment_getinterface(g_msgBus, alljoyn_message_getinterface(msg));
        if (intf != NULL) {
            foundMember = alljoyn_interfacedescription_getmember(intf, "my_ping", &my_ping_member);
            if (foundMember) {

                alljoyn_message reply = NULL;
                alljoyn_proxybusobject remoteObj = NULL;

                reply = alljoyn_message_create(g_msgBus);

                remoteObj = alljoyn_proxybusobject_create(g_msgBus, alljoyn_message_getsender(msg), OBJECT_PATH, alljoyn_message_getsessionid(msg));
                alljoyn_proxybusobject_addinterface(remoteObj, intf);
                /*
                 * Make a fire-and-forget method call. If the signal was encrypted encrypt the ping
                 */
                status = alljoyn_proxybusobject_methodcall(remoteObj, INTERFACE_NAME, "my_ping", arg, 1, reply, 5000, alljoyn_message_isencrypted(msg) ? ALLJOYN_MESSAGE_FLAG_ENCRYPTED : 0);
                if (status != ER_OK) {
                    printf("MethodCall on %s.%s failed due to %s. \n", INTERFACE_NAME, "my_ping", QCC_StatusText(status));
                }

                /* Destroy the message reply. */
                alljoyn_message_destroy(reply);
                /* Destroy the proxy bus object. */
                alljoyn_proxybusobject_destroy(remoteObj);
            }
        }
        /* Destroy the msgarg */
        alljoyn_msgarg_destroy(arg);
    }
}

QStatus AJ_CALL property_get(const void* context, const char* ifcName, const char* propName, alljoyn_msgarg val)
{
    QStatus status = ER_OK;
    if (0 == strcmp("int_val", propName)) {
        status = alljoyn_msgarg_set_int32(val, g_prop_int_val);
    } else if (0 == strcmp("str_val", propName)) {
        status = alljoyn_msgarg_set_string(val, g_prop_str_val);
    } else if (0 == strcmp("ro_str", propName)) {
        status = alljoyn_msgarg_set_string(val, g_prop_ro_str);
    } else {
        status = ER_BUS_NO_SUCH_PROPERTY;
    }
    return status;
}

QStatus AJ_CALL property_set(const void* context, const char* ifcName, const char* propName, alljoyn_msgarg val)
{
    QStatus status = ER_OK;
    int set_i;
    char*set_string;

    if ((0 == strcmp("int_val", propName)) && (alljoyn_msgarg_gettype(val) == ALLJOYN_INT32)) {
        status = alljoyn_msgarg_get(val, "i", &set_i);
        if (ER_OK == status) {
            g_prop_int_val = set_i;
        }
    } else if ((0 == strcmp("str_val", propName)) && (alljoyn_msgarg_gettype(val) == ALLJOYN_STRING)) {
        status = alljoyn_msgarg_get(val, "s", &set_string);
        if (ER_OK == status) {
            strcpy(g_prop_str_val, set_string);
        }
    } else if (0 == strcmp("ro_str", propName)) {
        status = ER_BUS_PROPERTY_ACCESS_DENIED;
    } else {
        status = ER_BUS_NO_SUCH_PROPERTY;
    }

    return status;
}

void AJ_CALL busobject_object_unregistered(const void* context)
{
    printf("Bus object unregistered. \n");
}

/* ObjectRegistered callback */
void AJ_CALL busobject_object_registered(const void* context)
{
    QStatus status = ER_OK;

    /* Enable concurrent callbacks since some of the calls below could block */
    alljoyn_busattachment_enableconcurrentcallbacks(g_msgBus);

    status = alljoyn_busattachment_bindsessionport(g_msgBus, &SESSION_PORT, g_sessionOpts, g_sessionPortListener);
    if (status != ER_OK) {
        printf("BindSessionPort failed with %s. \n", QCC_StatusText(status));
    }
    /* Add rule for receiving test signals */
    status = alljoyn_busattachment_addmatch(g_msgBus, "type='signal',interface='org.alljoyn.alljoyn_test',member='my_signal'");
    if (status != ER_OK) {
        printf("Failed to register Match rule for 'org.alljoyn.alljoyn_test.my_signal' with error %s. \n", QCC_StatusText(status));
    }

    /* Request a well-known name */
    status = alljoyn_busattachment_requestname(g_msgBus, g_wellKnownName, DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (status != ER_OK) {
        printf("RequestName(%s) failed with %s", g_wellKnownName, QCC_StatusText(status));
        return;
    }

    /* Begin Advertising the well-known name */
    /* transport mask change as per user input. */
    status = alljoyn_busattachment_advertisename(g_msgBus, g_wellKnownName, alljoyn_sessionopts_get_transports(g_sessionOpts));
    if (ER_OK != status) {
        printf("Advertise(%s) failed with %s", g_wellKnownName, QCC_StatusText(status));
        return;
    }

}

/* This is similar to calling BusObject constructor. */
QStatus AJ_CALL bus_object_init() {

    QStatus status = ER_OK;
    QCC_BOOL foundMember = QCC_FALSE;
    alljoyn_interfacedescription_member my_ping_member, my_delayed_ping_member, my_time_ping_member, my_signal_member;

    alljoyn_interfacedescription intf = NULL;
    alljoyn_interfacedescription valuesintf = NULL;

    alljoyn_busobject_methodentry methodEntries[3];

    intf = alljoyn_busattachment_getinterface(g_msgBus, INTERFACE_NAME);
    valuesintf = alljoyn_busattachment_getinterface(g_msgBus, INTERFACE_VALUE_NAME);

    if (!intf) {
        printf("ERROR - Could not fetch %s interface from the bus. \n", INTERFACE_NAME);
        return ER_FAIL;
    }

    if (!valuesintf) {
        printf("ERROR - Could not fetch %s interface from the bus. \n", INTERFACE_VALUE_NAME);
        return ER_FAIL;
    }

    /* Add interfaces to the bus object. */
    alljoyn_busobject_addinterface(g_testObj, intf);
    alljoyn_busobject_addinterface(g_testObj, valuesintf);

    /*Get the members. */
    foundMember = alljoyn_interfacedescription_getmember(intf, "my_signal", &my_signal_member);
    if (!foundMember) {
        printf("ERROR - Could not fetch %s member from %s interface. \n", "my_signal", INTERFACE_NAME);
        return ER_FAIL;
    }

    foundMember = QCC_FALSE;
    foundMember = alljoyn_interfacedescription_getmember(intf, "my_ping", &my_ping_member);
    if (!foundMember) {
        printf("ERROR - Could not fetch %s member from %s interface. \n", "my_ping", INTERFACE_NAME);
        return ER_FAIL;
    }

    foundMember = QCC_FALSE;
    foundMember = alljoyn_interfacedescription_getmember(intf, "delayed_ping", &my_delayed_ping_member);
    if (!foundMember) {
        printf("ERROR - Could not fetch %s member from %s interface. \n", "delayed_ping", INTERFACE_NAME);
        return ER_FAIL;
    }

    foundMember = QCC_FALSE;
    foundMember = alljoyn_interfacedescription_getmember(intf, "time_ping", &my_time_ping_member);
    if (!foundMember) {
        printf("ERROR - Could not fetch %s member from %s interface. \n", "time_ping", INTERFACE_NAME);
        return ER_FAIL;
    }


    /* Register a signal handler. */
    status = alljoyn_busattachment_registersignalhandler(g_msgBus, &signal_handler, my_signal_member, NULL);
    if (ER_OK != status) {
        printf("Failed to register signal handler with %s", QCC_StatusText(status));
        return status;
    }

    /* Register Method handlers. */
    methodEntries[0].member = &my_ping_member;
    methodEntries[0].method_handler = ping;

    methodEntries[1].member = &my_delayed_ping_member;
    methodEntries[1].method_handler = delayed_ping;

    methodEntries[2].member = &my_time_ping_member;
    methodEntries[2].method_handler = time_ping;


    status = alljoyn_busobject_addmethodhandlers(g_testObj, methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    if (ER_OK != status) {
        printf("Failed to register method handlers because of %s\n", QCC_StatusText(status));
        return status;
    }

    return status;
}

void usage(void)
{
    printf("Usage: bbcservice [-h <name>] [-m] [-e] [-x] [-i #] [-n <name>] [-b] [-t] [-r] [-l]\n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -?                    = Print this help message\n");
    printf("   -k <key store name>   = The key store file name\n");
    printf("   -kx #                 = Authentication key expiration (seconds)\n");
    printf("   -m                    = Session is a multi-point session\n");
    printf("   -e                    = Echo received signals back to sender\n");
    printf("   -x                    = Compress signals echoed back to sender\n");
    printf("   -i #                  = Signal report interval (number of signals rx per update; default = 1000)\n");
    printf("   -n <well-known name>  = Well-known name to advertise\n");
    printf("   -t                    = Advertise over TCP (enables selective advertising)\n");
    printf("   -l                    = Advertise locally (enables selective advertising)\n");
    printf("   -r                    = Advertise using the Rendezvous Server (enables selective advertising)\n");
    printf("   -w                    = Advertise over Wi-Fi Direct (enables selective advertising)\n");
    printf("   -a                    = Cancel advertising while servicing a single client (causes rediscovery between iterations)\n");
    printf("   -p                    = Respond to an incoming signal by pinging back to the sender\n");
}

int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    const char* keyStore = NULL;
    int i = 0;

    alljoyn_interfacedescription intf = NULL;
    alljoyn_interfacedescription intfvalue = NULL;

    /* SessionPort Listeners. */
    alljoyn_sessionportlistener_callbacks spl_cbs = {
        &accept_session_joiner,
        &session_joined
    };

    /* SessionPort Listeners. */
    alljoyn_sessionlistener_callbacks sl_cbs = {
        &session_lost, //session lost callback
        NULL, //session member added callback
        NULL //session member lost callback
    };

    /* Bus Object callbacks */
    alljoyn_busobject_callbacks busObjCbs = {
        &property_get,
        &property_set,
        &busobject_object_registered,
        &busobject_object_unregistered
    };

    /* Auth listener callbacks. */
    alljoyn_authlistenerasync_callbacks authcbs = {
        request_credentials_async,
        verify_credentials_async,
        security_violation,
        authentication_complete
    };

    alljoyn_authlistener authListener;

    printf("AllJoyn Library version: %s\n", alljoyn_getversion());
    printf("AllJoyn Library build info: %s\n", alljoyn_getbuildinfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    g_wellKnownName = (char*)DEFAULT_WELLKNOWN_NAME;

    g_sessionOpts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);

    /* Parse command line args */
    for (i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i]) || 0 == strcmp("-?", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-p", argv[i])) {
            if (g_echo_signal) {
                printf("options -e and -p are mutually exclusive\n");
                usage();
                exit(1);
            }
            g_ping_back = QCC_TRUE;
        } else if (0 == strcmp("-e", argv[i])) {
            if (g_ping_back) {
                printf("options -p and -e are mutually exclusive\n");
                usage();
                exit(1);
            }
            g_echo_signal = QCC_TRUE;;
        } else if (0 == strcmp("-x", argv[i])) {
            g_compress = QCC_TRUE;
        } else if (0 == strcmp("-i", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_reportInterval = strtoul(argv[i], NULL, 10);
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
        } else if (0 == strcmp("-m", argv[i])) {
            alljoyn_sessionopts_set_multipoint(g_sessionOpts, QCC_TRUE);
        } else if (0 == strcmp("-t", argv[i])) {
            alljoyn_sessionopts_set_transports(g_sessionOpts, ALLJOYN_TRANSPORT_WLAN);
        } else if (0 == strcmp("-l", argv[i])) {
            alljoyn_sessionopts_set_transports(g_sessionOpts, ALLJOYN_TRANSPORT_LOCAL);
        } else if (0 == strcmp("-w", argv[i])) {
            alljoyn_sessionopts_set_transports(g_sessionOpts, ALLJOYN_TRANSPORT_WFD);
        } else if (0 == strcmp("-a", argv[i])) {
            g_cancelAdvertise = QCC_TRUE;
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    //Create bus attachment
    g_msgBus = alljoyn_busattachment_create("bbcservice", QCC_TRUE);

    //Create and add interfaces to the bus

    status = alljoyn_busattachment_createinterface(g_msgBus, INTERFACE_NAME, &intf);
    if (status != ER_OK) {
        printf("Could not create %s interface because of %s. \n", INTERFACE_NAME, QCC_StatusText(status));
        return status;
    }

    status = alljoyn_busattachment_createinterface(g_msgBus, INTERFACE_VALUE_NAME, &intfvalue);
    if (status != ER_OK) {
        printf("Could not create %s interface because of %s. \n", INTERFACE_VALUE_NAME, QCC_StatusText(status));
        return status;
    }


    /* Activate org.alljoyn.alljoyn_test */
    status = alljoyn_interfacedescription_addmethod(intf, "my_ping", "s", "s", "i,i", 0, NULL);
    if (status != ER_OK) {
        printf("Could not add method %s to interface %s because of %s. \n", "my_ping", INTERFACE_NAME, QCC_StatusText(status));
        return status;
    }

    status = alljoyn_interfacedescription_addmethod(intf, "delayed_ping", "su", "s", "i,i", 0, NULL);
    if (status != ER_OK) {
        printf("Could not add method %s to interface %s because of %s. \n", "delayed_ping", INTERFACE_NAME, QCC_StatusText(status));
        return status;
    }

    status = alljoyn_interfacedescription_addmethod(intf, "time_ping", "uq", "uq", "i,i", 0, NULL);
    if (status != ER_OK) {
        printf("Could not add method %s to interface %s because of %s. \n", "time_ping", INTERFACE_NAME, QCC_StatusText(status));
        return status;
    }

    status = alljoyn_interfacedescription_addmember(intf, ALLJOYN_MESSAGE_SIGNAL, "my_signal", "a{ys}",  NULL, "inStr", 0);
    if (status != ER_OK) {
        printf("Could not add signal %s to interface %s because of %s. \n", "my_signal", INTERFACE_NAME, QCC_StatusText(status));
        return status;
    }



    alljoyn_interfacedescription_activate(intf);

    /*Activate org.alljoyn.alljoyn_test.values */
    status = alljoyn_interfacedescription_addproperty(intfvalue, "int_val", "i", ALLJOYN_PROP_ACCESS_RW);
    if (status != ER_OK) {
        printf("Could not add property %s to interface %s because of %s. \n", "int_val", INTERFACE_VALUE_NAME, QCC_StatusText(status));
        return status;
    }

    status = alljoyn_interfacedescription_addproperty(intfvalue, "str_val", "s", ALLJOYN_PROP_ACCESS_RW);
    if (status != ER_OK) {
        printf("Could not add method %s to interface %s because of %s. \n", "str_val", INTERFACE_VALUE_NAME, QCC_StatusText(status));
        return status;
    }

    status = alljoyn_interfacedescription_addproperty(intfvalue, "ro_str", "s", ALLJOYN_PROP_ACCESS_READ);
    if (status != ER_OK) {
        printf("Could not add method %s to interface %s because of %s. \n", "ro_str", INTERFACE_VALUE_NAME, QCC_StatusText(status));
        return status;
    }

    alljoyn_interfacedescription_activate(intfvalue);

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

    /* SessionPort Listeners. */
    g_sessionPortListener = alljoyn_sessionportlistener_create(&spl_cbs, NULL);

    /* SessionPort Listeners. */
    g_sessionListener = alljoyn_sessionlistener_create(&sl_cbs, NULL);


    /* Bus Object call backs. */
    g_testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
    status = bus_object_init();
    if (status != ER_OK) {
        printf("Bus object init failed because of %s. \n", QCC_StatusText(status));
        return status;
    }

    alljoyn_busattachment_registerbusobject(g_msgBus, g_testObj);

    /* Auth listener callbacks. */
    authListener = alljoyn_authlistenerasync_create(&authcbs, NULL);

    status = alljoyn_busattachment_enablepeersecurity(g_msgBus, "ALLJOYN_SRP_KEYX ALLJOYN_PIN_KEYX ALLJOYN_RSA_KEYX ALLJOYN_SRP_LOGON", authListener, keyStore, keyStore != NULL);
    if (ER_OK != status) {
        printf("enablePeerSecurity failed (%s)\n", QCC_StatusText(status));
        return status;
    }

    /* Add a logon entry. */
    alljoyn_busattachment_addlogonentry(g_msgBus, "ALLJOYN_SRP_LOGON", "sleepy", "123456");

    if (ER_OK == status) {
        printf("bbcservice %s ready to accept connections\n", g_wellKnownName);
        while (g_interrupt == QCC_FALSE) {
#ifdef _WIN32
            Sleep(100 * 10 * 300);
#else
            usleep(100 * 1000 * 10 * 300);
#endif
        }
    }

    /* Clean up. */
    alljoyn_busattachment_unregisterbusobject(g_msgBus, g_testObj);

    alljoyn_sessionopts_destroy(g_sessionOpts);
    alljoyn_authlistenerasync_destroy(authListener);
    alljoyn_busobject_destroy(g_testObj);
    alljoyn_sessionportlistener_destroy(g_sessionPortListener);
    alljoyn_sessionlistener_destroy(g_sessionListener);
    alljoyn_busattachment_destroy(g_msgBus);

    return 0;
}

