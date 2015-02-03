#ifndef _ALLJOYN_OBSINTERNAL_H
#define _ALLJOYN_OBSINTERNAL_H
/**
 * @file
 *
 * This file defines internal state for a BusAttachment
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

#ifndef __cplusplus
#error Only include BusInternal.h in C++ code.
#endif

#include <set>
#include <map>

#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/ManagedObj.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Observer.h>
#include <alljoyn/InterfaceDescription.h>

namespace ajn {
class Observer::Internal {
  public:
    typedef std::set<qcc::String> InterfaceSet;

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
            : listener(listener), enabled(enabled) {}
    };
    typedef qcc::ManagedObj<WrappedListener*> ProtectedObserverListener;
    typedef std::set<ProtectedObserverListener> ObserverListenerSet;
    ObserverListenerSet listeners;
    qcc::Mutex listenersLock;

  public:
    InterfaceSet mandatory;

    Internal(BusAttachment& bus,
             Observer* observer,
             InterfaceSet mandatory);

    ~Internal();

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

}


#endif
