/*
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
 */
//----------------------------------------------------------------------------------------------

#include "chatclasses.h"
//----------------------------------------------------------------------------------------------
ChatConnection::ChatConnection(FPPrintCallBack output, FPJoinedCallBack joinNotifier)
{
    busAttachment = NULL;
    chatObject = NULL;
    busListener = NULL;
    advertisedName = qcc::String();
    joinName = qcc::String();
    sessionId = 0;
    joinComplete = false;
    ManagedOutput = output;
    JoinNotifier = joinNotifier;
}

bool ChatConnection::invariants()
{
    if (NULL == JoinNotifier) {
        NotifyUser(MSG_ERROR, "Join Notifier not set");
        return false;
    }
    if (NULL == ManagedOutput) {
        NotifyUser(MSG_ERROR, "Managed Output  not set");
        return false;
    }
    if (advertisedName.empty() && joinName.empty()) {
        NotifyUser(MSG_ERROR, "Neither advertised or joinName set");
        return false;
    }
    return true;
}

void ChatConnection::createMessageBus()
{
    QStatus status = ER_OK;
    NotifyUser(MSG_STATUS, "Create message bus.");
    ajn::BusAttachment* bus = new BusAttachment("chat", true);
    this->busAttachment = bus;
    this->busListener = new MyBusListener();
    this->busListener->SetListenCallback(JoinNotifier);
    this->busListener->SetConnection(this);
    /* Create org.alljoyn.bus.samples.chat interface */
    InterfaceDescription* chatIntf;
    status = bus->CreateInterface(CHAT_SERVICE_INTERFACE_NAME, chatIntf);
    if (ER_OK == status) {
        chatIntf->AddSignal("Chat", "s",  "str", 0);
        chatIntf->Activate();
    } else {
        NotifyUser(MSG_ERROR, "Failed to create interface \"%s\" (%s)\n", CHAT_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
    }
    /* Create and register the bus object that will be used to send and receive signals */
    ChatObject* chatObject = new ChatObject(*bus, CHAT_SERVICE_OBJECT_PATH);
    this->chatObject = chatObject;
    this->busAttachment->RegisterBusObject(*chatObject);
    chatObject->SetConnection(this);
}

void ChatConnection::Connect()
{
    QStatus status = ER_OK;
    assert(invariants());
    createMessageBus();
    NotifyUser(MSG_STATUS, "Start the message bus.");
    /* Start the msg bus */
    if (ER_OK == status) {
        status = this->busAttachment->Start();
        if (ER_OK != status) {
            NotifyUser(MSG_ERROR, "BusAttachment::Start failed (%s)\n", QCC_StatusText(status));
        }
    }
    /* Register a bus listener */
    if (ER_OK == status) {
        // make sure the callback has been set
        this->busAttachment->RegisterBusListener(*(this->busListener));
    }
    NotifyUser(MSG_STATUS, "Registered BusListener");

    /* Connect to the local daemon */
    NotifyUser(MSG_STATUS, "Connect to the local daemon.");
    if (ER_OK == status) {
        status = this->busAttachment->Connect();
    }
    if (ER_OK != status) {
        NotifyUser(MSG_ERROR, "BusAttachment::Connect(%s) failed (%s)\n", this->busAttachment->GetConnectSpec().c_str(), QCC_StatusText(status));
    }
    if (!this->advertisedName.empty()) {
        NotifyUser(MSG_STATUS, "Request name");
        status = this->busAttachment->RequestName(this->advertisedName.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
        if (ER_OK != status) {
            NotifyUser(MSG_ERROR, "RequestName(%s) failed (status=%s)\n", this->advertisedName.c_str(), QCC_StatusText(status));
            status = (status == ER_OK) ? ER_FAIL : status;
        }
        /* Bind the session port*/
        NotifyUser(MSG_STATUS, "Bind session port.");
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        if (ER_OK == status) {
            SessionPort sp = CHAT_PORT;
            status = this->busAttachment->BindSessionPort(sp, opts, *(this->busListener));
            if (ER_OK != status) {
                NotifyUser(MSG_ERROR, "BindSessionPort failed (%s)\n", QCC_StatusText(status));
            }
        }

        /* Advertise same well-known name */
        if (ER_OK == status) {
            status = this->busAttachment->AdvertiseName(this->advertisedName.c_str(), opts.transports);
            if (status != ER_OK) {
                NotifyUser(MSG_ERROR, "Failed to advertise name %s (%s)\n", this->advertisedName.c_str(), QCC_StatusText(status));
            }
        }
    } else {
        /* Discover name */
        status = this->busAttachment->FindAdvertisedName(this->joinName.c_str());
        if (status != ER_OK) {
            NotifyUser(MSG_ERROR, "org.alljoyn.Bus.FindAdvertisedName failed (%s)\n", QCC_StatusText(status));
        }
        NotifyUser(MSG_STATUS, "Found Advertised Name \n");
    }
    NotifyUser(MSG_STATUS, "Ready...");
}


//----------------------------------------------------------------------------------------------
/* Bus object */
ChatObject::ChatObject(BusAttachment& bus, const char* path) : BusObject(bus, path), chatSignalMember(NULL)
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
        NotifyUser(MSG_ERROR, "Failed to register signal handler for ChatObject::Chat (%s)\n", QCC_StatusText(status));
    }
}

void ChatObject::SetConnection(ChatConnection* connect)
{
    connection = connect;
}

/** Send a Chat signal */
QStatus ChatObject::SendChatSignal(const char* msg)
{
    MsgArg chatArg("s", msg);
    uint8_t flags = 0;
    if (0 == connection->sessionId) {
        NotifyUser(MSG_ERROR, "Sending Chat signal without a session id\n");
        return ER_ALLJOYN_JOINSESSION_REPLY_NO_SESSION;
    }
    return Signal(NULL, connection->sessionId, *chatSignalMember, &chatArg, 1, 0, flags);
}

/** Receive a signal from another Chat client */
void ChatObject::ChatSignalHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
{
    NotifyUser(MSG_REMOTE, "%s: %s\n", msg->GetSender(), msg->GetArg(0)->v_string.str);
}

//----------------------------------------------------------------------------------------------------------------
void MyBusListener::FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
{
    if (!connection->joinName.empty()) {
        const char* convName = name + strlen(NAME_PREFIX);
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        QStatus status = connection->busAttachment->JoinSession(name, CHAT_PORT, this, connection->sessionId, opts);
        if (ER_OK == status) {
            NotifyUser(MSG_STATUS, "Joined conversation \"%s\"\n", convName);
        } else {
            NotifyUser(MSG_ERROR, "JoinSession failed (status=%s)\n", QCC_StatusText(status));
        }
        // NotifyUser(MSG_STATUS,"Discovering chat conversation: \"%s\"\n", name);
    }
    connection->joinComplete = true;
}

void MyBusListener::SetConnection(ChatConnection* connect)
{
    connection = connect;
}

void MyBusListener::NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
{
    NotifyUser(MSG_STATUS, "NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n", busName, previousOwner ? previousOwner : "<none>",
               newOwner ? newOwner : "<none>");
}

bool MyBusListener::AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
{
    if (sessionPort != CHAT_PORT) {
        NotifyUser(MSG_ERROR, "Rejecting join attempt on non-chat session port %d\n", sessionPort);
        return false;
    }

    NotifyUser(MSG_STATUS, "Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
               joiner, opts.proximity, opts.traffic, opts.transports);
    return true;
}

void MyBusListener::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    connection->sessionId = id;
    NotifyUser(MSG_STATUS, "SessionJoined with %s (id=%d)\n", joiner, id);
    int n = (int)id;
    (JoinedEvent)("joined", n);
}

void MyBusListener::SetListenCallback(FPJoinedCallBack callback)
{
    JoinedEvent = callback;
}

