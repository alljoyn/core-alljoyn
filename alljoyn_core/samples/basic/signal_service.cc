/**
 * @file
 * @brief Sample implementation of an AllJoyn service.
 *
 * This sample will show how to set up an AllJoyn service that will registered with the
 * well-known name 'org.alljoyn.Bus.signal_sample'.  The service will register a signal method 'nameChanged'
 * as well as a property 'name'.
 *
 * When the property 'sampleName' is changed by any client this service will emit the new name using
 * the 'nameChanged' sessionless signal.
 *
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

#include <qcc/String.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Init.h>
#include <alljoyn/MsgArg.h>
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

static volatile sig_atomic_t s_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    s_interrupt = true;
}

static const char* tags[] = { "en", "de", "hi"  };
static const char* objId = "obj";
static const char* objDescription[] =  { "This is the object", "Es ist das Objekt", "Ye Object hai" };

class MyTranslator : public Translator {
  public:

    virtual ~MyTranslator() { }

    virtual size_t NumTargetLanguages() {
        return sizeof(tags) / sizeof(*tags);
    }

    virtual void GetTargetLanguage(size_t index, qcc::String& ret) {
        ret.assign(tags[index]);
    }

    virtual const char* Translate(const char* sourceLanguage, const char* targetLanguage, const char* source) {
        QCC_UNUSED(sourceLanguage);

        size_t i = 0;
        if (targetLanguage != nullptr) {
            if (strcmp(targetLanguage, "de") == 0) {
                i = 1;
            } else if (strcmp(targetLanguage, "hi") == 0) {
                i = 2;
            }
        }

        if (0 == strcmp(source, objId)) {
            return objDescription[i];
        }

        return nullptr;
    }

};

class BasicSampleObject : public BusObject {
  public:
    BasicSampleObject(BusAttachment& bus, const char* path) :
        BusObject(path),
        nameChangedMember(nullptr),
        prop_name("Default name")
    {
        /* Add org.alljoyn.Bus.signal_sample interface */
        InterfaceDescription* intf = nullptr;
        QStatus status = bus.CreateInterface(INTERFACE_NAME, intf);
        if (status == ER_OK) {
            intf->AddSignal("nameChanged", "s", "newName", MEMBER_ANNOTATE_SESSIONLESS);
            intf->AddMethod("testMethod", "s", "s", "inStr,outStr");
            intf->AddProperty("name", "s", PROP_ACCESS_RW);

            intf->SetDescriptionForLanguage("This is the first interface", "en");
            intf->SetDescriptionForLanguage("Dies ist das erste Schnittstelle", "de");
            intf->SetDescriptionForLanguage("Ye pehla Interface hai", "hi");
            intf->SetMemberDescriptionForLanguage("nameChanged", "Emitted when the name changes", "en");
            intf->SetMemberDescriptionForLanguage("nameChanged", "Emittiert, wenn der Name andert", "de");
            intf->SetMemberDescriptionForLanguage("nameChanged", "Naam badalne pe emitte karen", "hi");
            intf->SetMemberDescriptionForLanguage("testMethod", "This is the first method", "en");
            intf->SetMemberDescriptionForLanguage("testMethod", "Dies ist die erste Methode", "de");
            intf->SetMemberDescriptionForLanguage("testMethod", "Ye pehla method hai", "hi");
            intf->SetPropertyDescriptionForLanguage("name", "This is the actual name", "en");
            intf->SetPropertyDescriptionForLanguage("name", "Dies ist der eigentliche Name", "de");
            intf->SetPropertyDescriptionForLanguage("name", "Ye asli naam hai", "hi");

            intf->Activate();
        } else {
            printf("Failed to create interface %s (%s).\n", INTERFACE_NAME, QCC_StatusText(status));
        }

        status = AddInterface(*intf);

        if (status == ER_OK) {
            /* Register the signal handler 'nameChanged' with the bus */
            nameChangedMember = intf->GetMember("nameChanged");
            QCC_ASSERT(nameChangedMember);
        } else {
            printf("Failed to Add interface: %s (%s).", INTERFACE_NAME, QCC_StatusText(status));
        }

        SetDescription("", objId);
        SetDescriptionTranslator(&translator);
    }

    QStatus EmitNameChangedSignal(const qcc::String& newName)
    {
        printf("Emiting nameChanged sessionless signal.\n");
        QCC_ASSERT(nameChangedMember);

        MsgArg arg("s", newName.c_str());
        arg.Stabilize();
        uint8_t flags = ALLJOYN_FLAG_SESSIONLESS;
        QStatus status = Signal(nullptr, 0, *nameChangedMember, &arg, 1, 0, flags);
        if (status != ER_OK) {
            printf("Emiting signal failed (%s).\n", QCC_StatusText(status));
        }
        return status;
    }

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);

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
        QCC_UNUSED(ifcName);

        QStatus status = ER_OK;
        if ((0 == strcmp("name", propName)) && (val.typeId == ALLJOYN_STRING)) {
            printf("Set 'name' property was called changing name to '%s', current name: '%s'\n", val.v_string.str, prop_name.c_str());
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

void DoCleanup(void)
{
    fflush(stdout);
    s_msgBus->CancelAdvertiseName(SERVICE_NAME, TRANSPORT_ANY);
    if (s_msgBus->IsConnected()) {
        s_msgBus->Disconnect();
    }
    if (ER_OK == s_msgBus->Stop()) {
        s_msgBus->Join();
    }
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

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    QStatus status = ER_OK;
    BasicSampleObject* testObj = nullptr;

    /* Create message bus */
    s_msgBus = new BusAttachment("myApp", true);
    if (s_msgBus == nullptr) {
        status = ER_OUT_OF_MEMORY;
    }

    /* This test for nullptr is only required if new() behavior is to return nullptr
     * instead of throwing an exception upon an out of memory failure.
     */
    if (ER_OK == status) {
        /* Register a bus listener */
        if (ER_OK == status) {
            s_msgBus->RegisterBusListener(s_busListener);
        }

        if (ER_OK == status) {
            status = StartMessageBus();
        }

        testObj = new BasicSampleObject(*s_msgBus, SERVICE_PATH);

        if (ER_OK == status) {
            status = RegisterBusObjectAndConnect(testObj);
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
    } else {
        status = ER_OUT_OF_MEMORY;
    }
    /* Clean up */
    DoCleanup();
    if (s_msgBus != nullptr) {
        delete s_msgBus;
    }
    if (testObj != nullptr) {
        delete testObj;
    }

    printf("Signal service exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
