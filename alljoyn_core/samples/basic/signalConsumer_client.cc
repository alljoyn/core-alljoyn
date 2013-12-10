/**
 * @file
 * @brief Sample implementation of an AllJoyn client.
 *
 * This client will subscribe to the nameChanged signal sent from the 'org.alljoyn.Bus.signal_sample'
 * service.  When a name change signal is sent this will print out the new value for the
 * 'name' property that was sent by the service.
 *
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/version.h>
#include <alljoyn/AllJoynStd.h>

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

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.Bus.signal_sample";
static const char* SERVICE_NAME = "org.alljoyn.Bus.signal_sample";
static const char* SERVICE_PATH = "/";
static const SessionPort SERVICE_PORT = 25;

/** Static top level message bus object */
static BusAttachment* s_msgBus = NULL;

static bool s_joinComplete = false;
static SessionId s_sessionId = 0;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig)
{
    s_interrupt = true;
}

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener {
  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        if (0 == strcmp(name, SERVICE_NAME)) {
            printf("FoundAdvertisedName(name='%s', prefix='%s')\n", name, namePrefix);

            /* We found a remote bus that is advertising basic service's well-known name so connect to it. */
            /* Since we are in a callback we must enable concurrent callbacks before calling a synchronous method. */
            s_msgBus->EnableConcurrentCallbacks();
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status = s_msgBus->JoinSession(name, SERVICE_PORT, NULL, s_sessionId, opts);
            if (ER_OK == status) {
                printf("JoinSession SUCCESS (Session id=%d).\n", s_sessionId);
            } else {
                printf("JoinSession failed (status=%s).\n", QCC_StatusText(status));
            }
        }
        s_joinComplete = true;
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

class SignalListeningObject : public BusObject {
  public:
    SignalListeningObject(BusAttachment& bus, const char* path) :
        BusObject(path),
        nameChangedMember(NULL)
    {
        /* Add org.alljoyn.Bus.signal_sample interface */
        InterfaceDescription* intf = NULL;
        QStatus status = bus.CreateInterface(INTERFACE_NAME, intf);
        if (status == ER_OK) {
            printf("Interface created successfully.\n");
            intf->AddSignal("nameChanged", "s", "newName", 0);
            intf->AddProperty("name", "s", PROP_ACCESS_RW);
            intf->Activate();
        } else {
            printf("Failed to create interface %s\n", INTERFACE_NAME);
        }

        status = AddInterface(*intf);

        if (status == ER_OK) {
            printf("Interface successfully added to the bus.\n");
            /* Register the signal handler 'nameChanged' with the bus*/
            nameChangedMember = intf->GetMember("nameChanged");
            assert(nameChangedMember);
        } else {
            printf("Failed to Add interface: %s.", INTERFACE_NAME);
        }

        /* register the signal handler for the the 'nameChanged' signal */
        status =  bus.RegisterSignalHandler(this,
                                            static_cast<MessageReceiver::SignalHandler>(&SignalListeningObject::NameChangedSignalHandler),
                                            nameChangedMember,
                                            NULL);
        if (status != ER_OK) {
            printf("Failed to register signal handler for %s.nameChanged.\n", SERVICE_NAME);
        } else {
            printf("Registered signal handler for %s.nameChanged.\n", SERVICE_NAME);
        }
    }

    QStatus SubscribeNameChangedSignal(void) {
        assert(bus);
        return bus->AddMatch("type='signal',interface='org.alljoyn.Bus.signal_sample',member='nameChanged'");
    }

    void NameChangedSignalHandler(const InterfaceDescription::Member* member,
                                  const char* sourcePath,
                                  Message& msg)
    {
        printf("--==## signalConsumer: Name Changed signal Received ##==--\n");
        printf("\tNew name: '%s'.\n", msg->GetArg(0)->v_string.str);
    }

  private:
    const InterfaceDescription::Member* nameChangedMember;

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

/** Register the bus object and connect, report the result to stdout, and return the status code. */
QStatus RegisterBusObjectAndConnect(SignalListeningObject* obj)
{
    printf("Registering the bus object.\n");
    s_msgBus->RegisterBusObject(*obj);

    QStatus status = s_msgBus->Connect();

    if (ER_OK == status) {
        printf("Connected to '%s'.\n", s_msgBus->GetConnectSpec().c_str());
    } else {
        printf("Failed to connect to '%s'.\n", s_msgBus->GetConnectSpec().c_str());
    }

    return status;
}

/** Register a bus listener in order to get discovery indications and report the event to stdout. */
void RegisterBusListener(void)
{
    /* Static bus listener */
    static MyBusListener s_busListener;

    s_msgBus->RegisterBusListener(s_busListener);
    printf("BusListener registered.\n");
}


/** Find the advertised name, report the result to stdout, and return the status code. */
QStatus FindAdvertisedName(void)
{
    QStatus status = s_msgBus->FindAdvertisedName(SERVICE_NAME);

    if (ER_OK == status) {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') succeeded.\n", SERVICE_NAME);
    } else {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') failed (%s))\n", SERVICE_NAME, QCC_StatusText(status));
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

/** Subscribe to the name changed signal, report the result to stdout, and return the status code. */
QStatus SubscribeToNameChangedSignal(SignalListeningObject* object)
{
    QStatus status = object->SubscribeNameChangedSignal();

    if (ER_OK == status) {
        printf("Successfully subscribed to the name changed signal.\n");
    } else {
        printf("Failed to subscribe to the name changed signal.\n");
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

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    QStatus status = ER_OK;

    /* Create message bus */
    s_msgBus = new BusAttachment("myApp", true);

    /* This test for NULL is only required if new() behavior is to return NULL
     * instead of throwing an exception upon an out of memory failure.
     */
    if (!s_msgBus) {
        status = ER_OUT_OF_MEMORY;
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    SignalListeningObject object(*s_msgBus, SERVICE_PATH);

    if (ER_OK == status) {
        status = RegisterBusObjectAndConnect(&object);
    }

    if (ER_OK == status) {
        RegisterBusListener();
        status = FindAdvertisedName();
    }

    if (ER_OK == status) {
        status = WaitForJoinSessionCompletion();
    }

    if (ER_OK == status) {
        status = SubscribeToNameChangedSignal(&object);
    }

    /* Wait for the name changes until the user signals for an exit. */
    if (ER_OK == status) {
        WaitForSigInt();
    }

    /* Deallocate bus */
    delete s_msgBus;
    s_msgBus = NULL;

    printf("Signal consumer client exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

    return (int) status;
}
