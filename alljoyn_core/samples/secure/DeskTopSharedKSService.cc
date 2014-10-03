/**
 * @file
 * @brief Sample implementation of an AllJoyn service.
 *
 * This sample has an implementation of a sercure sample that is setup to use a
 * shared keystore.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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
#include <time.h>

#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>
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
static const SessionPort SERVICE_PORT = 42;

static volatile sig_atomic_t s_interrupt = false;

/**
 * Control-C signal handler
 */
static void SigIntHandler(int sig)
{
    s_interrupt = true;
}

/*
 *  Implementation of a BusObject
 *  This class contains the implementation of the secure interface.
 *  The Ping method is the code that will be called when a remote process
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
        printf("ObjectRegistered has been called\n");
    }


    void Ping(const InterfaceDescription::Member* member, Message& msg)
    {
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
 * The BusListener class is responsible for providing the NameOwnerChanged call
 * back method
 * The SessionPortListener is responsible for providing the AcceptSessionJoiner
 * call back method
 */
class MyBusListener : public BusListener, public SessionPortListener {
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }
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
static BusAttachment* s_msgBus = NULL;
static MyBusListener s_busListener;

/*
 * This is the local implementation of an AuthListener.  SrpKeyXListener is
 * designed to only handle SRP Key Exchange Authentication requests.
 *
 * When a Password request (CRED_PASSWORD) comes in using ALLJOYN_SRP_KEYX the
 * code will generate a 6 digit random pin code.  The client must enter the same
 * pin code into his AuthListener for the Authentication to be successful.
 *
 * If any other authMechanism is used other than SRP Key Exchange authentication
 * will fail.
 */
class SrpKeyXListener : public AuthListener {
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {
        printf("RequestCredentials for authenticating %s using mechanism %s\n", authPeer, authMechanism);
        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                if (authCount <= 3) {
                    /* seed the random number */
                    srand(time(NULL));
                    int pin = rand() % 1000000;
                    char pinStr[7];
                    snprintf(pinStr, 7, "%06d", pin);
                    printf("One Time Password : %s\n", pinStr);
                    fflush(stdout);
                    creds.SetPassword(pinStr);
                    return true;
                } else {
                    return false;
                }
            }
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        printf("Authentication %s %s.\n", authMechanism, success ? "successful" : "failed");
    }
};

/** Create the interface, report the result to stdout, and return the result status. */
QStatus CreateInterface(void)
{
    /* Add org.alljoyn.bus.samples.secure.SecureInterface interface */
    InterfaceDescription* testIntf = NULL;
    QStatus status = s_msgBus->CreateInterface(INTERFACE_NAME, testIntf, AJ_IFC_SECURITY_REQUIRED);

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
    QStatus status = s_msgBus->Start();

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
    QStatus status = s_msgBus->RegisterBusObject(*obj);

    if (ER_OK == status) {
        printf("BusAttachment::RegisterBusObject succeeded.\n");
    } else {
        printf("BusAttachment::RegisterBusObject failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Enable the security, report the result to stdout, and return the status code. */
QStatus EnableSecurity(void)
{
    /*
     * note the location of the keystore file has been specified and the
     * isShared parameter is being set to true. So this keystore file can
     * be used by multiple applications
     */
    QStatus status = s_msgBus->EnablePeerSecurity("ALLJOYN_SRP_KEYX", new SrpKeyXListener(), "/.alljoyn_keystore/s_central.ks", true);

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
    QStatus status = s_msgBus->Connect();

    if (ER_OK == status) {
        printf("Connected to '%s'.\n", s_msgBus->GetConnectSpec().c_str());
    } else {
        printf("Failed to connect to '%s'.\n", s_msgBus->GetConnectSpec().c_str());
    }

    return status;
}

/** Request the service name, report the result to stdout, and return the status code. */
QStatus RequestName(void)
{
    const uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
    QStatus status = s_msgBus->RequestName(SERVICE_NAME, flags);

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
    QStatus status = s_msgBus->BindSessionPort(sp, opts, s_busListener);

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
    QStatus status = s_msgBus->AdvertiseName(SERVICE_NAME, mask);

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
int main(int argc, char** argv, char** envArg)
{
    printf("AllJoyn Library version: %s.\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s.\n", ajn::GetBuildInfo());

    QStatus status = ER_OK;

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Create message bus */
    s_msgBus = new BusAttachment("SRPSecurityServiceA", true);

    /* if we are unable to allocate a BusAttachment exit */
    if (!s_msgBus) {
        status = ER_OUT_OF_MEMORY;
    } else {
        if (ER_OK == status) {
            status = CreateInterface();
        }

        if (ER_OK == status) {
            s_msgBus->RegisterBusListener(s_busListener);
        }

        if (ER_OK == status) {
            status = StartMessageBus();
        }

        BasicSampleObject testObj(*s_msgBus, SERVICE_PATH);

        if (ER_OK == status) {
            status = RegisterBusObject(&testObj);
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
    }

    /* Clean up msg bus */
    delete s_msgBus;
    s_msgBus = NULL;

    printf("Basic service exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

    return (int) status;
}
