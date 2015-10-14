/**
 * @file
 * @brief Sample implementation of an AllJoyn service.
 *
 * This sample will show how to set up an AllJoyn service that will registered with the
 * wellknown name 'org.alljoyn.Bus.method_sample'.  The service will register a method call
 * with the name 'cat'  this method will take two input strings and return a
 * Concatenated version of the two strings.
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
#include <vector>

#include <qcc/String.h>

#include <alljoyn/AboutObj.h>
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
static const char* INTERFACE_NAME = "com.example.sample";
static const char* SERVICE_NAME = "com.example.sample";
static const char* SERVICE_PATH = "/sample";
static const SessionPort SERVICE_PORT = 16;

static volatile sig_atomic_t s_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    s_interrupt = true;
}

class BasicSampleObject : public BusObject {
  public:
    BasicSampleObject(BusAttachment& bus, const char* path) :
        BusObject(path)
    {
        /** Add the test interface to this object */
        const InterfaceDescription* exampleIntf = bus.GetInterface(INTERFACE_NAME);
        assert(exampleIntf);
        AddInterface(*exampleIntf, ANNOUNCED);

        /** Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { exampleIntf->GetMember("cat"), static_cast<MessageReceiver::MethodHandler>(&BasicSampleObject::Cat) }
        };
        QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        if (ER_OK != status) {
            printf("Failed to register method handlers for BasicSampleObject.\n");
        }
    }

    void ObjectRegistered()
    {
        BusObject::ObjectRegistered();
        printf("ObjectRegistered has been called.\n");
    }

    void Cat(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);
        /* Concatenate the two input strings and reply with the result. */
        qcc::String inStr1 = msg->GetArg(0)->v_string.str;
        qcc::String inStr2 = msg->GetArg(1)->v_string.str;
        qcc::String outStr = inStr1 + inStr2;

        MsgArg outArg("s", outStr.c_str());
        QStatus status = MethodReply(msg, &outArg, 1);
        if (ER_OK != status) {
            printf("Ping: Error sending reply.\n");
        }
    }
};

class MyBusListener : public BusListener, public SessionPortListener, public SessionListener {
  public:
    MyBusListener(BusAttachment& bus) : bus(bus) { }

    MyBusListener& operator=(const MyBusListener&);

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s.\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != SERVICE_PORT) {
            printf("Rejecting join attempt on unexpected session port %d.\n", sessionPort);
            return false;
        }
        printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x).\n",
               joiner, opts.proximity, opts.traffic, opts.transports);
        return true;
    }

    void SessionJoined(SessionPort sessionPort, SessionId sessionId, const char* joiner)
    {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        bus.SetSessionListener(sessionId, this);
    }

    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        printf("SessionLost(%08x) was called. Reason = %u.\n", sessionId, reason);
    }

  private:
    BusAttachment& bus;
};

/** The bus listener object. */
static MyBusListener* s_busListener;

/** Top level message bus object. */
static BusAttachment* s_msgBus = NULL;

/** Create the interface, report the result to stdout, and return the result status. */
QStatus CreateInterface(void)
{
    /* Add org.alljoyn.Bus.method_sample interface */
    InterfaceDescription* testIntf = NULL;
    QStatus status = s_msgBus->CreateInterface(INTERFACE_NAME, testIntf);

    if (status == ER_OK) {
        printf("Interface created.\n");
        testIntf->AddMethod("cat", "ss",  "s", "inStr1,inStr2,outStr", 0);
        testIntf->Activate();
    } else {
        printf("Failed to create interface '%s'.\n", INTERFACE_NAME);
    }

    return status;
}

/** Register the bus object and connect, report the result to stdout, and return the status code. */
QStatus RegisterBusObject(BasicSampleObject* obj)
{
    QStatus status = s_msgBus->RegisterBusObject(*obj);

    if (ER_OK == status) {
        printf("RegisterBusObject succeeded.\n");
    } else {
        printf("RegisterBusObject failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Connect, report the result to stdout, and return the status code. */
QStatus ConnectBusAttachment(void)
{
    QStatus status = s_msgBus->Connect();

    if (ER_OK == status) {
        printf("Connect to '%s' succeeded.\n", s_msgBus->GetConnectSpec().c_str());
    } else {
        printf("Failed to connect to '%s' (%s).\n", s_msgBus->GetConnectSpec().c_str(), QCC_StatusText(status));
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

/** Create the session, report the result to stdout, and return the status code. */
QStatus CreateSession(TransportMask mask)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, mask);
    SessionPort sp = SERVICE_PORT;
    QStatus status = s_msgBus->BindSessionPort(sp, opts, *s_busListener);

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
    BasicSampleObject* testObj = NULL;

    /* Create message bus */
    s_msgBus = new BusAttachment("myApp", true);

    /* Create Bus listener */
    s_busListener = new MyBusListener(*s_msgBus);

    if (s_msgBus) {
        if (ER_OK == status) {
            status = CreateInterface();
        }

        if (ER_OK == status) {
            s_msgBus->RegisterBusListener(*s_busListener);
        }

        if (ER_OK == status) {
            status = StartMessageBus();
        }

        testObj = new BasicSampleObject(*s_msgBus, SERVICE_PATH);

        if (ER_OK == status) {
            status = RegisterBusObject(testObj);
        }

        // Setup the about data
        // The default language is specified in the constructor. If the default language
        // is not specified any Field that should be localized will return an error
        AboutData aboutData("en");
        //AppId is a 128bit uuid
        uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                            0x1E, 0x82, 0x11, 0xE4,
                            0x86, 0x51, 0xD1, 0x56,
                            0x1D, 0x5D, 0x46, 0xB0 };
        status = aboutData.SetAppId(appId, 16);
        status = aboutData.SetDeviceName("My Device Name");
        //DeviceId is a string encoded 128bit UUID
        status = aboutData.SetDeviceId("93c06771-c725-48c2-b1ff-6a2a59d445b8");
        status = aboutData.SetAppName("Application");
        status = aboutData.SetManufacturer("Manufacturer");
        status = aboutData.SetModelNumber("123456");
        status = aboutData.SetDescription("A poetic description of this application");
        status = aboutData.SetDateOfManufacture("2014-03-24");
        status = aboutData.SetSoftwareVersion("0.1.2");
        status = aboutData.SetHardwareVersion("0.0.1");
        status = aboutData.SetSupportUrl("http://www.example.org");

        if (ER_OK == status) {
            status = ConnectBusAttachment();
        }

        const TransportMask SERVICE_TRANSPORT_TYPE = TRANSPORT_ANY;
        if (ER_OK == status) {
            status = CreateSession(SERVICE_TRANSPORT_TYPE);
        }

        if (ER_OK == status) {
            // Announce about signal
            AboutObj* aboutObj = new AboutObj(*s_msgBus);
            // Note the ObjectDescription that is part of the Announce signal is found
            // automatically by introspecting the BusObjects registered with the bus
            // attachment.
            status = aboutObj->Announce(SERVICE_PORT, aboutData);
            if (ER_OK == status) {
                printf("AboutObj Announce Succeeded.\n");
            } else {
                printf("AboutObj Announce failed (%s)\n", QCC_StatusText(status));
                exit(1);
            }
        }
        /* Perform the service asynchronously until the user signals for an exit. */
        if (ER_OK == status) {
            WaitForSigInt();
        }
    } else {
        status = ER_OUT_OF_MEMORY;
    }

    /* Clean up */
    delete s_msgBus;
    s_msgBus = NULL;
    delete s_busListener;
    s_busListener = NULL;
    delete testObj;
    testObj = NULL;

    printf("Basic service exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
