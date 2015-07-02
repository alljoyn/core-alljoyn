/**
 * @file
 *
 * This file implements the ProxyBusObject class.
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
#include <map>
#include <set>
#include <vector>

#include <qcc/Condition.h>
#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/ManagedObj.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>
#include <qcc/StringSource.h>
#include <qcc/Util.h>
#include <qcc/XmlElement.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/Message.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/Status.h>

#include "AllJoynPeerObj.h"
#include "BusInternal.h"
#include "LocalTransport.h"
#include "Router.h"
#include "XmlHelper.h"

#define QCC_MODULE "ALLJOYN_PBO"

#define SYNC_METHOD_ALERTCODE_OK     0
#define SYNC_METHOD_ALERTCODE_ABORT  1

using namespace qcc;
using namespace std;

namespace ajn {

#if defined(QCC_OS_GROUP_WINDOWS)
#pragma pack(push, CBContext, 4)
#endif
template <typename _cbType> struct CBContext {
    CBContext(ProxyBusObject::Listener* listener, _cbType callback, void* context) :
        listener(listener), callback(callback), context(context) { }

    ProxyBusObject::Listener* listener;
    _cbType callback;
    void* context;
};
#if defined(QCC_OS_GROUP_WINDOWS)
#pragma pack(pop, CBContext)
#endif

struct _PropertiesChangedCB {
    _PropertiesChangedCB(ProxyBusObject::PropertiesChangedListener& listener,
                         const char** properties,
                         size_t numProps,
                         void* context) :
        listener(listener), context(context), isRegistered(true), numRunning(0)
    {
        if (properties) {
            for (size_t i = 0; i < numProps; ++i) {
                this->properties.insert(String(properties[i]));
            }
        }
    }

    ProxyBusObject::PropertiesChangedListener& listener;
    void* context;
    set<StringMapKey> properties;  // Properties to monitor - empty set == all properties.
    bool isRegistered;
    int32_t numRunning;
    _PropertiesChangedCB& operator=(const _PropertiesChangedCB&) { return *this; }
};

typedef ManagedObj<_PropertiesChangedCB> PropertiesChangedCB;

class CachedProps {
    qcc::Mutex lock;
    typedef std::map<qcc::StringMapKey, MsgArg> ValueMap;
    ValueMap values;
    const InterfaceDescription* description;
    bool isFullyCacheable;
    size_t numProperties;
    uint32_t lastMessageSerial;
    bool IsCacheable(const char* propname);
    bool IsValidMessageSerial(uint32_t messageSerial);

  public:
    CachedProps() :
        lock(), values(), description(NULL),
        isFullyCacheable(false),
        numProperties(0), lastMessageSerial(0) { }

    CachedProps(const InterfaceDescription*intf) :
        lock(), values(), description(intf),
        isFullyCacheable(false), lastMessageSerial(0) {
        numProperties = description->GetProperties();
        if (numProperties > 0) {
            isFullyCacheable = true;
            const InterfaceDescription::Property** props = new const InterfaceDescription::Property* [numProperties];
            description->GetProperties(props, numProperties);
            for (size_t i = 0; i < numProperties; ++i) {
                if (props[i]->cacheable == false) {
                    isFullyCacheable = false;
                    break;
                }
            }
            delete[] props;
        }
    }

    CachedProps(const CachedProps& other) :
        lock(), values(other.values), description(other.description),
        isFullyCacheable(other.isFullyCacheable),
        numProperties(other.numProperties), lastMessageSerial(other.lastMessageSerial) { }

    CachedProps& operator=(const CachedProps& other) {
        if (&other != this) {
            values = other.values;
            description = other.description;
            isFullyCacheable = other.isFullyCacheable;
            numProperties = other.numProperties;
            lastMessageSerial = other.lastMessageSerial;
        }
        return *this;
    }

    ~CachedProps() { }

    bool Get(const char* propname, MsgArg& val);
    bool GetAll(MsgArg& val);
    void Set(const char* propname, const MsgArg& val, const uint32_t messageSerial);
    void SetAll(const MsgArg& allValues, const uint32_t messageSerial);
    void PropertiesChanged(MsgArg* changed, size_t numChanged, MsgArg* invalidated, size_t numInvalidated, const uint32_t messageSerial);
};


/**
 * Internal context structure used between synchronous method_call and method_return
 */
struct _SyncReplyContext {
    _SyncReplyContext(BusAttachment& bus) : replyMsg(bus), thread(Thread::GetThread()) { }
    bool operator<(_SyncReplyContext& other) const { return this < &other; }
    Message replyMsg;
    Thread* thread;
    Event event;
};
typedef ManagedObj<_SyncReplyContext> SyncReplyContext;

class ProxyBusObject::Internal : public MessageReceiver {
  public:
    Internal() :
        bus(nullptr),
        sessionId(0),
        hasProperties(false),
        isSecure(false),
        cacheProperties(false),
        registeredPropChangedHandler(false),
        handlerThreads()
    {
        QCC_DbgPrintf(("Creating empty PBO internal: %p", this));
    }
    Internal(BusAttachment& bus,
             String path,
             String service,
             SessionId sessionId,
             bool isSecure) :
        bus(&bus),
        path(path),
        serviceName(service),
        uniqueName((serviceName[0] == ':') ? service : ""),
        sessionId(sessionId),
        hasProperties(false),
        isSecure(isSecure),
        cacheProperties(false),
        registeredPropChangedHandler(false),
        handlerThreads()
    {
        QCC_DbgPrintf(("Creating PBO internal: %p   path=%s   serviceName=%s   uniqueName=%s", this, path.c_str(), serviceName.c_str(), uniqueName.c_str()));
    }
    Internal(BusAttachment& bus,
             String path,
             String serviceName,
             String uniqueName,
             SessionId sessionId,
             bool isSecure) :
        bus(&bus),
        path(path),
        serviceName(serviceName),
        uniqueName(uniqueName),
        sessionId(sessionId),
        hasProperties(false),
        isSecure(isSecure),
        cacheProperties(false),
        registeredPropChangedHandler(false),
        handlerThreads()
    {
        QCC_DbgPrintf(("Creating PBO internal: %p   path=%s   serviceName=%s   uniqueName=%s", this, path.c_str(), serviceName.c_str(), uniqueName.c_str()));
    }

    ~Internal();

    /**
     * @internal
     * Add PropertiesChanged match rule for an interface
     *
     * @param intf the interface name
     * @param blocking true if this method may block on the AddMatch call
     */
    void AddPropertiesChangedRule(const char* intf, bool blocking);

    /**
     * @internal
     * Remove PropertiesChanged match rule for an interface
     *
     * @param intf the interface name
     */
    void RemovePropertiesChangedRule(const char* intf);

    /**
     * @internal
     * Remove all PropertiesChanged match rules for this proxy
     */
    void RemoveAllPropertiesChangedRules();

    /**
     * @internal
     * Handle property changed signals. (Internal use only)
     */
    void PropertiesChangedHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& message);

    bool operator==(const ProxyBusObject::Internal& other) const
    {
        return ((this == &other) ||
                ((path == other.path) && (serviceName == other.serviceName)));
    }

    bool operator<(const ProxyBusObject::Internal& other) const
    {
        return ((path < other.path) ||
                ((path == other.path) && (serviceName < other.serviceName)));
    }

    BusAttachment* bus;                 /**< Bus associated with object */
    String path;                        /**< Object path of this object */
    String serviceName;                 /**< Remote destination alias */
    mutable String uniqueName;          /**< Remote destination unique name */
    SessionId sessionId;                /**< Session to use for communicating with remote object */
    bool hasProperties;                 /**< True if proxy object implements properties */
    mutable RemoteEndpoint b2bEp;       /**< B2B endpoint to use or NULL to indicates normal sessionId based routing */
    bool isSecure;                      /**< Indicates if this object is secure or not */
    bool cacheProperties;               /**< true if cacheable properties are cached */
    mutable Mutex lock;                 /**< Lock that protects access to internal state */
    Condition listenerDone;             /**< Signals that the properties changed listener is done */
    Condition handlerDone;              /**< Signals that the properties changed signal handler is done */
    bool registeredPropChangedHandler;  /**< true if our PropertiesChangedHandler is registered */
    map<qcc::Thread*, _PropertiesChangedCB*> handlerThreads;   /**< Thread actively calling PropertiesChangedListeners */

    /** The interfaces this object implements */
    map<qcc::StringMapKey, const InterfaceDescription*> ifaces;

    /** The property caches for the various interfaces */
    mutable map<qcc::StringMapKey, CachedProps> caches;

    /** Names of child objects of this object */
    vector<ProxyBusObject> children;

    /** Map of outstanding synchronous method calls to ProxyBusObjects. */
    mutable map<const ProxyBusObject* const, set<SyncReplyContext> > syncMethodCalls;
    mutable Condition syncMethodComplete;

    /** Match rule bookkeeping */
    map<qcc::StringMapKey, int> matchRuleRefcounts;

    /** Property changed handlers */
    multimap<StringMapKey, PropertiesChangedCB> propertiesChangedCBs;
};

ProxyBusObject::Internal::~Internal()
{
    lock.Lock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("Destroying PBO internal (%p) for %s on %s (%s)", this, path.c_str(), serviceName.c_str(), uniqueName.c_str()));
    if (registeredPropChangedHandler && bus) {
        /*
         * Unregister the PropertiesChanged signal handler without holding the
         * PBO lock, because the signal handler itself acquires the lock. The
         * unregistration procedure busy-waits for a signal handler to finish
         * before proceeding with the unregistration, so if we hold the lock here,
         * we can create a deadlock.
         */
        const InterfaceDescription* iface = bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
        if (iface) {
            bus->UnregisterSignalHandler(this,
                                         static_cast<MessageReceiver::SignalHandler>(&Internal::PropertiesChangedHandler),
                                         iface->GetMember("PropertiesChanged"),
                                         path.c_str());
        }
    }

    if (bus) {
        bus->UnregisterAllHandlers(this);
    }

    /* remove match rules added by the property caching & change notification mechanism */
    RemoveAllPropertiesChangedRules();

    /* Clean up properties changed listeners */
    while (!handlerThreads.empty()) {
        /*
         * The Properties Changed signal handler is still running.
         * Wait for it to complete.
         */
        handlerDone.Wait(lock);
    }

    while (!propertiesChangedCBs.empty()) {
        PropertiesChangedCB ctx = propertiesChangedCBs.begin()->second;
        ctx->isRegistered = false;
        propertiesChangedCBs.erase(propertiesChangedCBs.begin());
    }
    lock.Unlock(MUTEX_CONTEXT);
}

static inline bool SecurityApplies(const ProxyBusObject* obj, const InterfaceDescription* ifc)
{
    InterfaceSecurityPolicy ifcSec = ifc->GetSecurityPolicy();
    if (ifcSec == AJ_IFC_SECURITY_REQUIRED) {
        return true;
    } else {
        return obj->IsSecure() && (ifcSec != AJ_IFC_SECURITY_OFF);
    }
}

QStatus ProxyBusObject::GetAllProperties(const char* iface, MsgArg& value, uint32_t timeout) const
{
    QStatus status;
    const InterfaceDescription* valueIface = internal->bus->GetInterface(iface);
    if (!valueIface) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    } else {
        /* If all values are stored in the cache, we can reply immediately */
        bool cached = false;
        internal->lock.Lock(MUTEX_CONTEXT);
        if (internal->cacheProperties) {
            map<qcc::StringMapKey, CachedProps>::iterator it = internal->caches.find(iface);
            if (it != internal->caches.end()) {
                cached = it->second.GetAll(value);
            }
        }
        internal->lock.Unlock(MUTEX_CONTEXT);
        if (cached) {
            QCC_DbgPrintf(("GetAllProperties(%s) -> cache hit", iface));
            return ER_OK;
        }

        QCC_DbgPrintf(("GetAllProperties(%s) -> perform method call", iface));
        uint8_t flags = 0;
        /*
         * If the object or the property interface is secure method call must be encrypted.
         */
        if (SecurityApplies(this, valueIface)) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        Message reply(*internal->bus);
        MsgArg arg = MsgArg("s", iface);
        const InterfaceDescription* propIface = internal->bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
        if (propIface == NULL) {
            status = ER_BUS_NO_SUCH_INTERFACE;
        } else {
            const InterfaceDescription::Member* getAllProperties = propIface->GetMember("GetAll");
            assert(getAllProperties);
            status = MethodCall(*getAllProperties, &arg, 1, reply, timeout, flags);
            if (ER_OK == status) {
                value = *(reply->GetArg(0));
                /* use the retrieved property values to update the cache, if applicable */
                internal->lock.Lock(MUTEX_CONTEXT);
                if (internal->cacheProperties) {
                    map<qcc::StringMapKey, CachedProps>::iterator it = internal->caches.find(iface);
                    if (it != internal->caches.end()) {
                        it->second.SetAll(value, reply->GetCallSerial());
                    }
                }
                internal->lock.Unlock(MUTEX_CONTEXT);
            }
        }
    }
    return status;
}

void ProxyBusObject::GetAllPropsMethodCB(Message& message, void* context)
{
    CBContext<Listener::GetAllPropertiesCB>* ctx = reinterpret_cast<CBContext<Listener::GetAllPropertiesCB>*>(context);
    std::pair<void*, qcc::String>* wrappedContext = reinterpret_cast<std::pair<void*, qcc::String>*>(ctx->context);
    void* unwrappedContext = wrappedContext->first;
    const char* iface = wrappedContext->second.c_str();

    if (message->GetType() == MESSAGE_METHOD_RET) {
        /* use the retrieved property values to update the cache, if applicable */
        internal->lock.Lock(MUTEX_CONTEXT);
        if (internal->cacheProperties) {
            map<qcc::StringMapKey, CachedProps>::iterator it = internal->caches.find(iface);
            if (it != internal->caches.end()) {
                it->second.SetAll(*message->GetArg(0), message->GetCallSerial());
            }
        }
        internal->lock.Unlock(MUTEX_CONTEXT);
        /* alert the application */
        (ctx->listener->*ctx->callback)(ER_OK, this, *message->GetArg(0), unwrappedContext);
    } else {
        const MsgArg noVal;
        QStatus status = ER_BUS_NO_SUCH_PROPERTY;
        if (message->GetErrorName() != NULL && ::strcmp(message->GetErrorName(), org::alljoyn::Bus::ErrorName) == 0) {
            const char* err;
            uint16_t rawStatus;
            if (message->GetArgs("sq", &err, &rawStatus) == ER_OK) {
                status = static_cast<QStatus>(rawStatus);
                QCC_DbgPrintf(("Asynch GetAllProperties call returned %s", err));
            }
        }
        (ctx->listener->*ctx->callback)(status, this, noVal, unwrappedContext);
    }
    delete wrappedContext;
    delete ctx;
}

QStatus ProxyBusObject::GetAllPropertiesAsync(const char* iface,
                                              ProxyBusObject::Listener* listener,
                                              ProxyBusObject::Listener::GetPropertyCB callback,
                                              void* context,
                                              uint32_t timeout)
{
    QStatus status;
    const InterfaceDescription* valueIface = internal->bus->GetInterface(iface);
    if (!valueIface) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    } else {
        /* If all values are stored in the cache, we can reply immediately */
        bool cached = false;
        MsgArg value;
        internal->lock.Lock(MUTEX_CONTEXT);
        if (internal->cacheProperties) {
            map<qcc::StringMapKey, CachedProps>::iterator it = internal->caches.find(iface);
            if (it != internal->caches.end()) {
                cached = it->second.GetAll(value);
            }
        }
        internal->lock.Unlock(MUTEX_CONTEXT);
        if (cached) {
            QCC_DbgPrintf(("GetAllPropertiesAsync(%s) -> cache hit", iface));
            internal->bus->GetInternal().GetLocalEndpoint()->ScheduleCachedGetPropertyReply(this, listener, callback, context, value);
            return ER_OK;
        }

        QCC_DbgPrintf(("GetAllPropertiesAsync(%s) -> perform method call", iface));
        uint8_t flags = 0;
        /*
         * If the object or the property interface is secure method call must be encrypted.
         */
        if (SecurityApplies(this, valueIface)) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        MsgArg arg = MsgArg("s", iface);
        const InterfaceDescription* propIface = internal->bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
        if (propIface == NULL) {
            status = ER_BUS_NO_SUCH_INTERFACE;
        } else {
            std::pair<void*, qcc::String>* wrappedContext = new std::pair<void*, qcc::String>(context, iface);
            CBContext<Listener::GetAllPropertiesCB>* ctx = new CBContext<Listener::GetAllPropertiesCB>(listener, callback, wrappedContext);
            const InterfaceDescription::Member* getAllProperties = propIface->GetMember("GetAll");
            assert(getAllProperties);
            status = MethodCallAsync(*getAllProperties,
                                     this,
                                     static_cast<MessageReceiver::ReplyHandler>(&ProxyBusObject::GetAllPropsMethodCB),
                                     &arg,
                                     1,
                                     reinterpret_cast<void*>(ctx),
                                     timeout,
                                     flags);
            if (status != ER_OK) {
                delete wrappedContext;
                delete ctx;
            }
        }
    }
    return status;
}

QStatus ProxyBusObject::GetProperty(const char* iface, const char* property, MsgArg& value, uint32_t timeout) const
{
    QStatus status;
    const InterfaceDescription* valueIface = internal->bus->GetInterface(iface);
    if (!valueIface) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    } else {
        /* if the property is cached, we can reply immediately */
        bool cached = false;
        internal->lock.Lock(MUTEX_CONTEXT);
        if (internal->cacheProperties) {
            map<qcc::StringMapKey, CachedProps>::iterator it = internal->caches.find(iface);
            if (it != internal->caches.end()) {
                cached = it->second.Get(property, value);
            }
        }
        internal->lock.Unlock(MUTEX_CONTEXT);
        if (cached) {
            QCC_DbgPrintf(("GetProperty(%s, %s) -> cache hit", iface, property));
            return ER_OK;
        }

        QCC_DbgPrintf(("GetProperty(%s, %s) -> perform method call", iface, property));
        uint8_t flags = 0;
        /*
         * If the object or the property interface is secure method call must be encrypted.
         */
        if (SecurityApplies(this, valueIface)) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        Message reply(*internal->bus);
        MsgArg inArgs[2];
        size_t numArgs = ArraySize(inArgs);
        MsgArg::Set(inArgs, numArgs, "ss", iface, property);
        const InterfaceDescription* propIface = internal->bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
        if (propIface == NULL) {
            status = ER_BUS_NO_SUCH_INTERFACE;
        } else {
            const InterfaceDescription::Member* getProperty = propIface->GetMember("Get");
            assert(getProperty);
            status = MethodCall(*getProperty, inArgs, numArgs, reply, timeout, flags);
            if (ER_OK == status) {
                value = *(reply->GetArg(0));
                /* use the retrieved property value to update the cache, if applicable */
                internal->lock.Lock(MUTEX_CONTEXT);
                if (internal->cacheProperties) {
                    map<qcc::StringMapKey, CachedProps>::iterator it = internal->caches.find(iface);
                    if (it != internal->caches.end()) {
                        it->second.Set(property, value, reply->GetCallSerial());
                    }
                }
                internal->lock.Unlock(MUTEX_CONTEXT);
            }
        }
    }
    return status;
}

void ProxyBusObject::GetPropMethodCB(Message& message, void* context)
{
    CBContext<Listener::GetPropertyCB>* ctx = reinterpret_cast<CBContext<Listener::GetPropertyCB>*>(context);
    std::pair<void*, std::pair<qcc::String, qcc::String> >* wrappedContext = reinterpret_cast<std::pair<void*, std::pair<qcc::String, qcc::String> >*>(ctx->context);
    void* unwrappedContext = wrappedContext->first;
    const char* iface = wrappedContext->second.first.c_str();
    const char* property = wrappedContext->second.second.c_str();

    if (message->GetType() == MESSAGE_METHOD_RET) {
        /* use the retrieved property value to update the cache, if applicable */
        internal->lock.Lock(MUTEX_CONTEXT);
        if (internal->cacheProperties) {
            map<qcc::StringMapKey, CachedProps>::iterator it = internal->caches.find(iface);
            if (it != internal->caches.end()) {
                it->second.Set(property, *message->GetArg(0), message->GetCallSerial());
            }
        }
        internal->lock.Unlock(MUTEX_CONTEXT);
        /* let the application know we've got a result */
        (ctx->listener->*ctx->callback)(ER_OK, this, *message->GetArg(0), unwrappedContext);
    } else {
        const MsgArg noVal;
        QStatus status = ER_BUS_NO_SUCH_PROPERTY;
        if (message->GetErrorName() != NULL && ::strcmp(message->GetErrorName(), org::alljoyn::Bus::ErrorName) == 0) {
            const char* err;
            uint16_t rawStatus;
            if (message->GetArgs("sq", &err, &rawStatus) == ER_OK) {
                status = static_cast<QStatus>(rawStatus);
                QCC_DbgPrintf(("Asynch GetProperty call returned %s", err));
            }
        }
        (ctx->listener->*ctx->callback)(status, this, noVal, unwrappedContext);
    }
    delete ctx;
    delete wrappedContext;
}

QStatus ProxyBusObject::GetPropertyAsync(const char* iface,
                                         const char* property,
                                         ProxyBusObject::Listener* listener,
                                         ProxyBusObject::Listener::GetPropertyCB callback,
                                         void* context,
                                         uint32_t timeout)
{
    QStatus status;
    const InterfaceDescription* valueIface = internal->bus->GetInterface(iface);
    if (!valueIface) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    } else {
        /* if the property is cached, we can reply immediately */
        bool cached = false;
        MsgArg value;
        internal->lock.Lock(MUTEX_CONTEXT);
        if (internal->cacheProperties) {
            map<qcc::StringMapKey, CachedProps>::iterator it = internal->caches.find(iface);
            if (it != internal->caches.end()) {
                cached = it->second.Get(property, value);
            }
        }
        internal->lock.Unlock(MUTEX_CONTEXT);
        if (cached) {
            QCC_DbgPrintf(("GetPropertyAsync(%s, %s) -> cache hit", iface, property));
            internal->bus->GetInternal().GetLocalEndpoint()->ScheduleCachedGetPropertyReply(this, listener, callback, context, value);
            return ER_OK;
        }

        QCC_DbgPrintf(("GetProperty(%s, %s) -> perform method call", iface, property));
        uint8_t flags = 0;
        if (SecurityApplies(this, valueIface)) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        MsgArg inArgs[2];
        size_t numArgs = ArraySize(inArgs);
        MsgArg::Set(inArgs, numArgs, "ss", iface, property);
        const InterfaceDescription* propIface = internal->bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
        if (propIface == NULL) {
            status = ER_BUS_NO_SUCH_INTERFACE;
        } else {
            /* we need to keep track of interface and property name to cache the GetProperty reply */
            std::pair<void*, std::pair<qcc::String, qcc::String> >* wrappedContext = new std::pair<void*, std::pair<qcc::String, qcc::String> >(context, std::make_pair(qcc::String(iface), qcc::String(property)));
            CBContext<Listener::GetPropertyCB>* ctx = new CBContext<Listener::GetPropertyCB>(listener, callback, wrappedContext);
            const InterfaceDescription::Member* getProperty = propIface->GetMember("Get");
            assert(getProperty);
            status = MethodCallAsync(*getProperty,
                                     this,
                                     static_cast<MessageReceiver::ReplyHandler>(&ProxyBusObject::GetPropMethodCB),
                                     inArgs,
                                     numArgs,
                                     reinterpret_cast<void*>(ctx),
                                     timeout,
                                     flags);
            if (status != ER_OK) {
                delete ctx;
                delete wrappedContext;
            }
        }
    }
    return status;
}

QStatus ProxyBusObject::SetProperty(const char* iface, const char* property, MsgArg& value, uint32_t timeout) const
{
    QStatus status;
    const InterfaceDescription* valueIface = internal->bus->GetInterface(iface);
    if (!valueIface) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    } else {
        uint8_t flags = 0;
        /*
         * If the object or the property interface is secure method call must be encrypted.
         */
        if (SecurityApplies(this, valueIface)) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        Message reply(*internal->bus);
        MsgArg inArgs[3];
        size_t numArgs = ArraySize(inArgs);
        MsgArg::Set(inArgs, numArgs, "ssv", iface, property, &value);
        const InterfaceDescription* propIface = internal->bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
        if (propIface == NULL) {
            status = ER_BUS_NO_SUCH_INTERFACE;
        } else {
            const InterfaceDescription::Member* setProperty = propIface->GetMember("Set");
            assert(setProperty);
            status = MethodCall(*setProperty,
                                inArgs,
                                numArgs,
                                reply,
                                timeout,
                                flags);
            if ((status == ER_BUS_REPLY_IS_ERROR_MESSAGE) &&
                (reply->GetErrorName() != NULL) &&
                (::strcmp(reply->GetErrorName(), org::alljoyn::Bus::ErrorName) == 0)) {
                const char* err;
                uint16_t rawStatus;
                if (reply->GetArgs("sq", &err, &rawStatus) == ER_OK) {
                    status = static_cast<QStatus>(rawStatus);
                    QCC_DbgPrintf(("SetProperty call returned %s", err));
                }
            }
        }
    }
    return status;
}

void ProxyBusObject::SetPropMethodCB(Message& message, void* context)
{
    QStatus status = ER_OK;
    CBContext<Listener::SetPropertyCB>* ctx = reinterpret_cast<CBContext<Listener::SetPropertyCB>*>(context);

    if (message->GetType() != MESSAGE_METHOD_RET) {
        status = ER_BUS_NO_SUCH_PROPERTY;
        if (message->GetErrorName() != NULL && ::strcmp(message->GetErrorName(), org::alljoyn::Bus::ErrorName) == 0) {
            const char* err;
            uint16_t rawStatus;
            if (message->GetArgs("sq", &err, &rawStatus) == ER_OK) {
                status = static_cast<QStatus>(rawStatus);
                QCC_DbgPrintf(("Asynch SetProperty call returned %s", err));
            }
        }
    }
    (ctx->listener->*ctx->callback)(status, this, ctx->context);
    delete ctx;
}


QStatus ProxyBusObject::RegisterPropertiesChangedListener(const char* iface,
                                                          const char** properties,
                                                          size_t propertiesSize,
                                                          ProxyBusObject::PropertiesChangedListener& listener,
                                                          void* context)
{
    QCC_DbgTrace(("ProxyBusObject::RegisterPropertiesChangedListener(this = %p, iface = %s, properties = %p, propertiesSize = %u, listener = %p, context = %p",
                  this, iface, properties, propertiesSize, &listener, context));
    const InterfaceDescription* ifc = internal->bus->GetInterface(iface);
    if (!ifc) {
        return ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    }
    for (size_t i  = 0; i < propertiesSize; ++i) {
        if (!ifc->HasProperty(properties[i])) {
            return ER_BUS_NO_SUCH_PROPERTY;
        }
    }

    bool replace = false;
    String ifaceStr = iface;
    PropertiesChangedCB ctx(listener, properties, propertiesSize, context);
    pair<StringMapKey, PropertiesChangedCB> cbItem(ifaceStr, ctx);
    internal->lock.Lock(MUTEX_CONTEXT);
    // remove old version first
    multimap<StringMapKey, PropertiesChangedCB>::iterator it = internal->propertiesChangedCBs.lower_bound(iface);
    multimap<StringMapKey, PropertiesChangedCB>::iterator end = internal->propertiesChangedCBs.upper_bound(iface);
    while (it != end) {
        PropertiesChangedCB propChangedCb = it->second;
        if (&propChangedCb->listener == &listener) {
            propChangedCb->isRegistered = false;
            internal->propertiesChangedCBs.erase(it);
            replace = true;
            break;
        }
        ++it;
    }
    internal->propertiesChangedCBs.insert(cbItem);
    internal->lock.Unlock(MUTEX_CONTEXT);

    QStatus status = ER_OK;
    if (!replace) {
        if (internal->uniqueName.empty()) {
            internal->uniqueName = internal->bus->GetNameOwner(internal->serviceName.c_str());
        }
        internal->AddPropertiesChangedRule(iface, true);
    }

    return status;
}

QStatus ProxyBusObject::UnregisterPropertiesChangedListener(const char* iface,
                                                            ProxyBusObject::PropertiesChangedListener& listener)
{
    QCC_DbgTrace(("ProxyBusObject::UnregisterPropertiesChangedListener(iface = %s, listener = %p)", iface, &listener));
    if (!internal->bus->GetInterface(iface)) {
        return ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    }

    String ifaceStr = iface;
    bool removed = false;

    internal->lock.Lock(MUTEX_CONTEXT);
    map<qcc::Thread*, _PropertiesChangedCB*>::iterator thread = internal->handlerThreads.find(Thread::GetThread());
    if (thread != internal->handlerThreads.end() && (&thread->second->listener == &listener)) {
        QCC_LogError(ER_DEADLOCK, ("Attempt to unregister listener from said listener would cause deadlock"));
        internal->lock.Unlock(MUTEX_CONTEXT);
        return ER_DEADLOCK;
    }

    multimap<StringMapKey, PropertiesChangedCB>::iterator it = internal->propertiesChangedCBs.lower_bound(iface);
    multimap<StringMapKey, PropertiesChangedCB>::iterator end = internal->propertiesChangedCBs.upper_bound(iface);
    while (it != end) {
        PropertiesChangedCB ctx = it->second;
        if (&ctx->listener == &listener) {
            ctx->isRegistered = false;
            internal->propertiesChangedCBs.erase(it);
            removed = true;

            while (ctx->numRunning > 0) {
                /*
                 * Some thread is trying to remove listeners while the listeners are
                 * being called.  Wait until the listener callbacks are done first.
                 */
                internal->listenerDone.Wait(internal->lock);
            }

            break;
        }
        ++it;
    }
    internal->lock.Unlock(MUTEX_CONTEXT);

    QStatus status = ER_OK;
    if (removed) {
        internal->RemovePropertiesChangedRule(iface);
    }

    return status;
}

void ProxyBusObject::Internal::PropertiesChangedHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& message)
{
    QCC_UNUSED(member);
    QCC_UNUSED(srcPath);

    QCC_DbgTrace(("Internal::PropertiesChangedHandler(this = %p, member = %s, srcPath = %s, message = <>)", this, member->name.c_str(), srcPath));

    const char* ifaceName;
    MsgArg* changedProps;
    size_t numChangedProps;
    MsgArg* invalidProps;
    size_t numInvalidProps;

    if (uniqueName != message->GetSender()) {
        QCC_LogError(ER_FAIL, ("message not for us? (uniqueName = %s   sender = %s)", uniqueName.c_str(), message->GetSender()));
        return;
    }

    QStatus status = message->GetArgs("sa{sv}as", &ifaceName, &numChangedProps, &changedProps, &numInvalidProps, &invalidProps);
    if (status != ER_OK) {
        QCC_LogError(status, ("invalid message args"));
        return;
    }

    lock.Lock(MUTEX_CONTEXT);
    /* first, update caches */
    if (cacheProperties) {
        map<StringMapKey, CachedProps>::iterator it = caches.find(ifaceName);
        if (it != caches.end()) {
            it->second.PropertiesChanged(changedProps, numChangedProps, invalidProps, numInvalidProps, message->GetCallSerial());
        }
    }

    /* then, alert listeners */
    handlerThreads[Thread::GetThread()] = nullptr;
    multimap<StringMapKey, PropertiesChangedCB>::iterator it = propertiesChangedCBs.lower_bound(ifaceName);
    multimap<StringMapKey, PropertiesChangedCB>::iterator end = propertiesChangedCBs.upper_bound(ifaceName);
    list<PropertiesChangedCB> handlers;
    while (it != end) {
        if (it->second->isRegistered) {
            handlers.push_back(it->second);
        }
        ++it;
    }
    lock.Unlock(MUTEX_CONTEXT);

    size_t i;
    MsgArg changedOut;
    MsgArg* changedOutDict = (numChangedProps > 0) ? new MsgArg[numChangedProps] : NULL;
    size_t changedOutDictSize;
    MsgArg invalidOut;
    const char** invalidOutArray = (numInvalidProps > 0) ? new const char*[numInvalidProps] : NULL;
    size_t invalidOutArraySize;

    while (handlers.begin() != handlers.end()) {
        PropertiesChangedCB ctx = *handlers.begin();

        lock.Lock(MUTEX_CONTEXT);
        bool isRegistered = ctx->isRegistered;
        handlerThreads[Thread::GetThread()] = ctx.unwrap();
        ++ctx->numRunning;
        lock.Unlock(MUTEX_CONTEXT);

        if (isRegistered) {
            changedOutDictSize = 0;
            invalidOutArraySize = 0;

            if (ctx->properties.empty()) {
                // handler wants all changed/invalid properties in signal
                changedOut.Set("a{sv}", numChangedProps, changedProps);
                changedOutDictSize = numChangedProps;
                for (i = 0; i < numInvalidProps; ++i) {
                    const char* propName;
                    invalidProps[i].Get("s", &propName);
                    invalidOutArray[invalidOutArraySize++] = propName;

                }
                invalidOut.Set("as", numInvalidProps, invalidOutArray);
            } else {
                for (i = 0; i < numChangedProps; ++i) {
                    const char* propName;
                    MsgArg* propValue;
                    changedProps[i].Get("{sv}", &propName, &propValue);
                    if (ctx->properties.find(propName) != ctx->properties.end()) {
                        changedOutDict[changedOutDictSize++].Set("{sv}", propName, propValue);
                    }
                }
                if (changedOutDictSize > 0) {
                    changedOut.Set("a{sv}", changedOutDictSize, changedOutDict);
                } else {
                    changedOut.Set("a{sv}", 0, NULL);
                }

                for (i = 0; i < numInvalidProps; ++i) {
                    const char* propName;
                    invalidProps[i].Get("s", &propName);
                    if (ctx->properties.find(propName) != ctx->properties.end()) {
                        invalidOutArray[invalidOutArraySize++] = propName;
                    }
                }
                if (invalidOutArraySize > 0) {
                    invalidOut.Set("as", invalidOutArraySize, invalidOutArray);
                } else {
                    invalidOut.Set("as", 0, NULL);
                }
            }

            // only call listener if anything to report
            if ((changedOutDictSize > 0) || (invalidOutArraySize > 0)) {
                ProxyBusObject pbo(ManagedObj<ProxyBusObject::Internal>::wrap(this));
                ctx->listener.PropertiesChanged(pbo, ifaceName, changedOut, invalidOut, ctx->context);
            }
        }
        handlers.pop_front();

        lock.Lock(MUTEX_CONTEXT);
        --ctx->numRunning;
        handlerThreads[Thread::GetThread()] = nullptr;
        listenerDone.Broadcast();
        lock.Unlock(MUTEX_CONTEXT);
    }

    delete [] changedOutDict;
    delete [] invalidOutArray;

    lock.Lock(MUTEX_CONTEXT);
    handlerThreads.erase(Thread::GetThread());;
    handlerDone.Signal();
    lock.Unlock(MUTEX_CONTEXT);
}

QStatus ProxyBusObject::SetPropertyAsync(const char* iface,
                                         const char* property,
                                         MsgArg& value,
                                         ProxyBusObject::Listener* listener,
                                         ProxyBusObject::Listener::SetPropertyCB callback,
                                         void* context,
                                         uint32_t timeout)
{
    QStatus status;
    const InterfaceDescription* valueIface = internal->bus->GetInterface(iface);
    if (!valueIface) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    } else {
        uint8_t flags = 0;
        if (SecurityApplies(this, valueIface)) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        MsgArg inArgs[3];
        size_t numArgs = ArraySize(inArgs);
        MsgArg::Set(inArgs, numArgs, "ssv", iface, property, &value);
        const InterfaceDescription* propIface = internal->bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
        if (propIface == NULL) {
            status = ER_BUS_NO_SUCH_INTERFACE;
        } else {
            CBContext<Listener::SetPropertyCB>* ctx = new CBContext<Listener::SetPropertyCB>(listener, callback, context);
            const InterfaceDescription::Member* setProperty = propIface->GetMember("Set");
            assert(setProperty);
            status = MethodCallAsync(*setProperty,
                                     this,
                                     static_cast<MessageReceiver::ReplyHandler>(&ProxyBusObject::SetPropMethodCB),
                                     inArgs,
                                     numArgs,
                                     reinterpret_cast<void*>(ctx),
                                     timeout,
                                     flags);
            if (status != ER_OK) {
                delete ctx;
            }
        }
    }
    return status;
}

size_t ProxyBusObject::GetInterfaces(const InterfaceDescription** ifaces, size_t numIfaces) const
{
    internal->lock.Lock(MUTEX_CONTEXT);
    size_t count = internal->ifaces.size();
    if (ifaces) {
        count = min(count, numIfaces);
        map<qcc::StringMapKey, const InterfaceDescription*>::const_iterator it = internal->ifaces.begin();
        for (size_t i = 0; i < count && it != internal->ifaces.end(); ++i, ++it) {
            ifaces[i] = it->second;
        }
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return count;
}

const InterfaceDescription* ProxyBusObject::GetInterface(const char* ifaceName) const
{
    StringMapKey key = ifaceName;
    internal->lock.Lock(MUTEX_CONTEXT);
    map<StringMapKey, const InterfaceDescription*>::const_iterator it = internal->ifaces.find(key);
    const InterfaceDescription* ret = (it == internal->ifaces.end()) ? NULL : it->second;
    internal->lock.Unlock(MUTEX_CONTEXT);
    return ret;
}


QStatus ProxyBusObject::AddInterface(const InterfaceDescription& iface) {
    StringMapKey key(qcc::String(iface.GetName()));
    pair<StringMapKey, const InterfaceDescription*> item(key, &iface);
    internal->lock.Lock(MUTEX_CONTEXT);

    pair<map<StringMapKey, const InterfaceDescription*>::const_iterator, bool> ret = internal->ifaces.insert(item);
    QStatus status = ret.second ? ER_OK : ER_BUS_IFACE_ALREADY_EXISTS;

    if ((status == ER_OK) && internal->cacheProperties && iface.HasCacheableProperties()) {
        internal->caches.insert(std::make_pair(key, CachedProps(&iface)));
        /* add match rules in case the PropertiesChanged signals are emitted as global broadcast */
        internal->AddPropertiesChangedRule(iface.GetName(), false);
    }

    if ((status == ER_OK) && !internal->hasProperties) {
        const InterfaceDescription* propIntf = internal->bus->GetInterface(::ajn::org::freedesktop::DBus::Properties::InterfaceName);
        assert(propIntf);
        if (iface == *propIntf) {
            internal->hasProperties = true;
        } else if (iface.GetProperties() > 0) {
            AddInterface(*propIntf);
        }
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return status;
}


QStatus ProxyBusObject::AddInterface(const char* ifaceName)
{
    const InterfaceDescription* iface = internal->bus->GetInterface(ifaceName);
    if (!iface) {
        return ER_BUS_NO_SUCH_INTERFACE;
    } else {
        return AddInterface(*iface);
    }
}

void ProxyBusObject::Internal::AddPropertiesChangedRule(const char* intf, bool blocking)
{
    QCC_DbgTrace(("%s(%s)", __FUNCTION__, intf));
    lock.Lock(MUTEX_CONTEXT);
    /* make sure we have the signal handler online */
    if (!registeredPropChangedHandler) {
        QCC_DbgPrintf(("Registering signal handler"));
        const InterfaceDescription* propIntf = bus->GetInterface(::ajn::org::freedesktop::DBus::Properties::InterfaceName);
        assert(propIntf);
        bus->RegisterSignalHandler(this,
                                   static_cast<MessageReceiver::SignalHandler>(&Internal::PropertiesChangedHandler),
                                   propIntf->GetMember("PropertiesChanged"),
                                   path.c_str());
        registeredPropChangedHandler = true;
    }

    std::map<qcc::StringMapKey, int>::iterator it = matchRuleRefcounts.find(intf);
    if (it == matchRuleRefcounts.end()) {
        QCC_DbgPrintf(("Adding match rule"));
        String rule = String("type='signal',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',arg0='") + intf + "'";
        if (blocking) {
            bus->AddMatch(rule.c_str());
        } else {
            bus->AddMatchNonBlocking(rule.c_str());
        }
        matchRuleRefcounts[qcc::String(intf)] = 1;
    } else {
        it->second++;
    }
    lock.Unlock(MUTEX_CONTEXT);
}

void ProxyBusObject::Internal::RemovePropertiesChangedRule(const char* intf)
{
    lock.Lock(MUTEX_CONTEXT);
    std::map<qcc::StringMapKey, int>::iterator it = matchRuleRefcounts.find(intf);
    if (it != matchRuleRefcounts.end()) {
        if (--it->second == 0) {
            String rule = String("type='signal',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',arg0='") + intf + "'";
            bus->RemoveMatchNonBlocking(rule.c_str());
            matchRuleRefcounts.erase(it);
        }
    }
    lock.Unlock(MUTEX_CONTEXT);
}

void ProxyBusObject::Internal::RemoveAllPropertiesChangedRules()
{
    lock.Lock(MUTEX_CONTEXT);
    std::map<qcc::StringMapKey, int>::iterator it = matchRuleRefcounts.begin();
    for (; it != matchRuleRefcounts.end(); ++it) {
        String rule = String("type='signal',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',arg0='") + it->first.c_str() + "'";
        bus->RemoveMatchNonBlocking(rule.c_str());
    }
    matchRuleRefcounts.clear();
    lock.Unlock(MUTEX_CONTEXT);
}

bool ProxyBusObject::IsValid() const {
    return internal->bus != nullptr;
}

bool ProxyBusObject::IsSecure() const {
    return internal->isSecure;
}

void ProxyBusObject::EnablePropertyCaching()
{
    internal->lock.Lock(MUTEX_CONTEXT);
    if (!internal->cacheProperties) {
        internal->cacheProperties = true;
        map<StringMapKey, const InterfaceDescription*>::const_iterator it = internal->ifaces.begin();
        for (; it != internal->ifaces.end(); ++it) {
            if (it->second->HasCacheableProperties()) {
                internal->caches.insert(std::make_pair(qcc::String(it->first.c_str()), CachedProps(it->second)));
                /* add match rules in case the PropertiesChanged signals are emitted as global broadcast */
                internal->AddPropertiesChangedRule(it->first.c_str(), false);
            }
        }
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
}

size_t ProxyBusObject::GetChildren(ProxyBusObject** children, size_t numChildren)
{
    internal->lock.Lock(MUTEX_CONTEXT);
    size_t count = internal->children.size();
    if (children) {
        count = min(count, numChildren);
        for (size_t i = 0; i < count; i++) {
            children[i] = &(internal->children[i]);
        }
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return count;
}

size_t ProxyBusObject::GetManagedChildren(void* children, size_t numChildren)
{
    /*
     * Use ManagedObj<ProxyBusObject> rather than _ProxyBusObject to avoid
     * deprecation warnings/errors.
     */
    ManagedObj<ProxyBusObject>** pboChildren = reinterpret_cast<ManagedObj<ProxyBusObject>**>(children);
    internal->lock.Lock(MUTEX_CONTEXT);
    size_t count = internal->children.size();
    if (pboChildren) {
        count = min(count, numChildren);
        for (size_t i = 0; i < count; i++) {
            pboChildren[i] = new ManagedObj<ProxyBusObject>((internal->children)[i]);
        }
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return count;
}

ProxyBusObject* ProxyBusObject::GetChild(const char* inPath)
{
    /* Add a trailing slash to this path */
    qcc::String pathSlash = (internal->path == "/") ? internal->path : internal->path + '/';

    /* Create absolute version of inPath */
    qcc::String inPathStr = ('/' == inPath[0]) ? inPath : pathSlash + inPath;

    /* Sanity check to make sure path is possible */
    if ((0 != inPathStr.find(pathSlash)) || (inPathStr[inPathStr.length() - 1] == '/')) {
        return NULL;
    }

    /* Find each path element as a child within the parent's vector of children */
    size_t idx = internal->path.size() + 1;
    ProxyBusObject* cur = this;
    internal->lock.Lock(MUTEX_CONTEXT);
    while (idx != qcc::String::npos) {
        size_t end = inPathStr.find_first_of('/', idx);
        qcc::String item = inPathStr.substr(0, end);
        vector<ProxyBusObject>& ch = cur->internal->children;
        vector<ProxyBusObject>::iterator it = ch.begin();
        while (it != ch.end()) {
            if (it->GetPath() == item) {
                cur = &(*it);
                break;
            }
            ++it;
        }
        if (it == ch.end()) {
            internal->lock.Unlock(MUTEX_CONTEXT);
            return NULL;
        }
        idx = ((qcc::String::npos == end) || ((end + 1) == inPathStr.size())) ? qcc::String::npos : end + 1;
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return cur;
}

void* ProxyBusObject::GetManagedChild(const char* inPath)
{
    ProxyBusObject* ch = GetChild(inPath);
    if (ch) {
        return new ManagedObj<ProxyBusObject>(*ch);
    }
    return nullptr;
}

QStatus ProxyBusObject::AddChild(const ProxyBusObject& child)
{
    qcc::String childPath = child.GetPath();

    /* Sanity check to make sure path is possible */
    if (((internal->path.size() > 1) && (0 != childPath.find(internal->path + '/'))) ||
        ((internal->path.size() == 1) && (childPath[0] != '/')) ||
        (childPath[childPath.length() - 1] == '/')) {
        return ER_BUS_BAD_CHILD_PATH;
    }

    /* Find each path element as a child within the parent's vector of children */
    /* Add new children as necessary */
    size_t idx = internal->path.size() + 1;
    ProxyBusObject* cur = this;
    internal->lock.Lock(MUTEX_CONTEXT);
    while (idx != qcc::String::npos) {
        size_t end = childPath.find_first_of('/', idx);
        qcc::String item = childPath.substr(0, end);
        vector<ProxyBusObject>& ch = cur->internal->children;
        vector<ProxyBusObject>::iterator it = ch.begin();
        while (it != ch.end()) {
            if (it->GetPath() == item) {
                cur = &(*it);
                break;
            }
            ++it;
        }
        if (it == ch.end()) {
            if (childPath == item) {
                ch.push_back(child);
                internal->lock.Unlock(MUTEX_CONTEXT);
                return ER_OK;
            } else {
                const char* tempServiceName = internal->serviceName.c_str();
                const char* tempPath = item.c_str();
                const char* tempUniqueName = internal->uniqueName.c_str();
                ProxyBusObject ro(*internal->bus, tempServiceName, tempUniqueName, tempPath, internal->sessionId);
                ch.push_back(ro);
                cur = &ro;
            }
        }
        idx = ((qcc::String::npos == end) || ((end + 1) == childPath.size())) ? qcc::String::npos : end + 1;
    }
    internal->lock.Unlock(MUTEX_CONTEXT);
    return ER_BUS_OBJ_ALREADY_EXISTS;
}

QStatus ProxyBusObject::RemoveChild(const char* inPath)
{
    QStatus status;

    /* Add a trailing slash to this path */
    qcc::String pathSlash = (internal->path == "/") ? internal->path : internal->path + '/';

    /* Create absolute version of inPath */
    qcc::String childPath = ('/' == inPath[0]) ? inPath : pathSlash + inPath;

    /* Sanity check to make sure path is possible */
    if ((0 != childPath.find(pathSlash)) || (childPath[childPath.length() - 1] == '/')) {
        return ER_BUS_BAD_CHILD_PATH;
    }

    /* Navigate to child and remove it */
    size_t idx = internal->path.size() + 1;
    ProxyBusObject* cur = this;
    internal->lock.Lock(MUTEX_CONTEXT);
    while (idx != qcc::String::npos) {
        size_t end = childPath.find_first_of('/', idx);
        qcc::String item = childPath.substr(0, end);
        vector<ProxyBusObject>& ch = cur->internal->children;
        vector<ProxyBusObject>::iterator it = ch.begin();
        while (it != ch.end()) {
            if (it->GetPath() == item) {
                if (end == qcc::String::npos) {
                    ch.erase(it);
                    internal->lock.Unlock(MUTEX_CONTEXT);
                    return ER_OK;
                } else {
                    cur = &(*it);
                    break;
                }
            }
            ++it;
        }
        if (it == ch.end()) {
            status = ER_BUS_OBJ_NOT_FOUND;
            internal->lock.Unlock(MUTEX_CONTEXT);
            QCC_LogError(status, ("Cannot find object path %s", item.c_str()));
            return status;
        }
        idx = ((qcc::String::npos == end) || ((end + 1) == childPath.size())) ? qcc::String::npos : end + 1;
    }
    /* Shouldn't get here */
    internal->lock.Unlock(MUTEX_CONTEXT);
    return ER_FAIL;
}



QStatus ProxyBusObject::MethodCallAsync(const InterfaceDescription::Member& method,
                                        MessageReceiver* receiver,
                                        MessageReceiver::ReplyHandler replyHandler,
                                        const MsgArg* args,
                                        size_t numArgs,
                                        void* context,
                                        uint32_t timeout,
                                        uint8_t flags) const
{

    QStatus status;
    Message msg(*internal->bus);
    LocalEndpoint localEndpoint = internal->bus->GetInternal().GetLocalEndpoint();
    if (!localEndpoint->IsValid()) {
        return ER_BUS_ENDPOINT_CLOSING;
    }
    /*
     * This object must implement the interface for this method
     */
    if (!ImplementsInterface(method.iface->GetName())) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Object %s does not implement %s", internal->path.c_str(), method.iface->GetName()));
        return status;
    }
    if (!replyHandler) {
        flags |= ALLJOYN_FLAG_NO_REPLY_EXPECTED;
    }
    /*
     * If the interface is secure or encryption is explicitly requested the method call must be encrypted.
     */
    if (SecurityApplies(this, method.iface)) {
        flags |= ALLJOYN_FLAG_ENCRYPTED;
    }
    if ((flags & ALLJOYN_FLAG_ENCRYPTED) && !internal->bus->IsPeerSecurityEnabled()) {
        return ER_BUS_SECURITY_NOT_ENABLED;
    }
    status = msg->CallMsg(method.signature, internal->serviceName, internal->sessionId, internal->path, method.iface->GetName(), method.name, args, numArgs, flags);
    if (status == ER_OK) {
        if (!(flags & ALLJOYN_FLAG_NO_REPLY_EXPECTED)) {
            status = localEndpoint->RegisterReplyHandler(receiver, replyHandler, method, msg, context, timeout);
        }
        if (status == ER_OK) {
            if (internal->b2bEp->IsValid()) {
                status = internal->b2bEp->PushMessage(msg);
            } else {
                BusEndpoint busEndpoint = BusEndpoint::cast(localEndpoint);
                status = internal->bus->GetInternal().GetRouter().PushMessage(msg, busEndpoint);
            }
            if (status != ER_OK) {
                bool unregistered = localEndpoint->UnregisterReplyHandler(msg);
                if (!unregistered) {
                    /*
                     * Unregister failed, so the reply handler must have already been called.
                     *
                     * The contract of this function is that the reply handler will be called iff
                     * the status is ER_OK, so set the status to ER_OK to indicate that the reply
                     * handler was called.
                     */
                    status = ER_OK;
                }
            }
        }
    }
    return status;
}

QStatus ProxyBusObject::MethodCallAsync(const char* ifaceName,
                                        const char* methodName,
                                        MessageReceiver* receiver,
                                        MessageReceiver::ReplyHandler replyHandler,
                                        const MsgArg* args,
                                        size_t numArgs,
                                        void* context,
                                        uint32_t timeout,
                                        uint8_t flags) const
{
    internal->lock.Lock(MUTEX_CONTEXT);
    map<StringMapKey, const InterfaceDescription*>::const_iterator it = internal->ifaces.find(StringMapKey(ifaceName));
    if (it == internal->ifaces.end()) {
        internal->lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_NO_SUCH_INTERFACE;
    }
    const InterfaceDescription::Member* member = it->second->GetMember(methodName);
    internal->lock.Unlock(MUTEX_CONTEXT);
    if (NULL == member) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }
    return MethodCallAsync(*member, receiver, replyHandler, args, numArgs, context, timeout, flags);
}

QStatus ProxyBusObject::MethodCall(const InterfaceDescription::Member& method,
                                   const MsgArg* args,
                                   size_t numArgs,
                                   Message& replyMsg,
                                   uint32_t timeout,
                                   uint8_t flags,
                                   Message* callMsg) const
{
    QStatus status;
    Message msg(*internal->bus);
    LocalEndpoint localEndpoint = internal->bus->GetInternal().GetLocalEndpoint();
    if (!localEndpoint->IsValid()) {
        return ER_BUS_ENDPOINT_CLOSING;
    }
    /*
     * if we're being called from the LocalEndpoint (callback) thread, do not allow
     * blocking calls unless BusAttachment::EnableConcurrentCallbacks has been called first
     */
    bool isDaemon = internal->bus->GetInternal().GetRouter().IsDaemon();
    if (localEndpoint->IsReentrantCall() && !isDaemon) {
        status = ER_BUS_BLOCKING_CALL_NOT_ALLOWED;
        goto MethodCallExit;
    }
    /*
     * This object must implement the interface for this method
     */
    if (!ImplementsInterface(method.iface->GetName())) {
        status = ER_BUS_OBJECT_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Object %s does not implement %s", internal->path.c_str(), method.iface->GetName()));
        goto MethodCallExit;
    }
    /*
     * If the object or interface is secure or encryption is explicitly requested the method call must be encrypted.
     */
    if (SecurityApplies(this, method.iface)) {
        flags |= ALLJOYN_FLAG_ENCRYPTED;
    }
    if ((flags & ALLJOYN_FLAG_ENCRYPTED) && !internal->bus->IsPeerSecurityEnabled()) {
        status = ER_BUS_SECURITY_NOT_ENABLED;
        goto MethodCallExit;
    }
    status = msg->CallMsg(method.signature, internal->serviceName, internal->sessionId, internal->path, method.iface->GetName(), method.name, args, numArgs, flags);
    if (status != ER_OK) {
        goto MethodCallExit;
    }
    /*
     * If caller asked for a copy of the sent message, copy it now that we've successfully created it.
     */
    if (NULL != callMsg) {
        *callMsg = msg;
    }
    if (flags & ALLJOYN_FLAG_NO_REPLY_EXPECTED) {
        /*
         * Push the message to the router and we are done
         */
        if (internal->b2bEp->IsValid()) {
            status = internal->b2bEp->PushMessage(msg);
        } else {
            BusEndpoint busEndpoint = BusEndpoint::cast(localEndpoint);
            status = internal->bus->GetInternal().GetRouter().PushMessage(msg, busEndpoint);
        }
    } else {
        SyncReplyContext ctxt(*internal->bus);
        /*
         * Synchronous calls are really asynchronous calls that block waiting for a builtin
         * reply handler to be called.
         */
        SyncReplyContext* heapCtx = new SyncReplyContext(ctxt);
        status = localEndpoint->RegisterReplyHandler(const_cast<MessageReceiver*>(static_cast<const MessageReceiver* const>(this)),
                                                     static_cast<MessageReceiver::ReplyHandler>(&ProxyBusObject::SyncReplyHandler),
                                                     method,
                                                     msg,
                                                     heapCtx,
                                                     timeout);
        if (status != ER_OK) {
            delete heapCtx;
            heapCtx = NULL;
            goto MethodCallExit;
        }

        if (internal->b2bEp->IsValid()) {
            status = internal->b2bEp->PushMessage(msg);
        } else {
            BusEndpoint busEndpoint = BusEndpoint::cast(localEndpoint);
            status = internal->bus->GetInternal().GetRouter().PushMessage(msg, busEndpoint);
        }

        Thread* thisThread = Thread::GetThread();
        if (status == ER_OK) {
            internal->lock.Lock(MUTEX_CONTEXT);
            if (!isExiting) {
                internal->syncMethodCalls[this].insert(ctxt);
                internal->lock.Unlock(MUTEX_CONTEXT);
                /*
                 * In case of a timeout, the SyncReplyHandler will be called by
                 * the LocalEndpoint replyTimer. So wait forever to be signaled
                 * by the SyncReplyHandler or the ProxyBusObject destructor (in
                 * case the ProxyBusObject is being destroyed) or this thread is
                 * stopped.
                 */
                status = Event::Wait(ctxt->event);
                internal->lock.Lock(MUTEX_CONTEXT);

                internal->syncMethodCalls[this].erase(ctxt);
                internal->syncMethodComplete.Broadcast();
            } else {
                status = ER_BUS_STOPPING;
            }
            internal->lock.Unlock(MUTEX_CONTEXT);
        }

        if (status == ER_OK) {
            replyMsg = ctxt->replyMsg;
        } else if ((status == ER_ALERTED_THREAD) && (SYNC_METHOD_ALERTCODE_ABORT == thisThread->GetAlertCode())) {
            thisThread->ResetAlertCode();
            /*
             * We can't touch anything in this case since the external thread that was waiting
             * can't know whether this object still exists.
             */
            status = ER_BUS_METHOD_CALL_ABORTED;
        } else if (localEndpoint->UnregisterReplyHandler(msg)) {
            /*
             * The handler was deregistered so we need to delete the context here.
             */
            delete heapCtx;
        }
        if (status == ER_ALERTED_THREAD) {
            thisThread->ResetAlertCode();
        }
    }

MethodCallExit:
    /*
     * Let caller know that the method call reply was an error message
     */
    if (status == ER_OK) {
        if (replyMsg->GetType() == MESSAGE_ERROR) {
            status = ER_BUS_REPLY_IS_ERROR_MESSAGE;
        } else if (replyMsg->GetType() == MESSAGE_INVALID && !(flags & ALLJOYN_FLAG_NO_REPLY_EXPECTED)) {
            status = ER_FAIL;
        }
    } else {
        /*
         * We should not need to duplicate the status information into a synthesized
         * replyMessage.  However 14.12 and prior behaved this way, so preserve the
         * existing behavior.
         */
        String sender;
        if (internal->bus->IsStarted()) {
            sender = internal->bus->GetInternal().GetLocalEndpoint()->GetUniqueName();
        }
        replyMsg->ErrorMsg(sender, status, 0);
    }

    if ((status == ER_OK) && internal->uniqueName.empty()) {
        internal->uniqueName = replyMsg->GetSender();
    }
    return status;
}

QStatus ProxyBusObject::MethodCall(const char* ifaceName,
                                   const char* methodName,
                                   const MsgArg* args,
                                   size_t numArgs,
                                   Message& replyMsg,
                                   uint32_t timeout,
                                   uint8_t flags) const
{
    internal->lock.Lock(MUTEX_CONTEXT);
    map<StringMapKey, const InterfaceDescription*>::const_iterator it = internal->ifaces.find(StringMapKey(ifaceName));
    if (it == internal->ifaces.end()) {
        internal->lock.Unlock(MUTEX_CONTEXT);
        return ER_BUS_NO_SUCH_INTERFACE;
    }
    const InterfaceDescription::Member* member = it->second->GetMember(methodName);
    internal->lock.Unlock(MUTEX_CONTEXT);
    if (NULL == member) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }
    return MethodCall(*member, args, numArgs, replyMsg, timeout, flags);
}

void ProxyBusObject::SetSecure(bool isSecure) {
    internal->isSecure = isSecure;
}

void ProxyBusObject::SyncReplyHandler(Message& msg, void* context)
{
    if (context != NULL) {
        SyncReplyContext* ctx = reinterpret_cast<SyncReplyContext*> (context);

        /* Set the reply message */
        (*ctx)->replyMsg = msg;

        /* Wake up sync method_call thread */
        QStatus status = (*ctx)->event.SetEvent();
        if (ER_OK != status) {
            QCC_LogError(status, ("SetEvent failed"));
        }
        delete ctx;
    }
}

QStatus ProxyBusObject::SecureConnection(bool forceAuth)
{
    if (!internal->bus->IsPeerSecurityEnabled()) {
        return ER_BUS_SECURITY_NOT_ENABLED;
    }
    LocalEndpoint localEndpoint = internal->bus->GetInternal().GetLocalEndpoint();
    if (!localEndpoint->IsValid()) {
        return ER_BUS_ENDPOINT_CLOSING;
    } else {
        AllJoynPeerObj* peerObj = localEndpoint->GetPeerObj();
        if (forceAuth) {
            peerObj->ForceAuthentication(internal->serviceName);
        }
        return peerObj->AuthenticatePeer(MESSAGE_METHOD_CALL, internal->serviceName);
    }
}

QStatus ProxyBusObject::SecureConnectionAsync(bool forceAuth)
{
    if (!internal->bus->IsPeerSecurityEnabled()) {
        return ER_BUS_SECURITY_NOT_ENABLED;
    }
    LocalEndpoint localEndpoint = internal->bus->GetInternal().GetLocalEndpoint();
    if (!localEndpoint->IsValid()) {
        return ER_BUS_ENDPOINT_CLOSING;
    } else {
        AllJoynPeerObj* peerObj =  localEndpoint->GetPeerObj();
        if (forceAuth) {
            peerObj->ForceAuthentication(internal->serviceName);
        }
        return peerObj->AuthenticatePeerAsync(internal->serviceName);
    }
}

const qcc::String& ProxyBusObject::GetPath(void) const {
    return internal->path;
}

const qcc::String& ProxyBusObject::GetServiceName(void) const {
    return internal->serviceName;
}

const qcc::String& ProxyBusObject::GetUniqueName(void) const {
    return internal->uniqueName;
}

SessionId ProxyBusObject::GetSessionId(void) const {
    return internal->sessionId;
}

QStatus ProxyBusObject::IntrospectRemoteObject(uint32_t timeout)
{
    /* Need to have introspectable interface in order to call Introspect */
    const InterfaceDescription* introIntf = GetInterface(org::freedesktop::DBus::Introspectable::InterfaceName);
    if (!introIntf) {
        introIntf = internal->bus->GetInterface(org::freedesktop::DBus::Introspectable::InterfaceName);
        assert(introIntf);
        AddInterface(*introIntf);
    }

    /* Attempt to retrieve introspection from the remote object using sync call */
    Message reply(*internal->bus);
    const InterfaceDescription::Member* introMember = introIntf->GetMember("Introspect");
    assert(introMember);
    QStatus status = MethodCall(*introMember, NULL, 0, reply, timeout);

    /* Parse the XML reply */
    if (ER_OK == status) {
        QCC_DbgPrintf(("Introspection XML: %s\n", reply->GetArg(0)->v_string.str));
        qcc::String ident = reply->GetSender();
        if (internal->uniqueName.empty()) {
            internal->uniqueName = ident;
        }
        ident += " : ";
        ident += reply->GetObjectPath();
        status = ParseXml(reply->GetArg(0)->v_string.str, ident.c_str());
    }
    return status;
}

QStatus ProxyBusObject::IntrospectRemoteObjectAsync(ProxyBusObject::Listener* listener,
                                                    ProxyBusObject::Listener::IntrospectCB callback,
                                                    void* context,
                                                    uint32_t timeout)
{
    /* Need to have introspectable interface in order to call Introspect */
    const InterfaceDescription* introIntf = GetInterface(org::freedesktop::DBus::Introspectable::InterfaceName);
    if (!introIntf) {
        introIntf = internal->bus->GetInterface(org::freedesktop::DBus::Introspectable::InterfaceName);
        assert(introIntf);
        AddInterface(*introIntf);
    }

    /* Attempt to retrieve introspection from the remote object using async call */
    const InterfaceDescription::Member* introMember = introIntf->GetMember("Introspect");
    assert(introMember);
    CBContext<Listener::IntrospectCB>* ctx = new CBContext<Listener::IntrospectCB>(listener, callback, context);
    QStatus status = MethodCallAsync(*introMember,
                                     this,
                                     static_cast<MessageReceiver::ReplyHandler>(&ProxyBusObject::IntrospectMethodCB),
                                     NULL,
                                     0,
                                     reinterpret_cast<void*>(ctx),
                                     timeout);
    if (ER_OK != status) {
        delete ctx;
    }
    return status;
}

void ProxyBusObject::IntrospectMethodCB(Message& msg, void* context)
{
    QStatus status;
    if (NULL != msg->GetArg(0)) {
        QCC_DbgPrintf(("Introspection XML: %s", msg->GetArg(0)->v_string.str));
    }
    CBContext<Listener::IntrospectCB>* ctx = reinterpret_cast<CBContext<Listener::IntrospectCB>*>(context);

    if (msg->GetType() == MESSAGE_METHOD_RET) {
        /* Parse the XML reply to update this ProxyBusObject instance (plus any new interfaces) */
        qcc::String ident = msg->GetSender();
        if (internal->uniqueName.empty()) {
            internal->uniqueName = ident;
        }
        ident += " : ";
        ident += msg->GetObjectPath();
        status = ParseXml(msg->GetArg(0)->v_string.str, ident.c_str());
    } else if (msg->GetErrorName() != NULL && ::strcmp("org.freedesktop.DBus.Error.ServiceUnknown", msg->GetErrorName()) == 0) {
        status = ER_BUS_NO_SUCH_SERVICE;
    } else {
        status = ER_FAIL;
    }

    /* Call the callback */
    (ctx->listener->*ctx->callback)(status, this, ctx->context);
    delete ctx;
}

QStatus ProxyBusObject::ParseXml(const char* xml, const char* ident)
{
    StringSource source(xml);

    /* Parse the XML to update this ProxyBusObject instance (plus any new children and interfaces) */
    XmlParseContext pc(source);
    QStatus status = XmlElement::Parse(pc);
    if (status == ER_OK) {
        XmlHelper xmlHelper(internal->bus, ident ? ident : internal->path.c_str());
        status = xmlHelper.AddProxyObjects(*this, pc.GetRoot());
    }
    return status;
}

ProxyBusObject::~ProxyBusObject()
{
    /*
     * Need to wake up threads waiting on a synchronous method call since the
     * object it is calling into is being destroyed.  It's actually a pretty bad
     * situation to have one thread destroy a PBO instance that another thread
     * is calling into, but we try to handle that situation as well as possible.
     */
    internal->lock.Lock(MUTEX_CONTEXT);
    isExiting = true;
    set<SyncReplyContext>& replyCtxSet = internal->syncMethodCalls[this];
    set<SyncReplyContext>::iterator it;
    for (it = replyCtxSet.begin(); it != replyCtxSet.end(); ++it) {
        Thread* thread = (*it)->thread;
        QCC_LogError(ER_BUS_METHOD_CALL_ABORTED, ("Thread %s (%p) deleting ProxyBusObject called into by thread %s (%p)",
                                                  Thread::GetThreadName(), Thread::GetThread(),
                                                  thread->GetName(), thread));
        thread->Alert(SYNC_METHOD_ALERTCODE_ABORT);
    }

    /*
     * Now we wait for the outstanding synchronous method calls for this PBO to
     * get cleaned up.
     */
    while (!replyCtxSet.empty()) {
        internal->syncMethodComplete.Wait(internal->lock);
    }
    internal->syncMethodCalls.erase(this);
    internal->lock.Unlock(MUTEX_CONTEXT);
}


ProxyBusObject::ProxyBusObject(BusAttachment& bus, const char* service, const char* path, SessionId sessionId, bool isSecure) :
    internal(bus, path, service, sessionId, isSecure),
    isExiting(false)
{
    /* The Peer interface is implicitly defined for all objects */
    AddInterface(org::freedesktop::DBus::Peer::InterfaceName);
}

ProxyBusObject::ProxyBusObject(BusAttachment& bus, const char* service, const char* uniqueName, const char* path, SessionId sessionId, bool isSecure) :
    internal(bus, path, service, uniqueName, sessionId, isSecure),
    isExiting(false)
{
    /* The Peer interface is implicitly defined for all objects */
    AddInterface(org::freedesktop::DBus::Peer::InterfaceName);
}

ProxyBusObject::ProxyBusObject() : internal(), isExiting(false) {
}

ProxyBusObject::ProxyBusObject(const ProxyBusObject& other) : internal(other.internal), isExiting(false) {
}

ProxyBusObject::ProxyBusObject(ManagedObj<ProxyBusObject::Internal> internal) : internal(internal), isExiting(false) {
}

bool ProxyBusObject::operator==(const ProxyBusObject& other) const {
    return internal == other.internal;
}
bool ProxyBusObject::operator<(const ProxyBusObject& other) const {
    return internal < other.internal;
}

ProxyBusObject& ProxyBusObject::operator=(const ProxyBusObject& other)
{
    if (this != &other) {
        internal = other.internal;
        isExiting = false;
    }
    return *this;
}

void ProxyBusObject::SetB2BEndpoint(RemoteEndpoint& b2bEp)
{
    internal->b2bEp = b2bEp;
}

bool CachedProps::Get(const char* propname, MsgArg& val)
{
    bool found = false;
    lock.Lock(MUTEX_CONTEXT);
    ValueMap::iterator it = values.find(propname);
    if (it != values.end()) {
        found = true;
        val = it->second;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return found;
}

bool CachedProps::GetAll(MsgArg& val)
{
    if (!isFullyCacheable || numProperties == 0) {
        return false;
    }

    bool found = false;
    lock.Lock(MUTEX_CONTEXT);
    if (values.size() == numProperties) {
        found = true;
        MsgArg* dict = new MsgArg[numProperties];
        ValueMap::iterator it = values.begin();
        for (int i = 0; it != values.end(); ++it, ++i) {
            MsgArg* inner;
            it->second.Get("v", &inner);
            dict[i].Set("{sv}", it->first.c_str(), inner);
            /* dict[i].Set("{sv}", it->first.c_str(), &(it->second)); */
        }
        val.Set("a{sv}", numProperties, dict);
        val.Stabilize();
        delete[] dict;
    }
    lock.Unlock(MUTEX_CONTEXT);
    return found;
}

bool CachedProps::IsValidMessageSerial(uint32_t messageSerial)
{
    uint32_t threshold = (uint32_t) (0x1 << 31);
    if (messageSerial >= lastMessageSerial) {
        // messageSerial should be higher than the last.
        // The check returns true unless the diff is too big.
        // In this case we assume an out-of-order message is processed.
        // The message was sent prior to a wrap around of the uint32_t counter.
        return ((messageSerial - lastMessageSerial) < threshold);
    }
    // The messageSerial is smaller than the last. This is an out-of-order
    // message (return false) unless the diff is too big. if the diff is high
    // we assume we hit a wrap around of the message serial counter (return true).
    return ((lastMessageSerial - messageSerial) > threshold);
}

bool CachedProps::IsCacheable(const char* propname)
{
    const InterfaceDescription::Property* prop = description->GetProperty(propname);
    return (prop != NULL && prop->cacheable);
}

void CachedProps::Set(const char* propname, const MsgArg& val, const uint32_t messageSerial)
{
    if (!IsCacheable(propname)) {
        return;
    }

    lock.Lock(MUTEX_CONTEXT);
    if (!IsValidMessageSerial(messageSerial)) {
        values.clear();
    } else {
        values[qcc::String(propname)] = val;
        lastMessageSerial = messageSerial;
    }
    lock.Unlock(MUTEX_CONTEXT);
}

void CachedProps::SetAll(const MsgArg& allValues, const uint32_t messageSerial)
{
    lock.Lock(MUTEX_CONTEXT);

    size_t nelem;
    MsgArg* elems;
    QStatus status = allValues.Get("a{sv}", &nelem, &elems);
    if (status != ER_OK) {
        goto error;
    }

    if (!IsValidMessageSerial(messageSerial)) {
        status = ER_FAIL;
        goto error;
    }

    for (size_t i = 0; i < nelem; ++i) {
        const char* prop;
        MsgArg* val;
        status = elems[i].Get("{sv}", &prop, &val);
        if (status != ER_OK) {
            goto error;
        }
        if (IsCacheable(prop)) {
            values[qcc::String(prop)].Set("v", val);
            values[qcc::String(prop)].Stabilize();
        }
    }

    lastMessageSerial = messageSerial;

    lock.Unlock(MUTEX_CONTEXT);
    return;

error:
    /* We can't make sense of the property values for some reason.
     * Play it safe and invalidate all properties */
    QCC_LogError(status, ("Failed to parse GetAll return value or inconsistent message serial number. Invalidating property cache."));
    values.clear();
    lock.Unlock(MUTEX_CONTEXT);
}

void CachedProps::PropertiesChanged(MsgArg* changed, size_t numChanged, MsgArg* invalidated, size_t numInvalidated, const uint32_t messageSerial)
{
    lock.Lock(MUTEX_CONTEXT);

    QStatus status;

    if (!IsValidMessageSerial(messageSerial)) {
        status = ER_FAIL;
        goto error;
    }

    for (size_t i = 0; i < numChanged; ++i) {
        const char* prop;
        MsgArg* val;
        status = changed[i].Get("{sv}", &prop, &val);
        if (status != ER_OK) {
            goto error;
        }
        if (IsCacheable(prop)) {
            values[qcc::String(prop)].Set("v", val);
            values[qcc::String(prop)].Stabilize();
        }
    }

    for (size_t i = 0; i < numInvalidated; ++i) {
        char* prop;
        status = invalidated[i].Get("s", &prop);
        if (status != ER_OK) {
            goto error;
        }
        values.erase(prop);
    }

    lastMessageSerial = messageSerial;

    lock.Unlock(MUTEX_CONTEXT);
    return;

error:
    /* We can't make sense of the property update signal for some reason.
     * Play it safe and invalidate all properties */
    QCC_LogError(status, ("Failed to parse PropertiesChanged signal or inconsistent message serial number. Invalidating property cache."));
    values.clear();
    lock.Unlock(MUTEX_CONTEXT);
}

}
