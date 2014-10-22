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
#include "ns/IpNameService.h"

namespace ajn {

/** Forward Declaration */
class BusController;

/**
 * BusObject responsible for implementing the standard AllJoyn interface org.alljoyn.sl.
 */
class SessionlessObj : public BusObject, public NameListener, public SessionListener, public SessionPortListener,
    public BusAttachment::JoinSessionAsyncCB, public qcc::AlarmListener, public IpNameServiceListener {

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

    void NameOwnerChanged(const qcc::String& busName,
                          const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                          const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer);

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
     * Implements SessionPortListener::AcceptSessionJoiner.
     *
     * @param port    Session port of join attempt.
     * @param joiner  Unique name of joiner.
     * @param opts    SesionOpts specified by joiner.
     *
     * @return true if session is accepted. false otherwise.
     */
    bool AcceptSessionJoiner(SessionPort port,
                             const char* joiner,
                             const SessionOpts& opts);

    /**
     * Called by the bus when a session has been successfully joined.
     * Implements SessionPortListener::SessionJoined.
     *
     * @param port    Session port of join attempt.
     * @param sid     Id of session.
     * @param joiner  Unique name of joiner.
     */
    void SessionJoined(SessionPort port,
                       SessionId sid,
                       const char* joiner);

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
     * Process incoming RequestRangeMatch signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void RequestRangeMatchSignalHandler(const InterfaceDescription::Member* member,
                                        const char* sourcePath,
                                        Message& msg);
    /**
     * The parameters of the backoff algorithm.
     */
    struct BackoffLimits {
        uint32_t periodMs; /**< the initial backoff period */
        uint32_t linear; /**< the maximum linear backoff coefficient before switching to exponential */
        uint32_t exponential; /**< the maximum exponential backoff coefficient before switching to constant */
        uint32_t maxSecs; /**< the total backoff time in seconds before giving up */
        BackoffLimits(uint32_t periodMs, uint32_t linear, uint32_t exponential, uint32_t maxSecs)
            : periodMs(periodMs), linear(linear), exponential(exponential), maxSecs(maxSecs) { }
    };

    /**
     * Computes the next time to try fetching signals from a remote router.
     *
     * This implements the backoff algorithm.
     *
     * @param[in] backoff the backoff algorithm parameters
     * @param[in] doInitialBackoff true to do the initial backoff or false to skip.
     * @param[in] retries the number of fetches tried so far.
     * @param[in,out] firstJoinTime set when retries is 0, used in computing nextJoinTime when retries is greater than 0.
     * @param[out] nextJoinTime the next time to try fetching signals.
     *
     * @return ER_FAIL if the total retry time is exhausted, ER_OK otherwise
     */
    static QStatus GetNextJoinTime(const BackoffLimits& backoff, bool doInitialBackoff,
                                   uint32_t retries, qcc::Timespec& firstJoinTime, qcc::Timespec& nextJoinTime);

  private:
    friend struct RemoteCacheSnapshot;

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
     * Emit the range of cached sessionless signals [fromId, toId).
     *
     * When sid is 0, rules in the specified local range are applied before
     * emitting the signals.
     *
     * When sid is non-0, rules in the remote rules are applied before emitting
     * the signals.
     *
     * @param sender           Unique name of requestor/sender
     * @param sid              Session ID
     * @param fromId           Beginning of changeId range (inclusive)
     * @param toId             End of changeId range (exclusive)
     * @param fromLocalRulesId Beginning of rules ID range (inclusive)
     * @param toLocalRulesId   End of rules ID range (exclusive)
     * @param remoteRules      Remotely-supplied rules to apply
     */
    void HandleRangeRequest(const char* sender, SessionId sid,
                            uint32_t fromId, uint32_t toId,
                            uint32_t fromLocalRulesId = 0, uint32_t toLocalRulesId = 0,
                            std::vector<qcc::String> remoteRules = std::vector<qcc::String>());

    /**
     * SessionLost helper handler.
     *
     * @param sid    Session ID of lost session.
     * @param reason The reason for the session being lost.
     */
    void DoSessionLost(SessionId sid, SessionLostReason reason);

    Bus& bus;                             /**< The bus */
    BusController* busController;         /**< BusController that created this BusObject */
    DaemonRouter& router;                 /**< The router */

    /* The version implemented. */
    static const uint32_t version;

    static const Rule legacyRule;

    const InterfaceDescription* sessionlessIface;  /**< org.alljoyn.sl interface */

    const InterfaceDescription::Member* requestSignalsSignal;    /**< org.alljoyn.sl.RequestSignal signal */
    const InterfaceDescription::Member* requestRangeSignal;      /**< org.alljoyn.sl.RequestRange signal */
    const InterfaceDescription::Member* requestRangeMatchSignal; /**< org.alljoyn.sl.RequestRangeMatch signal */

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

    typedef std::map<SessionlessMessageKey, SessionlessMessage> LocalCache;
    /** Storage for sessionless messages waiting to be delivered */
    LocalCache localCache;

    struct RoutedMessage {
        RoutedMessage(const Message& msg) : sender(msg->GetSender()), serial(msg->GetCallSerial()) { }
        qcc::String sender;
        uint32_t serial;
        bool operator==(const RoutedMessage& other) const { return (sender == other.sender) && (serial == other.serial); }
    };

    class RemoteCache {
      public:
        RemoteCache(const qcc::String& name, uint32_t version, const qcc::String& guid, const qcc::String& iface, uint32_t changeId, TransportMask transport) :
            name(name), version(version), guid(guid), changeId(changeId), transport(transport), haveReceived(false),
            receivedChangeId(std::numeric_limits<uint32_t>::max()), appliedRulesId(std::numeric_limits<uint32_t>::max()),
            state(IDLE), retries(0), sid(0) {
            ifaces.insert(iface);
        }

        /* The most recent advertisement */
        qcc::String name;
        /* The union of all the advertisements */
        uint32_t version;
        qcc::String guid;
        std::set<qcc::String> ifaces;
        uint32_t changeId;
        TransportMask transport;

        /* State */
        bool haveReceived; /* true once we have received something from the remote cache */
        uint32_t receivedChangeId;
        uint32_t appliedRulesId;

        /* Work item */
        uint32_t fromChangeId, toChangeId;
        uint32_t fromRulesId, toRulesId;
        /* Work item state */
        enum {
            IDLE = 0,
            IN_PROGRESS
        } state;
        uint32_t retries;
        qcc::Timespec firstJoinTime;
        qcc::Timespec nextJoinTime;
        SessionId sid;
        std::list<RoutedMessage> routedMessages;
    };

    typedef std::map<qcc::String, RemoteCache> RemoteCaches;
    /** The state of found remote caches */
    RemoteCaches remoteCaches;

    /** Find the remote cache work item with the matching session ID */
    RemoteCaches::iterator FindRemoteCache(SessionId sid);

    /** Erase info associated with the remote cache */
    void EraseRemoteCache(RemoteCaches::iterator cit);

    qcc::Mutex lock;             /**< Mutex that protects this object's data structures */
    uint32_t curChangeId;        /**< Change id assoc with current pushed signal(s) */
    bool isDiscoveryStarted;     /**< True when FindAdvetiseName is ongoing */
    SessionOpts sessionOpts;     /**< SessionOpts used by internal session */
    SessionPort sessionPort;     /**< SessionPort used by internal session */
    bool advanceChangeId;        /**< Set to true when changeId should be advanced on next SLS send request */
    uint32_t nextRulesId;        /**< The next added rule change ID */
    const BackoffLimits backoff; /**< The backoff algorithm parameters */

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
     * Internal helper for sending the RequestRangeMatch signal.
     *
     * @param[in] name        Advertised name of sender
     * @param[in] sid         Session ID
     * @param[in] fromId      Beginning of changeId range (inclusive)
     * @param[in] toId        End of changeId range (exclusive)
     * @param[in] matchRules  Match rules to apply to changeId range
     */
    QStatus RequestRangeMatch(const char* name, SessionId sid, uint32_t fromId, uint32_t toId, std::vector<qcc::String>& matchRules);

    /**
     * Internal helper for sending sessionless signals.
     *
     * @param[in] msg The sessionless signal
     * @param[in] ep The destination endpoint
     * @param[in] sid Session ID
     */
    QStatus SendThroughEndpoint(Message& msg, BusEndpoint& ep, SessionId sid);

    /*
     * Internal helper for sending sessionless signals filtered by our rule table.
     *
     * @param[in] sid Session ID
     * @param[in] msg The sessionless signal
     * @param[in] fromRulesId Beginning of rules ID range (inclusive)
     * @param[in] toRulesId End of rules ID range (exclusive)
     */
    void SendMatchingThroughEndpoint(SessionId sid, Message msg, uint32_t fromRulesId, uint32_t toRulesId,
                                     bool onlySendIfImplicit = false);

    /**
     * A match rule that includes a change ID for recording when it was entered
     * into the rule table.
     */
    struct TimestampedRule : public Rule {
        TimestampedRule(Rule& rule, uint32_t id) : Rule(rule), id(id) { }
        uint32_t id;
    };

    /** Table of endpoint name, timestamped rule. */
    std::multimap<qcc::String, TimestampedRule> rules;

    /** Rule iterator */
    typedef std::multimap<qcc::String, TimestampedRule>::iterator RuleIterator;

    /**
     * An implicit match rule that includes a list of explicit rules that it is
     * associated with.
     */
    struct ImplicitRule : public Rule {
        ImplicitRule(const Rule& rule, const RuleIterator& explicitRule) : Rule(rule) { explicitRules.push_back(explicitRule); }
        std::vector<RuleIterator> explicitRules;
    };

    /** List of implicit rules. */
    std::vector<ImplicitRule> implicitRules;

    /** Implicit rule iterator */
    typedef std::vector<ImplicitRule>::iterator ImplicitRuleIterator;

    /**
     * Add an implicit, explicit rule entry.  Each implicit rule is associated
     * with at least one explicit rule.
     *
     * @param[in] implicitRule the implicit rule
     * @param[in] explicitRule the iterator of the explicit rule in rules
     */
    void AddImplicitRule(const Rule& implicitRule, const RuleIterator& explicitRule);

    /**
     * Remove explicit rules associated with an endpoint from the implicit
     * rules.  When all explicit associations are removed, the implicit rule is
     * removed.
     *
     * @param[in] epName the name of the endpoint
     */
    void RemoveImplicitRules(const qcc::String& epName);

    /**
     * Remove an explicit rule from all implicit rule entries.  When all
     * explicit associations for an implicit rule are removed, the implicit
     * rule is removed.
     *
     * @param[in] explicitRule the iterator of the explicit rule in rules
     */
    void RemoveImplicitRules(const RuleIterator& explicitRule);

    /**
     * Remove implicit rules that have the sender value of the remote cache.
     *
     * @param[in] cache the remote cache
     */
    void RemoveImplicitRules(const RemoteCache& cache);

    /**
     * Returns true if the message matches the implicit rule associated with
     * an endpoint and a message's sender, but does not match any explicit
     * rule associated with the endpoint.
     *
     * If true is returned, the implicit rule in question is disassociated
     * from the endpoint (i.e. all of its explicit rules that were associated
     * with the endpoint are removed).
     *
     * @param[in] epName the name of the endpoint
     * @param[in] msg the Message to compare with the implicit rules
     *
     * @return true if the Message matches only the implicit rule associated with
     *              the endpoint and the Message's sender.
     */
    bool IsOnlyImplicitMatch(const qcc::String& epName, Message& msg);

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
     * @param[in] cache
     * @param[in] fromRulesId The beginning of rules ID range (inclusive)
     * @param[in] toRulesId The end of rules ID range (exclusive)
     *
     * @return true if the sessionless match rules include a match for the
     * cache.
     */
    bool IsMatch(RemoteCache& cache, uint32_t fromRulesId, uint32_t toRulesId);

    typedef enum {
        NONE = 0,               /**< No work, we have received all signals and applied all the rules. */
        APPLY_NEW_RULES = 1,    /**< There are new rules that need to be applied the remote caches */
        REQUEST_NEW_SIGNALS = 2 /**< There are new signals in the cache that need to be received */
    } WorkType;

    /*
     * Return the pending work needing to be done for a remote cache.
     */
    WorkType PendingWork(RemoteCache& cache);

    /*
     * Schedule work to be done for any of the remote caches we know about.
     */
    void ScheduleWork(bool doInitialBackoff = true);

    /*
     * Schedule work to be done for a remote cache.
     */
    QStatus ScheduleWork(RemoteCache& cache, bool addAlarm = true, bool doInitialBackoff = true);

    bool QueryHandler(TransportMask transport, MDNSPacket query, uint16_t recvPort,
                      const qcc::IPEndpoint& ns4);
    bool SendResponseIfMatch(TransportMask transport, const qcc::IPEndpoint& ns4, const qcc::String& ruleStr);
    bool ResponseHandler(TransportMask transport, MDNSPacket response, uint16_t recvPort);

    void FoundAdvertisedNameHandler(const char* name, TransportMask transport, const char* prefix, bool doInitialBackoff = true);

    QStatus FindAdvertisementByTransport(const char* matching, TransportMask transports);
    QStatus CancelFindAdvertisementByTransport(const char* matching, TransportMask transports);

    void HandleRangeMatchRequest(const char* sender, SessionId sid, uint32_t fromChangeId, uint32_t toChangeId,
                                 std::vector<qcc::String>& matchRules);
};

}

#endif
