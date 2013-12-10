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
#include <share.h>

using namespace ajn;
using namespace std;
using namespace qcc;

// Use these callback types to invoke delegates in managed code.
//
typedef void (*FPPrintCallback)(char* data, int& stringSize, int& informType);
typedef void (*FPJoinedCallback)(const char* data, int& stringSize);

// Text message types that can be communicated to managed code
enum NotifyType {
    MSG_ERROR = 0,
    MSG_REMOTE = 1,
    MSG_STATUS = 2,
    MSG_SYSTEM = 3
};

// helper to format strings passed to managed code

void NotifyUser(NotifyType informType, const char* format, ...);

//--------------------------------------------------------------------------------------------------
static const char* NAME_PREFIX = "org.alljoyn.bus.samples.photochat.";

static const SessionPort PHOTOCHAT_PORT = 25;

class AllJoynConnection;
class AllJoynBusListener;
class ChatObject;
class XferObject;

//----------------------------------------------------------------------------------------------
class AllJoynConnection {
  public:
    // properties
    qcc::String advertisedName;
    qcc::String joinName;
    qcc::String myTag;
    SessionId sessionId;
    bool joinComplete;
    //
    ajn::BusAttachment* busAttachment;
    AllJoynBusListener* busListener;

  public:
    AllJoynConnection(FPPrintCallback output, FPJoinedCallback joinNotifier); // , FPQueryCallback queryCallback);
    ~AllJoynConnection();
    bool IsConnected();
    void Connect(char* identity, bool asAdvertiser);
    int CreateProxy(const char* ifPath, const char* objPath, const char* name);
    void ReleaseProxy(int index);

    ProxyBusObject* FetchProxy(int index);

    XferObject* GetXferObject();
    ChatObject* GetChatObject();

  private:
    bool m_fConnected;
    QStatus status;
    FPPrintCallback ManagedOutput;
    FPJoinedCallback JoinNotifier;

    int nProxies;
    ProxyBusObject* proxies[16];
    ChatObject* chatObject;
    XferObject* xferObject;

    qcc::String saveAsFilename;

    //local helpers
    bool invariants();
    void createMessageBus();
    void startMessageBus();
    void bindSessionPort(SessionOpts& opts);
    void createBusObjects(const char* uniqueTag);
};

//----------------------------------------------------------------------------------------------
class AllJoynBusListener : public BusListener, public SessionPortListener, public SessionListener {
  public:
    FPJoinedCallback JoinedEvent;
    AllJoynConnection* connection;

  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts);
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner);
    void SetConnection(AllJoynConnection* connect);
    void SetListenCallback(FPJoinedCallback callball);
};

//----------------------------------------------------------------------------------------------
class AllJoynBusObject : public BusObject {
  public:
    AllJoynBusObject(AllJoynConnection* connection, const char* path);

    virtual bool CreateInterfaces() { return true; }
    virtual bool RegisterInterfaces() { return true; }

  protected:
    QStatus status;
    AllJoynConnection* ajConnection;
};

//----------------------------------------------------------------------------------------------
static const char* XFER_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.samples.chat.xfer";
static const char* XFER_SERVICE_OBJECT_PATH = "/xferService";

typedef void (*FPQueryCallback)(const char* data, int& accept);
typedef void (*FPXferCallback)(const char* data, int& retval);

class XferObject : public AllJoynBusObject {
  public:
    XferObject(AllJoynConnection* connection, const char* path);
    bool CreateInterfaces();
    bool RegisterInterfaces();
    void SetQueryCallback(FPQueryCallback cb);
    void SetXferCallback(FPXferCallback cb);

    const char* GetSaveAsFilename();
    void SetSaveAsFilename(const char* filename);

    void Query(const InterfaceDescription::Member* member, Message& msg);
    void InitiateXferIn(const InterfaceDescription::Member* member, Message& msg);
    void ReceiveSegment(const InterfaceDescription::Member* member, Message& msg);
    void TransferStatus(const InterfaceDescription::Member* member, Message& msg);
    void ErrorCode(const InterfaceDescription::Member* member, Message& msg);
    void EndXfer(const InterfaceDescription::Member* member, Message& msg);

  private:
    FPQueryCallback queryCallback;
    FPXferCallback xferCallback;
    const char* localName;

    HANDLE hFile;
    qcc::String saveAsFilename;
    int commonSegSize;
    int state;
    // 0 - avail, 1 - busy -1 error
    int errorCode;
    bool openFile(const char* filename);
    void closeFile();
    bool writeSegment(int serial, const uint8_t* segment, int segSize);
};
//----------------------------------------------------------------------------------------------
static const char* CHAT_SERVICE_INTERFACE_NAME = "org.alljoyn.bus.samples.chat";
static const char* CHAT_SERVICE_OBJECT_PATH = "/chatService";

//----------------------------------------------------------------------------------------------
class ChatObject : public AllJoynBusObject {
  public:
    ChatObject(AllJoynConnection* connection, const char* path);

    bool CreateInterfaces();
    bool RegisterInterfaces();

    QStatus SendChatSignal(const char* msg);
    void ChatSignalHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg);

  private:
    const InterfaceDescription::Member* chatSignalMember;
    const char* localName;
};