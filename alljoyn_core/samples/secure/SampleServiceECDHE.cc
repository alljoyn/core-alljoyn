/**
 * @file
 * @brief Sample implementation of an AllJoyn secure service using ECDHE.
 *
 */

/******************************************************************************
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

#include <qcc/String.h>
#include <qcc/Log.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Init.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

using namespace std;
using namespace qcc;
using namespace ajn;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.bus.samples.secure.SecureInterface";
static const char* SERVICE_NAME = "org.alljoyn.bus.samples.secure";
static const char* SERVICE_PATH = "/SecureService";
static const char* KEYX_ECDHE_NULL = "ALLJOYN_ECDHE_NULL";
static const char* KEYX_ECDHE_PSK = "ALLJOYN_ECDHE_PSK";
static const char* KEYX_ECDHE_ECDSA = "ALLJOYN_ECDHE_ECDSA";
static const char* ECDHE_KEYX = "ALLJOYN_ECDHE_ECDSA";
static const SessionPort SERVICE_PORT = 42;

static volatile sig_atomic_t s_interrupt = false;

/**
 * Control-C signal handler
 */
static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    s_interrupt = true;
}

/*
 *  Implementation of a BusObject
 *  This class contains the implementation of the secure interface.
 *  The Ping method is the code that will be called when the a remote process
 *  makes a remote method call to Ping.
 *
 *
 */
class BasicSampleObject : public BusObject {
  public:
    BasicSampleObject(BusAttachment& bus, const char* path) :
        BusObject(path)
    {
        /** Add the test interface to this object */
        const InterfaceDescription* exampleIntf = bus.GetInterface(INTERFACE_NAME);
        assert(exampleIntf);
        AddInterface(*exampleIntf);

        /** Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { exampleIntf->GetMember("Ping"), static_cast<MessageReceiver::MethodHandler>(&BasicSampleObject::Ping) }
        };
        QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        if (ER_OK != status) {
            printf("Failed to register method handlers for BasicSampleObject");
        }
    }

    void ObjectRegistered()
    {
        BusObject::ObjectRegistered();
    }


    void Ping(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);

        qcc::String outStr = msg->GetArg(0)->v_string.str;
        printf("Ping : %s\n", outStr.c_str());
        printf("Reply : %s\n", outStr.c_str());
        MsgArg outArg("s", outStr.c_str());
        QStatus status = MethodReply(msg, &outArg, 1);
        if (ER_OK != status) {
            printf("Ping: Error sending reply\n");
        }
    }
};

/*
 * The MyBusListener class implements the public methods for two classes
 * BusListener and SessionPortListener
 * back method
 * The SessionPortListener is responsible for providing the AcceptSessionJoiner
 * call back method
 */
class MyBusListener : public BusListener, public SessionPortListener {

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != SERVICE_PORT) {
            printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
            return false;
        }
        printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
               joiner, opts.proximity, opts.traffic, opts.transports);
        return true;
    }
};

/** Static top level message bus object */
static BusAttachment* g_msgBus = NULL;
static MyBusListener s_busListener;


/*
 * This is the local implementation of the an AuthListener.  ECDHEKeyXListener is
 * designed to only handle ECDHE Key Exchange Authentication requests.
 *
 * If any other authMechanism is used other than ECDHE Key Exchange authentication
 * will fail.
 */
class ECDHEKeyXListener : public AuthListener {
  public:
    ECDHEKeyXListener()
    {
    }

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
    {
        QCC_UNUSED(userId);

        printf("RequestCredentials for authenticating %s using mechanism %s authCount %d\n", authPeer, authMechanism, authCount);

        if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0) {
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_PSK) == 0) {
            /*
             * Solicit the Pre shared secret
             */
            if ((credMask& AuthListener::CRED_USER_NAME) == AuthListener::CRED_USER_NAME) {
                printf("RequestCredentials received psk ID %s\n", creds.GetUserName().c_str());
            }
            /*
             * Based on the pre shared secret id, the application can retrieve
             * the pre shared secret from storage or from the end user.
             * In this example, the pre shared secret is a hard coded string.
             * Pre-shared keys should be 128 bits long, and generated with a
             * cryptographically secure random number generator.
             */
            String psk("faaa0af3dd3f1e0379da046a3ab6ca44");
            creds.SetPassword(psk);
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_ECDSA) == 0) {
            static const char ecdsaPrivateKeyPEM[] = {
                "-----BEGIN EC PRIVATE KEY-----\n"
                "MDECAQEEIICSqj3zTadctmGnwyC/SXLioO39pB1MlCbNEX04hjeioAoGCCqGSM49\n"
                "AwEH\n"
                "-----END EC PRIVATE KEY-----"
            };

            static const char ecdsaCertChainX509PEM[] = {
                "-----BEGIN CERTIFICATE-----\n"
                "MIIBWjCCAQGgAwIBAgIHMTAxMDEwMTAKBggqhkjOPQQDAjArMSkwJwYDVQQDDCAw\n"
                "ZTE5YWZhNzlhMjliMjMwNDcyMGJkNGY2ZDVlMWIxOTAeFw0xNTAyMjYyMTU1MjVa\n"
                "Fw0xNjAyMjYyMTU1MjVaMCsxKTAnBgNVBAMMIDZhYWM5MjQwNDNjYjc5NmQ2ZGIy\n"
                "NmRlYmRkMGM5OWJkMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEP/HbYga30Afm\n"
                "0fB6g7KaB5Vr5CDyEkgmlif/PTsgwM2KKCMiAfcfto0+L1N0kvyAUgff6sLtTHU3\n"
                "IdHzyBmKP6MQMA4wDAYDVR0TBAUwAwEB/zAKBggqhkjOPQQDAgNHADBEAiAZmNVA\n"
                "m/H5EtJl/O9x0P4zt/UdrqiPg+gA+wm0yRY6KgIgetWANAE2otcrsj3ARZTY/aTI\n"
                "0GOQizWlQm8mpKaQ3uE=\n"
                "-----END CERTIFICATE-----"
            };
            /*
             * The application may provide the DSA private key and public key in the certificate.
             */
            if ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                String pk(ecdsaPrivateKeyPEM, strlen(ecdsaPrivateKeyPEM));
                creds.SetPrivateKey(pk);
            }
            if ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                String cert(ecdsaCertChainX509PEM, strlen(ecdsaCertChainX509PEM));
                creds.SetCertChain(cert);
            }

            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        }
        return false;
    }

    bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
    {
        QCC_UNUSED(authPeer);
        /* only the ECDHE_ECDSA calls for peer credential verification */
        if (strcmp(authMechanism, KEYX_ECDHE_ECDSA) == 0) {
            if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                /*
                 * AllJoyn sends back the certificate chain for the application to verify.
                 * The application has to option to verify the certificate chain.  If the cert chain is validated and trusted then return true; otherwise, return false.
                 */
                printf("VerifyCredentials receives cert chain %s\n", creds.GetCertChain().c_str());
            }
            return true;
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_UNUSED(authPeer);
        printf("SampleServiceECDHE::AuthenticationComplete Authentication %s %s\n", authMechanism, success ? "successful" : "failed");
    }

};


/** Create the interface, report the result to stdout, and return the result status. */
QStatus CreateInterface(void)
{
    /* Add org.alljoyn.bus.samples.secure.SecureInterface interface */
    InterfaceDescription* testIntf = NULL;
    QStatus status = g_msgBus->CreateInterface(INTERFACE_NAME, testIntf, AJ_IFC_SECURITY_REQUIRED);

    if (ER_OK == status) {
        status = testIntf->AddMethod("Ping", "s",  "s", "inStr,outStr", 0);

        if (ER_OK == status) {
            testIntf->Activate();
            printf("Successfully created the 'Ping' method for the '%s' interface.\n", INTERFACE_NAME);
        } else {
            printf("Failed to add 'Ping' method to the interface '%s'.\n", INTERFACE_NAME);
        }
    } else {
        printf("Failed to create interface '%s'.\n", INTERFACE_NAME);
    }

    return status;
}

/** Start the message bus, report the result to stdout, and return the status code. */
QStatus StartMessageBus(void)
{
    QStatus status = g_msgBus->Start();

    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("Start of BusAttachment failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Register the bus object and connect, report the result to stdout, and return the status code. */
QStatus RegisterBusObject(BasicSampleObject* obj)
{
    printf("Registering the bus object.\n");
    QStatus status = g_msgBus->RegisterBusObject(*obj);

    if (ER_OK == status) {
        printf("BusAttachment::RegisterBusObject succeeded.\n");
    } else {
        printf("BusAttachment::RegisterBusObject failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Enable the security, report the result to stdout, and return the status code. */
QStatus EnableSecurity()
{
    QCC_SetDebugLevel("ALLJOYN_AUTH", 3);
    QCC_SetDebugLevel("CRYPTO", 3);
    QCC_SetDebugLevel("AUTH_KEY_EXCHANGER", 3);
    QStatus status = g_msgBus->EnablePeerSecurity(ECDHE_KEYX, new ECDHEKeyXListener(), "/.alljoyn_keystore/s_ecdhe.ks", false);

    if (ER_OK == status) {
        printf("BusAttachment::EnablePeerSecurity successful.\n");
    } else {
        printf("BusAttachment::EnablePeerSecurity failed (%s).\n", QCC_StatusText(status));
    }
    return status;
}

/** Connect the bus, report the result to stdout, and return the status code. */
QStatus Connect(void)
{
    QStatus status = g_msgBus->Connect();

    if (ER_OK == status) {
        printf("Connected to '%s'.\n", g_msgBus->GetConnectSpec().c_str());
    } else {
        printf("Failed to connect to '%s'.\n", g_msgBus->GetConnectSpec().c_str());
    }

    return status;
}

/** Request the service name, report the result to stdout, and return the status code. */
QStatus RequestName(void)
{
    const uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
    QStatus status = g_msgBus->RequestName(SERVICE_NAME, flags);

    if (ER_OK == status) {
        printf("RequestName('%s') succeeded.\n", SERVICE_NAME);
    } else {
        printf("RequestName('%s') failed (status=%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}

/** Create the session, report the result to stdout, and return the status code. */
QStatus CreateSession(TransportMask mask)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, mask);
    SessionPort sp = SERVICE_PORT;
    QStatus status = g_msgBus->BindSessionPort(sp, opts, s_busListener);

    if (ER_OK == status) {
        printf("BindSessionPort succeeded.\n");
    } else {
        printf("BindSessionPort failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Advertise the service name, report the result to stdout, and return the status code. */
QStatus AdvertiseName(TransportMask mask)
{
    QStatus status = g_msgBus->AdvertiseName(SERVICE_NAME, mask);

    if (ER_OK == status) {
        printf("Advertisement of the service name '%s' succeeded.\n", SERVICE_NAME);
    } else {
        printf("Failed to advertise name '%s' (%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}

/** Wait for SIGINT before continuing. */
void WaitForSigInt(void)
{
    while (s_interrupt == false) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
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

    QStatus status = ER_OK;

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Create message bus */
    g_msgBus = new BusAttachment("ECDHESecurityServiceA", true);

    if (!g_msgBus) {
        status = ER_OUT_OF_MEMORY;
    }

    if (ER_OK == status) {
        status = CreateInterface();
    }

    if (ER_OK == status) {
        g_msgBus->RegisterBusListener(s_busListener);
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    BasicSampleObject* testObj = NULL;
    if ((ER_OK == status) && (g_msgBus)) {
        testObj = new BasicSampleObject(*g_msgBus, SERVICE_PATH);
        status = RegisterBusObject(testObj);
        printf("RegisterBusObject at path %s returns status %d\n", SERVICE_PATH, status);
    }

    if (ER_OK == status) {
        status = EnableSecurity();
    }

    if (ER_OK == status) {
        status = Connect();
    }

    /*
     * Advertise this service on the bus.
     * There are three steps to advertising this service on the bus.
     * 1) Request a well-known name that will be used by the client to discover
     *    this service.
     * 2) Create a session.
     * 3) Advertise the well-known name.
     */
    if (ER_OK == status) {
        status = RequestName();
    }

    const TransportMask SERVICE_TRANSPORT_TYPE = TRANSPORT_ANY;

    if (ER_OK == status) {
        status = CreateSession(SERVICE_TRANSPORT_TYPE);
    }

    if (ER_OK == status) {
        status = AdvertiseName(SERVICE_TRANSPORT_TYPE);
    }

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        WaitForSigInt();
    }

    /* Clean up msg bus */
    delete testObj;
    delete g_msgBus;
    g_msgBus = NULL;

    printf("Basic service exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
