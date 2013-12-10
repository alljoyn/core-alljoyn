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

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>

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



/** Static top level message bus object */
static BusAttachment* g_msgBus = NULL;

/*constants*/
static const char* SERVICE_NAME = "org.alljoyn.Bus.method_sample";
static const char* SERVICE_PATH = "/method_sample";

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

class BasicSampleObject : public BusObject {
  public:
    BasicSampleObject(BusAttachment& bus, const char* path) :
        BusObject(bus, path)
    {
        /** Add the test interface to this object */
        const InterfaceDescription* exampleIntf = bus.GetInterface(SERVICE_NAME);
        assert(exampleIntf);
        AddInterface(*exampleIntf);

        /** Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { exampleIntf->GetMember("cat"), static_cast<MessageReceiver::MethodHandler>(&BasicSampleObject::Cat) }
        };
        QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        if (ER_OK != status) {
            printf("Failed to register method handlers for BasicSampleObject");
        }
    }

    /* override virtual function from base BusObject class
     * this method will call the 'RequestName' from the daemon.
     * If successful this will register the name 'org.alljoyn.Bus.method_sample'
     * as the well-known name of this service.*/
    void ObjectRegistered(void) {
        // this function must call ObjectRegistered from the base class.
        BusObject::ObjectRegistered();

        /* Request a well-known name */
        /* Note that you cannot make a blocking method call here */
        const ProxyBusObject& dbusObj = bus.GetDBusProxyObj();
        MsgArg args[2];
        args[0].Set("s", SERVICE_NAME);
        args[1].Set("u", DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        QStatus status = dbusObj.MethodCallAsync(ajn::org::freedesktop::DBus::InterfaceName,
                                                 "RequestName",
                                                 this,
                                                 static_cast<MessageReceiver::ReplyHandler>(&BasicSampleObject::RequestNameCB),
                                                 args, sizeof(args) / sizeof(args[0]));
        if (ER_OK != status) {
            printf("Failed to request name %s", SERVICE_NAME);
        }
    }

    /* The return value for the 'RequestName' call will be checked by this function
     * the ObjectRegistered function specified that this function should be used
     * @see { ObjectRegistered() } */
    void RequestNameCB(Message& msg, void* context)
    {
        if (msg->GetArg(0)->v_uint32 == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
            printf("Obtained the well-known name: %s\n", SERVICE_NAME);
            /* Begin Advertising the well known name to remote busses */
            const ProxyBusObject& alljoynObj = bus.GetAllJoynProxyObj();
            MsgArg arg("s", SERVICE_NAME);
            QStatus status = alljoynObj.MethodCallAsync(ajn::org::alljoyn::Bus::InterfaceName,
                                                        "AdvertiseName",
                                                        this,
                                                        static_cast<MessageReceiver::ReplyHandler>(&BasicSampleObject::AdvertiseRequestCB),
                                                        &arg,
                                                        1);
            if (ER_OK != status) {
                printf("Sending org.alljoyn.Bus.Advertise failed.\n");
            }
        } else {
            printf("Failed to request interface name 'org.alljoyn.Bus.method_sample'\n");
            exit(1);
        }
    }

    void AdvertiseRequestCB(Message& msg, void* context)
    {
        /* Make sure request was processed */
        size_t numArgs;
        const MsgArg* args;
        msg->GetArgs(numArgs, args);

        if ((MESSAGE_METHOD_RET != msg->GetType()) || (ALLJOYN_ADVERTISENAME_REPLY_SUCCESS != args[0].v_uint32)) {
            printf("Failed to advertise name \"%s\". org.alljoyn.Bus.Advertise returned %d\n",
                   SERVICE_NAME,
                   args[0].v_uint32);
        } else {
            printf("Advertising the well-known name: %s\n", SERVICE_NAME);
        }
    }


    void Cat(const InterfaceDescription::Member* member, Message& msg)
    {
        /* Concatenate the two input strings and reply with the result. */
        qcc::String inStr1 = msg->GetArg(0)->v_string.str;
        qcc::String inStr2 = msg->GetArg(1)->v_string.str;
        qcc::String outStr = inStr1 + inStr2;

        MsgArg outArg("s", outStr.c_str());
        QStatus status = MethodReply(msg, &outArg, 1);
        if (ER_OK != status) {
            printf("Ping: Error sending reply\n");
        }
    }
};

/** Main entry point */
int main(int argc, char** argv, char** envArg)
{
    QStatus status = ER_OK;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

#ifdef _WIN32
    qcc::String connectArgs = "tcp:addr=127.0.0.1,port=9956";
#else
    qcc::String connectArgs = "unix:abstract=bluebus";
#endif

    /* Create message bus */
    g_msgBus = new BusAttachment("myApp", true);


    /* Add org.alljoyn.Bus.method_sample interface */
    InterfaceDescription* testIntf = NULL;
    status = g_msgBus->CreateInterface(SERVICE_NAME, testIntf);
    if (status == ER_OK) {
        printf("Interface Created.\n");
        testIntf->AddMethod("cat", "ss",  "s", "inStr1,inStr2,outStr", 0);
        testIntf->Activate();
    } else {
        printf("Failed to create interface 'org.alljoyn.Bus.method_sample'\n");
    }

    /* Start the msg bus */
    status = g_msgBus->Start();
    if (ER_OK == status) {
        printf("BusAttachement started.\n");
        /* Register  local objects and connect to the daemon */
        BasicSampleObject testObj(*g_msgBus, SERVICE_PATH);
        g_msgBus->RegisterBusObject(testObj);

        /* Create the client-side endpoint */
        status = g_msgBus->Connect(connectArgs.c_str());
        if (ER_OK != status) {
            printf("Failed to connect to \"%s\"\n", connectArgs.c_str());
            exit(1);
        } else {
            printf("Connected to '%s'\n", connectArgs.c_str());
        }

        if (ER_OK == status) {
            while (g_interrupt == false) {
#ifdef _WIN32
                Sleep(100);
#else
                usleep(100 * 1000);
#endif
            }
        }
    } else {
        printf("BusAttachment::Start failed\n");
    }

    /* Clean up msg bus */
    if (g_msgBus) {
        BusAttachment* deleteMe = g_msgBus;
        g_msgBus = NULL;
        delete deleteMe;
    }

    return (int) status;
}
