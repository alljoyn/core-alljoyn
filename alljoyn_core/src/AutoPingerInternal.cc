/******************************************************************************
 * Copyright (c) 2014-2015, AllSeen Alliance. All rights reserved.
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

#include "AutoPingerInternal.h"
#include <alljoyn/BusAttachment.h>
#include <qcc/time.h>
#include <algorithm>
#include <memory>
#include <set>
#include <cassert>

#define PING_TIMEOUT 5000

#define QCC_MODULE "AUTOPINGER"

namespace ajn {

// Destination data
struct Destination {
    Destination(const qcc::String& _destination, const AutoPingerInternal::PingState _oldState) :
        destination(_destination), oldState(_oldState) { }
    qcc::String destination;

    /* mutable so we can modify this even while iterating;
     * see UpdatePingStateOfDestination()
     */
    mutable AutoPingerInternal::PingState oldState;

    bool operator==(const Destination& dest) const {
        return (destination == dest.destination);
    }

    bool operator<(const Destination& dest) const {
        return (destination < dest.destination);
    }
};

// Group data
struct PingGroup {
    PingGroup(uint32_t pingInterval,      /* milliseconds */
              qcc::AlarmListener* alarmListener,
              void* context,
              PingListener& _pingListener) :
        alarm(pingInterval, alarmListener, context, pingInterval), pingListener(_pingListener) { }

    ~PingGroup() {
        qcc::String* ctx = static_cast<qcc::String*>(alarm->GetContext());
        alarm->SetContext(NULL);
        if (NULL != ctx) {
            delete ctx;
        }
    }

    qcc::Alarm alarm;
    PingListener& pingListener;
    std::map<Destination, unsigned int> destinations;
};

// Context used to pass additional info in callbacks
class PingAsyncContext  {
  public:
    PingAsyncContext(AutoPingerInternal* _pinger,
                     const qcc::String& _group,
                     const qcc::String& _destination,
                     const AutoPingerInternal::PingState _oldState,
                     PingListener& listener) :
        pinger(_pinger), group(_group), destination(_destination), oldState(_oldState), pingListener(listener)  { }

    AutoPingerInternal* pinger;
    qcc::String group;
    qcc::String destination;
    AutoPingerInternal::PingState oldState;
    PingListener& pingListener;
};

static std::set<PingAsyncContext*>* ctxs = NULL;
static qcc::Mutex* globalPingerLock = NULL;
static bool callbackInProgress = false;

// Callback handler for async ping calls
class AutoPingAsyncCB : public BusAttachment::PingAsyncCB {
  public:
    void PingCB(QStatus status, void* context) {
        PingAsyncContext* ctx = (PingAsyncContext*)context;
        bool found = false;

        globalPingerLock->Lock();
        std::set<PingAsyncContext*>::iterator it = ctxs->find(ctx);
        if (it != ctxs->end()) {
            found = true;
            ctxs->erase(it);
        }

        if (found) {
            if (ctx->pinger->IsRunning() && !ctx->pinger->pausing) {
                if (ER_OK != status) {
                    if (ER_ALLJOYN_PING_REPLY_IN_PROGRESS != status) {
                        if (ctx->oldState != AutoPingerInternal::LOST) {
                            // update state
                            if (ctx->pinger->UpdatePingStateOfDestination(ctx->group, ctx->destination, AutoPingerInternal::LOST)) {

                                // call external listener
                                callbackInProgress = true;
                                globalPingerLock->Unlock();
                                ctx->pingListener.DestinationLost(ctx->group, ctx->destination);
                                globalPingerLock->Lock();
                                callbackInProgress = false;
                            }
                        }
                    }
                } else {
                    if (ctx->oldState != AutoPingerInternal::AVAILABLE) {
                        // update state
                        if (ctx->pinger->UpdatePingStateOfDestination(ctx->group, ctx->destination, AutoPingerInternal::AVAILABLE)) {

                            // call external listener
                            callbackInProgress = true;
                            globalPingerLock->Unlock();
                            ctx->pingListener.DestinationFound(ctx->group, ctx->destination);
                            globalPingerLock->Lock();
                            callbackInProgress = false;
                        }
                    }
                }

            } else {
                QCC_DbgPrintf(("AutoPingerInternal: ignoring callback - pinger not running"));
            }
        } else {
            QCC_DbgPrintf(("AutoPingerInternal: ignoring callback - ping already gone"));
        }
        globalPingerLock->Unlock();

        delete ctx;
    }
};

static AutoPingAsyncCB* pingCallback = NULL;

void AutoPingerInternal::Init()
{
    ctxs = new std::set<PingAsyncContext*>();
    globalPingerLock = new qcc::Mutex();
    pingCallback = new AutoPingAsyncCB();
}

void AutoPingerInternal::Cleanup()
{
    delete ctxs;
    ctxs = NULL;
    delete globalPingerLock;
    globalPingerLock = NULL;
    delete pingCallback;
    pingCallback = NULL;
}

AutoPingerInternal::AutoPingerInternal(ajn::BusAttachment& _busAttachment) :
    timer("autopinger"), busAttachment(_busAttachment), pausing(false)
{
    QCC_DbgPrintf(("AutoPingerInternal constructed"));
    timer.Start();
}

AutoPingerInternal::~AutoPingerInternal()
{
    pausing = true;
    timer.RemoveAlarmsWithListener(*this);

    // Stop timer thread
    if (timer.IsRunning()) {
        timer.Stop();
    }

    // Wait for thread to finish up
    timer.Join();

    // Invalidate all ctx;
    globalPingerLock->Lock();
    for (std::set<PingAsyncContext*>::iterator it = ctxs->begin(); it != ctxs->end();) {
        if ((*it)->pinger == this) {
            ctxs->erase(it++);
        } else {
            it++;
        }
    }

    /* If there are still any callbacks in progress, wait for them to
     * terminate before cleaning up the ping groups. If we don't do this,
     * we may end up deleting a ping listener that is in the process of
     * being invoked from another thread. */
    while (callbackInProgress) {
        globalPingerLock->Unlock();
        qcc::Sleep(10);
        globalPingerLock->Lock();
    }

    // Cleanup all groups
    for (std::map<qcc::String, PingGroup*>::iterator pair = pingGroups.begin(); pair != pingGroups.end(); ++pair) {
        delete pair->second;
    }
    globalPingerLock->Unlock();

    QCC_DbgPrintf(("AutoPingerInternal destructed"));
}

void AutoPingerInternal::AlarmTriggered(const qcc::Alarm& alarm, QStatus reason)
{
    qcc::String* groupName(reinterpret_cast<qcc::String*>(alarm->GetContext()));

    globalPingerLock->Lock();
    if ((false == pausing) && (NULL != groupName)) {
        // Ping all destination of the group
        PingGroupDestinations(*groupName);
    }
    globalPingerLock->Unlock();
}

void AutoPingerInternal::PingGroupDestinations(const qcc::String& group)
{
    /* called with global lock taken */
    QCC_DbgPrintf(("AutoPingerInternal: start pinging destination in group: '%s'", group.c_str()));
    std::map<qcc::String, PingGroup*>::const_iterator it = pingGroups.find(group);
    if (it != pingGroups.end()) {
        std::map<Destination, unsigned int>::iterator mapIt = (*it).second->destinations.begin();
        for (; mapIt != (*it).second->destinations.end(); ++mapIt) {
            PingDestination(group, mapIt->first.destination, mapIt->first.oldState, it->second->pingListener);
        }
    }
}

void AutoPingerInternal::PingDestination(const qcc::String& group, const qcc::String& destination, PingState oldState, PingListener& pingListener)
{
    /* called with global lock taken */
    PingAsyncContext* context =
        new PingAsyncContext(this, group, destination, oldState, pingListener);

    std::pair<std::set<PingAsyncContext*>::iterator, bool> pair = ctxs->insert(context);
    if (ER_OK != busAttachment.PingAsync(destination.c_str(), PING_TIMEOUT, pingCallback, context)) {
        ctxs->erase(pair.first);
        delete context;
    }
}

void AutoPingerInternal::Pause()
{
    // Stop all pending alarms
    globalPingerLock->Lock();
    pausing = true;
    timer.RemoveAlarmsWithListener(*this);
    globalPingerLock->Unlock();

    QCC_DbgPrintf(("AutoPingerInternal paused"));
}

void AutoPingerInternal::Resume()
{
    globalPingerLock->Lock();

    assert(timer.IsRunning());
    if (true == pausing) {
        // re-add all Alarm objects
        std::map<qcc::String, PingGroup*>::const_iterator it = pingGroups.begin();
        for (; it != pingGroups.end(); ++it) {
            timer.AddAlarmNonBlocking((*it).second->alarm);
        }

        pausing = false;
        QCC_DbgPrintf(("AutoPingerInternal resumed"));
    }

    globalPingerLock->Unlock();
}

void AutoPingerInternal::AddPingGroup(const qcc::String& group, PingListener& listener, uint32_t pingInterval)
{
    globalPingerLock->Lock();

    uint32_t intervalMillisec = pingInterval * 1000;
    std::map<qcc::String, PingGroup*>::iterator it = pingGroups.find(group);
    if (it != pingGroups.end()) {
        // Group already exists => just update its ping time
        QCC_DbgPrintf(("AutoPingerInternal: updating existing group: '%s' with new ping time: %u", group.c_str(), pingInterval));

        if (timer.RemoveAlarm((*it).second->alarm, false)) {
            // Cleanup old alarm
            void* context = (*it).second->alarm->GetContext();
            (*it).second->alarm->SetContext(NULL);
            if (NULL == context) {
                // There was no context, create one
                context = (void*)(new qcc::String(group));
            }

            // Alarm is a managed object (auto cleanup when overwritten)
            qcc::AlarmListener* alarmListener = (qcc::AlarmListener*)this;
            (*it).second->alarm = qcc::Alarm(intervalMillisec, alarmListener, context, intervalMillisec);
            timer.AddAlarmNonBlocking((*it).second->alarm);
        }
    } else {
        // Create a new group element
        QCC_DbgPrintf(("AutoPingerInternal: adding new group: '%s' with ping time: %u", group.c_str(), pingInterval));

        void* context = (void*)(new qcc::String(group));
        PingGroup* pingGroup = new PingGroup(intervalMillisec, this, context, listener);
        pingGroups.insert(std::pair<qcc::String, PingGroup*>(group, pingGroup));
        timer.AddAlarmNonBlocking(pingGroup->alarm);
    }

    globalPingerLock->Unlock();
}

void AutoPingerInternal::RemovePingGroup(const qcc::String& group)
{
    QCC_DbgPrintf(("AutoPingerInternal: removing group: '%s'", group.c_str()));
    globalPingerLock->Lock();
    /* Caution: removing a ping group also means removing the associated listener.
     * There may be ping calls in flight that will cause a callback to this listener
     * to be triggered. We have no choice but to busy-wait until there is no
     * callback in progress. */
    while (callbackInProgress) {
        globalPingerLock->Unlock();
        qcc::Sleep(10);
        globalPingerLock->Lock();
    }

    std::map<qcc::String, PingGroup*>::iterator it = pingGroups.find(group);
    if (it != pingGroups.end()) {
        // destructor of PingGroup cleans-up context
        timer.RemoveAlarm((*it).second->alarm, false);
        delete it->second;
        pingGroups.erase(it);
    }
    globalPingerLock->Unlock();
}

QStatus AutoPingerInternal::SetPingInterval(const qcc::String& group, uint32_t pingInterval)
{
    QStatus status = ER_FAIL;
    globalPingerLock->Lock();
    std::map<qcc::String, PingGroup*>::iterator it = pingGroups.find(group);
    if (it != pingGroups.end()) {
        QCC_DbgPrintf(("AutoPingerInternal: updating group: '%s' with ping time: %u", group.c_str(), pingInterval));

        if (timer.RemoveAlarm((*it).second->alarm, false)) {
            // Cleanup old alarm
            void* context = (*it).second->alarm->GetContext();
            (*it).second->alarm->SetContext(NULL);
            if (NULL == context) {
                context = (void*)(new qcc::String(group));
            }

            // Alarm is a managed object (auto cleanup when overwritten)
            uint32_t intervalMillisec = pingInterval * 1000;
            qcc::AlarmListener* alarmListener = (qcc::AlarmListener*)this;
            (*it).second->alarm = qcc::Alarm(intervalMillisec, alarmListener, context, intervalMillisec);
            timer.AddAlarmNonBlocking((*it).second->alarm);

            status = ER_OK;
        }
    } else {
        status = ER_BUS_PING_GROUP_NOT_FOUND;
        QCC_LogError(status, ("AutoPingerInternal: cannot update ping time for non-existing group: '%s'", group.c_str()));
    }
    globalPingerLock->Unlock();
    return status;
}

QStatus AutoPingerInternal::AddDestination(const qcc::String& group, const qcc::String& destination)
{
    QStatus status = ER_FAIL;
    globalPingerLock->Lock();
    std::map<qcc::String, PingGroup*>::iterator it = pingGroups.find(group);
    if (it != pingGroups.end()) {
        status = ER_OK;
        Destination dummy(destination, AutoPingerInternal::UNKNOWN);
        std::map<Destination, unsigned int>::iterator dit;
        if ((dit = it->second->destinations.find(dummy)) == it->second->destinations.end()) {
            QCC_DbgPrintf(("AutoPingerInternal: adding destination: '%s' to group: %s", destination.c_str(), group.c_str()));
            (*it).second->destinations[(Destination(destination, AutoPingerInternal::UNKNOWN))] = 1;
            /* for a fresh destination, we don't want to make the application
             * wait until the next periodic ping to learn about the initial
             * state of the destination. Therefore, we launch an
             * out-of-sequence ping right now. */
            PingDestination(group, destination, AutoPingerInternal::UNKNOWN, it->second->pingListener);
        } else {
            dit->second++;
            QCC_DbgPrintf(("AutoPingerInternal: destination: '%s' already present in group: %u; increasing refcount", destination.c_str(), group.c_str()));
        }
    } else {
        status = ER_BUS_PING_GROUP_NOT_FOUND;
        QCC_LogError(status, ("AutoPingerInternal: cannot add destination: '%s' to non-existing group: %u", destination.c_str(), group.c_str()));
    }
    globalPingerLock->Unlock();

    return status;
}

QStatus AutoPingerInternal::RemoveDestination(const qcc::String& group, const qcc::String& destination, bool removeAll)
{
    QStatus status = ER_FAIL;
    QCC_DbgPrintf(("AutoPingerInternal: remove destination: '%s' from group: %s", destination.c_str(), group.c_str()));
    globalPingerLock->Lock();
    /* in theory, it would be a good idea to busy-wait here until we're sure there are no
     * callbacks in flight. The downside of that is that it would be impossible to call
     * RemoveDestination from within a DestinationLost callback. That's not acceptable.
     */
    std::map<qcc::String, PingGroup*>::iterator it = pingGroups.find(group);
    if (it != pingGroups.end()) {
        status = ER_OK;
        Destination dummy(destination, AutoPingerInternal::UNKNOWN);
        std::map<Destination, unsigned int>::iterator dit = it->second->destinations.find(dummy);
        if (dit != it->second->destinations.end()) {
            if (removeAll == true) {
                dit->second = 0;
            } else {
                --dit->second;
            }

            if (dit->second == 0) {
                it->second->destinations.erase(dit);
            }
        }
    }
    globalPingerLock->Unlock();

    return status;
}

bool AutoPingerInternal::UpdatePingStateOfDestination(const qcc::String& group,
                                                      const qcc::String& destination,
                                                      const AutoPingerInternal::PingState state)
{
    /* this method is always called with the global lock taken */
    QCC_DbgPrintf(("AutoPingerInternal: UpdatePingStateOfDestination: '%s' from group: %s", destination.c_str(), group.c_str()));
    bool result = false;
    std::map<qcc::String, PingGroup*>::iterator it = pingGroups.find(group);
    if (it != pingGroups.end()) {
        Destination dummy(destination, AutoPingerInternal::UNKNOWN);
        std::map<Destination, unsigned int>::iterator dit = it->second->destinations.find(dummy);

        // Update state
        if (dit != (*it).second->destinations.end()) {
            if (state != dit->first.oldState) {
                /* this only works because oldState is mutable.
                   Alternatively, you have to add it again to the map (and erase the old one) */
                dit->first.oldState = state;
                result = true; /* state gets updated */
            }
        }
    }
    return result;
}

bool AutoPingerInternal::IsRunning()
{
    return timer.IsRunning();
}

}
