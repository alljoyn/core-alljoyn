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

#include <qcc/Mutex.h>
#include <qcc/Condition.h>
#include <qcc/LockLevel.h>

#include <alljoyn/Observer.h>
#include <alljoyn/AutoPinger.h>

#include "ObserverManager.h"
#include "CoreObserver.h"
#include "BusInternal.h"

#include <algorithm>

#include <qcc/Debug.h>
#define QCC_MODULE "OBSERVER"


/* Concurrency Model
 * -----------------

 * The ObserverManager mainly concerns itself with taking input from various
 * subsystems (About, session listeners, AutoPinger, ...), building a
 * consistent set of discovered objects out of that, and alerting the
 * application by means of callbacks whenever the set of discovered objects
 * (that is of interest to the application) changes.
 *
 * The application expects all listener callbacks to come from the local
 * endpoint's dispatcher threads, because that's where all the Core callbacks
 * come from.
 *
 * The innate unpredictability of BusAttachment::EnableConcurrentCallbacks
 * makes it hard to build a concurrency model that is guaranteed to never
 * result in deadlocks. The only way we came up with to do this, is to
 * serialize all accesses to the ObserverManager's internal data structures on
 * a single work queue.
 *
 * So this is what happens: every action (be it a message-initiated action like
 * receiving an About announcement or an application-initiated action like
 * creating a new Observer) results in a WorkItem being added to the work
 * queue. Whenever ObserverManager code is executed on the local endpoint's
 * dispatcher thread, it will check the queue to see if there is work waiting.
 *
 * The wqLock mutex and processingWork flag make sure that only one dispatcher
 * thread ever simultaneously processes work from the work queue. No additional
 * locking is needed to protect the ObserverManager's internal data structures.
 *
 * Note: initially, the ObserverManager wasn't too choosy in which context it
 * started doing work (i.e. it would do work in the SessionLost handler or in
 * the Announce handler). It turns out that this behavior resulted in some
 * unexpected deadlocks (where, for example, a previously scheduled action
 * caused a session to be left with LeaveSessionAsync from the context of the
 * SessionLost callback for that same session). Therefore, the Observer now
 * only performs work when it is triggered directly from its own private alarm
 * in the LocalEndpoint.
 */


#define PING_GROUP "OBSERVER"
#define PING_INTERVAL 30

using namespace ajn;

struct ObserverManager::WorkItem {
    ObserverManager* mgr;
    virtual void Execute() = 0;
    virtual ~WorkItem() { }
};

struct ObserverManager::AnnouncementWork : public ObserverManager::WorkItem {

    ObserverManager::Peer peer;
    ObserverManager::ObjectSet announced;

    AnnouncementWork(const qcc::String& busname, SessionPort port,
                     const ObserverManager::ObjectSet& announced)
        : peer(busname, port), announced(announced) { }
    virtual ~AnnouncementWork() { }
    void Execute() {
        mgr->ProcessAnnouncement(peer, announced);
    }
};

struct ObserverManager::SessionEstablishedWork : public ObserverManager::WorkItem {
    ObserverManager::Peer peer;

    SessionEstablishedWork(const qcc::String& busname, SessionPort port, SessionId sessionid)
        : peer(busname, port) {
        peer.sessionid = sessionid;
    }
    virtual ~SessionEstablishedWork() { }
    void Execute() {
        mgr->ProcessSessionEstablished(peer);
    }
};

struct ObserverManager::SessionEstablishmentFailedWork : public ObserverManager::WorkItem {
    ObserverManager::Peer peer;

    SessionEstablishmentFailedWork(const qcc::String& busname, SessionPort port)
        : peer(busname, port) { }
    virtual ~SessionEstablishmentFailedWork() { }
    void Execute() {
        mgr->ProcessSessionEstablishmentFailed(peer);
    }
};

struct ObserverManager::SessionLostWork : public ObserverManager::WorkItem {
    SessionId sessionid;

    SessionLostWork(SessionId sessionid) : sessionid(sessionid) { }
    virtual ~SessionLostWork() { }
    void Execute() {
        mgr->ProcessSessionLost(sessionid);
    }
};

struct ObserverManager::DestinationLostWork : public ObserverManager::WorkItem {
    qcc::String busname;

    DestinationLostWork(qcc::String busname) : busname(busname) { }
    virtual ~DestinationLostWork() { }
    void Execute() {
        mgr->ProcessDestinationLost(busname);
    }
};

struct ObserverManager::RegisterObserverWork : public ObserverManager::WorkItem {
    CoreObserver* observer;

    RegisterObserverWork(CoreObserver* observer)
        : observer(observer) { }
    virtual ~RegisterObserverWork() { }
    void Execute() {
        mgr->ProcessRegisterObserver(observer);
    }
};

struct ObserverManager::UnregisterObserverWork : public ObserverManager::WorkItem {
    CoreObserver* observer;

    UnregisterObserverWork(CoreObserver* observer)
        : observer(observer) { }
    virtual ~UnregisterObserverWork() {
        /* delete observer here to avoid memory leaks during shutdown */
        delete observer;
    }
    void Execute() {
        mgr->ProcessUnregisterObserver(observer);
    }
  private:
    /* Private copy constructor and assign operator to prevent double free */
    UnregisterObserverWork(const UnregisterObserverWork&);
    UnregisterObserverWork& operator=(const UnregisterObserverWork&);

};

struct ObserverManager::EnablePendingListenersWork : public ObserverManager::WorkItem {
    CoreObserver* observer;
    InterfaceSet interfaces;

    EnablePendingListenersWork(CoreObserver* observer)
        : observer(observer), interfaces(observer->mandatory) { }
    virtual ~EnablePendingListenersWork() { }
    void Execute() {
        mgr->ProcessEnablePendingListeners(observer, interfaces);
    }
};

const char** ObserverManager::SetToArray(const InterfaceSet& set)
{
    const char** intfnames = new const char*[set.size()];
    InterfaceSet::iterator it;
    size_t i;
    for (i = 0, it = set.begin(); i < set.size(); ++i, ++it) {
        intfnames[i] = it->c_str();
    }
    return intfnames;
}

ObserverManager::ObjectSet ObserverManager::ParseObjectDescriptionArg(const qcc::String& busname, const MsgArg& arg)
{
    ObjectSet objects;

    QStatus status = ER_OK;
    MsgArg* structarg;
    size_t struct_size;
    status = arg.Get("a(oas)", &struct_size, &structarg);
    if (status != ER_OK) {
        goto error;
    }

    for (size_t i = 0; i < struct_size; ++i) {
        char* objectPath;
        size_t numberItfs;
        MsgArg* interfacesArg;
        status = structarg[i].Get("(oas)", &objectPath, &numberItfs, &interfacesArg);
        if (status != ER_OK) {
            goto error;
        }
        DiscoveredObject obj;
        obj.id = ObjectId(busname, objectPath);
        for (size_t j = 0; j < numberItfs; ++j) {
            char* intfName;
            status = interfacesArg[j].Get("s", &intfName);
            if (status != ER_OK) {
                goto error;
            }
            obj.implements.insert(intfName);
        }
        objects.insert(obj);
    }

error:
    if (status != ER_OK) {
        return ObjectSet();
    } else {
        return objects;
    }
}

ObserverManager::ObserverManager(BusAttachment& bus) :
    bus(bus),
    pinger(NULL),
    wqLock(qcc::LOCK_LEVEL_OBSERVERMANAGER_WQLOCK),
    processingWork(false),
    stopping(false),
    started(false)
{
}

void ObserverManager::Start()
{
    wqLock.Lock(MUTEX_CONTEXT);
    if (started) {
        wqLock.Unlock(MUTEX_CONTEXT);
        return;
    }
    started = true;

    bus.RegisterAboutListener(*this);
    pinger = new AutoPinger(bus);
    pinger->AddPingGroup(PING_GROUP, *this, PING_INTERVAL);
    wqLock.Unlock(MUTEX_CONTEXT);
}

ObserverManager::~ObserverManager()
{
    QCC_DbgTrace(("ObserverManager::~ObserverManager"));
    Stop();
    Join();

    /* clean up any remaining InterfaceCombinations */
    CombinationMap::iterator it;
    for (it = combinations.begin(); it != combinations.end(); ++it) {
        delete it->second;
    }
    combinations.clear();
}

void ObserverManager::Stop() {
    wqLock.Lock(MUTEX_CONTEXT);
    if (!started || stopping) {
        wqLock.Unlock(MUTEX_CONTEXT);
        return;
    }
    stopping = true;
    wqLock.Unlock(MUTEX_CONTEXT);

    /* stop the AutoPinger */
    if (pinger) {
        pinger->RemovePingGroup(PING_GROUP);
    }

    /* unregister for About callbacks */
    bus.UnregisterAboutListener(*this);
}

void ObserverManager::Join()
{
    wqLock.Lock(MUTEX_CONTEXT);
    if (!started || !stopping) {
        wqLock.Unlock(MUTEX_CONTEXT);
        return;
    }

    /* wait for any in-flight work item to land */
    while (processingWork) {
        processingDone.Wait(wqLock);
    }

    /* clear the work queue */
    while (!work.empty()) {
        delete work.front();
        work.pop();
    }
    wqLock.Unlock(MUTEX_CONTEXT);

    /* destruct the AutoPinger (joins the AutoPinger timer thread) */
    if (pinger) {
        delete pinger;
        pinger = NULL;
    }
}

void ObserverManager::RegisterObserver(CoreObserver* observer)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    WorkItem* workitem = new RegisterObserverWork(observer);
    ScheduleWork(workitem);
    /* RegisterObserver is typically called from the application threads
     * instead of the dispatcher thread. We cannot do work on the application
     * threads (that would cause listener callbacks to be invoked from the
     * wrong thread context), so instead we make sure the dispatcher calls
     * us back to do the work.
     */
    TriggerDoWork();
}

void ObserverManager::ProcessRegisterObserver(CoreObserver* observer)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    InterfaceCombination* ic;
    CombinationMap::iterator it = combinations.find(observer->mandatory);
    if (it == combinations.end()) {
        QCC_DbgPrintf(("First observer for this set of interfaces."));
        /* first observer for this particular set of mandatory interfaces */
        ic = new InterfaceCombination(this, observer->mandatory);
        combinations[observer->mandatory] = ic;
        const char** intfs = SetToArray(observer->mandatory);
        bus.WhoImplementsNonBlocking(intfs, observer->mandatory.size());
        delete[] intfs;
    } else {
        QCC_DbgPrintf(("Extra observer for an existing set of interfaces."));
        ic = it->second;
    }
    ic->AddObserver(observer);
}

void ObserverManager::UnregisterObserver(CoreObserver* observer)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    WorkItem* workitem = new UnregisterObserverWork(observer);
    ScheduleWork(workitem);
    /* UnregisterObserver is typically called from the application threads
     * instead of the dispatcher thread. We cannot do work on the application
     * threads (that would cause listener callbacks to be invoked from the
     * wrong thread context), so instead we make sure the dispatcher calls
     * us back to do the work.
     */
    TriggerDoWork();
}

void ObserverManager::ProcessUnregisterObserver(CoreObserver* observer)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    CombinationMap::iterator it = combinations.find(observer->mandatory);
    if (it != combinations.end()) {
        InterfaceCombination* ic = it->second;
        bool keep = ic->RemoveObserver(observer);
        if (!keep) {
            /* clean up everything related to this InterfaceCombination */
            combinations.erase(it);
            const char** intfs = SetToArray(observer->mandatory);
            bus.CancelWhoImplementsNonBlocking(intfs, observer->mandatory.size());
            delete[] intfs;
            delete ic;
            CheckRelevanceAllPeers();
        }
    } else {
        QCC_LogError(ER_FAIL, ("Unregistering an observer that was not registered"));
    }
}

void ObserverManager::EnablePendingListeners(CoreObserver* observer)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    WorkItem* workitem = new EnablePendingListenersWork(observer);
    ScheduleWork(workitem);
    TriggerDoWork();
}

void ObserverManager::ProcessEnablePendingListeners(CoreObserver* observer, const InterfaceSet& interfaces)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /* we can't be 100% sure that the observer hasn't been destroyed between scheduling
     * and executing this work item. Therefore, we don't dereference the observer pointer
     * before we're certain it's still registered in the relevant InterfaceCombination.
     * This is why we pass the set of mandatory interfaces as a separate argument rather
     * than retrieving it from the observer itself.
     */
    CombinationMap::iterator cit = combinations.find(interfaces);
    if (cit == combinations.end()) {
        return;
    }
    std::vector<CoreObserver*>::iterator obsit;
    for (obsit = cit->second->observers.begin(); obsit != cit->second->observers.end(); ++obsit) {
        if (*obsit == observer) {
            break;
        }
    }
    if (obsit == cit->second->observers.end()) {
        return;
    }
    observer->EnablePendingListeners();
}

void ObserverManager::HandleNewPeerAnnouncement(const Peer& peer, const ObjectSet& announced)
{
    QCC_DbgTrace(("%s: %s", __FUNCTION__, peer.busname.c_str()));
    bool relevant = CheckRelevance(announced);
    if (!relevant) {
        QCC_DbgPrintf(("not relevant"));
        return;
    }

    /* add to list of pending peers and wait for the session to be established */
    std::pair<DiscoveryMap::iterator, bool> inserted =
        pending.insert(std::make_pair(peer, announced));
    Peer* ctx = new Peer(peer);

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                     SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = bus.JoinSessionAsync(peer.busname.c_str(), peer.port,
                                          this, opts, this, ctx);
    if (ER_OK != status) {
        /* could not set up session. abort. */
        QCC_LogError(status, ("JoinSessionAsync invocation failed"));
        pending.erase(inserted.first);
        delete ctx;
    }
}

void ObserverManager::HandlePendingPeerAnnouncement(DiscoveryMap::iterator peerit, const ObjectSet& announced)
{
    QCC_DbgTrace(("%s(%s)", __FUNCTION__, peerit->first.busname.c_str()));
    bool relevant = CheckRelevance(announced);
    if (!relevant) {
        QCC_DbgPrintf(("not relevant"));
        /* while we were waiting for the session to be set up, the peer
         * has removed its last object of interest. We'll replace the
         * announced object set with an empty set to indicate to the
         * SessionEstablished callback that it can discard the session. */
        peerit->second.clear();
        return;
    }

    /* simply update the set of announced objects */
    peerit->second = announced;
}

void ObserverManager::HandleActivePeerAnnouncement(DiscoveryMap::iterator peerit, const ObjectSet& announced)
{
    QCC_DbgTrace(("%s(%s)", __FUNCTION__, peerit->first.busname.c_str()));
    ObjectSet previous = peerit->second;

    ObjectSet added, removed;
    set_difference(announced.begin(), announced.end(),
                   previous.begin(), previous.end(),
                   std::inserter(added, added.begin()));
    set_difference(previous.begin(), previous.end(),
                   announced.begin(), announced.end(),
                   std::inserter(removed, removed.begin()));

    CombinationMap::iterator cit;
    for (cit = combinations.begin(); cit != combinations.end(); ++cit) {
        cit->second->ObjectsLost(removed);
    }
    bool relevant = false;
    for (cit = combinations.begin(); cit != combinations.end(); ++cit) {
        relevant = cit->second->ObjectsDiscovered(added, peerit->first.sessionid) || relevant;
    }

    if (!relevant) {
        /* if we don't yet know for sure that the peer is still relevant, check it here
         * exhaustively */
        relevant = CheckRelevance(announced);
    }

    if (!relevant) {
        /* this peer is no longer relevant to us, tear down session and remove from the
         * active peer list */
        QCC_DbgPrintf(("not relevant"));
        bus.LeaveJoinedSessionAsync(peerit->first.sessionid, this, NULL);
        pinger->RemoveDestination(PING_GROUP, peerit->first.busname);
        active.erase(peerit);
    } else {
        /* update the set of discovered objects */
        peerit->second = announced;
    }
}

bool ObserverManager::CheckRelevance(const ObjectSet& objects)
{
    ObjectSet::iterator oit;
    for (oit = objects.begin(); oit != objects.end(); ++oit) {
        CombinationMap::iterator cit;
        for (cit = combinations.begin(); cit != combinations.end(); ++cit) {
            if (oit->ImplementsAll(cit->second->interfaces)) {
                return true;
            }
        }
    }
    return false;
}

void ObserverManager::CheckRelevanceAllPeers()
{
    DiscoveryMap::iterator it;
    for (it = pending.begin(); it != pending.end(); ++it) {
        bool relevant = CheckRelevance(it->second);
        if (!relevant) {
            it->second.clear();
        }
    }

    std::vector<DiscoveryMap::iterator> irrelevant;
    for (it = active.begin(); it != active.end(); ++it) {
        bool relevant = CheckRelevance(it->second);
        if (!relevant) {
            irrelevant.push_back(it);
        }
    }
    for (std::vector<DiscoveryMap::iterator>::iterator iit = irrelevant.begin();
         iit != irrelevant.end(); ++iit) {
        bus.LeaveJoinedSessionAsync((*iit)->first.sessionid, this, NULL);
        pinger->RemoveDestination(PING_GROUP, (*iit)->first.busname);
        active.erase(*iit);
    }
}

void ObserverManager::ScheduleWork(WorkItem* workitem)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    wqLock.Lock(MUTEX_CONTEXT);
    if (started && !stopping) {
        workitem->mgr = this;
        work.push(workitem);
    } else {
        delete workitem;
    }
    wqLock.Unlock(MUTEX_CONTEXT);
}

void ObserverManager::DoWork()
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    for (;;) {
        WorkItem* workitem = NULL;
        wqLock.Lock(MUTEX_CONTEXT);
        if (!processingWork && !work.empty() && started && !stopping) {
            workitem = work.front();
            work.pop();
            processingWork = true;
        }
        wqLock.Unlock(MUTEX_CONTEXT);

        if (!workitem) {
            break;
        }

        QCC_DbgPrintf(("%s: got work item.", __FUNCTION__));

        workitem->Execute();
        delete workitem;

        wqLock.Lock(MUTEX_CONTEXT);
        processingWork = false;
        processingDone.Broadcast();
        wqLock.Unlock(MUTEX_CONTEXT);
    }
}

void ObserverManager::TriggerDoWork()
{
    bus.GetInternal().GetLocalEndpoint()->TriggerObserverWork();
}

void ObserverManager::Announced(const char* busName, uint16_t version,
                                SessionPort port, const MsgArg& objectDescriptionArg,
                                const MsgArg& aboutDataArg)
{
    QCC_UNUSED(version);
    QCC_UNUSED(aboutDataArg);
    QCC_DbgPrintf(("Received announcement from '%s'", busName));

    ObjectSet announced = ParseObjectDescriptionArg(busName, objectDescriptionArg);
#ifndef NDEBUG
    for (ObjectSet::iterator it = announced.begin(); it != announced.end(); ++it) {
        QCC_DbgPrintf(("- %s", it->id.objectPath.c_str()));
        for (InterfaceSet::iterator iit = it->implements.begin();
             iit != it->implements.end(); ++iit) {
            QCC_DbgPrintf(("-- %s", iit->c_str()));
        }
    }
#endif

    AnnouncementWork* workitem = new AnnouncementWork(busName, port, announced);
    ScheduleWork(workitem);
    TriggerDoWork();
}

void ObserverManager::ProcessAnnouncement(const Peer& peer, const ObjectSet& announced)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    DiscoveryMap::iterator peerit = active.find(peer);
    if (peerit != active.end()) {
        /* update of a peer with which we have an active session */
        HandleActivePeerAnnouncement(peerit, announced);
    } else {
        peerit = pending.find(peer);
        if (peerit != pending.end()) {
            /* we're actually waiting for a session with this peer to be set up */
            HandlePendingPeerAnnouncement(peerit, announced);
        } else {
            /* this is the first time we hear from this peer */
            HandleNewPeerAnnouncement(peer, announced);
        }
    }
}

void ObserverManager::JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void*ctx)
{
    QCC_UNUSED(opts);
    QCC_DbgTrace(("%s", __FUNCTION__));
    Peer* peer = reinterpret_cast<Peer*>(ctx);
    WorkItem* workitem;
    if (ER_OK == status) {
        workitem = new SessionEstablishedWork(peer->busname, peer->port, sessionId);
    } else {
        workitem = new SessionEstablishmentFailedWork(peer->busname, peer->port);
    }
    delete peer;

    ScheduleWork(workitem);
    TriggerDoWork();
}

void ObserverManager::ProcessSessionEstablished(const ObserverManager::Peer& peer)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /* we expect the peer in question to be part of the pending set. */
    DiscoveryMap::iterator peerit = pending.find(peer);
    if (peerit == pending.end()) {
        /* this is awkward... */
        QCC_LogError(ER_FAIL,
                     ("Unexpected: session is established, but the peer is not part of the pending set"));
    } else if (peerit->second.empty()) {
        /* in the time it took us to set up the session, the peer removed the last
         * of its relevant objects. */
        pending.erase(peerit);
        bus.LeaveJoinedSessionAsync(peer.sessionid, this, NULL);
    } else {
        /* move peer from pending set to active set */
        DiscoveryMap::iterator newit = active.insert(std::make_pair(peer, peerit->second)).first;
        pending.erase(peerit);
        pinger->AddDestination(PING_GROUP, peer.busname);

        QCC_DbgPrintf(("Moving peer %s from pending to active state.", peer.busname.c_str()));
        /* notify interested observers of the newly announced objects */
        CombinationMap::iterator cit;
        for (cit = combinations.begin(); cit != combinations.end(); ++cit) {
            cit->second->ObjectsDiscovered(newit->second, peer.sessionid);
        }
    }
}

void ObserverManager::ProcessSessionEstablishmentFailed(const ObserverManager::Peer& peer)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /* we expect the peer in question to be part of the pending set. */
    DiscoveryMap::iterator peerit = pending.find(peer);
    if (peerit == pending.end()) {
        /* this is awkward... */
        QCC_LogError(ER_FAIL,
                     ("Unexpected: session establishment failed, but the peer is not part of the pending set"));
    } else {
        pending.erase(peerit);
    }
}

void ObserverManager::SessionLost(SessionId sessionId, SessionLostReason reason)
{
    QCC_UNUSED(reason);
    QCC_DbgPrintf(("Session lost for '%u'", (unsigned) sessionId));
    WorkItem* workitem = new SessionLostWork(sessionId);
    ScheduleWork(workitem);
    TriggerDoWork();
}

void ObserverManager::ProcessSessionLost(SessionId sessionid)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    DiscoveryMap::iterator peerit;
    for (peerit = active.begin(); peerit != active.end(); ++peerit) {
        if (peerit->first.sessionid == sessionid) {
            break;
        }
    }
    if (peerit != active.end()) {
        /* remove from the active list, notify interested observers */
        CombinationMap::iterator cit;
        for (cit = combinations.begin(); cit != combinations.end(); ++cit) {
            cit->second->ObjectsLost(peerit->second);
        }

        pinger->RemoveDestination(PING_GROUP, peerit->first.busname);
        active.erase(peerit);
    } else {
        QCC_LogError(ER_FAIL, ("Unexpected: lost a session we didn't ask for to begin with"));
    }
}

void ObserverManager::LeaveSessionCB(QStatus status, void* context)
{
    QCC_UNUSED(status);
    QCC_UNUSED(context);
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void ObserverManager::DestinationLost(const qcc::String& group, const qcc::String& destination)
{
    QCC_UNUSED(group);
    QCC_DbgPrintf(("Destination lost for '%s'", destination.c_str()));
    WorkItem* workitem = new DestinationLostWork(destination);
    ScheduleWork(workitem);
    TriggerDoWork();
}

void ObserverManager::ProcessDestinationLost(const qcc::String& busname)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /* we no longer care about this bus name */
    pinger->RemoveDestination(PING_GROUP, busname);

    DiscoveryMap::iterator peerit;
    for (peerit = active.begin(); peerit != active.end(); ++peerit) {
        if (peerit->first.busname == busname) {
            break;
        }
    }
    if (peerit != active.end()) {
        /* remove from the active list, notify interested observers, drop session */
        bus.LeaveJoinedSessionAsync(peerit->first.sessionid, this, NULL);
        CombinationMap::iterator cit;
        for (cit = combinations.begin(); cit != combinations.end(); ++cit) {
            cit->second->ObjectsLost(peerit->second);
        }
        active.erase(peerit);
    }
}

bool ObserverManager::InterfaceCombination::ObjectsDiscovered(const ObjectSet& objects, SessionId sessionid)
{
    bool relevant = false;

    ObjectSet::iterator oit;
    for (oit = objects.begin(); oit != objects.end(); ++oit) {
        QCC_DbgPrintf(("Checking object %s:%s", oit->id.uniqueBusName.c_str(), oit->id.objectPath.c_str()));
        if (!oit->ImplementsAll(interfaces)) {
            QCC_DbgPrintf(("Not relevant..."));
            continue;
        }
        relevant = true;

        std::vector<CoreObserver*>::iterator it;
        for (it = observers.begin(); it != observers.end(); ++it) {
            (*it)->ObjectDiscovered(oit->id, oit->implements, sessionid);
        }
    }
    return relevant;
}

bool ObserverManager::InterfaceCombination::ObjectsLost(const ObjectSet& objects)
{
    bool relevant = false;

    ObjectSet::iterator oit;
    for (oit = objects.begin(); oit != objects.end(); ++oit) {
        if (!oit->ImplementsAll(interfaces)) {
            continue;
        }
        relevant = true;

        std::vector<CoreObserver*>::iterator it;
        for (it = observers.begin(); it != observers.end(); ++it) {
            (*it)->ObjectLost(oit->id);
        }
    }
    return relevant;
}

void ObserverManager::InterfaceCombination::AddObserver(CoreObserver* observer)
{
    std::vector<CoreObserver*>::iterator it;
    bool found = false;
    for (it = observers.begin(); it != observers.end(); ++it) {
        if (*it == observer) {
            found = true;
            break;
        }
    }
    if (found) {
        QCC_LogError(ER_FAIL, ("Attempt to register the same observer twice"));
        return;
    }

    observers.push_back(observer);

    /* let the observer know about existing relevant objects */
    DiscoveryMap::iterator peerit;
    for (peerit = obsmgr->active.begin(); peerit != obsmgr->active.end(); ++peerit) {
        ObjectSet::iterator oit;
        for (oit = peerit->second.begin(); oit != peerit->second.end(); ++oit) {
            if (oit->ImplementsAll(interfaces)) {
                observer->ObjectDiscovered(oit->id, oit->implements, peerit->first.sessionid);
            }
        }
    }
}

bool ObserverManager::InterfaceCombination::RemoveObserver(CoreObserver* observer)
{
    std::vector<CoreObserver*>::iterator it;
    bool found = false;
    for (it = observers.begin(); it != observers.end(); ++it) {
        if (*it == observer) {
            observers.erase(it);
            found = true;
            break;
        }
    }
    if (!found) {
        QCC_LogError(ER_FAIL, ("Unregistering an observer that was not registered"));
    }
    return observers.size() != 0;
}

