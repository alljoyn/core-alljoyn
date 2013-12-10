/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <set>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Log.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>
// #include <qcc/Environ.h>
#include <qcc/String.h>
#include <assert.h>
#include <stdio.h>


using namespace ajn;

/* constants */
static const char* CHAT_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.samples.chat";
static const char* NAME_PREFIX = "org.alljoyn.bus.samples.chat";
static const char* CHAT_SERVICE_OBJECT_PATH = "/chatService";
static const SessionPort CHAT_PORT = 10;  /**< Well-known session port for autochat */

/* forward declaration */
class ChatObject;
class MyBusListener;

/* static data */
static ajn::BusAttachment* s_bus = NULL;
static ChatObject* s_chatObj = NULL;
static MyBusListener*  s_busListener = NULL;
static qcc::String s_advertisedname;

static std::set<qcc::String> connections;

/* Bus object */
class ChatObject : public BusObject {
  public:

    ChatObject(BusAttachment& bus, const char* path) : BusObject(path), chatSignalMember(NULL)
    {
        QStatus status;

        /* Add the chat interface to this object */
        const InterfaceDescription* chatIntf = bus.GetInterface(CHAT_SERVICE_INTERFACE_NAME);
        assert(chatIntf);
        AddInterface(*chatIntf);

        /* Store the Chat signal member away so it can be quickly looked up when signals are sent */
        chatSignalMember = chatIntf->GetMember("Chat");
        assert(chatSignalMember);

        /* Register signal handler */
        status =  bus.RegisterSignalHandler(this,
                                            static_cast<MessageReceiver::SignalHandler>(&ChatObject::ChatSignalHandler),
                                            chatSignalMember,
                                            NULL);

        if (ER_OK != status) {
            printf("Failed to register signal handler for ChatObject::Chat (%s)\n", QCC_StatusText(status));
        }
    }

    /** Send a Chat signal */
    QStatus SendChatSignal(const char* msg) {
        MsgArg chatArg("s", msg);

        uint8_t flags = 0;
        flags |= ALLJOYN_FLAG_GLOBAL_BROADCAST;
        return Signal(NULL, 0, *chatSignalMember, &chatArg, 1, 0, flags);
    }

    /** Receive a signal from another Chat client */
    void ChatSignalHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
    {
        printf("%s: %s", msg->GetSender(), msg->GetArg(0)->v_string.str);
    }

    void NameAcquiredCB(Message& msg, void* context)
    {
        /* Check name acquired result */
        size_t numArgs;
        const MsgArg* args;
        msg->GetArgs(numArgs, args);

        if (args[0].v_uint32 == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
            /* Begin Advertising the well known name to remote busses */
            const ProxyBusObject& alljoynObj = s_bus->GetAllJoynProxyObj();
            MsgArg arg("s", s_advertisedname.c_str());
            QStatus status = alljoynObj.MethodCallAsync(org::alljoyn::Bus::InterfaceName,
                                                        "AdvertiseName",
                                                        this,
                                                        static_cast<MessageReceiver::ReplyHandler>(&ChatObject::AdvertiseRequestCB),
                                                        &arg,
                                                        1);
            if (ER_OK != status) {
                printf("Sending org.alljoyn.bus.Advertise failed\n");
            } else {
                printf("Advertising name %s\n", s_advertisedname.c_str());
            }
        } else {
            printf("Failed to obtain name \"%s\". RequestName returned %d\n", s_advertisedname.c_str(), args[0].v_uint32);
        }



    }


    void AdvertiseRequestCB(Message& msg, void* context)
    {
        /* Make sure request was processed */
        size_t numArgs;
        const MsgArg* args;
        msg->GetArgs(numArgs, args);

        if ((MESSAGE_METHOD_RET != msg->GetType()) || (ALLJOYN_ADVERTISENAME_REPLY_SUCCESS != args[0].v_uint32)) {
            printf("Failed to advertise name \"%s\". org.alljoyn.bus.Advertise returned %d\n", s_advertisedname.c_str(), args[0].v_uint32);
        }

    }


    void ObjectRegistered(void) {

        BusObject::ObjectRegistered();

        /* Request a well-known name */
        /* Note that you cannot make a blocking method call here */
        const ProxyBusObject& dbusObj = s_bus->GetDBusProxyObj();
        MsgArg args[2];
        args[0].Set("s", s_advertisedname.c_str());
        args[1].Set("u", 6);
        QStatus status = dbusObj.MethodCallAsync(org::freedesktop::DBus::InterfaceName,
                                                 "RequestName",
                                                 s_chatObj,
                                                 static_cast<MessageReceiver::ReplyHandler>(&ChatObject::NameAcquiredCB),
                                                 args,
                                                 2);
        if (ER_OK != status) {
            printf("Failed to request name %s \n", s_advertisedname.c_str());
        } else {
            printf("Requested name %s\n", s_advertisedname.c_str());
        }
    }

    /** Release the well-known name if it was acquired */
    void ReleaseName() {
        if (s_bus) {
            uint32_t disposition = 0;

            const ProxyBusObject& dbusObj = s_bus->GetDBusProxyObj();
            Message reply(*s_bus);
            MsgArg arg;
            arg.Set("s", s_advertisedname.c_str());
            QStatus status = dbusObj.MethodCall(org::freedesktop::DBus::InterfaceName,
                                                "ReleaseName",
                                                &arg,
                                                1,
                                                reply,
                                                5000);
            if (ER_OK == status) {
                disposition = reply->GetArg(0)->v_uint32;
            }
            if ((ER_OK != status) || (disposition != DBUS_RELEASE_NAME_REPLY_RELEASED)) {
                printf("Failed to release name %s (%s, disposition=%d)\n", s_advertisedname.c_str(), QCC_StatusText(status), disposition);
            }
        }
    }

  private:
    const InterfaceDescription::Member* chatSignalMember;
};

class MyBusListener : public BusListener {
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        printf("FoundName signal received for %s\n", name);

        /* We found a remote bus that is advertising autochat's well-known name so connect to it */
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, transport);
        QStatus status = s_bus->JoinSession(name, CHAT_PORT, NULL, sessionId, opts);
        if (ER_OK == status) {
            printf("Joined session %s with id %d\n", name, sessionId);
            connections.insert(name);
        } else {
            printf("JoinSession failed (status=%s)\n", QCC_StatusText(status));
        }

    }
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (!newOwner && connections.count(busName)) {
            connections.erase(busName);
            printf("Chatter %s has left the building\n", busName);
            return;
        }
        if (newOwner && (s_advertisedname.compare(busName) != 0)) {
            qcc::String name = busName;
            if (name.find(NAME_PREFIX) == 0) {
                printf("Chatter %s has entered the building\n", busName);
                connections.insert(name);
            }
        }
    }
    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
};


#ifdef __cplusplus
extern "C" {
#endif

static void usage()
{
    printf("Usage: chat [-h] [-n <name>] [-d <daemon_bus_address>\n");
    exit(1);
}

int main(int argc, char** argv)
{
    int n = 0;
    QStatus status = ER_OK;
    qcc::String daemonAddr = "unix:abstract=alljoyn";
    qcc::String myName;
// #ifdef _WIN32
//    daemonAddr = env->Find("BUS_ADDRESS", "tcp:addr=127.0.0.1,port=9956");
// #else
//    daemonAddr = env->Find("BUS_ADDRESS", "unix:abstract=alljoyn");
// #endif

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == ::strcmp("-n", argv[i])) {
            if (++i < argc) {
                myName = argv[i];
                s_advertisedname = NAME_PREFIX;
                s_advertisedname += ".";
                s_advertisedname += myName;
                continue;
            } else {
                printf("Missing parameter for \"-n\" option\n");
                usage();
            }
        } else if (0 == ::strcmp("-d", argv[i])) {
            if (++i < argc) {
                daemonAddr = static_cast<qcc::String>(argv[i]);
                continue;
            } else {
                printf("Missing parameter for \"-d\" option\n");
                usage();
            }
        } else if (0 == ::strcmp("-h", argv[i])) {
            usage();
            continue;
        } else {
            printf("Unknown argument \"%s\"\n", argv[i]);
            usage();
        }
    }

    /* Set AllJoyn logging */
    // QCC_SetLogLevels("ALLJOYN=7;ALL=1");
    // QCC_UseOSLogging(true);

    /* Create message bus */
    BusAttachment* bus = new BusAttachment("chat", true);
    s_bus = bus;

    /* Create org.alljoyn.bus.samples.chat interface */
    InterfaceDescription* chatIntf = NULL;
    status = bus->CreateInterface(CHAT_SERVICE_INTERFACE_NAME, chatIntf);
    if (ER_OK == status) {
        chatIntf->AddSignal("Chat", "s",  "str", 0);
        chatIntf->Activate();
    } else {
        printf("Failed to create interface \"%s\" (%s)\n", CHAT_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
    }

    /* Create and register the bus object that will be used to send out signals */
    ChatObject chatObj(*bus, CHAT_SERVICE_OBJECT_PATH);
    bus->RegisterBusObject(chatObj);
    s_chatObj = &chatObj;

    /* Start the msg bus */
    if (ER_OK == status) {
        status = bus->Start();
        if (ER_OK != status) {
            printf("BusAttachment::Start failed (%s)\n", QCC_StatusText(status));
        }
    }

    /* Register a bus listener in order to get discovery indications */
    if (ER_OK == status) {
        s_busListener = new MyBusListener();
        s_bus->RegisterBusListener(*s_busListener);
    }

    /* Add a rule to allow org.alljoyn.samples.chat.Chat signals to be routed here */
    if (ER_OK == status) {
        MsgArg arg("s", "type='signal',interface='org.alljoyn.bus.samples.chat',member='Chat'");
        Message reply(*bus);
        const ProxyBusObject& dbusObj = bus->GetDBusProxyObj();
        status = dbusObj.MethodCall(org::freedesktop::DBus::InterfaceName,
                                    "AddMatch",
                                    &arg,
                                    1,
                                    reply);
        if (status != ER_OK) {
            printf("Failed to register Match rule for 'org.alljoyn.bus.samples.chat.Chat': %s\n", QCC_StatusText(status));
        }
    }

    Message reply(*bus);
    const ProxyBusObject& alljoynObj = bus->GetAllJoynProxyObj();

    // Look for the prefix
    MsgArg serviceName("s", NAME_PREFIX);
    status = alljoynObj.MethodCall(::org::alljoyn::Bus::InterfaceName,
                                   "FindAdvertisedName",
                                   &serviceName,
                                   1,
                                   reply,
                                   5000);
    if (ER_OK == status) {
        if (reply->GetType() != MESSAGE_METHOD_RET) {
            status = ER_BUS_REPLY_IS_ERROR_MESSAGE;
        } else if (reply->GetArg(0)->v_uint32 != ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS) {
            status = ER_FAIL;
        }
    } else {
        printf("%s.FindAdvertisedName failed\n", org::alljoyn::Bus::InterfaceName);
    }

    while (true) {
        if (connections.size() > 0) {
            char abuf[128];
            snprintf(abuf, 128, "this is autochat message %d from %s\n", n++, myName.c_str());
            chatObj.SendChatSignal(abuf);
            qcc::Sleep(500 + 8 * qcc::Rand8());
        } else {
            n = 0;
            qcc::Sleep(2000);
        }
    }

    /* Unregister the ServiceObject. */
    chatObj.ReleaseName();
    bus->UnregisterBusObject(chatObj);
    if (s_chatObj) {
        s_chatObj = NULL;
    }

    if (NULL != s_busListener) {
        delete s_busListener;
        s_busListener = NULL;
    }

    /* Cleanup */
    if (bus) {
        delete bus;
        bus = NULL;
        s_bus = NULL;
    }

    return (int) status;
}

#ifdef __cplusplus
}
#endif
