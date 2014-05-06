/**
 * @file
 * SessionObj is for implementing the daemon-to-daemon inteface org.alljoyn.sl
 */

/******************************************************************************
 * Copyright (c) 2012,2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_SESSIONLESSOBJ_H
#define _ALLJOYN_SESSIONLESSOBJ_H

#include <qcc/platform.h>

#include <map>
#include <set>
#include <queue>

#include <qcc/String.h>
#include <qcc/Timer.h>

#include <alljoyn/BusObject.h>
#include <alljoyn/Message.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/SessionListener.h>

#include "Bus.h"
#include "DaemonRouter.h"
#include "NameTable.h"
#include "RuleTable.h"
#include "Transport.h"

namespace ajn {

/** Forward Declaration */
class BusController;

/**
 * BusObject responsible for implementing the standard AllJoyn interface org.alljoyn.sl.
 */
class SessionlessObj : public BusObject, public NameListener, public SessionListener, public SessionPortListener,
    public BusAttachment::JoinSessionAsyncCB, public qcc::AlarmListener {

  public:
    /**
     * Constructor
     *
     * @param bus            Bus to associate with org.freedesktop.DBus message handler.
     * @param router         The DaemonRouter associated with the bus.
     * @param busController  Controller that created this object.
     */
    SessionlessObj(Bus& bus, BusController* busController);

    /**
     * Destructor
     */
    ~SessionlessObj();

    /**
     * Initialize and register this DBusObj instance.
     *
     * @return ER_OK if successful.
     */
    QStatus Init();

    /**
     * Stop SessionlessObj.
     *
     * @return ER_OK if successful.
     */
    QStatus Stop();

    /**
     * Join SessionlessObj.
     *
     * @return ER_OK if successful.
     */
    QStatus Join();

    /**
     * Called when object is successfully registered.
     */
    void ObjectRegistered(void);

    /**
     * Add a rule for an endpoint.
     *
     * @param epName   The name of the endpoint that this rule applies to.
     * @param rule     Rule for endpoint
     */
    void AddRule(const qcc::String& epName, Rule& rule);

    /**
     * Remove a rule for an endpoint.
     *
     * @param epName      The name of the endpoint that rule applies to.
     * @param rule        Rule to remove.
     */
    void RemoveRule(const qcc::String& epName, Rule& rule);

    /**
     * Push a sessionless signal.
     *
     * @param msg    Message to be pushed.
     */
    QStatus PushMessage(Message& msg);

    /**
     * Route an incoming sessionless signal if possible.
     *
     * @param sid   Session ID associated with sessionless message.
     * @param msg   Sesionless messgae to be routed.
     *
     * @return true if message was delivered, false otherwise.
     */
    bool RouteSessionlessMessage(uint32_t sid, Message& msg);

    /**
     * Remove a sessionless signal with a given serial number from the store/forward cache.
     *
     * @param sender      Unique name of message sender.
     * @param serialNum   Serial number of message to be removed from cache.
     * @param   ER_OK if successful
     */
    QStatus CancelMessage(const qcc::String& sender, uint32_t serialNum);

    /**
     * NameListener implementation called when a bus name changes ownership.
     *
     * @param busName   Unique or well-known bus name.
     * @param oldOwner  Unique name of old owner of name or NULL if none existed.
     * @param newOwner  Unique name of new owner of name or NULL if none (now) exists.
     */
    void NameOwnerChanged(const qcc::String& busName,
                          const qcc::String* oldOwner,
                          const qcc::String* newOwner);

    /**
     * Receive FoundAdvertisedName signals.
     *
     * @param   member      FoundAdvertisedName interface member.
     * @param   sourcePath  Sender of signal.
     * @param   msg         FoundAdvertisedName message.
     */
    void FoundAdvertisedNameSignalHandler(const InterfaceDescription::Member* member,
                                          const char* sourcePath,
                                          Message& msg);

    /**
     * Accept/reject join attempt on sessionlesss port.
     * Implements SessionPortListener::AceptSessionJoiner.
     *
     * @param port    Session port of join attempt.
     * @param joiner  Unique name of joiner.
     * @param opts    SesionOpts specified by joiner.
     * @return   true if session is accepted. false otherwise.
     */
    bool AcceptSessionJoiner(SessionPort port,
                             const char* joiner,
                             const SessionOpts& opts);

    /**
     * Receive SessionLost signals.
     *
     * @param   member      SessionLost interface member.
     * @param   sourcePath  Sender of signal.
     * @param   msg         SessionLost message.
     */
    void SessionLostSignalHandler(const InterfaceDescription::Member* member,
                                  const char* sourcePath,
                                  Message& msg);

    /**
     * Process incoming RequestSignals signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void RequestSignalsSignalHandler(const InterfaceDescription::Member* member,
                                     const char* sourcePath,
                                     Message& msg);

    /**
     * Process incoming RequestRange signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void RequestRangeSignalHandler(const InterfaceDescription::Member* member,
                                   const char* sourcePath,
                                   Message& msg);

    /**
     * Trigger (re)reception of sessionless signals from all remote daemons.
     *
     * @param epName   The name of the endpoint that is requesting to re-receive sessionless messages.
     * @param rule     The rule that triggered the re-receive.
     */
    QStatus RereceiveMessages(const qcc::String& epName, const Rule& rule);

  private:
    friend class RemoteQueueSnapshot;

    /**
     * SessionlessObj worker.
     *
     * @param alarm  The alarm object for the timeout that expired.
     */
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    /**
     * JoinSession Callback.
     */
    void JoinSessionCB(QStatus status, SessionId sid, const SessionOpts& opts, void* context);

    /**
     * Emit the range of cached sessionless signals [fromId, toId)
     *
     * @param sender    Unique name of requestor/sender
     * @param sid       Session ID
     * @param fromId    Beginning of changeId range (inclusive)
     * @param toId      End of changeId range (exclusive)
     */
    void HandleRangeRequest(const char* sender, SessionId sid, uint32_t fromId, uint32_t toId);

    /**
     * SessionLost helper handler.
     *
     * @param sid    Session ID of lost session.
     * @param reason The reason for the session being lost.
     */
    void DoSessionLost(SessionId sid, SessionLostReason reason);

    /**
     * Returns the number of rules that specify sessionless=TRUE.
     *
     * This must be called with this lock.
     *
     * @return the number of rules that specify sessionless=TRUE.
     */
    uint32_t RuleCount();

    Bus& bus;                             /**< The bus */
    BusController* busController;         /**< BusController that created this BusObject */
    DaemonRouter& router;                 /**< The router */

    /* The version implemented. */
    static const uint32_t version;

    const InterfaceDescription* sessionlessIface;  /**< org.alljoyn.sl interface */

    const InterfaceDescription::Member* requestSignalsSignal;   /**< org.alljoyn.sl.RequestSignal signal */
    const InterfaceDescription::Member* requestRangeSignal;     /**< org.alljoyn.sl.RequestRange signal */

    qcc::Timer timer;                     /**< Timer object for reaping expired names */

    /** A key into the local sessionless message queue */
    class SessionlessMessageKey : public qcc::String {
      public:
        SessionlessMessageKey(const char* sender, const char* iface, const char* member, const char* objPath) :
            qcc::String(sender, 0, ::strlen(sender) + ::strlen(iface) + ::strlen(member) + ::strlen(objPath) + 4)
        {
            append(':');
            append(iface);
            append(':');
            append(member);
            append(':');
            append(objPath);
        }
    };
    typedef std::pair<uint32_t, Message> SessionlessMessage;

    typedef std::map<SessionlessMessageKey, SessionlessMessage> LocalQueue;
    /** Storage for sessionless messages waiting to be delivered */
    LocalQueue localQueue;

    /** CatchupState is used to track individual local clients that are behind the state of the server for a particular remote host */
    struct CatchupState {
        CatchupState() : changeId(0) { }
        CatchupState(const qcc::String& epName, const Rule& rule, uint32_t changeId) :
            epName(epName), rule(rule), changeId(changeId) { }
        qcc::String epName;
        Rule rule;
        uint32_t changeId;
    };
    /** Map session IDs to catchupStates */
    std::map<uint32_t, CatchupState> catchupMap;

    struct RoutedMessage {
        RoutedMessage(const Message& msg) : sender(msg->GetSender()), serial(msg->GetCallSerial()) { }
        qcc::String sender;
        uint32_t serial;
        bool operator==(const RoutedMessage& other) const { return (sender == other.sender) && (serial == other.serial); }
    };

    class RemoteQueueKey : public qcc::String {
      public:
        RemoteQueueKey(const qcc::String& guid, const qcc::String& iface) : qcc::String(guid + ":" + iface) { }
    };

    class RemoteQueue {
      public:
        RemoteQueue(uint32_t version, const qcc::String& guid, const qcc::String& iface, uint32_t changeId, TransportMask transport) :
            version(version), guid(guid), iface(iface), changeId(changeId), transport(transport),
            lastReceivedChangeId(std::numeric_limits<uint32_t>::max()), inProgressChangeId(0), inProgressTimestamp(0),
            nextJoinTimestamp(0), retries(0), sid(0) { }

        void ReceiveStarted() {
            inProgressChangeId = changeId;
            inProgressTimestamp = qcc::GetTimestamp64();
        }
        bool IsReceiveInProgress() {
            return inProgressTimestamp != 0;
        }
        void ReceiveCompleted() {
            inProgressTimestamp = 0;
            sid = 0;
        }

        /* The state from the most recent advertisement */
        uint32_t version;
        qcc::String guid;
        qcc::String iface;
        uint32_t changeId;
        TransportMask transport;

        uint32_t lastReceivedChangeId;
        uint32_t inProgressChangeId;
        uint64_t inProgressTimestamp;
        uint64_t nextJoinTimestamp;
        uint32_t retries;
        SessionId sid;
        std::queue<CatchupState> catchupList;
        std::list<RoutedMessage> routedMessages;
    };

    typedef std::map<RemoteQueueKey, RemoteQueue> RemoteQueues;
    /** The state of found remote queues */
    RemoteQueues remoteQueues;

    /** Find the remote queue with the matching session ID */
    RemoteQueues::iterator FindRemoteQueue(SessionId sid);

    qcc::Mutex lock;            /**< Mutex that protects this object's data structures */
    uint32_t curChangeId;       /**< Change id assoc with current pushed signal(s) */
    bool isDiscoveryStarted;    /**< True when FindAdvetiseName is ongoing */
    SessionOpts sessionOpts;    /**< SessionOpts used by internal session */
    SessionPort sessionPort;    /**< SessionPort used by internal session */
    bool advanceChangeId;       /**< Set to true when changeId should be advanced on next SLS send request */

    /**
     * Try join with random backoff of 1 to 256ms.
     *
     * @param[in,out] queue the host to schedule the retry for
     */
    void ScheduleTry(RemoteQueue& queue);

    /**
     * Retry join with random backoff of 200ms to ~8.5s.
     *
     * @param[in,out] queue the host to schedule the retry for
     *
     * @return ER_OK if retry scheduled, failure if retries exhausted
     */
    QStatus ScheduleRetry(RemoteQueue& queue);

    /**
     * Internal helper for scheduling a join try or retry.
     *
     * @param[in,out] queue the host to schedule the join for
     * @param[in] delay the delay in msecs
     */
    void ScheduleJoin(RemoteQueue& queue, uint32_t delayMs);

    /**
     * Internal helper for parsing an advertised name into its guid, change
     * ID, and version parts.
     *
     * An advertised name has the following form: "org.alljoyn.sl.xGUID.xCID".
     * The 'x' of "xGUID" is abused to indicate a version.  'x' is version 0,
     * 'y' is version 1, etc.  This is to enable backwards compatibility for
     * version 0 implementations.
     *
     * @param[in] name
     * @param[out] version
     * @param[out] guid
     * @param[out] iface
     * @param[out] changeId
     *
     * @return ER_OK if parsed succesfully
     */
    QStatus ParseAdvertisedName(const qcc::String& name, uint32_t* version, qcc::String* guid, qcc::String* iface, uint32_t* changeId);

    /**
     * Internal helper for sending the RequestSignals signal.
     *
     * @param[in] name   Advertised name of sender
     * @param[in] sid    Session ID
     * @param[in] fromId Beginning of changeId range (inclusive)
     */
    QStatus RequestSignals(const char* name, SessionId sid, uint32_t fromId);

    /**
     * Internal helper for sending the RequestRange signal.
     *
     * @param[in] name   Advertised name of sender
     * @param[in] sid    Session ID
     * @param[in] fromId Beginning of changeId range (inclusive)
     * @param[in] toId   End of changeId range (exclusive)
     */
    QStatus RequestRange(const char* name, SessionId sid, uint32_t fromId, uint32_t toId);

    /**
     * Internal helper for sending sessionless signals.
     *
     * @param[in] msg The sessionless signal
     * @param[in] ep The destination endpoint
     * @param[in] sid Session ID
     */
    QStatus SendThroughEndpoint(Message& msg, BusEndpoint& ep, SessionId sid);

    /**
     * A match rule that includes a timestamp for recording when it was entered
     * into the rule table.
     */
    struct TimestampedRule : public Rule {
        TimestampedRule(Rule& rule) : Rule(rule), timestamp(qcc::GetTimestamp64()) { }
        uint64_t timestamp;
    };

    /** Table of endpoint name, timestamped rule. */
    std::multimap<qcc::String, TimestampedRule> rules;

    /** Rule iterator */
    typedef std::multimap<qcc::String, TimestampedRule>::iterator RuleIterator;

    /*
     * Advertise or cancel the SL advertisements.
     *
     * Walks through the local queue and determines if the advertisements need to
     * be updated or cancelled.  Updates lastAdvertisements.  Called without any
     * locks.
     */
    void UpdateAdvertisements();

    /*
     * Map from last advertised name to last advertised change ID.
     */
    std::map<qcc::String, uint32_t> lastAdvertisements;

    /*
     * Helper to create a segmented advertised name.
     */
    qcc::String AdvertisedName(const qcc::String& iface, uint32_t changeId);

    /*
     * Helper to request and advertise a name.
     */
    QStatus AdvertiseName(const qcc::String& name);

    /*
     * Helper to cancel advertisement and release a name.
     */
    void CancelAdvertisedName(const qcc::String& name);

    /*
     * List of advertised names currently being looked for.
     */
    std::set<qcc::String> findingNames;

    /*
     * Find advertised names based from the sessionless match rules.
     */
    void FindAdvertisedNames();

    /*
     * Stop finding advertised names from the sessionless match rules.
     */
    void CancelFindAdvertisedNames();

    /*
     * @param[in] queue
     *
     * @return true if the sessionless match rules include a match for the
     * queue.
     */
    bool IsMatch(RemoteQueue& queue);
};

}

#endif
