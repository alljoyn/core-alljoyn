/**
 * @file
 * @brief Sample implementation of an AllJoyn service.
 *
 * This sample will show how to set up an AllJoyn service that will registered with the
 * well-known name 'org.alljoyn.Bus.signal_sample'.  The service will register a signal method 'nameChanged'
 * as well as a property 'name'.
 *
 * When the property 'sampleName' is changed by any client this service will emit the new name using
 * the 'nameChanged' signal.
 *
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014 AllSeen Alliance. All rights reserved.
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

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>
#include <alljoyn/Status.h>

using namespace std;
using namespace qcc;
using namespace ajn;

/** Static top level message bus object */
static BusAttachment* s_msgBus = NULL;

static SessionId s_sessionId = 0;

static const char* INTERFACE_NAME = "org.alljoyn.Bus.signal_sample";
static const char* SERVICE_NAME = "org.alljoyn.Bus.signal_sample";
static const char* SERVICE_PATH = "/";
static const SessionPort SERVICE_PORT = 25;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig)
{
    s_interrupt = true;
}

static const char* tags[] = { "en", "de" };
static const char* objId = "obj";
static const char* objDescription[] =  { "This is the object", "DE: This is the object" };
static const char* ifcId = "ifc";
static const char* ifcDescription[] =  { "This is the interface", "DE: This is the interface" };
static const char* nameChangedId = "nameChanged";
static const char* nameChangedDescription[] =  { "Emitted when the name changes", "DE: Emitted whent he name changes" };
static const char* argId = "arg";
static const char* nameChangedArgDescription[] =  { "This is the new name", "DE: This is the new name" };
static const char* propId = "prop";
static const char* namePropDescription[] =  { "This is the actual name", "DE: This is the actual name" };

class MyTranslator : public Translator {
  public:

    virtual ~MyTranslator() { }

    virtual size_t NumTargetLanguages() {
        return 2;
    }

    virtual void GetTargetLanguage(size_t index, qcc::String& ret) {
        ret.assign(tags[index]);
    }

    virtual const char* Translate(const char* sourceLanguage, const char* targetLanguage, const char* source) {
        size_t i = 0;
        if (targetLanguage && (0 == strcmp(targetLanguage, "de"))) {
            i = 1;
        }

        if (0 == strcmp(source, objId)) {
            return objDescription[i];
        }
        if (0 == strcmp(source, ifcId)) {
            return ifcDescription[i];
        }
        if (0 == strcmp(source, nameChangedId)) {
            return nameChangedDescription[i];
        }
        if (0 == strcmp(source, argId)) {
            return nameChangedArgDescription[i];
        }
        if (0 == strcmp(source, propId)) {
            return namePropDescription[i];
        }
        return NULL;
    }

};

class BasicSampleObject : public BusObject {
  public:
    BasicSampleObject(BusAttachment& bus, const char* path) :
        BusObject(path),
        nameChangedMember(NULL),
        prop_name("Default name")
    {
        /* Add org.alljoyn.Bus.signal_sample interface */
        InterfaceDescription* intf = NULL;
        QStatus status = bus.CreateInterface(INTERFACE_NAME, intf);
        if (status == ER_OK) {
            intf->AddSignal("nameChanged", "s", "newName", 0);
            intf->AddProperty("name", "s", PROP_ACCESS_RW);

            intf->SetDescriptionLanguage("");
            intf->SetDescription(ifcId);
            intf->SetMemberDescription("nameChanged", nameChangedId);
            intf->SetArgDescription("nameChanged", "newName", argId);
            intf->SetPropertyDescription("name", propId);

            intf->SetDescriptionTranslator(&translator);

            intf->Activate();
        } else {
            printf("Failed to create interface %s\n", INTERFACE_NAME);
        }

        status = AddInterface(*intf);

        if (status == ER_OK) {
            /* Register the signal handler 'nameChanged' with the bus*/
            nameChangedMember = intf->GetMember("nameChanged");
            assert(nameChangedMember);
        } else {
            printf("Failed to Add interface: %s", INTERFACE_NAME);
        }

        SetDescription("", objId);
        SetDescriptionTranslator(&translator);
    }

    QStatus EmitNameChangedSignal(qcc::String newName)
    {
        printf("Emiting Name Changed Signal.\n");
        assert(nameChangedMember);
        if (0 == s_sessionId) {
            printf("Sending NameChanged signal without a session id\n");
        }
        MsgArg arg("s", newName.c_str());
        uint8_t flags = ALLJOYN_FLAG_GLOBAL_BROADCAST;
        QStatus status = Signal(NULL, 0, *nameChangedMember, &arg, 1, 0, flags);

        return status;
    }

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        printf("Get 'name' property was called returning: %s\n", prop_name.c_str());
        QStatus status = ER_OK;
        if (0 == strcmp("name", propName)) {
            val.typeId = ALLJOYN_STRING;
            val.v_string.str = prop_name.c_str();
            val.v_string.len = prop_name.length();
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }

    QStatus Set(const char* ifcName, const char* propName, MsgArg& val)
    {
        QStatus status = ER_OK;
        if ((0 == strcmp("name", propName)) && (val.typeId == ALLJOYN_STRING)) {
            printf("Set 'name' property was called changing name to %s\n", val.v_string.str);
            prop_name = val.v_string.str;
            EmitNameChangedSignal(prop_name);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }
  private:
    const InterfaceDescription::Member* nameChangedMember;
    qcc::String prop_name;
    MyTranslator translator;
};

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

static MyBusListener s_busListener;

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
QStatus RegisterBusObjectAndConnect(BasicSampleObject* obj)
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

    /* Register a bus listener */
    if (ER_OK == status) {
        s_msgBus->RegisterBusListener(s_busListener);
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    BasicSampleObject testObj(*s_msgBus, SERVICE_PATH);

    if (ER_OK == status) {
        status = RegisterBusObjectAndConnect(&testObj);
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
    delete s_msgBus;
    s_msgBus = NULL;

    printf("Signal service exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

    return (int) status;
}
