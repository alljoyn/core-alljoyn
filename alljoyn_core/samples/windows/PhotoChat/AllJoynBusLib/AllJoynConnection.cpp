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

#include "AllJoynConnection.h"

//----------------------------------------------------------------------------------------
// AllJoynConnection
//----------------------------------------------------------------------------------------
AllJoynConnection::AllJoynConnection(FPPrintCallback output, FPJoinedCallback joinNotifier) // , FPQueryCallback queryCallback)
{
    m_fConnected = false;
    busAttachment = NULL;
    busListener = NULL;
    advertisedName = qcc::String();
    joinName = qcc::String();
    sessionId = 0;
    joinComplete = false;
    ManagedOutput = output;
    JoinNotifier = joinNotifier;
    //chatObject = NULL;
    xferObject = NULL; //    QueryCallback = queryCallback;

    for (int i = 0; i < 16; i++)
        proxies[i] = NULL;
    nProxies = 0;
}

int AllJoynConnection::CreateProxy(const char* ifPath, const char* objPath, const char* name)
{
    int ret = -1;
    if (16 < nProxies + 1) {
        return ret;
    }
    qcc::String path = ifPath;
    path += '.';
    path += name;
    NotifyUser(MSG_SYSTEM, "CREATE PROXY = %s %s", name, path.c_str(), ifPath);
    ProxyBusObject* prox = new ProxyBusObject(*busAttachment, name, XFER_SERVICE_OBJECT_PATH, sessionId);
    const InterfaceDescription* xferIntf = busAttachment->GetInterface(XFER_SERVICE_INTERFACE_NAME);
    assert(xferIntf);
    QStatus stat = prox->AddInterface(*xferIntf);
    if (ER_OK == stat) {
        ret = nProxies;
        proxies[nProxies++] = prox;
    }
    return ret;
}

void AllJoynConnection::ReleaseProxy(int index)
{
    if (NULL == proxies[index]) {
        NotifyUser(MSG_ERROR, "INVALID PROXY INDEX = %d", index);
    } else {
        delete proxies[index];
        proxies[index] = NULL;
    }
}

// FACTOR UP
ProxyBusObject* AllJoynConnection::FetchProxy(int index)
{
    if (NULL == proxies[index]) {
        NotifyUser(MSG_ERROR, "INVALID PROXY = %d", index);
        return NULL;
    }
    return proxies[index];
}

bool AllJoynConnection::IsConnected()
{
    return m_fConnected;
}

void AllJoynConnection::Connect(char* tag, bool asAdvertiser)
{
    status = ER_OK;
    if (asAdvertiser) {
        advertisedName = NAME_PREFIX;
        advertisedName += "xfer";
        joinName = "";
        NotifyUser(MSG_STATUS, "%s is advertiser \n", advertisedName.c_str());
    } else {
        joinName = NAME_PREFIX;
        joinName += "xfer";
        advertisedName = "";
        NotifyUser(MSG_STATUS, "%s is joiner\n", joinName.c_str());
    }
    assert(invariants());
    myTag = tag;
    createMessageBus();
    startMessageBus();
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    bindSessionPort(opts);
    if (!this->advertisedName.empty()) {
        NotifyUser(MSG_STATUS, "Request name");
        status = this->busAttachment->RequestName(this->advertisedName.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
        if (ER_OK != status) {
            NotifyUser(MSG_ERROR, "RequestName(%s) failed (status=%s)\n", this->advertisedName.c_str(), QCC_StatusText(status));
            status = (status == ER_OK) ? ER_FAIL : status;
        }
        /* Advertise same well-known name */
        if (ER_OK == status) {
            status = this->busAttachment->AdvertiseName(this->advertisedName.c_str(), opts.transports);
            if (status != ER_OK) {
                NotifyUser(MSG_ERROR, "Failed to advertise name %s (%s)\n", this->advertisedName.c_str(), QCC_StatusText(status));
            }
        }
    } else {
        status = this->busAttachment->FindAdvertisedName(this->joinName.c_str());
        if (status != ER_OK) {
            NotifyUser(MSG_ERROR, "org.alljoyn.Bus.FindAdvertisedName failed (%s)\n", QCC_StatusText(status));
        }
        NotifyUser(MSG_STATUS, "Found Advertised Name \n");
    }

    createBusObjects(tag);
    m_fConnected = (ER_OK == status);
    NotifyUser(MSG_STATUS, "Ready %s ...", tag);
}

bool AllJoynConnection::invariants()
{
    if (NULL == ManagedOutput) {
        MessageBox(NULL, L"Managed Output not set", L"Alljoyn", MB_OK);
        return false;
    }
    if (NULL == JoinNotifier) {
        NotifyUser(MSG_ERROR, "Join Notifier not set");
        return false;
    }
    if (advertisedName.empty() && joinName.empty()) {
        NotifyUser(MSG_ERROR, "Neither advertised or joinName set");
        return false;
    }
    return true;
}

XferObject* AllJoynConnection::GetXferObject()
{
    return xferObject;
}

ChatObject* AllJoynConnection::GetChatObject()
{
    return chatObject;
}

void AllJoynConnection::createMessageBus()
{
    status = ER_OK;
    NotifyUser(MSG_STATUS, "Create message bus.");
    ajn::BusAttachment* bus = new BusAttachment("chat", true);
    this->busAttachment = bus;
    NotifyUser(MSG_STATUS, "Create listener.");
    this->busListener = new AllJoynBusListener();
    this->busListener->SetListenCallback(JoinNotifier);
    this->busListener->SetConnection(this);
}

void AllJoynConnection::startMessageBus()
{
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

    /* Get env vars */
    const char* connectSpec = getenv("BUS_ADDRESS");
    if (connectSpec == NULL) {
        connectSpec = "tcp:addr=127.0.0.1,port=9956";
        NotifyUser(MSG_STATUS, "Connect spec defaulted to %s", connectSpec);
    } else {
        NotifyUser(MSG_STATUS, "Got environment BUS_ADDRESS %s", connectSpec);
    }

    /* Connect to the local daemon */
    NotifyUser(MSG_STATUS, "Connect to the local daemon.");
    if (ER_OK == status) {
        status = this->busAttachment->Connect(connectSpec);
    }
    if (ER_OK != status) {
        NotifyUser(MSG_ERROR, "BusAttachment::Connect(%s) failed (%s)\n", connectSpec, QCC_StatusText(status));
    }
}

void AllJoynConnection::bindSessionPort(SessionOpts& opts)
{
    /* Bind the session port*/
    NotifyUser(MSG_STATUS, "Bind session port.");
    if (ER_OK == status) {
        SessionPort sp = PHOTOCHAT_PORT;
        status = this->busAttachment->BindSessionPort(sp, opts, *(this->busListener));
        if (ER_OK != status) {
            NotifyUser(MSG_ERROR, "BindSessionPort failed (%s)\n", QCC_StatusText(status));
        }
    }
}

void AllJoynConnection::createBusObjects(const char* localTag)
{
    xferObject = new XferObject(this, localTag);
    chatObject = new ChatObject(this, localTag);
    xferObject->CreateInterfaces();
    xferObject->RegisterInterfaces();
    chatObject->CreateInterfaces();
    chatObject->RegisterInterfaces();
    busAttachment->RegisterBusObject(*xferObject);
    busAttachment->RegisterBusObject(*chatObject);
}

AllJoynConnection::~AllJoynConnection()
{
    if (NULL != chatObject) {
        delete chatObject;
    }
    if (NULL != xferObject) {
        delete xferObject;
    }
}

//----------------------------------------------------------------------------------------
// AllJoynBusListener (BusListener, SessionPortListener, SessionListener )
//----------------------------------------------------------------------------------------
void AllJoynBusListener::FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
{
    if (!connection->joinName.empty() && 0 == connection->sessionId) {
        const char* convName = name + strlen(NAME_PREFIX);
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        QStatus status = connection->busAttachment->JoinSession(name, PHOTOCHAT_PORT, this, connection->sessionId, opts);
        if (ER_OK == status) {
            NotifyUser(MSG_STATUS, "Joined conversation \"%s\"\n", convName);
        } else {
            NotifyUser(MSG_ERROR, "JoinSession failed (status=%s)\n", QCC_StatusText(status));
        }
    }
    connection->joinComplete = true;
}

void AllJoynBusListener::SetConnection(AllJoynConnection* connect)
{
    connection = connect;
}

void AllJoynBusListener::NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
{
    NotifyUser(MSG_STATUS, "NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n", busName, previousOwner ? previousOwner : "<none>",
               newOwner ? newOwner : "<none>");
}

bool AllJoynBusListener::AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
{
    if (sessionPort != PHOTOCHAT_PORT) {
        NotifyUser(MSG_ERROR, "Rejecting join attempt on non-chat session port %d\n", sessionPort);
        return false;
    }
    NotifyUser(MSG_STATUS, "Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
               joiner, opts.proximity, opts.traffic, opts.transports);
    return true;
}

void AllJoynBusListener::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    connection->sessionId = id;

    NotifyUser(MSG_STATUS, "SessionJoined with %s (id=%d)\n", joiner, id);
    int n = (int)id;
    JoinedEvent(joiner, n);
}

void AllJoynBusListener::SetListenCallback(FPJoinedCallback callback)
{
    JoinedEvent = callback;
}


//---------------------------------------------------------------------------
// AllJoynBusObject
AllJoynBusObject::AllJoynBusObject(AllJoynConnection* connection, const char* path)
    : BusObject(*connection->busAttachment, path)
{
    NotifyUser(MSG_SYSTEM, "create %s", path);
    ajConnection = connection;
}

//----------------------------------------------------------------------------------------
// ChatObject (BusObject)
//----------------------------------------------------------------------------------------
ChatObject::ChatObject(AllJoynConnection* connection, const char* tag)
    : AllJoynBusObject(connection, CHAT_SERVICE_OBJECT_PATH)
{
    localName = tag;
}

bool ChatObject::CreateInterfaces()
{
    const char* ifName = CHAT_SERVICE_INTERFACE_NAME;
    InterfaceDescription* chatIntf = NULL;

    status = ajConnection->busAttachment->CreateInterface(ifName, chatIntf);
    assert(chatIntf);
    if (ER_OK == status) {
        chatIntf->AddSignal("Chat", "s",  "str", 0);
        chatIntf->Activate();
    } else {
        NotifyUser(MSG_ERROR, "Failed to create interface \"%s\" (%s)\n", CHAT_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
        return false;
    }
    NotifyUser(MSG_SYSTEM, "Create interface \"%s\" (%s)\n", CHAT_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
    return true;
}

bool ChatObject::RegisterInterfaces()
{
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
        return false;
    }
    return true;
}

/** Send a Chat signal */
QStatus ChatObject::SendChatSignal(const char* msg)
{
    MsgArg chatArg("s", msg);
    uint8_t flags = 0;
    if (0 == ajConnection->sessionId) {
        NotifyUser(MSG_ERROR, "Sending Chat signal without a session id\n");
        return ER_ALLJOYN_JOINSESSION_REPLY_NO_SESSION;
    }
    return Signal(NULL, ajConnection->sessionId, *chatSignalMember, &chatArg, 1, 0, flags);
}

/** Receive a signal from another Chat client */
void ChatObject::ChatSignalHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
{
    NotifyUser(MSG_REMOTE, "%s: %s\n", msg->GetSender(), msg->GetArg(0)->v_string.str);
}

//---------------------------------------------------------------------------
// XferObject (AllJoynBusObject)
XferObject::XferObject(AllJoynConnection* connection, const char* tag)
    : AllJoynBusObject(connection, XFER_SERVICE_OBJECT_PATH)
{
    localName = tag;
    errorCode = 0;
    state = 0;
    commonSegSize = 0;
}

bool XferObject::CreateInterfaces()
{
    const char* ifName = XFER_SERVICE_INTERFACE_NAME;
    InterfaceDescription* xferIntf = NULL;
    status = ajConnection->busAttachment->CreateInterface(ifName, xferIntf);
    assert(xferIntf);
    if (ER_OK == status) {
        xferIntf->AddMethod("query", "si",  "i", "filename, filesize, acceptsize ", 0);
        xferIntf->AddMethod("initiate", "ii",  "i", "segmentSize, nSegs, acceptsize ", 0);
        xferIntf->AddMethod("receive", "ayii",  "i", "segment, serialNum. segSize , success ", 0);
        xferIntf->AddMethod("status", "i",  "i", "unused , status ", 0);
        xferIntf->AddMethod("close", "i",  "i", "unused , success ", 0);
        xferIntf->AddMethod("error", "i",  "i", "unused , error ", 0);
        xferIntf->Activate();
    } else {
        NotifyUser(MSG_ERROR, "Failed to create interface \"%s\" (%s)\n", XFER_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
        return false;
    }
    NotifyUser(MSG_SYSTEM, "Create interface \"%s\" (%s)\n", XFER_SERVICE_INTERFACE_NAME, QCC_StatusText(status));
    return true;
}

bool XferObject::RegisterInterfaces()
{
    const InterfaceDescription* serviceIntf = ajConnection->busAttachment->GetInterface(XFER_SERVICE_INTERFACE_NAME);
    assert(serviceIntf);
    AddInterface(*serviceIntf);
    /** Register the method handlers with the object */
    MethodEntry methodEntries[6];
    methodEntries[0].member = serviceIntf->GetMember("query");
    methodEntries[0].handler = (MessageReceiver::MethodHandler)(&XferObject::Query);
    methodEntries[1].member = serviceIntf->GetMember("initiate");
    methodEntries[1].handler = (MessageReceiver::MethodHandler)(&XferObject::InitiateXferIn);
    methodEntries[2].member = serviceIntf->GetMember("receive");
    methodEntries[2].handler = (MessageReceiver::MethodHandler)(&XferObject::ReceiveSegment);
    methodEntries[3].member = serviceIntf->GetMember("status");
    methodEntries[3].handler = (MessageReceiver::MethodHandler)(&XferObject::TransferStatus);
    methodEntries[4].member = serviceIntf->GetMember("close");
    methodEntries[4].handler = (MessageReceiver::MethodHandler)(&XferObject::EndXfer);
    methodEntries[5].member = serviceIntf->GetMember("error");
    methodEntries[5].handler = (MessageReceiver::MethodHandler)(&XferObject::ErrorCode);
    status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    if (ER_OK != status) {
        NotifyUser(MSG_ERROR, "Failed to register method handlers for XferObject");
        return false;
    }
    NotifyUser(MSG_SYSTEM, "register method handlers for XferObject");
    return true;
}

void XferObject::SetQueryCallback(FPQueryCallback cb)
{
    queryCallback = cb;
}


void XferObject::SetXferCallback(FPXferCallback cb)
{
    xferCallback = cb;
}

const char* XferObject::GetSaveAsFilename()
{
    return saveAsFilename.c_str();
}

void XferObject::SetSaveAsFilename(const char* name)
{
    state = 1;
    saveAsFilename = name;
}

// Distributed Methods
void XferObject::Query(const InterfaceDescription::Member* member, Message& msg)
{
    qcc::String filename = msg->GetArg(0)->v_string.str;
    NotifyUser(MSG_STATUS, "QUERY RECEIVED (%s)", filename.c_str());
    // call into managed code
    int accept = msg->GetArg(1)->v_uint32;
    queryCallback(filename.c_str(), accept);
    MsgArg outArg("i", accept);
    QStatus status = MethodReply(msg, &outArg, 1);
    if (ER_OK != status) {
        NotifyUser(MSG_ERROR, "XferObjectQuery : Error sending reply");
    }
}

void XferObject::InitiateXferIn(const InterfaceDescription::Member* member, Message& msg)
{
    int segmentSize = msg->GetArg(0)->v_uint32;
    int nSegs = msg->GetArg(1)->v_uint32;
    int accept = 0;
    xferCallback("Initiate", accept);
    const char* filename = GetSaveAsFilename();
    commonSegSize = segmentSize;
    if (openFile(filename)) {
        accept = 1;
    }
    MsgArg outArg("i", accept);
    QStatus status = MethodReply(msg, &outArg, 1);
    if (ER_OK != status) {
        NotifyUser(MSG_ERROR, "XferObjectQuery : Error sending reply");
    }
}

void XferObject::ReceiveSegment(const InterfaceDescription::Member* member, Message& msg)
{
    const char* ccptr = msg->GetArg(0)->v_string.str;
    int serialNum = msg->GetArg(1)->v_uint32;
    int segSize = msg->GetArg(2)->v_uint32;
    int accept = 0;
    xferCallback("Receive", serialNum);

    uint8_t* bytes = (uint8_t*)ccptr;
    if (writeSegment(serialNum, bytes, segSize)) {
        accept = 1;
    }
    MsgArg outArg("i", accept);
    QStatus status = MethodReply(msg, &outArg, 1);
    if (ER_OK != status) {
        NotifyUser(MSG_ERROR, "XferObjectReceiveSegment : Error sending reply");
    }
}

void XferObject::TransferStatus(const InterfaceDescription::Member* member, Message& msg)
{
    MsgArg outArg("i", state);
    QStatus status = MethodReply(msg, &outArg, 1);
    if (ER_OK != status) {
        NotifyUser(MSG_ERROR, "XferObject::TranserStatus : Error sending reply");
    }
}

void XferObject::ErrorCode(const InterfaceDescription::Member* member, Message& msg)
{
    MsgArg outArg("i", errorCode);
    QStatus status = MethodReply(msg, &outArg, 1);
    if (ER_OK != status) {
        NotifyUser(MSG_ERROR, "XferObject::ErrorCode : Error sending reply");
    }
}

void XferObject::EndXfer(const InterfaceDescription::Member* member, Message& msg)
{
    closeFile();
    if (state != -1) {
        state = 0;
    }
    saveAsFilename = "";
    NotifyUser(MSG_SYSTEM, "Closed %s", saveAsFilename.c_str());
}

bool XferObject::openFile(const char* filename)
{
    hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        NotifyUser(MSG_SYSTEM, "XferObjectQuery : Error opening file %s %d %x", filename, err, err);
        return false;
    }
    return true;
}

void XferObject::closeFile()
{
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
}

bool XferObject::writeSegment(int serial, const uint8_t* seg, int segSize)
{
    long lDistance = serial - 1;
    if (lDistance < 0) {
        return false;
    }
    lDistance *= commonSegSize;
    DWORD dwPtr = SetFilePointer(hFile, lDistance, NULL, FILE_BEGIN);
    if (dwPtr == INVALID_SET_FILE_POINTER) {   // Test for failure
        // Obtain the error code.
        DWORD dwError = GetLastError();
        // Deal with failure
        // . . .
    }     // End of error handler

    DWORD expected = (DWORD)segSize;
    DWORD actual = 0;
    if (!WriteFile(hFile, (LPCVOID)seg, expected, &actual, NULL)) {
        DWORD err = GetLastError();
        NotifyUser(MSG_SYSTEM, "XferObject: error writing segment %d %x", err, err);
        return false;
    }
    return (actual == expected);
}

//---------------------------------------------------------------------------
