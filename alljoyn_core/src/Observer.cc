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

#include <set>

#include <qcc/String.h>
#include <qcc/Mutex.h>

#include <alljoyn/Observer.h>
#include <alljoyn/InterfaceDescription.h>

#include "BusInternal.h"
#include "CoreObserver.h"
#include "ObserverManager.h"
#include "BusUtil.h"

#include <qcc/Debug.h>
#define QCC_MODULE "OBSERVER"

using namespace std;

namespace ajn {

ObjectId::ObjectId() {
}

ObjectId::ObjectId(const qcc::String& busname, const qcc::String& path) :
    uniqueBusName(IsLegalUniqueName(busname.c_str()) ? busname : ""),
    objectPath(IsLegalObjectPath(path.c_str()) ? path : "") {
}

ObjectId::ObjectId(const ManagedProxyBusObject& mpbo) :
    uniqueBusName(mpbo->GetUniqueName()), objectPath(mpbo->GetPath()) {
}

ObjectId::ObjectId(const ProxyBusObject* ppbo) :
    uniqueBusName((ppbo != NULL ? ppbo->GetUniqueName() : "")),
    objectPath((ppbo != NULL ? ppbo->GetPath() : "")) {
}

ObjectId::ObjectId(const ProxyBusObject& pbo) :
    uniqueBusName(pbo.GetUniqueName()), objectPath(pbo.GetPath()) {
}

ObjectId::ObjectId(const ObjectId& other) :
    uniqueBusName(other.uniqueBusName), objectPath(other.objectPath) {
}

ObjectId& ObjectId::operator=(const ObjectId& other) {
    uniqueBusName = other.uniqueBusName;
    objectPath = other.objectPath;
    return *this;
}

bool ObjectId::operator==(const ObjectId& other) const {
    return uniqueBusName == other.uniqueBusName && objectPath == other.objectPath;
}

bool ObjectId::operator<(const ObjectId& other) const {
    return (uniqueBusName == other.uniqueBusName) ? (objectPath < other.objectPath) :
           (uniqueBusName < other.uniqueBusName);
}

bool ObjectId::IsValid() const {
    return (uniqueBusName != "") && (objectPath != "");
}

class Observer::Internal : public CoreObserver {
  private:
    BusAttachment& bus;
    Observer* observer;

    /* proxy object bookkeeping */
    typedef std::map<ObjectId, ManagedProxyBusObject> ObjectMap;
    ObjectMap proxies;
    qcc::Mutex proxiesLock;

    /* listener bookkeeping */
    struct WrappedListener {
        /* WrappedListener exists to keep track of whether a given listener
         * is already enabled. triggerOnExisting listeners start off as
         * disabled, until the ObserverManager has the chance to fire the
         * initial callbacks (for "existing" objects) from the work queue. */
        Observer::Listener* listener;
        bool enabled;
        WrappedListener(Observer::Listener* listener, bool enabled)
            : listener(listener), enabled(enabled) { }
    };
    typedef qcc::ManagedObj<WrappedListener*> ProtectedObserverListener;
    typedef std::set<ProtectedObserverListener> ObserverListenerSet;
    ObserverListenerSet listeners;
    qcc::Mutex listenersLock;

  public:
    Internal(BusAttachment& bus,
             Observer* observer,
             InterfaceSet mandatory);

    virtual ~Internal();

    /**
     * Detach from the publicly visible Observer.
     *
     * Because of complicated threading/locking issues, it is not
     * straightforward to just destroy the Internal object from the
     * Observer destructor. Instead, we have a two-phase approach:
     * ~Observer detaches from Internal, and when it is safe to do
     * so, ObserverManager destroys the Internal object.
     */
    void Detach();

    /* implementation of public Observer functionality */
    void RegisterListener(Observer::Listener& listener, bool triggerOnExisting);
    void UnregisterListener(Observer::Listener& listener);
    void UnregisterAllListeners();

    ManagedProxyBusObject Get(const ObjectId& oid);
    ManagedProxyBusObject GetFirst();
    ManagedProxyBusObject GetNext(const ObjectId& oid);

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

Observer::Internal::Internal(BusAttachment& bus,
                             Observer* observer,
                             InterfaceSet mandatory) :
    CoreObserver(mandatory),
    bus(bus),
    observer(observer)
{
    ObserverManager& obsmgr = bus.GetInternal().GetObserverManager();
    obsmgr.RegisterObserver(this);
}

Observer::Internal::~Internal()
{
}

void Observer::Internal::Detach()
{
    UnregisterAllListeners();
    observer = NULL;

    ObserverManager& obsmgr = bus.GetInternal().GetObserverManager();
    obsmgr.UnregisterObserver(this);
}

void Observer::Internal::RegisterListener(Observer::Listener& listener, bool triggerOnExisting)
{
    Observer::Listener* pListener = &listener;
    WrappedListener* wListener = new WrappedListener(pListener, !triggerOnExisting);
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

void Observer::Internal::UnregisterListener(Observer::Listener& listener)
{
    listenersLock.Lock(MUTEX_CONTEXT);

    /* Look for listener on ListenerSet */
    ObserverListenerSet::iterator it;
    for (it = listeners.begin(); it != listeners.end(); ++it) {
        if ((**it)->listener == &listener) {
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

void Observer::Internal::EnablePendingListeners()
{
    /* make a list of all pending listeners */
    vector<ProtectedObserverListener> pendingListeners;
    listenersLock.Lock(MUTEX_CONTEXT);
    ObserverListenerSet::iterator lit;
    for (lit = listeners.begin(); lit != listeners.end(); ++lit) {
        ProtectedObserverListener pol = *lit;
        if ((*pol)->enabled == false) {
            pendingListeners.push_back(pol);
        }
    }
    listenersLock.Unlock(MUTEX_CONTEXT);

    /* for each pending listener, invoke ObjectDiscovered for all known proxies */
    proxiesLock.Lock(MUTEX_CONTEXT);
    vector<ProtectedObserverListener>::iterator plit;
    for (plit = pendingListeners.begin(); plit != pendingListeners.end(); ++plit) {
        ProtectedObserverListener pol = *plit;
        (*pol)->enabled = true;

        ObjectMap::iterator it = proxies.begin();
        while (it != proxies.end()) {
            ObjectId id = it->first;
            ManagedProxyBusObject proxy = it->second;
            proxiesLock.Unlock(MUTEX_CONTEXT);
            (*pol)->listener->ObjectDiscovered(proxy);
            proxiesLock.Lock(MUTEX_CONTEXT);
            it = proxies.upper_bound(id);
        }

    }

    proxiesLock.Unlock(MUTEX_CONTEXT);
}

void Observer::Internal::UnregisterAllListeners()
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
            delete (*l);
        }
        it = listeners.begin();
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
}

ManagedProxyBusObject Observer::Internal::Get(const ObjectId& oid)
{
    ManagedProxyBusObject obj;
    if (!oid.IsValid()) {
        return obj;
    }
    proxiesLock.Lock(MUTEX_CONTEXT);
    ObjectMap::iterator it = proxies.find(oid);
    if (it != proxies.end()) {
        obj = it->second;
    }
    proxiesLock.Unlock(MUTEX_CONTEXT);
    return obj;
}

ManagedProxyBusObject Observer::Internal::GetFirst()
{
    ManagedProxyBusObject obj;
    proxiesLock.Lock(MUTEX_CONTEXT);
    ObjectMap::iterator it = proxies.begin();
    if (it != proxies.end()) {
        obj = it->second;
    }
    proxiesLock.Unlock(MUTEX_CONTEXT);
    return obj;
}

ManagedProxyBusObject Observer::Internal::GetNext(const ObjectId& oid)
{
    ManagedProxyBusObject obj;
    if (!oid.IsValid()) {
        return obj;
    }
    proxiesLock.Lock(MUTEX_CONTEXT);
    ObjectMap::iterator it = proxies.upper_bound(oid);
    if (it != proxies.end()) {
        obj = it->second;
    }
    proxiesLock.Unlock(MUTEX_CONTEXT);
    return obj;
}

void Observer::Internal::ObjectDiscovered(const ObjectId& oid,
                                          const std::set<qcc::String>& interfaces,
                                          SessionId sessionid)
{
    /* create proxy object */
    //TODO figure out what to do with secure bus objects
    const char* busname = oid.uniqueBusName.c_str();
    const char* path = oid.objectPath.c_str();
    QCC_DbgTrace(("ObjectDiscovered(%s:%s)", busname, path));

    ManagedProxyBusObject proxy(bus, busname, path, sessionid);
    InterfaceSet::iterator it;
    for (it = interfaces.begin(); it != interfaces.end(); ++it) {
        proxy->AddInterface(it->c_str());
    }

    /* insert in proxy map */
    proxiesLock.Lock(MUTEX_CONTEXT);
    proxies[oid] = proxy;
    proxiesLock.Unlock(MUTEX_CONTEXT);

    /* alert listeners */
    listenersLock.Lock(MUTEX_CONTEXT);
    ObserverListenerSet::iterator lit = listeners.begin();
    while (lit != listeners.end()) {
        ProtectedObserverListener pol = *lit;
        if ((*pol)->enabled == false) {
            ++lit;
            continue;
        }
        listenersLock.Unlock(MUTEX_CONTEXT);
        (*pol)->listener->ObjectDiscovered(proxy);
        listenersLock.Lock(MUTEX_CONTEXT);
        lit = listeners.upper_bound(pol);
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
}

void Observer::Internal::ObjectLost(const ObjectId& oid)
{
    /* remove from proxy map */
    bool found = false;
    ManagedProxyBusObject proxy;

    proxiesLock.Lock(MUTEX_CONTEXT);
    ObjectMap::iterator it = proxies.find(oid);
    if (it != proxies.end()) {
        found = true;
        proxy = it->second;
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
            listenersLock.Unlock(MUTEX_CONTEXT);
            (*pol)->listener->ObjectLost(proxy);
            listenersLock.Lock(MUTEX_CONTEXT);
            lit = listeners.upper_bound(pol);
        }
        listenersLock.Unlock(MUTEX_CONTEXT);
    }
}

Observer::Observer(BusAttachment& bus,
                   const char* mandatoryInterfaces[],
                   size_t numMandatoryInterfaces)
{
    set<qcc::String> mandatory;
    bool inError = false;

    if (mandatoryInterfaces) {
        for (size_t i = 0; i < numMandatoryInterfaces; ++i) {
            const InterfaceDescription* intf = bus.GetInterface(mandatoryInterfaces[i]);
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
    }

    if (inError) {
        internal = NULL;
    } else {
        internal = new Internal(bus, this, mandatory);
    }
}

Observer::~Observer()
{
    if (internal) {
        internal->Detach();
        internal = NULL;
    }
}

void Observer::RegisterListener(Listener& listener, bool triggerOnExisting)
{
    if (!internal) {
        return;
    }
    internal->RegisterListener(listener, triggerOnExisting);
}

void Observer::UnregisterListener(Listener& listener)
{
    if (!internal) {
        return;
    }
    internal->UnregisterListener(listener);
}

void Observer::UnregisterAllListeners()
{
    if (!internal) {
        return;
    }
    internal->UnregisterAllListeners();
}

ManagedProxyBusObject Observer::Get(const ObjectId& oid)
{
    if (!internal) {
        return ManagedProxyBusObject();
    }
    return internal->Get(oid);
}

ManagedProxyBusObject Observer::GetFirst()
{
    if (!internal) {
        return ManagedProxyBusObject();
    }
    return internal->GetFirst();
}

ManagedProxyBusObject Observer::GetNext(const ObjectId& oid)
{
    if (!internal) {
        return ManagedProxyBusObject();
    }
    return internal->GetNext(oid);
}

}
