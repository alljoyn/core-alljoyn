/**
 * @file
 * SessionObj is for implementing the daemon-to-daemon inteface org.alljoyn.Sessionless
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
 * BusObject responsible for implementing the standard AllJoyn interface org.alljoyn.Sessionless.
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
     * Trigger (re)reception of sessionless signals from a single or from all
     * remote daemons.
     *
     * @param sender    Unique name of client that is requesting to re-receive sessionless messages.
     * @param guid      GUID of remote host that sender wants to re-receive messages from or empty for all remote hosts
     */
    QStatus RereceiveMessages(const qcc::String& sender, const qcc::String& guid);

  private:
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
     * Internal helper for FoundAdvertisedName.
     *
     * @param name        Advertised name that has been found.
     * @param transport   Transport that received the advertisment
     * @param catchUp     true if caller is intending to trigger rereceiption of sessionless signals.
     */
    QStatus HandleFoundAdvertisedName(const char* name, TransportMask transport, bool catchUp);

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
    uint32_t RuleCount() const;

    Bus& bus;                             /**< The bus */
    BusController* busController;         /**< BusController that created this BusObject */
    DaemonRouter& router;                 /**< The router */

    const InterfaceDescription* sessionlessIface;  /**< org.alljoyn.Sessionless interface */

    const InterfaceDescription::Member* requestSignalsSignal;   /**< org.alljoyn.Sessionless.RequestSignal signal */
    const InterfaceDescription::Member* requestRangeSignal;     /**< org.alljoyn.Sessionless.RequestRange signal */

    qcc::Timer timer;                     /**< Timer object for reaping expired names */

    /* Class used as key for messageMap */
    class MessageMapKey : public qcc::String {
      public:
        MessageMapKey(const char* sender, const char* iface, const char* member, const char* objPath) :
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

    /** Storage for sessionless messages waiting to be delivered */
    std::map<MessageMapKey, std::pair<uint32_t, Message> > messageMap;

    /** Count the number of rules (per endpoint) that specify sesionless=TRUE */
    std::map<qcc::String, uint32_t> ruleCountMap;

    /** CatchupState is used to track individual local clients that are behind the state of the server for a particular remote host */
    struct CatchupState {
        CatchupState() : changeId(0), sid(0) { }
        CatchupState(const qcc::String& sender, const qcc::String& guid, uint32_t changeId, SessionId sid) : sender(sender), guid(guid), changeId(changeId), sid(sid) { }
        qcc::String sender;
        qcc::String guid;
        uint32_t changeId;
        uint32_t sid;
    };
    /** Map session IDs to catchupStates */
    std::map<uint32_t, CatchupState> catchupMap;

    struct RoutedMessage {
        RoutedMessage(const Message& msg) : sender(msg->GetSender()), serial(msg->GetCallSerial()) { }
        qcc::String sender;
        uint32_t serial;
        bool operator==(const RoutedMessage& other) const { return (sender == other.sender) && (serial == other.serial); }
    };

    /** Track the changeIds of the advertisments coming from other daemons */
    struct ChangeIdEntry {
      public:
        ChangeIdEntry(const char* advName, TransportMask transport, uint32_t changeId, uint32_t advChangeId, uint64_t nextJoinTimestamp) :
            advName(advName), transport(transport), changeId(changeId), advChangeId(advChangeId), nextJoinTimestamp(nextJoinTimestamp), retries(0), sid(0), catchupList() { }
        qcc::String advName;
        TransportMask transport;
        uint32_t changeId;
        uint32_t advChangeId;
        uint64_t nextJoinTimestamp;
        uint32_t retries;
        qcc::String inProgress;
        SessionId sid;
        std::queue<CatchupState> catchupList;
        std::list<RoutedMessage> routedMessages;
    };
    /** Map remote guid to ChangeIdEntry */
    std::map<qcc::String, ChangeIdEntry> changeIdMap;

    /** Find the ChangeIdEntry with the matching session ID */
    std::map<qcc::String, ChangeIdEntry>::iterator FindChangeIdEntry(SessionId sid);

    qcc::Mutex lock;            /**< Mutex that protects messageMap this obj's data structures */
    uint32_t curChangeId;       /**< Change id assoc with current pushed signal(s) */
    uint32_t lastAdvChangeId;   /**< Last advertised change id */
    qcc::String lastAdvName;    /**< Last advertised name */
    qcc::String findPrefix;     /**< FindAdvertiseName prefix */
    qcc::String advPrefix;      /**< AdvertiseName prefix */
    bool isDiscoveryStarted;    /**< True when FindAdvetiseName is ongoing */
    SessionOpts sessionOpts;    /**< SessionOpts used by internal session */
    SessionPort sessionPort;    /**< SessionPort used by internal session */
    bool advanceChangeId;       /**< Set to true when changeId should be advanced on next SLS send request */

    /**
     * Retry join with random backoff of 200ms to ~8.5s.
     *
     * @param[in,out] entry the host to schedule the retry for
     *
     * @return ER_OK if retry scheduled, failure if retries exhausted
     */
    QStatus ScheduleRetry(ChangeIdEntry& entry);

    /**
     * Internal helper for parsing an advertised name into its guid and change
     * ID parts.
     *
     * @param[in] name
     * @param[out] guid
     * @param[out] changeId
     *
     * @return ER_OK if parsed succesfully
     */
    QStatus ParseAdvertisedName(const qcc::String& name, qcc::String* guid, uint32_t* changeId);

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
};

}

#endif
