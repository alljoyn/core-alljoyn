/**
 * @file
 * An Observer takes care of discovery, session management and ProxyBusObject
 * creation for bus objects that implement a specific set of interfaces.
 */
/******************************************************************************
 * Copyright (c) 2015 AllSeen Alliance. All rights reserved.
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

#include <alljoyn_c/Observer.h>
#include <alljoyn/Observer.h>
#include <qcc/Mutex.h>
#include <vector>
#include <set>

#include "DeferredCallback.h"
#include "CoreObserver.h"
#include "BusInternal.h"

#include <qcc/Debug.h>
#define QCC_MODULE "ALLJOYN_C"

/* {{{ alljoyn_proxybusobject_ref */
struct _alljoyn_proxybusobject_ref_handle {
    alljoyn_proxybusobject proxy;
    int refcount;
    qcc::Mutex lock;
};

alljoyn_proxybusobject_ref AJ_CALL alljoyn_proxybusobject_ref_create(alljoyn_proxybusobject proxy)
{
    alljoyn_proxybusobject_ref ref = new _alljoyn_proxybusobject_ref_handle;
    ref->proxy = proxy;
    ref->refcount = 1;
    return ref;
}

alljoyn_proxybusobject AJ_CALL alljoyn_proxybusobject_ref_get(alljoyn_proxybusobject_ref ref)
{
    return ref->proxy;
}

void AJ_CALL alljoyn_proxybusobject_ref_incref(alljoyn_proxybusobject_ref ref)
{
    ref->lock.Lock(MUTEX_CONTEXT);
    ++ref->refcount;
    ref->lock.Unlock(MUTEX_CONTEXT);
}

void AJ_CALL alljoyn_proxybusobject_ref_decref(alljoyn_proxybusobject_ref ref)
{
    bool remove = false;
    ref->lock.Lock(MUTEX_CONTEXT);
    if (--ref->refcount == 0) {
        remove = true;
    }
    ref->lock.Unlock(MUTEX_CONTEXT);
    if (remove) {
        alljoyn_proxybusobject_destroy(ref->proxy);
        delete ref;
    }
}
/* }}} */

/* {{{ alljoyn_observerlistener */

struct _alljoyn_observerlistener_handle {
    const void* ctx;
    alljoyn_observerlistener_callback callbacks;
};

alljoyn_observerlistener AJ_CALL alljoyn_observerlistener_create(const alljoyn_observerlistener_callback* callback,
                                                                 const void* context)
{
    alljoyn_observerlistener listener = new _alljoyn_observerlistener_handle;
    listener->ctx = context;
    listener->callbacks = *callback;
    return listener;
}

void AJ_CALL alljoyn_observerlistener_destroy(alljoyn_observerlistener listener)
{
    delete listener;
}

/* }}} */

namespace ajn {

class ObserverC : public CoreObserver {
  private:
    BusAttachment& bus;
    alljoyn_busattachment cbus;

    /* proxy object bookkeeping */
    typedef std::map<ObjectId, alljoyn_proxybusobject_ref> ObjectMap;
    ObjectMap proxies;
    qcc::Mutex proxiesLock;

    /* listener bookkeeping */
    struct WrappedListener {
        /* WrappedListener exists to keep track of whether a given listener
         * is already enabled. triggerOnExisting listeners start off as
         * disabled, until the ObserverManager has the chance to fire the
         * initial callbacks (for "existing" objects) from the work queue. */
        alljoyn_observerlistener listener;
        bool enabled;
        WrappedListener(alljoyn_observerlistener listener, bool enabled)
            : listener(listener), enabled(enabled) { }
    };
    typedef qcc::ManagedObj<WrappedListener*> ProtectedObserverListener;
    typedef std::set<ProtectedObserverListener> ObserverListenerSet;
    ObserverListenerSet listeners;
    qcc::Mutex listenersLock;

  public:
    ObserverC(alljoyn_busattachment cbus, InterfaceSet mandatory);

    virtual ~ObserverC();

    /* implementation of public Observer functionality */
    void RegisterListener(alljoyn_observerlistener listener, bool triggerOnExisting);
    void UnregisterListener(alljoyn_observerlistener listener);
    void UnregisterAllListeners();

    void Detach();

    alljoyn_proxybusobject_ref Get(const char* busname, const char* path);
    alljoyn_proxybusobject_ref GetFirst();
    alljoyn_proxybusobject_ref GetNext(alljoyn_proxybusobject_ref prevref);

    /* interface towards ObserverManager */
    void ObjectDiscovered(const ObjectId& oid, const std::set<qcc::String>& interfaces, SessionId sessionid);
    void ObjectLost(const ObjectId& oid);
    /**
     * Enable all disabled listeners for this observer.
     *
     * Called from the ObserverManager work queue to make sure the
     * initial callbacks of triggerOnExisting listeners are called
     * from the local endpoint dispatcher threads.
     */
    void EnablePendingListeners();
};

ObserverC::ObserverC(alljoyn_busattachment _cbus,
                     InterfaceSet mandatory) :
    CoreObserver(mandatory),
    bus(*(BusAttachment*)_cbus),
    cbus(_cbus)
{
    ObserverManager& obsmgr = bus.GetInternal().GetObserverManager();
    obsmgr.RegisterObserver(this);
}

ObserverC::~ObserverC()
{
}

void ObserverC::Detach()
{
    UnregisterAllListeners();

    ObserverManager& obsmgr = bus.GetInternal().GetObserverManager();
    obsmgr.UnregisterObserver(this);
}

void ObserverC::RegisterListener(alljoyn_observerlistener listener, bool triggerOnExisting)
{
    WrappedListener* wListener = new WrappedListener(listener, !triggerOnExisting);
    ProtectedObserverListener protectedListener(wListener);
    listenersLock.Lock(MUTEX_CONTEXT);
    listeners.insert(protectedListener);
    listenersLock.Unlock(MUTEX_CONTEXT);

    if (triggerOnExisting) {
        /* We don't want to do the callbacks from this thread, as it's
         * most probably an application thread. Let the ObserverManager
         * do this for us from the dispatcher thread. To avoid confusion,
         * we leave this listener disabled so that other announcements on
         * the work queue don't get reported out of order.
         */
        ObserverManager& obsmgr = bus.GetInternal().GetObserverManager();
        obsmgr.EnablePendingListeners(this);
    }
}

void ObserverC::UnregisterListener(alljoyn_observerlistener listener)
{
    listenersLock.Lock(MUTEX_CONTEXT);

    /* Look for listener on ListenerSet */
    ObserverListenerSet::iterator it;
    for (it = listeners.begin(); it != listeners.end(); ++it) {
        if ((**it)->listener == listener) {
            break;
        }
    }

    /* Wait for all refs to this ProtectedObserverListener to exit */
    while ((it != listeners.end()) && (it->GetRefCount() > 1)) {
        ProtectedObserverListener l = *it;
        listenersLock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(5);
        listenersLock.Lock(MUTEX_CONTEXT);
        it = listeners.find(l);
    }

    /* Delete the listeners entry */
    if (it != listeners.end()) {
        ProtectedObserverListener l = *it;
        listeners.erase(it);
        delete (*l);
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
}

void ObserverC::UnregisterAllListeners()
{
    listenersLock.Lock(MUTEX_CONTEXT);

    /* Look for listener on ListenerSet */
    ObserverListenerSet::iterator it = listeners.begin();
    while (it != listeners.end()) {
        /* Wait for all refs to ProtectedBusListener to exit */
        while ((it != listeners.end()) && (it->GetRefCount() > 1)) {
            ProtectedObserverListener l = *it;
            listenersLock.Unlock(MUTEX_CONTEXT);
            qcc::Sleep(5);
            listenersLock.Lock(MUTEX_CONTEXT);
            it = listeners.find(l);
        }

        /* Delete the listeners entry */
        if (it != listeners.end()) {
            ProtectedObserverListener l = *it;
            listeners.erase(it);
        }
        it = listeners.begin();
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
}

void ObserverC::EnablePendingListeners()
{
    /* make a list of all pending listeners */
    std::vector<ProtectedObserverListener> pendingListeners;
    listenersLock.Lock(MUTEX_CONTEXT);
    ObserverListenerSet::iterator lit;
    for (lit = listeners.begin(); lit != listeners.end(); ++lit) {
        ProtectedObserverListener pol = *lit;
        if ((*pol)->enabled == false) {
            pendingListeners.push_back(pol);
        }
    }
    listenersLock.Unlock(MUTEX_CONTEXT);

    /* for each pending listener, invoke object_discovered for all known proxies */
    proxiesLock.Lock(MUTEX_CONTEXT);
    std::vector<ProtectedObserverListener>::iterator plit;
    for (plit = pendingListeners.begin(); plit != pendingListeners.end(); ++plit) {
        ProtectedObserverListener pol = *plit;
        (*pol)->enabled = true;
        alljoyn_observerlistener listener = (*pol)->listener;
        if (listener->callbacks.object_discovered == NULL) {
            continue;
        }

        ObjectMap::iterator it = proxies.begin();
        while (it != proxies.end()) {
            ObjectId id = it->first;
            alljoyn_proxybusobject_ref proxyref = it->second;
            alljoyn_proxybusobject_ref_incref(proxyref);
            proxiesLock.Unlock(MUTEX_CONTEXT);
            listener->callbacks.object_discovered(listener->ctx, proxyref);
            proxiesLock.Lock(MUTEX_CONTEXT);
            alljoyn_proxybusobject_ref_decref(proxyref);
            it = proxies.upper_bound(id);
        }
    }

    proxiesLock.Unlock(MUTEX_CONTEXT);
}

alljoyn_proxybusobject_ref ObserverC::Get(const char* busname, const char* path)
{
    ObjectId oid(busname, path);
    if (!oid.IsValid()) {
        return NULL;
    }

    alljoyn_proxybusobject_ref proxyref;
    proxiesLock.Lock(MUTEX_CONTEXT);
    ObjectMap::iterator it = proxies.find(oid);
    if (it != proxies.end()) {
        proxyref = it->second;
        alljoyn_proxybusobject_ref_incref(proxyref);
    } else {
        proxyref = NULL;
    }
    proxiesLock.Unlock(MUTEX_CONTEXT);
    return proxyref;
}

alljoyn_proxybusobject_ref ObserverC::GetFirst()
{
    alljoyn_proxybusobject_ref proxyref = NULL;
    proxiesLock.Lock(MUTEX_CONTEXT);
    ObjectMap::iterator it = proxies.begin();
    if (it != proxies.end()) {
        proxyref = it->second;
        alljoyn_proxybusobject_ref_incref(proxyref);
    }
    proxiesLock.Unlock(MUTEX_CONTEXT);
    return proxyref;
}

alljoyn_proxybusobject_ref ObserverC::GetNext(alljoyn_proxybusobject_ref prevref)
{
    if (NULL == prevref) {
        return NULL;
    }
    ObjectId oid((ajn::ProxyBusObject*)alljoyn_proxybusobject_ref_get(prevref));
    alljoyn_proxybusobject_ref_decref(prevref);

    if (!oid.IsValid()) {
        return NULL; // should never happen, actually...
    }

    alljoyn_proxybusobject_ref proxyref = NULL;
    proxiesLock.Lock(MUTEX_CONTEXT);
    ObjectMap::iterator it = proxies.upper_bound(oid);
    if (it != proxies.end()) {
        proxyref = it->second;
        alljoyn_proxybusobject_ref_incref(proxyref);
    }
    proxiesLock.Unlock(MUTEX_CONTEXT);
    return proxyref;
}

void ObserverC::ObjectDiscovered(const ObjectId& oid,
                                 const std::set<qcc::String>& interfaces,
                                 SessionId sessionid)
{
    /* create proxy object */
    //TODO figure out what to do with secure bus objects
    const char* busname = oid.uniqueBusName.c_str();
    const char* path = oid.objectPath.c_str();
    QCC_DbgTrace(("ObjectDiscovered(%s:%s)", busname, path));

    alljoyn_proxybusobject proxy = alljoyn_proxybusobject_create(cbus, busname, path, sessionid);
    InterfaceSet::iterator it;
    for (it = interfaces.begin(); it != interfaces.end(); ++it) {
        alljoyn_proxybusobject_addinterface_by_name(proxy, it->c_str());
    }

    /* insert in proxy map */
    alljoyn_proxybusobject_ref proxyref = alljoyn_proxybusobject_ref_create(proxy);
    proxiesLock.Lock(MUTEX_CONTEXT);
    proxies[oid] = proxyref;
    proxiesLock.Unlock(MUTEX_CONTEXT);

    /* alert listeners */
    alljoyn_proxybusobject_ref_incref(proxyref);
    listenersLock.Lock(MUTEX_CONTEXT);
    ObserverListenerSet::iterator lit = listeners.begin();
    while (lit != listeners.end()) {
        ProtectedObserverListener pol = *lit;
        if ((*pol)->enabled == false) {
            ++lit;
            continue;
        }
        alljoyn_observerlistener listener = (*pol)->listener;
        if (listener->callbacks.object_discovered == NULL) {
            ++lit;
            continue;
        }
        listenersLock.Unlock(MUTEX_CONTEXT);
        listener->callbacks.object_discovered(listener->ctx, proxyref);
        listenersLock.Lock(MUTEX_CONTEXT);
        lit = listeners.upper_bound(pol);
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
    alljoyn_proxybusobject_ref_decref(proxyref);
}

void ObserverC::ObjectLost(const ObjectId& oid)
{
    /* remove from proxy map */
    bool found = false;
    alljoyn_proxybusobject_ref proxyref = NULL;

    proxiesLock.Lock(MUTEX_CONTEXT);
    ObjectMap::iterator it = proxies.find(oid);
    if (it != proxies.end()) {
        found = true;
        proxyref = it->second;
        proxies.erase(it);
    }
    proxiesLock.Unlock(MUTEX_CONTEXT);

    /* alert listeners */
    if (found) {
        listenersLock.Lock(MUTEX_CONTEXT);
        ObserverListenerSet::iterator lit = listeners.begin();
        while (lit != listeners.end()) {
            ProtectedObserverListener pol = *lit;
            if ((*pol)->enabled == false) {
                ++lit;
                continue;
            }
            alljoyn_observerlistener listener = (*pol)->listener;
            if (listener->callbacks.object_lost == NULL) {
                ++lit;
                continue;
            }
            listenersLock.Unlock(MUTEX_CONTEXT);
            listener->callbacks.object_lost(listener->ctx, proxyref);
            listenersLock.Lock(MUTEX_CONTEXT);
            lit = listeners.upper_bound(pol);
        }
        listenersLock.Unlock(MUTEX_CONTEXT);
        alljoyn_proxybusobject_ref_decref(proxyref);
    }
}

}


struct _alljoyn_observer_handle { };

alljoyn_observer AJ_CALL alljoyn_observer_create(alljoyn_busattachment bus, const char* mandatoryInterfaces[], size_t numMandatoryInterfaces)
{
    std::set<qcc::String> mandatory;
    bool inError = false;

    if (mandatoryInterfaces) {
        for (size_t i = 0; i < numMandatoryInterfaces; ++i) {
            alljoyn_interfacedescription intf = alljoyn_busattachment_getinterface(bus, mandatoryInterfaces[i]);
            if (NULL == intf) {
                QCC_LogError(ER_FAIL, ("Interface %s does not exist", mandatoryInterfaces[i]));
                inError = true;
            } else {
                mandatory.insert(mandatoryInterfaces[i]);
            }
        }
    }

    if (mandatory.empty()) {
        QCC_LogError(ER_FAIL, ("There must be at least one mandatory interface."));
        return NULL;
    }

    if (inError) {
        QCC_LogError(ER_FAIL, ("At least one of the mandatory interfaces is not declared in the bus attachment."));
        return NULL;
    }

    return (alljoyn_observer) new ajn::ObserverC(bus, mandatory);
}

void AJ_CALL alljoyn_observer_destroy(alljoyn_observer observer)
{
    /* actual destruction will be done by the ObserverManager in due time */
    if (observer != NULL) {
        ajn::ObserverC* obs = (ajn::ObserverC*) observer;
        obs->Detach();
    }
}

void AJ_CALL alljoyn_observer_registerlistener(alljoyn_observer observer, alljoyn_observerlistener listener, QCC_BOOL triggerOnExisting)
{
    if (observer != NULL) {
        ajn::ObserverC* obs = (ajn::ObserverC*) observer;
        obs->RegisterListener(listener, triggerOnExisting);
    }
}

void AJ_CALL alljoyn_observer_unregisterlistener(alljoyn_observer observer, alljoyn_observerlistener listener)
{
    if (observer != NULL) {
        ajn::ObserverC* obs = (ajn::ObserverC*) observer;
        obs->UnregisterListener(listener);
    }
}

void AJ_CALL alljoyn_observer_unregisteralllisteners(alljoyn_observer observer)
{
    if (observer != NULL) {
        ajn::ObserverC* obs = (ajn::ObserverC*) observer;
        obs->UnregisterAllListeners();
    }
}

alljoyn_proxybusobject_ref AJ_CALL alljoyn_observer_get(alljoyn_observer observer, const char* uniqueBusName, const char* objectPath)
{
    if (observer != NULL) {
        ajn::ObserverC* obs = (ajn::ObserverC*) observer;
        return obs->Get(uniqueBusName, objectPath);
    }
    return NULL;
}

alljoyn_proxybusobject_ref AJ_CALL alljoyn_observer_getfirst(alljoyn_observer observer)
{
    if (observer != NULL) {
        ajn::ObserverC* obs = (ajn::ObserverC*) observer;
        return obs->GetFirst();
    }
    return NULL;
}

alljoyn_proxybusobject_ref AJ_CALL alljoyn_observer_getnext(alljoyn_observer observer, alljoyn_proxybusobject_ref proxyref)
{
    if (observer != NULL) {
        ajn::ObserverC* obs = (ajn::ObserverC*) observer;
        return obs->GetNext(proxyref);
    }
    return NULL;
}

