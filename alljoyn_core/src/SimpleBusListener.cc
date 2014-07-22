/**
 * @file
 *
 * This file implements the default bus listener.
 */

/******************************************************************************
 * Copyright (c) 2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <queue>

#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/Debug.h>

#include <alljoyn/BusListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/SimpleBusListener.h>


#define QCC_MODULE "ALLJOYN"


using namespace qcc;
using namespace std;

namespace ajn {

static inline const char* CopyIn(qcc::String& dest, const char* src)
{
    if (src) {
        dest = src;
        return dest.c_str();
    } else {
        return NULL;
    }
}

SimpleBusListener::BusEvent& SimpleBusListener::BusEvent::operator=(const BusEvent& other)
{
    eventType = other.eventType;
    switch (eventType) {
    case BUS_EVENT_FOUND_ADVERTISED_NAME:
        foundAdvertisedName.name = CopyIn(strings[0], other.foundAdvertisedName.name);
        foundAdvertisedName.transport = other.foundAdvertisedName.transport;
        foundAdvertisedName.namePrefix = CopyIn(strings[1], other.foundAdvertisedName.namePrefix);
        break;

    case BUS_EVENT_LOST_ADVERTISED_NAME:
        lostAdvertisedName.name = CopyIn(strings[0], other.lostAdvertisedName.name);
        lostAdvertisedName.namePrefix = CopyIn(strings[1], other.lostAdvertisedName.namePrefix);
        break;

    case BUS_EVENT_NAME_OWNER_CHANGED:
        nameOwnerChanged.busName = CopyIn(strings[0], other.nameOwnerChanged.busName);
        nameOwnerChanged.previousOwner = CopyIn(strings[1], other.nameOwnerChanged.previousOwner);
        nameOwnerChanged.newOwner = CopyIn(strings[2], other.nameOwnerChanged.newOwner);
        break;

    default:
        break;
    }
    return *this;
}

class SimpleBusListener::Internal {
  public:
    Internal() : bus(NULL), waiter(false) { }
    Event waitEvent;
    qcc::Mutex lock;
    std::queue<BusEvent> eventQueue;
    BusAttachment* bus;
    bool waiter;

    void QueueEvent(BusEvent& ev) {
        lock.Lock(MUTEX_CONTEXT);
        eventQueue.push(ev);
        waitEvent.SetEvent();
        lock.Unlock(MUTEX_CONTEXT);
    }
};

SimpleBusListener::SimpleBusListener(uint32_t enabled) : enabled(enabled), internal(*(new Internal))
{
}

void SimpleBusListener::FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
{
    if (enabled & BUS_EVENT_FOUND_ADVERTISED_NAME) {
        BusEvent busEvent;
        busEvent.eventType = BUS_EVENT_FOUND_ADVERTISED_NAME;
        busEvent.foundAdvertisedName.name = name;
        busEvent.foundAdvertisedName.transport = transport;
        busEvent.foundAdvertisedName.namePrefix = namePrefix;
        internal.QueueEvent(busEvent);
    }
}

void SimpleBusListener::LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
{
    if (enabled & BUS_EVENT_LOST_ADVERTISED_NAME) {
        BusEvent busEvent;
        busEvent.eventType = BUS_EVENT_LOST_ADVERTISED_NAME;
        busEvent.lostAdvertisedName.name = name;
        busEvent.lostAdvertisedName.transport = transport;
        busEvent.lostAdvertisedName.namePrefix = namePrefix;
        internal.QueueEvent(busEvent);
    }
}

void SimpleBusListener::NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
{
    if (enabled & BUS_EVENT_NAME_OWNER_CHANGED) {
        BusEvent busEvent;
        busEvent.eventType = BUS_EVENT_NAME_OWNER_CHANGED;
        busEvent.nameOwnerChanged.busName = busName;
        busEvent.nameOwnerChanged.previousOwner = previousOwner;
        busEvent.nameOwnerChanged.newOwner = newOwner;
        internal.QueueEvent(busEvent);
    }
}

void SimpleBusListener::SetFilter(uint32_t enabled)
{
    internal.lock.Lock(MUTEX_CONTEXT);
    this->enabled = enabled;
    /*
     * Save all queued events that pass the filter.
     */
    size_t pass = 0;
    while (internal.eventQueue.size() != pass) {
        BusEvent ev = internal.eventQueue.front();
        internal.eventQueue.pop();
        if (ev.eventType & enabled) {
            internal.eventQueue.push(ev);
            ++pass;
        }
    }
    if (pass == 0) {
        internal.waitEvent.ResetEvent();
    }
    internal.lock.Unlock(MUTEX_CONTEXT);
}

QStatus SimpleBusListener::WaitForEvent(BusEvent& busEvent, uint32_t timeout)
{
    QStatus status = ER_OK;
    internal.lock.Lock(MUTEX_CONTEXT);
    busEvent.eventType = BUS_EVENT_NONE;
    if (!internal.bus) {
        status = ER_BUS_WAIT_FAILED;
        QCC_LogError(status, ("Listener has not been registered with a bus attachment"));
        goto ExitWait;
    }
    if (internal.bus->IsStopping() || !internal.bus->IsStarted()) {
        status = ER_BUS_WAIT_FAILED;
        QCC_LogError(status, ("Bus is not running"));
        goto ExitWait;
    }
    if (internal.waiter) {
        status = ER_BUS_WAIT_FAILED;
        QCC_LogError(status, ("Another thread is already waiting"));
        goto ExitWait;
    }
    if (internal.eventQueue.empty() && timeout) {
        internal.waiter = true;
        internal.lock.Unlock(MUTEX_CONTEXT);
        status = Event::Wait(internal.waitEvent, (timeout == 0xFFFFFFFF) ? Event::WAIT_FOREVER : timeout);
        internal.lock.Lock(MUTEX_CONTEXT);
        internal.waitEvent.ResetEvent();
        internal.waiter = false;
    }
    if (!internal.eventQueue.empty()) {
        busEvent = internal.eventQueue.front();
        internal.eventQueue.pop();
    }

ExitWait:
    internal.lock.Unlock(MUTEX_CONTEXT);
    return status;
}

void SimpleBusListener::BusStopping()
{
    /*
     * Unblock any waiting thread
     */
    internal.waitEvent.SetEvent();
}

void SimpleBusListener::ListenerUnregistered()
{
    internal.lock.Lock(MUTEX_CONTEXT);
    internal.bus = NULL;
    internal.lock.Unlock(MUTEX_CONTEXT);
}

void SimpleBusListener::ListenerRegistered(BusAttachment* bus)
{
    internal.lock.Lock(MUTEX_CONTEXT);
    internal.bus = bus;
    internal.lock.Unlock(MUTEX_CONTEXT);
}

SimpleBusListener::~SimpleBusListener()
{
    /*
     * Unblock any threads waiting
     */
    internal.lock.Lock(MUTEX_CONTEXT);
    internal.waitEvent.SetEvent();
    internal.lock.Unlock(MUTEX_CONTEXT);
    delete &internal;
}

}
