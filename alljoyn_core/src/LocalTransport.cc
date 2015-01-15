/**
 * @file
 * LocalTransport is a special type of Transport that is responsible
 * for all communication of all endpoints that terminate at registered
 * AllJoynObjects residing within this BusAttachment instance.
 */

/******************************************************************************
 * Copyright (c) 2009-2015, AllSeen Alliance. All rights reserved.
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

#include <list>

#include <qcc/Debug.h>
#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Thread.h>
#include <qcc/atomic.h>

#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>

#include "LocalTransport.h"
#include "Router.h"
#include "MethodTable.h"
#include "SignalTable.h"
#include "AllJoynPeerObj.h"
#include "BusUtil.h"
#include "BusInternal.h"

#define QCC_MODULE "LOCAL_TRANSPORT"

using namespace std;
using namespace qcc;

namespace ajn {


static const uint32_t LOCAL_ENDPOINT_CONCURRENCY = 4;


class _LocalEndpoint::Dispatcher : public qcc::Timer, public qcc::AlarmListener {
  public:
    Dispatcher(_LocalEndpoint* endpoint, uint32_t concurrency = LOCAL_ENDPOINT_CONCURRENCY) : Timer("lepDisp" + U32ToString(qcc::IncrementAndFetch(&dispatcherCnt)), true, concurrency, true, 10), AlarmListener(), endpoint(endpoint) { }
    QStatus DispatchMessage(Message& msg);

    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

  private:
    _LocalEndpoint* endpoint;
    static int32_t dispatcherCnt;
};

int32_t _LocalEndpoint::Dispatcher::dispatcherCnt = 0;

class _LocalEndpoint::DeferredCallbacks : public qcc::AlarmListener {
  public:
    DeferredCallbacks(_LocalEndpoint* ep) : endpoint(ep) { }

    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

  private:
    _LocalEndpoint* endpoint;
};

LocalTransport::~LocalTransport()
{
    Stop();
    Join();
}

QStatus LocalTransport::Start()
{
    isStoppedEvent.ResetEvent();
    return localEndpoint->Start();
}

QStatus LocalTransport::Stop()
{
    QStatus status = localEndpoint->Stop();
    isStoppedEvent.SetEvent();
    return status;
}

QStatus LocalTransport::Join()
{
    QStatus status = localEndpoint->Join();
    /* Pend caller until transport is stopped */
    Event::Wait(isStoppedEvent);
    return status;
}

bool LocalTransport::IsRunning()
{
    return !isStoppedEvent.IsSet();
}

class _LocalEndpoint::ReplyContext {
  public:
    ReplyContext(LocalEndpoint ep,
                 MessageReceiver* receiver,
                 MessageReceiver::ReplyHandler handler,
                 const InterfaceDescription::Member* method,
                 Message& methodCall,
                 void* context,
                 uint32_t timeout) :
        ep(ep),
        receiver(receiver),
        handler(handler),
        method(method),
        callFlags(methodCall->GetFlags()),
        serial(methodCall->msgHeader.serialNum),
        context(context)
    {
        uint32_t zero = 0;
        void* tempContext = (void*)this;
        AlarmListener* listener = ep.unwrap();
        alarm = Alarm(timeout, listener, tempContext, zero);
    }

    ~ReplyContext() {
        ep->replyTimer.RemoveAlarm(alarm, false /* don't block if alarm in progress */);
    }

    LocalEndpoint ep;                            /* The endpoint this reply context is associated with */
    MessageReceiver* receiver;                   /* The object to receive the reply */
    MessageReceiver::ReplyHandler handler;       /* The receiving object's handler function */
    const InterfaceDescription::Member* method;  /* The method that was called */
    uint8_t callFlags;                           /* Flags from the method call */
    uint32_t serial;                             /* Serial number for the method reply */
    void* context;                               /* The calling object's context */
    qcc::Alarm alarm;                            /* Alarm object for handling method call timeouts */

  private:
    ReplyContext(const ReplyContext& other);
    ReplyContext operator=(const ReplyContext& other);
};

_LocalEndpoint::_LocalEndpoint(BusAttachment& bus, uint32_t concurrency) :
    _BusEndpoint(ENDPOINT_TYPE_LOCAL),
    dispatcher(new Dispatcher(this, concurrency)),
    deferredCallbacks(new DeferredCallbacks(this)),
    running(false),
    isRegistered(false),
    bus(&bus),
    objectsLock(),
    replyMapLock(),
    replyTimer("replyTimer", true),
    dbusObj(NULL),
    alljoynObj(NULL),
    alljoynDebugObj(NULL),
    peerObj(NULL)
{
}

_LocalEndpoint::~_LocalEndpoint()
{
    QCC_DbgHLPrintf(("LocalEndpoint~LocalEndpoint"));
    /*
     * If bus is NULL the default constructor was used so this is just a placeholder endpoint
     */
    if (bus) {
        running = false;

        /*
         * Delete any stale reply contexts
         */
        replyMapLock.Lock(MUTEX_CONTEXT);
        for (map<uint32_t, ReplyContext*>::iterator iter = replyMap.begin(); iter != replyMap.end(); ++iter) {
            QCC_DbgHLPrintf(("LocalEndpoint~LocalEndpoint deleting reply handler for serial %u", iter->second->serial));
            delete iter->second;
        }
        replyMap.clear();
        replyMapLock.Unlock(MUTEX_CONTEXT);
        /*
         * Unregister all application registered bus objects
         */
        unordered_map<const char*, BusObject*, Hash, PathEq>::iterator it = localObjects.begin();
        while (it != localObjects.end()) {
            BusObject* obj = it->second;
            UnregisterBusObject(*obj);
            it = localObjects.begin();
        }

        /*
         * Shutdown the dispatcher and deferredCallbacks
         */
        if (dispatcher) {
            delete dispatcher;
            dispatcher = NULL;
        }

        if (deferredCallbacks) {
            delete deferredCallbacks;
            deferredCallbacks = NULL;
        }

        /*
         * Unregister AllJoyn registered bus objects
         */
        if (dbusObj) {
            delete dbusObj;
            dbusObj = NULL;
        }
        if (alljoynObj) {
            delete alljoynObj;
            alljoynObj = NULL;
        }
        if (alljoynDebugObj) {
            delete alljoynDebugObj;
            alljoynDebugObj = NULL;
        }
        if (peerObj) {
            delete peerObj;
            peerObj = NULL;
        }
    }
}

QStatus _LocalEndpoint::Start()
{
    QStatus status = ER_OK;

    /* Start the dispatcher */
    if (!dispatcher) {
        return ER_BUS_NO_ENDPOINT;
    }
    status = dispatcher->Start();

    /* Start the replyTimer */
    if (status == ER_OK) {
        status = replyTimer.Start();
    }

    /* Set the local endpoint's unique name */
    SetUniqueName(bus->GetInternal().GetRouter().GenerateUniqueName());

    if (!dbusObj) {
        /* Register well known org.freedesktop.DBus remote object */
        const InterfaceDescription* intf = bus->GetInterface(org::freedesktop::DBus::InterfaceName);
        if (intf) {
            dbusObj = new ProxyBusObject(*bus, org::freedesktop::DBus::WellKnownName, org::freedesktop::DBus::ObjectPath, 0);
            dbusObj->AddInterface(*intf);
        } else {
            status = ER_BUS_NO_SUCH_INTERFACE;
        }
    }

    if (!alljoynObj && (ER_OK == status)) {
        /* Register well known org.alljoyn.Bus remote object */
        const InterfaceDescription* mintf = bus->GetInterface(org::alljoyn::Bus::InterfaceName);
        if (mintf) {
            alljoynObj = new ProxyBusObject(*bus, org::alljoyn::Bus::WellKnownName, org::alljoyn::Bus::ObjectPath, 0);
            alljoynObj->AddInterface(*mintf);
        } else {
            status = ER_BUS_NO_SUCH_INTERFACE;
        }
    }

    /* Initialize the peer object */
    if (!peerObj && (ER_OK == status)) {
        peerObj = new AllJoynPeerObj(*bus);
        status = peerObj->Init(*bus);
    }

    /* Start the peer object */
    if (peerObj && (ER_OK == status)) {
        status = peerObj->Start();
    }

    /* Local endpoint is up and running, register with router */
    if (ER_OK == status) {
        running = true;
        BusEndpoint busEndpoint = BusEndpoint::wrap(this);
        bus->GetInternal().GetRouter().RegisterEndpoint(busEndpoint);
        isRegistered = true;
    }
    return status;
}

QStatus _LocalEndpoint::Stop(void)
{
    QCC_DbgTrace(("LocalEndpoint::Stop"));

    /* Local endpoint not longer running */
    running = false;

    if (peerObj) {
        peerObj->Stop();
    }

    /* Stop the dispatcher */
    if (dispatcher) {
        dispatcher->Stop();
    }

    /* Stop the replyTimer */
    replyTimer.Stop();
    return ER_OK;
}

QStatus _LocalEndpoint::Join(void)
{
    /*
     * Unregister the localEndpoint from the router
     * This must be done in Join rather than Stop since UnregisteringEndpoint may block
     */
    if (isRegistered) {
        bus->GetInternal().GetRouter().UnregisterEndpoint(this->GetUniqueName(), this->GetEndpointType());
        isRegistered = false;
    }

    if (peerObj) {
        peerObj->Join();
    }

    /* Join the dispatcher */
    if (dispatcher) {
        dispatcher->Join();
    }

    /* Join the replyTimer */
    replyTimer.Join();

    return ER_OK;
}

QStatus _LocalEndpoint::Diagnose(Message& message)
{
    QStatus status;
    BusObject* obj = FindLocalObject(message->GetObjectPath());

    /*
     * Try to figure out what went wrong
     */
    if (obj == NULL) {
        status = ER_BUS_NO_SUCH_OBJECT;
        QCC_LogError(status, ("No such object %s", message->GetObjectPath()));
    } else if (!obj->ImplementsInterface(message->GetInterface())) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Object %s has no interface %s (member=%s)", message->GetObjectPath(), message->GetInterface(), message->GetMemberName()));
    } else {
        status = ER_BUS_OBJECT_NO_SUCH_MEMBER;
        QCC_LogError(status, ("Object %s has no member %s", message->GetObjectPath(), message->GetMemberName()));
    }
    return status;
}

QStatus _LocalEndpoint::PeerInterface(Message& message)
{
    if (strcmp(message->GetMemberName(), "Ping") == 0) {
        QStatus status = message->UnmarshalArgs("", "");
        if (ER_OK != status) {
            return status;
        }
        message->ReplyMsg(message, NULL, 0);
        BusEndpoint busEndpoint = BusEndpoint::wrap(this);
        return bus->GetInternal().GetRouter().PushMessage(message, busEndpoint);
    }
    if (strcmp(message->GetMemberName(), "GetMachineId") == 0) {
        QStatus status = message->UnmarshalArgs("", "s");
        if (ER_OK != status) {
            return status;
        }
        MsgArg replyArg(ALLJOYN_STRING);
        // @@TODO Need OS specific support for returning a machine id GUID use the bus id for now
        qcc::String guidStr = bus->GetInternal().GetGlobalGUID().ToString();
        replyArg.v_string.str = guidStr.c_str();
        replyArg.v_string.len = guidStr.size();
        message->ReplyMsg(message, &replyArg, 1);
        BusEndpoint busEndpoint = BusEndpoint::wrap(this);
        return bus->GetInternal().GetRouter().PushMessage(message, busEndpoint);
    }
    return ER_BUS_OBJECT_NO_SUCH_MEMBER;
}


QStatus _LocalEndpoint::Dispatcher::DispatchMessage(Message& msg)
{
    uint32_t zero = 0;
    void* context = new Message(msg);
    qcc::AlarmListener* localEndpointListener = this;
    Alarm alarm(zero, localEndpointListener, context, zero);
    QStatus status = AddAlarm(alarm);
    if (status != ER_OK) {
        Message* temp = static_cast<Message*>(context);
        if (temp) {
            delete temp;
        }
    }
    return status;
}

void _LocalEndpoint::EnableReentrancy()
{
    if (dispatcher) {
        dispatcher->EnableReentrancy();
    }
}

bool _LocalEndpoint::IsReentrantCall()
{
    if (!dispatcher) {
        return false;
    }
    return dispatcher->ThreadHoldsLock();

}

void _LocalEndpoint::Dispatcher::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    Message* msg = static_cast<Message*>(alarm->GetContext());
    if (msg) {
        if (reason == ER_OK) {
            QStatus status = endpoint->DoPushMessage(*msg);
            // ER_BUS_STOPPING is a common shutdown error
            if (status != ER_OK && status != ER_BUS_STOPPING) {
                QCC_LogError(status, ("LocalEndpoint::DoPushMessage failed"));
            }
        }
        delete msg;
    }
}

QStatus _LocalEndpoint::PushMessage(Message& message)
{
    QStatus ret;

    if (running) {
        BusEndpoint ep = bus->GetInternal().GetRouter().FindEndpoint(message->GetSender());
        /* Determine if the source of this message is local to the process */
        Thread* curThread = Thread::GetThread();
        if (ep->GetEndpointType() == ENDPOINT_TYPE_LOCAL && (strncmp(curThread->GetThreadName(), "lepDisp", 7) == 0)) {
            ret = DoPushMessage(message);
        } else {
            ret = dispatcher->DispatchMessage(message);
        }
    } else {
        ret = ER_BUS_STOPPING;
    }
    return ret;
}

QStatus _LocalEndpoint::DoPushMessage(Message& message)
{
    QStatus status = ER_OK;

    if (!running) {
        status = ER_BUS_STOPPING;
        QCC_DbgHLPrintf(("Local transport not running discarding %s", message->Description().c_str()));
    } else {
        QCC_DbgPrintf(("Pushing %s into local endpoint", message->Description().c_str()));

        switch (message->GetType()) {
        case MESSAGE_METHOD_CALL:
            status = HandleMethodCall(message);
            break;

        case MESSAGE_SIGNAL:
            status = HandleSignal(message);
            break;

        case MESSAGE_METHOD_RET:
        case MESSAGE_ERROR:
            status = HandleMethodReply(message);
            break;

        default:
            status = ER_FAIL;
            break;
        }
    }
    return status;
}

QStatus _LocalEndpoint::RegisterBusObject(BusObject& object, bool isSecure)
{
    QStatus status = ER_OK;

    const char* objPath = object.GetPath();

    QCC_DbgPrintf(("RegisterBusObject %s", objPath));

    if (!IsLegalObjectPath(objPath)) {
        status = ER_BUS_BAD_OBJ_PATH;
        QCC_LogError(status, ("Illegal object path \"%s\" specified", objPath));
        return status;
    }

    objectsLock.Lock(MUTEX_CONTEXT);

    /* Register placeholder parents as needed */
    size_t off = 0;
    qcc::String pathStr(objPath);
    BusObject* lastParent = NULL;
    if (1 < pathStr.size()) {
        while (qcc::String::npos != (off = pathStr.find_first_of('/', off))) {
            qcc::String parentPath = pathStr.substr(0, max((size_t)1, off));
            off++;
            BusObject* parent = FindLocalObject(parentPath.c_str());
            if (!parent) {
                parent = new BusObject(parentPath.c_str(), true);
                QStatus status = DoRegisterBusObject(*parent, lastParent, true);
                if (ER_OK != status) {
                    delete parent;
                    QCC_LogError(status, ("Failed to register default object for path %s", parentPath.c_str()));
                    break;
                }
                defaultObjects.push_back(parent);
            } else {
                /*
                 * If the parent is secure then this object is secure also.
                 */
                isSecure |= parent->isSecure;
            }
            lastParent = parent;
        }
    }

    /* Now register the object itself */
    if (ER_OK == status) {
        object.isSecure = isSecure;
        status = DoRegisterBusObject(object, lastParent, false);
    }

    objectsLock.Unlock(MUTEX_CONTEXT);

    return status;
}

QStatus _LocalEndpoint::DoRegisterBusObject(BusObject& object, BusObject* parent, bool isPlaceholder)
{
    QCC_DbgPrintf(("DoRegisterBusObject %s", object.GetPath()));
    const char* objPath = object.GetPath();

    /* objectsLock is already obtained */

    /* If an object with this path already exists, replace it */
    BusObject* existingObj = FindLocalObject(objPath);
    if (NULL != existingObj) {
        existingObj->Replace(object);
        UnregisterBusObject(*existingObj);
    }

    /* Register object. */
    QStatus status = object.DoRegistration(*bus);
    if (ER_OK == status) {
        /* Link new object to its parent */
        if (parent) {
            parent->AddChild(object);
        }
        /* Add object to list of objects */
        localObjects[object.GetPath()] = &object;

        /* Register handler for the object's methods */
        methodTable.AddAll(&object);

        /*
         * If the bus is already running schedule call backs to report
         * that the objects are registered. If the bus is not running
         * the callbacks will be made later when the client router calls
         * OnBusConnected().
         */
        if (bus->GetInternal().GetRouter().IsBusRunning() && !isPlaceholder) {
            objectsLock.Unlock(MUTEX_CONTEXT);
            OnBusConnected();
            objectsLock.Lock(MUTEX_CONTEXT);
        }
    }

    return status;
}

void _LocalEndpoint::UnregisterBusObject(BusObject& object)
{
    QCC_DbgPrintf(("UnregisterBusObject %s", object.GetPath()));

    /* Remove members */
    methodTable.RemoveAll(&object);

    /* Remove from object list */
    objectsLock.Lock(MUTEX_CONTEXT);
    localObjects.erase(object.GetPath());
    objectsLock.Unlock(MUTEX_CONTEXT);

    /* Notify object and detach from bus*/
    if (object.isRegistered) {
        object.ObjectUnregistered();
        object.isRegistered = false;
    }

    /* Detach object from parent */
    objectsLock.Lock(MUTEX_CONTEXT);
    if (NULL != object.parent) {
        object.parent->RemoveChild(object);
    }

    /* If object has children, unregister them as well */
    while (true) {
        BusObject* child = object.RemoveChild();
        if (!child) {
            break;
        }
        object.InUseIncrement();
        objectsLock.Unlock(MUTEX_CONTEXT);
        UnregisterBusObject(*child);
        objectsLock.Lock(MUTEX_CONTEXT);
        object.InUseDecrement();
    }
    /* Delete the object if it was a default object */
    vector<BusObject*>::iterator dit = defaultObjects.begin();
    while (dit != defaultObjects.end()) {
        if (*dit == &object) {
            defaultObjects.erase(dit);
            delete &object;
            break;
        } else {
            ++dit;
        }
    }
    objectsLock.Unlock(MUTEX_CONTEXT);
}

BusObject* _LocalEndpoint::FindLocalObject(const char* objectPath) {
    objectsLock.Lock(MUTEX_CONTEXT);
    unordered_map<const char*, BusObject*, Hash, PathEq>::iterator iter = localObjects.find(objectPath);
    BusObject* ret = (iter == localObjects.end()) ? NULL : iter->second;
    objectsLock.Unlock(MUTEX_CONTEXT);
    return ret;
}

QStatus _LocalEndpoint::GetAnnouncedObjectDescription(MsgArg& objectDescriptionArg) {
    QStatus status = ER_OK;
    objectDescriptionArg.Clear();

    objectsLock.Lock(MUTEX_CONTEXT);
    size_t announcedObjectsCount = 0;
    // Find out how many of the localObjects contain Announced interfaces
    for (std::unordered_map<const char*, BusObject*, Hash, PathEq>::iterator it = localObjects.begin(); it != localObjects.end(); ++it) {
        if (it->second->GetAnnouncedInterfaceNames() > 0) {
            ++announcedObjectsCount;
        }
    }
    // Now that we know how many objects are have AnnouncedInterfaces lets Create
    // an array of MsgArgs. One MsgArg for each Object with Announced interfaces.
    MsgArg* announceObjectsArg = new MsgArg[announcedObjectsCount];
    size_t argCount = 0;
    // Fill the MsgArg for the announcedObjects.
    for (std::unordered_map<const char*, BusObject*, Hash, PathEq>::iterator it = localObjects.begin(); it != localObjects.end(); ++it) {
        size_t numInterfaces = it->second->GetAnnouncedInterfaceNames();
        if (numInterfaces > 0) {
            const char** interfaces = new const char*[numInterfaces];
            it->second->GetAnnouncedInterfaceNames(interfaces, numInterfaces);
            status = announceObjectsArg[argCount].Set("(oas)", it->first, numInterfaces, interfaces);
            announceObjectsArg[argCount].Stabilize();
            delete [] interfaces;
            ++argCount;
        }
        if (ER_OK != status) {
            delete [] announceObjectsArg;
            objectsLock.Unlock(MUTEX_CONTEXT);
            return status;
        }
    }
    // If argCount and announcedObjectsCount don't match something has gone wrong
    assert(argCount == announcedObjectsCount);

    status = objectDescriptionArg.Set("a(oas)", announcedObjectsCount, announceObjectsArg);
    objectDescriptionArg.Stabilize();
    delete [] announceObjectsArg;
    objectsLock.Unlock(MUTEX_CONTEXT);

    return status;
}

void _LocalEndpoint::UpdateSerialNumber(Message& msg)
{
    uint32_t serial = msg->msgHeader.serialNum;
    /*
     * If the previous serial number is not the latest we replace it.
     */
    if (serial != bus->GetInternal().PrevSerial()) {
        msg->SetSerialNumber();
        /*
         * If the message is a method call me must update the reply map
         */
        if (msg->GetType() == MESSAGE_METHOD_CALL) {
            replyMapLock.Lock(MUTEX_CONTEXT);
            ReplyContext* rc = RemoveReplyHandler(serial);
            if (rc) {
                rc->serial = msg->msgHeader.serialNum;
                replyMap[rc->serial] = rc;
            }
            replyMapLock.Unlock(MUTEX_CONTEXT);
        }
        QCC_DbgPrintf(("LocalEndpoint::UpdateSerialNumber for %s serial=%u was %u", msg->Description().c_str(), msg->msgHeader.serialNum, serial));
    }
}

QStatus _LocalEndpoint::RegisterReplyHandler(MessageReceiver* receiver,
                                             MessageReceiver::ReplyHandler replyHandler,
                                             const InterfaceDescription::Member& method,
                                             Message& methodCallMsg,
                                             void* context,
                                             uint32_t timeout)
{
    QStatus status = ER_OK;
    if (!running) {
        status = ER_BUS_STOPPING;
        QCC_LogError(status, ("Local transport not running"));
    } else {
        ReplyContext* rc =  new ReplyContext(LocalEndpoint::wrap(this), receiver, replyHandler, &method, methodCallMsg, context, timeout);
        QCC_DbgPrintf(("LocalEndpoint::RegisterReplyHandler"));
        /*
         * Add reply context.
         */
        replyMapLock.Lock(MUTEX_CONTEXT);
        replyMap[methodCallMsg->msgHeader.serialNum] = rc;
        replyMapLock.Unlock(MUTEX_CONTEXT);
        /*
         * Set timeout
         */
        status = replyTimer.AddAlarm(rc->alarm);
        if (status != ER_OK) {
            UnregisterReplyHandler(methodCallMsg);
        }
    }
    return status;
}

bool _LocalEndpoint::UnregisterReplyHandler(Message& methodCall)
{
    replyMapLock.Lock(MUTEX_CONTEXT);
    ReplyContext* rc = RemoveReplyHandler(methodCall->msgHeader.serialNum);
    replyMapLock.Unlock(MUTEX_CONTEXT);
    if (rc) {
        delete rc;
        return true;
    } else {
        return false;
    }
}

/*
 * NOTE: Must be called holding replyMapLock
 */
_LocalEndpoint::ReplyContext* _LocalEndpoint::RemoveReplyHandler(uint32_t serial)
{
    QCC_DbgPrintf(("LocalEndpoint::RemoveReplyHandler for serial=%u", serial));
    ReplyContext* rc = NULL;
    map<uint32_t, ReplyContext*>::iterator iter = replyMap.find(serial);
    if (iter != replyMap.end()) {
        rc = iter->second;
        replyMap.erase(iter);
        assert(rc->serial == serial);
    }
    return rc;
}

bool _LocalEndpoint::PauseReplyHandlerTimeout(Message& methodCallMsg)
{
    bool paused = false;
    if (methodCallMsg->GetType() == MESSAGE_METHOD_CALL) {
        replyMapLock.Lock();
        map<uint32_t, ReplyContext*>::iterator iter = replyMap.find(methodCallMsg->GetCallSerial());
        if (iter != replyMap.end()) {
            ReplyContext*rc = iter->second;
            paused = replyTimer.RemoveAlarm(rc->alarm);
        }
        replyMapLock.Unlock();
    }
    return paused;
}

bool _LocalEndpoint::ResumeReplyHandlerTimeout(Message& methodCallMsg)
{
    bool resumed = false;
    if (methodCallMsg->GetType() == MESSAGE_METHOD_CALL) {
        replyMapLock.Lock();
        map<uint32_t, ReplyContext*>::iterator iter = replyMap.find(methodCallMsg->GetCallSerial());
        if (iter != replyMap.end()) {
            ReplyContext*rc = iter->second;
            QStatus status = replyTimer.AddAlarm(rc->alarm);
            if (status == ER_OK) {
                resumed = true;
            } else {
                QCC_LogError(status, ("Failed to resume reply handler timeout for %s", methodCallMsg->Description().c_str()));
            }
        }
        replyMapLock.Unlock();
    }
    return resumed;
}

QStatus _LocalEndpoint::RegisterSignalHandler(MessageReceiver* receiver,
                                              MessageReceiver::SignalHandler signalHandler,
                                              const InterfaceDescription::Member* member,
                                              const char* matchRule)
{
    if (!receiver) {
        return ER_BAD_ARG_1;
    }
    if (!signalHandler) {
        return ER_BAD_ARG_2;
    }
    if (!member) {
        return ER_BAD_ARG_3;
    }
    if (!matchRule) {
        return ER_BAD_ARG_4;
    }
    signalTable.Add(receiver, signalHandler, member, matchRule);
    return ER_OK;
}

QStatus _LocalEndpoint::UnregisterSignalHandler(MessageReceiver* receiver,
                                                MessageReceiver::SignalHandler signalHandler,
                                                const InterfaceDescription::Member* member,
                                                const char* matchRule)
{
    if (!receiver) {
        return ER_BAD_ARG_1;
    }
    if (!signalHandler) {
        return ER_BAD_ARG_2;
    }
    if (!member) {
        return ER_BAD_ARG_3;
    }
    if (!matchRule) {
        return ER_BAD_ARG_4;
    }
    return signalTable.Remove(receiver, signalHandler, member, matchRule);
}

QStatus _LocalEndpoint::UnregisterAllHandlers(MessageReceiver* receiver)
{
    /*
     * Remove all the signal handlers for this receiver.
     */
    signalTable.RemoveAll(receiver);
    /*
     * Remove any reply handlers for this receiver
     */
    replyMapLock.Lock(MUTEX_CONTEXT);
    for (map<uint32_t, ReplyContext*>::iterator iter = replyMap.begin(); iter != replyMap.end();) {
        ReplyContext* rc = iter->second;
        if (rc->receiver == receiver) {
            replyMap.erase(iter);
            delete rc;
            iter = replyMap.begin();
        } else {
            ++iter;
        }
    }
    replyMapLock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

/*
 * Alarm handler for method calls that have not received a response within the timeout period.
 */
void _LocalEndpoint::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    ReplyContext* rc = reinterpret_cast<ReplyContext*>(alarm->GetContext());
    replyMapLock.Lock(MUTEX_CONTEXT);
    /* Search for the ReplyContext entry in the replyMap */
    bool found = false;
    for (map<uint32_t, ReplyContext*>::iterator iter = replyMap.begin(); iter != replyMap.end(); iter++) {
        if (rc == iter->second) {
            found = true;
            break;
        }
    }

    if (found == false) {
        /* If an entry for the ReplyContext is not found, it might have been deleted due to a MethodReply. */
        replyMapLock.Unlock(MUTEX_CONTEXT);
        return;
    }
    uint32_t serial = rc->serial;
    Message msg(*bus);
    QStatus status = ER_OK;

    /*
     * Clear the encrypted flag so the error response doesn't get rejected.
     */
    rc->callFlags &= ~ALLJOYN_FLAG_ENCRYPTED;
    replyMapLock.Unlock(MUTEX_CONTEXT);

    if (running) {
        QCC_DbgPrintf(("Timed out waiting for METHOD_REPLY with serial %d", serial));
        if (reason == ER_TIMER_EXITING) {
            msg->ErrorMsg("org.alljoyn.Bus.Exiting", serial);
        } else {
            msg->ErrorMsg("org.alljoyn.Bus.Timeout", serial);
        }
        /*
         * Forward the message via the dispatcher so we conform to our concurrency model.
         */
        status = dispatcher->DispatchMessage(msg);

    } else {
        msg->ErrorMsg("org.alljoyn.Bus.Exiting", serial);
        HandleMethodReply(msg);
    }
    /*
     * If the dispatch failed or we are no longer running handle the reply on this thread.
     */
    if (status != ER_OK) {
        msg->ErrorMsg("org.alljoyn.Bus.Exiting", serial);
        HandleMethodReply(msg);
    }
}

QStatus _LocalEndpoint::HandleMethodCall(Message& message)
{
    QStatus status = ER_OK;

    /* Look up the member */
    MethodTable::SafeEntry* safeEntry = methodTable.Find(message->GetObjectPath(),
                                                         message->GetInterface(),
                                                         message->GetMemberName());
    const MethodTable::Entry* entry = safeEntry ? safeEntry->entry : NULL;

    if (entry == NULL) {
        if (strcmp(message->GetInterface(), org::freedesktop::DBus::Peer::InterfaceName) == 0) {
            /*
             * Special case the Peer interface
             */
            status = PeerInterface(message);
        } else {
            /*
             * Figure out what error to report
             */
            status = Diagnose(message);
        }
    } else {
        if (!message->IsEncrypted()) {
            /*
             * If the interface is secure encryption is required. If the object is secure encryption
             * is required unless security is not applicable to the this interface.
             */
            InterfaceSecurityPolicy ifcSec = entry->member->iface->GetSecurityPolicy();
            if ((ifcSec == AJ_IFC_SECURITY_REQUIRED) || (entry->object->IsSecure() && (ifcSec != AJ_IFC_SECURITY_OFF))) {
                status = ER_BUS_MESSAGE_NOT_ENCRYPTED;
                QCC_LogError(status, ("Method call to secure %s was not encrypted", entry->object->IsSecure() ? "object" : "interface"));
            }
        }
        if (status == ER_OK) {
            status = message->UnmarshalArgs(entry->member->signature, entry->member->returnSignature.c_str());
        }
    }
    if (status == ER_OK) {
        /* Call the method handler */
        if (entry) {
            entry->object->CallMethodHandler(entry->handler, entry->member, message, entry->context);
        }
    } else if (message->GetType() == MESSAGE_METHOD_CALL && !(message->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED)) {
        /* We are rejecting a method call that expects a response so reply with an error message. */
        qcc::String errStr;
        qcc::String errMsg;
        switch (status) {
        case ER_BUS_MESSAGE_NOT_ENCRYPTED:
            errStr = "org.alljoyn.Bus.SecurityViolation";
            errMsg = "Expected secure method call";
            peerObj->HandleSecurityViolation(message, status);
            status = ER_OK;
            break;

        case ER_BUS_MESSAGE_DECRYPTION_FAILED:
            errStr = "org.alljoyn.Bus.SecurityViolation";
            errMsg = "Unable to authenticate method call";
            peerObj->HandleSecurityViolation(message, status);
            status = ER_OK;
            break;

        case ER_BUS_NOT_AUTHORIZED:
            errStr = "org.alljoyn.Bus.SecurityViolation";
            errMsg = "Method call not authorized";
            peerObj->HandleSecurityViolation(message, status);
            status = ER_OK;
            break;

        case ER_BUS_NO_SUCH_OBJECT:
            errStr = "org.freedesktop.DBus.Error.ServiceUnknown";
            errMsg = QCC_StatusText(status);
            break;

        default:
            errStr += "org.alljoyn.Bus.";
            errStr += QCC_StatusText(status);
            errMsg = message->Description();
            break;
        }
        message->ErrorMsg(message, errStr.c_str(), errMsg.c_str());
        BusEndpoint busEndpoint = BusEndpoint::wrap(this);
        status = bus->GetInternal().GetRouter().PushMessage(message, busEndpoint);
    } else {
        QCC_LogError(status, ("Ignoring message %s", message->Description().c_str()));
        status = ER_OK;
    }

    // safeEntry might be null here
    delete safeEntry;
    return status;
}

QStatus _LocalEndpoint::HandleSignal(Message& message)
{
    QStatus status = ER_OK;

    signalTable.Lock();

    /* Look up the signal */
    pair<SignalTable::const_iterator, SignalTable::const_iterator> range =
        signalTable.Find(message->GetInterface(), message->GetMemberName());

    /*
     * Quick exit if there are no handlers for this signal
     */
    if (range.first == range.second) {
        signalTable.Unlock();
        return ER_OK;
    }
    /*
     * Build a list of all signal handlers for this signal
     */
    list<SignalTable::Entry> callList;
    const InterfaceDescription::Member* signal = range.first->second.member;
    do {
        if (range.first->second.rule.IsMatch(message)) {
            callList.push_back(range.first->second);
        }
    } while (++range.first != range.second);
    /*
     * We have our callback list so we can unlock the signal table.
     */
    signalTable.Unlock();
    /*
     * Validate and unmarshal the signal
     */
    if (signal->iface->IsSecure() && !message->IsEncrypted()) {
        status = ER_BUS_MESSAGE_NOT_ENCRYPTED;
        QCC_LogError(status, ("Signal from secure interface was not encrypted"));
    } else {
        status = message->UnmarshalArgs(signal->signature);
    }
    if (status != ER_OK) {
        if ((status == ER_BUS_MESSAGE_DECRYPTION_FAILED) || (status == ER_BUS_MESSAGE_NOT_ENCRYPTED) || (status == ER_BUS_NOT_AUTHORIZED)) {
            peerObj->HandleSecurityViolation(message, status);
            status = ER_OK;
        }
    } else {
        list<SignalTable::Entry>::const_iterator callit;
        for (callit = callList.begin(); callit != callList.end(); ++callit) {
            (callit->object->*callit->handler)(callit->member, message->GetObjectPath(), message);
        }
    }
    return status;
}

QStatus _LocalEndpoint::HandleMethodReply(Message& message)
{
    QStatus status = ER_OK;

    replyMapLock.Lock();
    ReplyContext* rc = RemoveReplyHandler(message->GetReplySerial());
    replyMapLock.Unlock();
    if (rc) {
        if ((rc->callFlags & ALLJOYN_FLAG_ENCRYPTED) && !message->IsEncrypted()) {
            /*
             * If the response was an internally generated error response just keep that error.
             * Otherwise if reply was not encrypted so return an error to the caller. Internally
             * generated messages can be identified by their sender field.
             */
            if ((message->GetType() == MESSAGE_METHOD_RET) || (GetUniqueName() != message->GetSender())) {
                status = ER_BUS_MESSAGE_NOT_ENCRYPTED;
            }
            if ((message->GetType() == MESSAGE_ERROR)) {
                message->UnmarshalArgs("*");
            }
        } else {
            QCC_DbgPrintf(("Matched reply for serial #%d", message->GetReplySerial()));
            if (message->GetType() == MESSAGE_METHOD_RET) {
                status = message->UnmarshalArgs(rc->method->returnSignature);
            } else {
                status = message->UnmarshalArgs("*");
            }
        }
        if (status != ER_OK) {
            switch (status) {
            case ER_BUS_MESSAGE_DECRYPTION_FAILED:
            case ER_BUS_MESSAGE_NOT_ENCRYPTED:
            case ER_BUS_NOT_AUTHORIZED:
                message->ErrorMsg(status, message->GetReplySerial());
                peerObj->HandleSecurityViolation(message, status);
                break;

            default:
                message->ErrorMsg(status, message->GetReplySerial());
                break;
            }
            QCC_LogError(status, ("Reply message replaced with an internally generated error"));
            status = ER_OK;
        }
        ((rc->receiver)->*(rc->handler))(message, rc->context);
        delete rc;
    } else {
        status = ER_BUS_UNMATCHED_REPLY_SERIAL;
        QCC_DbgHLPrintf(("%s does not match any current method calls: %s", message->Description().c_str(), QCC_StatusText(status)));
    }
    return status;
}

void _LocalEndpoint::DeferredCallbacks::AlarmTriggered(const qcc::Alarm& alarm, QStatus reason)
{
    if (reason == ER_OK) {
        /*
         * Allow synchronous method calls from within the object registration callbacks
         */
        endpoint->bus->EnableConcurrentCallbacks();
        /*
         * Call ObjectRegistered for any unregistered bus objects
         */
        endpoint->objectsLock.Lock(MUTEX_CONTEXT);
        unordered_map<const char*, BusObject*, Hash, PathEq>::iterator iter = endpoint->localObjects.begin();
        while (endpoint->running && (iter != endpoint->localObjects.end())) {
            if (!iter->second->isRegistered) {
                BusObject* bo = iter->second;
                bo->isRegistered = true;
                bo->InUseIncrement();
                endpoint->objectsLock.Unlock(MUTEX_CONTEXT);
                bo->ObjectRegistered();
                endpoint->objectsLock.Lock(MUTEX_CONTEXT);
                bo->InUseDecrement();
                iter = endpoint->localObjects.begin();
            } else {
                ++iter;
            }
        }
        endpoint->objectsLock.Unlock(MUTEX_CONTEXT);
    }
}

void _LocalEndpoint::OnBusConnected()
{
    /*
     * Use the local endpoint's dispatcher to call back to report the object registrations.
     */
    uint32_t zero = 0;
    if (dispatcher) {
        QStatus status = dispatcher->AddAlarm(Alarm(zero, deferredCallbacks));
        if (ER_OK != status) {
            QCC_DbgHLPrintf(("OnBusConnected failure to add Alarm: %s", QCC_StatusText(status)));
        }
    }
}

void _LocalEndpoint::OnBusDisconnected() {
    /*
     * Call ObjectUnegistered for any unregistered bus objects
     */
    objectsLock.Lock(MUTEX_CONTEXT);
    unordered_map<const char*, BusObject*, Hash, PathEq>::iterator iter = localObjects.begin();
    while (iter != localObjects.end()) {
        if (iter->second->isRegistered) {
            BusObject* bo = iter->second;
            bo->isRegistered = false;
            bo->InUseIncrement();
            objectsLock.Unlock(MUTEX_CONTEXT);
            bo->ObjectUnregistered();
            objectsLock.Lock(MUTEX_CONTEXT);
            bo->InUseDecrement();
            iter = localObjects.begin();
        } else {
            ++iter;
        }
    }
    objectsLock.Unlock(MUTEX_CONTEXT);
}

const ProxyBusObject& _LocalEndpoint::GetAllJoynDebugObj()
{
    if (!alljoynDebugObj) {
        /* Register well known org.alljoyn.bus.Debug remote object */
        alljoynDebugObj = new ProxyBusObject(*bus, org::alljoyn::Daemon::WellKnownName, org::alljoyn::Daemon::Debug::ObjectPath, 0);
        const InterfaceDescription* intf;
        intf = bus->GetInterface(org::alljoyn::Daemon::Debug::InterfaceName);
        if (intf) {
            alljoynDebugObj->AddInterface(*intf);
        }
        intf = bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
        if (intf) {
            alljoynDebugObj->AddInterface(*intf);
        }
    }

    return *alljoynDebugObj;
}

}

