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

#pragma once

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Log.h>
#include <qcc/String.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>

using namespace ajn;

// Use these callback types to invoke delegates in managed code.
//
typedef void (*FPPrintCallBack)(char* data, int& stringSize, int& informType);
typedef void (*FPJoinedCallBack)(char* data, int& stringSize);

// Text message types that can be communicated to managed code
enum NotifyType {
    MSG_STATUS = 0,
    MSG_REMOTE = 1,
    MSG_ERROR = 2,
    MSG_SYSTEM = 3
};

// helper to format strings passed to managed code

void NotifyUser(NotifyType informType, const char* format, ...);

//--------------------------------------------------------------------------------------------------
static const char* CHAT_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.samples.chat";
static const char* NAME_PREFIX = "org.alljoyn.bus.samples.chat.";
static const char* CHAT_SERVICE_OBJECT_PATH = "/chatService";
static const SessionPort CHAT_PORT = 27;

class ChatObject;
class MyBusListener;
class ChatConnection;

//----------------------------------------------------------------------------------------------
class ChatObject : public BusObject {
  public:
    ChatObject(BusAttachment& bus, const char* path);
    QStatus SendChatSignal(const char* msg);
    void ChatSignalHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg);
    void SetConnection(ChatConnection* connect);
  private:
    const InterfaceDescription::Member* chatSignalMember;
    ChatConnection* connection;
};

//----------------------------------------------------------------------------------------------
class MyBusListener : public BusListener, public SessionPortListener, public SessionListener {
  public:
    FPJoinedCallBack JoinedEvent;
    ChatConnection* connection;

  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts);
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner);
    void SetConnection(ChatConnection* connect);
    void SetListenCallback(FPJoinedCallBack callball);
};

//----------------------------------------------------------------------------------------------
class ChatConnection {
  public:
    // properties
    qcc::String advertisedName;
    qcc::String joinName;
    SessionId sessionId;
    bool joinComplete;
    //
    ajn::BusAttachment* busAttachment;
    ChatObject* chatObject;
    MyBusListener* busListener;

  public:
    ChatConnection(FPPrintCallBack output, FPJoinedCallBack joinNotifier);
    ~ChatConnection();
    void Connect();

  private:
    FPPrintCallBack ManagedOutput;
    FPJoinedCallBack JoinNotifier;
    bool invariants();
    void createMessageBus();
};

