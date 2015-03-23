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

#ifndef OBSERVERMANAGER_H_
#define OBSERVERMANAGER_H_

#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <iterator>

#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Condition.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Observer.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/SessionListener.h>
#include <alljoyn/AutoPinger.h>
#include "CoreObserver.h"

namespace ajn {

class ObserverManager :
    private AboutListener,
    private SessionListener,
    private BusAttachment::JoinSessionAsyncCB,
    private BusAttachment::LeaveSessionAsyncCB,
    private PingListener {

    struct InterfaceCombination;
    friend struct InterfaceCombination;

    /* forward declaration of the various work queue tasks */
    struct WorkItem;
    struct AnnouncementWork;
    friend struct AnnouncementWork;
    struct SessionEstablishedWork;
    friend struct SessionEstablishedWork;
    struct SessionEstablishmentFailedWork;
    friend struct SessionEstablishmentFailedWork;
    struct SessionLostWork;
    friend struct SessionLostWork;
    struct DestinationLostWork;
    friend struct DestinationLostWork;
    struct RegisterObserverWork;
    friend struct RegisterObserverWork;
    struct UnregisterObserverWork;
    friend struct UnregisterObserverWork;
    struct EnablePendingListenersWork;
    friend struct EnablePendingListenersWork;

  public:
    /**
     * Constructor. Never call this directly, use BusAttachment::Internal::GetObserverManager
     */
    ObserverManager(BusAttachment& bus);

    /**
     * Destructor.
     */
    ~ObserverManager();

    /**
     * Start the ObserverManager.
     */
    void Start();

    /**
     * Request to stop all ObserverManager activities.
     */
    void Stop();

    /**
     * Blocks until all ObserverManager activities are stopped.
     */
    void Join();

    /**
     * Register a new Observer with the ObserverManager
     *
     * \param observer the observer to register
     */
    void RegisterObserver(CoreObserver* observer);

    /**
     * Unregister an Observer from the ObserverManager
     *
     * \param observer the observer that must be detached from the cache
     */
    void UnregisterObserver(CoreObserver* observer);

    /**
     * Enable all pending listeners for an Observer.
     *
     * When new listeners are added to an existing observer, the observer
     * will optionally invoke the listener callbacks for all already-discovered
     * objects. We can't do this from within the RegisterListener context,
     * because that's typically an application thread. Therefore, we do not
     * yet enable the listener, but schedule work on the dispatcher thread to
     * perform the initial callbacks and enable the listener.
     *
     * \param observer the observer for which we want to enable pending listeners
     */
    void EnablePendingListeners(CoreObserver* observer);

    /**
     * Perform queued-up work.
     *
     * This method must only be called from a LocalEndpoint::dispatcher thread!
     * There is some complex synchronization logic here that makes sure there
     * is only one thread ever processing work from the queue. This call does not
     * block to wait for work, it just picks up work if there is some and there
     * is nobody else around to do it.
     */
    void DoWork();

  private:

    /**
     * Set of interface names.
     */
    typedef std::set<qcc::String> InterfaceSet;

    /**
     * Represents a bus object as advertised by About
     */
    struct DiscoveredObject {
        ObjectId id;
        InterfaceSet implements;

        DiscoveredObject() { }

        bool operator<(const DiscoveredObject& other) const {
            return id < other.id;
        }
        bool operator==(const DiscoveredObject& other) const {
            return id == other.id;
        }

        bool ImplementsAll(const InterfaceSet& interfaces) const {
            InterfaceSet diff;
            set_difference(interfaces.begin(), interfaces.end(),
                           implements.begin(), implements.end(),
                           std::inserter(diff, diff.begin()));
            return diff.empty();
        }
        bool ImplementsAny(const InterfaceSet& interfaces) const {
            InterfaceSet intersection;
            set_intersection(interfaces.begin(), interfaces.end(),
                             implements.begin(), implements.end(),
                             std::inserter(intersection, intersection.begin()));
            return !intersection.empty();
        }
    };

    /**
     * Represents a peer doing About announcements
     */
    struct Peer {
        qcc::String busname;
        SessionPort port;
        SessionId sessionid;

        Peer() : busname(""), port(0), sessionid(0) { }
        Peer(qcc::String busname, SessionPort port) : busname(busname), port(port), sessionid(0) { }
        bool operator<(const Peer& other) const {
            return (busname == other.busname)
                   ? (port < other.port)
                   : busname < other.busname;
        }
    };
    typedef std::set<DiscoveredObject> ObjectSet;
    typedef std::map<Peer, ObjectSet> DiscoveryMap;

    /**
     * Data structure that keeps track of the common data for all
     * Observers that have the same set of mandatory interfaces.
     */
    struct InterfaceCombination {
        ObserverManager* obsmgr;
        InterfaceSet interfaces;
        std::vector<CoreObserver*> observers;

        InterfaceCombination(ObserverManager* mgr, const InterfaceSet& intfs) :
            obsmgr(mgr), interfaces(intfs)
        { }

        InterfaceCombination(const InterfaceCombination& other) :
            obsmgr(other.obsmgr), interfaces(other.interfaces), observers(other.observers)
        { }

        InterfaceCombination& operator=(const InterfaceCombination& other) {
            obsmgr = other.obsmgr;
            interfaces = other.interfaces;
            observers = other.observers;
            return *this;
        }

        /**
         * A set of objects is lost.
         * Will trigger observer notifications if relevant.
         * \return true if any of the objects is relevant to the observer at hand
         */
        bool ObjectsLost(const ObserverManager::ObjectSet& objects);

        /**
         * A set of objects is discovered.
         * Will trigger observer notifications if relevant.
         * \return true if any of the objects is relevant to the observer at hand
         */
        bool ObjectsDiscovered(const ObserverManager::ObjectSet& objects, SessionId sessionid);

        /**
         * A new observer is registered for this interface combination.
         * Will populate the observer with all relevant objects that were
         * discovered prior to the creation of this observer.
         */
        void AddObserver(CoreObserver* observer);

        /**
         * Remove an observer.
         * \return true if more observers are left.
         */
        bool RemoveObserver(CoreObserver* observer);

        bool operator<(const InterfaceCombination& other) const {
            return interfaces < other.interfaces;
        }
    };

    /**
     * The interface combinations that are currently of interest.
     */
    typedef std::map<InterfaceSet, InterfaceCombination*> CombinationMap;
    CombinationMap combinations;

    /**
     * Discovered objects, waiting for a session with the peer to be set up.
     */
    DiscoveryMap pending;

    /**
     * Discovered objects, active session with peer ongoing.
     */
    DiscoveryMap active;

    /**
     * Reference to BusAttachment
     */
    BusAttachment& bus;

    /**
     * An AutoPinger instance to do the periodic liveness checks for us.
     */
    AutoPinger* pinger;

    /**
     * Process an announcement from a peer with which we're currently in session.
     */
    void HandleActivePeerAnnouncement(DiscoveryMap::iterator peerit, const ObjectSet& announced);

    /**
     * Process an announcement from a peer for which we're waiting on a session.
     */
    void HandlePendingPeerAnnouncement(DiscoveryMap::iterator peerit, const ObjectSet& announced);

    /**
     * Process an announcement from a peer that we currently don't yet care about.
     */
    void HandleNewPeerAnnouncement(const Peer& peer, const ObjectSet& announced);

    /**
     * Iterates over all pending and active peers to check whether they still hold
     * any relevant objects for any of the remaining observers.
     */
    void CheckRelevanceAllPeers();

    /**
     * Checks whether any of the objects in the set holds relevance for a registered observer
     */
    bool CheckRelevance(const ObjectSet& objects);

    /****************************
     * work queue related stuff *
     ****************************/
    std::queue<WorkItem*> work;
    qcc::Mutex wqLock;
    qcc::Condition processingDone;
    bool processingWork;
    bool stopping;
    bool started;

    /**
     * Add a work item to the work queue
     */
    void ScheduleWork(WorkItem* workitem);

    /**
     * Make sure the dispatcher calls us to do work.
     *
     * Sometimes work items get added to the queue from an application thread.
     * We can't do work on the application thread (it might result in listener
     * callbacks being invoked in the application thread, which we want to avoid).
     * This method posts an alarm on the dispatcher thread that we can use to
     * process work items in the correct thread context.
     */
    void TriggerDoWork();

    /**
     * AboutListener::Announced
     *
     * The objectDescriptionArg contains an array with a signature of `a(oas)`
     * this is an array object paths with a list of interfaces found at those paths
     *
     * @param[in] busName              well know name of the remote BusAttachment
     * @param[in] version              version of the Announce signal from the remote About Object
     * @param[in] port                 SessionPort used by the announcer
     * @param[in] objectDescriptionArg  MsgArg the list of object paths and interfaces in the announcement
     * @param[in] aboutDataArg          MsgArg containing a dictionary of Key/Value pairs of the AboutData
     */
    void Announced(const char* busName, uint16_t version,
                   SessionPort port, const MsgArg& objectDescriptionArg,
                   const MsgArg& aboutDataArg);

    /**
     * Processes the About announcement received in Announced()
     */
    void ProcessAnnouncement(const Peer& peer, const ObjectSet& announced);

    /**
     * Derived from BusAttachment::JoinSessionAsyncCB.
     */
    void JoinSessionCB(QStatus status, SessionId sessionId,
                       const SessionOpts& opts, void* context);

    /**
     * Processes the newly established session from JoinSessionCB()
     */
    void ProcessSessionEstablished(const Peer& peer);

    /**
     * Processes the failed session establishment from JoinSessionCB()
     */
    void ProcessSessionEstablishmentFailed(const Peer& peer);

    /**
     * Derived from the SessionListener
     */
    void SessionLost(SessionId sessionId, SessionLostReason reason);

    /**
     * Processes the lost session from SessionLost()
     */
    void ProcessSessionLost(SessionId sessionid);

    /**
     * Derived from BusAttachment::LeaveSessionAsyncCB.
     */
    void LeaveSessionCB(QStatus status, void* context);

    /**
     * Derived from PingListener.
     *
     * @param  group Pinging group name
     * @param  destination Destination that was pinged
     */
    void DestinationLost(const qcc::String& group, const qcc::String& destination);

    /**
     * Process a DestinationLost work item
     */
    void ProcessDestinationLost(const qcc::String& busname);

    /**
     * Process a RegisterObserver work item
     */
    void ProcessRegisterObserver(CoreObserver* observer);

    /**
     * Process an UnregisterObserver work item
     */
    void ProcessUnregisterObserver(CoreObserver* observer);

    /**
     * Process an EnablePendingListeners work item
     *
     * \param observer the observer for which to enable all pending listeners
     * \param interfaces the set of mandatory interfaces for the observer
     */
    void ProcessEnablePendingListeners(CoreObserver* observer, const InterfaceSet& interfaces);

    /**
     * Helper function that parses the object description argument from the About announcement
     */
    static ObjectSet ParseObjectDescriptionArg(const qcc::String& busname, const MsgArg& arg);

    /**
     * Helper function that builds the array argument to WhoImplements and CancelWhoImplements
     */
    static const char** SetToArray(const InterfaceSet& set);

};
}

#endif /* OBSERVERMANAGER_H_ */
