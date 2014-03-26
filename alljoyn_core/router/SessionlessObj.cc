/**
 * @file
 * * This file implements the org.alljoyn.Bus and org.alljoyn.Daemon interfaces
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

#include <qcc/platform.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Session.h>

#include "SessionlessObj.h"
#include "BusController.h"

#define QCC_MODULE "SESSIONLESS"

using namespace std;
using namespace qcc;

/** Constants */
#define MAX_JOINSESSION_RETRIES 50

/**
 * Inside window calculation.
 * Returns true if p is in range [beg, beg+sz)
 * This function properly accounts for possible wrap-around in [beg, beg+sz) region.
 */
#define IN_WINDOW(tp, beg, sz, p) (((static_cast<tp>((beg) + (sz)) > (beg)) && ((p) >= (beg)) && ((p) < static_cast<tp>((beg) + (sz)))) || \
                                   ((static_cast<tp>((beg) + (sz)) < (beg)) && !(((p) < (beg)) && (p) >= static_cast<tp>((beg) + (sz)))))

/**
 * IS_GREATER_OR_EQUAL returns true if first (non-type) param is greater than or
 * equal to the second while taking into account the possibility of wrap-around
 */
#define IS_GREATER_OR_EQUAL(tp, a, b) IN_WINDOW(tp, (b), (numeric_limits<tp>::max() >> 1), (a))

/**
 * IS_GREATER returns true if first (non-type) param is greater than the second
 * while taking into account the possibility of wrap-around
 */
#define IS_GREATER(tp, a, b) (IS_GREATER_OR_EQUAL(tp, (a), (b)) && ((a) != (b)))

namespace ajn {

/** Constants */
#define SESSIONLESS_SESSION_PORT 100

/** Interface definitions for org.alljoyn.sessionless */
static const char* ObjectPath = "/org/alljoyn/sl";        /**< Object path */
static const char* InterfaceName = "org.alljoyn.sl";      /**< Interface name */
static const char* WellKnownName = "org.alljoyn.sl";      /**< Well known bus name */

/** The advertised name when the match rule does not specify an interface */
static const char* WildcardInterfaceName = "org.alljoyn";
static const char* WildcardAdvertisementPrefix = "org.alljoyn.sl.";

/*
 * Do not update the version without reviewing where it is used below.  It is
 * encoded in the 'x' part of the "org.alljoyn.sl.x" advertised name, thus care
 * must be taken to ensure that the advertised name is still a valid name.
 *
 * The encoding serves a few needs:
 * 1. Version 0 implementations will see the "org.alljoyn.sl." advertisements
 *    from version 1 implementations and behave as before,
 * 2. Version 1 implementations will see the "org.alljoyn.sl." advertisements
 *    from version 0 implementations and behave as before,
 * and most importantly,
 * 3. Version 1 implementations will see the "org.alljoyn.sl." advertisements
 *    from version 1 implementations and ignore them if not looking for the
 *    wildcard interface.
 */
const uint32_t SessionlessObj::version = 1;

/*
 * Internal context passed through JoinSessionAsync.  This holds a snapshot of
 * the remote queue state at the time we issue the JoinSessionAsync.
 */
struct RemoteQueueSnapshot {
  public:
    RemoteQueueSnapshot(SessionlessObj::RemoteQueue& queue) :
        version(queue.version), guid(queue.guid), iface(queue.iface), changeId(queue.changeId), transport(queue.transport) {
        name = iface.empty() ? String(WildcardInterfaceName) : iface;
        name.append(".sl.");
        name.append('x' + version);
        name.append(guid);
        name.append(".x");
        name.append(U32ToString(changeId, 16));
    }

    uint32_t version;
    qcc::String guid;
    qcc::String iface;
    uint32_t changeId;
    TransportMask transport;
    qcc::String name;
};

SessionlessObj::SessionlessObj(Bus& bus, BusController* busController) :
    BusObject(ObjectPath, false),
    bus(bus),
    busController(busController),
    router(reinterpret_cast<DaemonRouter&>(bus.GetInternal().GetRouter())),
    sessionlessIface(NULL),
    requestSignalsSignal(NULL),
    requestRangeSignal(NULL),
    timer("sessionless"),
    curChangeId(0),
    sessionOpts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY, SessionOpts::DAEMON_NAMES),
    sessionPort(SESSIONLESS_SESSION_PORT),
    advanceChangeId(false)
{
}

SessionlessObj::~SessionlessObj()
{
    /* Unbind session port */
    bus.UnbindSessionPort(sessionPort);

    /* Remove name listener */
    router.RemoveBusNameListener(this);

    /* Unregister bus object */
    bus.UnregisterBusObject(*this);
}

QStatus SessionlessObj::Init()
{
    QCC_DbgTrace(("SessionlessObj::Init"));

    QStatus status;

    /* Create the org.alljoyn.sl interface */
    InterfaceDescription* intf = NULL;
    status = bus.CreateInterface(InterfaceName, intf);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to create interface %s", InterfaceName));
        return status;
    }
    intf->AddSignal("RequestSignals", "u", NULL, 0);
    intf->AddSignal("RequestRange", "uu", NULL, 0);
    intf->Activate();

    /* Make this object implement org.alljoyn.sl */
    const InterfaceDescription* sessionlessIntf = bus.GetInterface(InterfaceName);
    if (!sessionlessIntf) {
        status = ER_BUS_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Failed to get %s interface", InterfaceName));
        return status;
    }

    /* Cache requestSignals and requestRange interface members */
    requestSignalsSignal = sessionlessIntf->GetMember("RequestSignals");
    assert(requestSignalsSignal);
    requestRangeSignal = sessionlessIntf->GetMember("RequestRange");
    assert(requestRangeSignal);

    /* Register a signal handler for requestSignals */
    status = bus.RegisterSignalHandler(this,
                                       static_cast<MessageReceiver::SignalHandler>(&SessionlessObj::RequestSignalsSignalHandler),
                                       requestSignalsSignal,
                                       NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to register RequestSignals signal handler"));
    }

    /* Register a signal handler for requestRange */
    status = bus.RegisterSignalHandler(this,
                                       static_cast<MessageReceiver::SignalHandler>(&SessionlessObj::RequestRangeSignalHandler),
                                       requestRangeSignal,
                                       NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to register RequestRange signal handler"));
    }

    /* Register signal handler for FoundAdvertisedName */
    /* (If we werent in the daemon, we could just use BusListener, but it doesnt work without the full BusAttachment implementation */
    const InterfaceDescription* ajIntf = bus.GetInterface(org::alljoyn::Bus::InterfaceName);
    assert(ajIntf);
    status = bus.RegisterSignalHandler(this,
                                       static_cast<MessageReceiver::SignalHandler>(&SessionlessObj::FoundAdvertisedNameSignalHandler),
                                       ajIntf->GetMember("FoundAdvertisedName"),
                                       NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to register FoundAdvertisedName signal handler"));
    }

    /* Register signal handler for SessionLostWithReason */
    /* (If we werent in the daemon, we could just use SessionListener, but it doesnt work without the full BusAttachment implementation */
    status = bus.RegisterSignalHandler(this,
                                       static_cast<MessageReceiver::SignalHandler>(&SessionlessObj::SessionLostSignalHandler),
                                       ajIntf->GetMember("SessionLostWithReason"),
                                       NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to register SessionLost signal handler"));
    }

    /* Register a name table listener */
    if (status == ER_OK) {
        router.AddBusNameListener(this);
    }

    /* Start the worker */
    if (status == ER_OK) {
        status = timer.Start();
    }

    /* Bind the session port and establish self as port listener */
    if (status == ER_OK) {
        status = bus.BindSessionPort(sessionPort, sessionOpts, *this);
    }

    /* Register sessionObj */
    if (ER_OK == status) {
        status = bus.RegisterBusObject(*this);
    }

    return status;
}

QStatus SessionlessObj::Stop()
{
    return timer.Stop();
}

QStatus SessionlessObj::Join()
{
    return timer.Join();
}

void SessionlessObj::ObjectRegistered(void)
{
    QCC_DbgTrace(("SessionlessObj::ObjectRegistered"));

    QStatus status;

    /* Acquire org.alljoyn.sl name */
    uint32_t disposition = DBUS_REQUEST_NAME_REPLY_EXISTS;
    status = router.AddAlias(WellKnownName,
                             bus.GetInternal().GetLocalEndpoint()->GetUniqueName(),
                             DBUS_NAME_FLAG_DO_NOT_QUEUE,
                             disposition,
                             NULL,
                             NULL);
    if ((ER_OK != status) || (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != disposition)) {
        status = (ER_OK == status) ? ER_FAIL : status;
        QCC_LogError(status, ("Failed to register well-known name \"%s\" (disposition=%d)", WellKnownName, disposition));
    }

    /* Must call base class */
    BusObject::ObjectRegistered();

    /* Notify parent */
    busController->ObjectRegistered(this);
}

void SessionlessObj::AddRule(const qcc::String& epName, Rule& rule)
{
    if (rule.sessionless == Rule::SESSIONLESS_TRUE) {
        QCC_DbgPrintf(("AddRule(epName=%s,rule=%s)", epName.c_str(), rule.ToString().c_str()));

        router.LockNameTable();
        lock.Lock();

        bool isNewRule = true;
        for (std::pair<RuleIterator, RuleIterator> range = rules.equal_range(epName); range.first != range.second; ++range.first) {
            if (range.first->second == rule) {
                isNewRule = false;
                break;
            }
        }
        rules.insert(std::pair<String, TimestampedRule>(epName, rule));

        /*
         * We need to re-receive previous signals for a new rule from any
         * senders we've previously received from.
         */
        if (isNewRule && (!remoteQueues.empty() || !localQueue.empty())) {
            lock.Unlock();
            router.UnlockNameTable();
            RereceiveMessages(epName, rule);
            router.LockNameTable();
            lock.Lock();
        }

        bus.EnableConcurrentCallbacks();
        FindAdvertisedNames();

        lock.Unlock();
        router.UnlockNameTable();
    }
}

void SessionlessObj::RemoveRule(const qcc::String& epName, Rule& rule)
{
    QCC_DbgTrace(("SessionlessObj::RemoveRule(%s, ...)", epName.c_str()));

    if (rule.sessionless == Rule::SESSIONLESS_TRUE) {
        router.LockNameTable();
        lock.Lock();

        std::pair<RuleIterator, RuleIterator> range = rules.equal_range(epName);
        while (range.first != range.second) {
            if (range.first->second == rule) {
                rules.erase(range.first);
                break;
            }
            ++range.first;
        }

        if (rules.empty()) {
            bus.EnableConcurrentCallbacks();
            CancelFindAdvertisedNames();
        }

        lock.Unlock();
        router.UnlockNameTable();
    }
}

QStatus SessionlessObj::PushMessage(Message& msg)
{
    QCC_DbgTrace(("SessionlessObj::PushMessage(%s)", msg->ToString().c_str()));

    /* Validate message */
    if (!msg->IsSessionless()) {
        return ER_FAIL;
    }

    /* Put the message in the local queue and kick the worker */
    SessionlessMessageKey key(msg->GetSender(), msg->GetInterface(), msg->GetMemberName(), msg->GetObjectPath());
    lock.Lock();
    advanceChangeId = true;
    SessionlessMessage val(curChangeId, msg);
    LocalQueue::iterator it = localQueue.find(key);
    if (it == localQueue.end()) {
        localQueue.insert(pair<SessionlessMessageKey, SessionlessMessage>(key, val));
    } else {
        it->second = val;
    }
    lock.Unlock();
    uint32_t zero = 0;
    SessionlessObj* slObj = this;
    QStatus status = timer.AddAlarm(Alarm(zero, slObj));

    return status;
}

bool SessionlessObj::RouteSessionlessMessage(SessionId sid, Message& msg)
{
    QCC_DbgPrintf(("RouteSessionlessMessage(sid=%u,msg={sender='%s',interface='%s',member='%s',path='%s'})",
                   sid, msg->GetSender(), msg->GetInterface(), msg->GetMemberName(), msg->GetObjectPath()));

    static const Rule legacyRule("type='error',sessionless='t'");

    router.LockNameTable();
    lock.Lock();

    /*
     * Check if we've already routed this message.  This may occur if we are
     * retrying and only received a subset of the sessionless signals during our
     * most recent attempt.
     */
    bool didRoute = false;
    RemoteQueues::iterator cit = FindRemoteQueue(sid);
    if (cit != remoteQueues.end()) {
        if (!cit->second.iface.empty() && (cit->second.iface != msg->GetInterface())) {
            didRoute = true;
        } else if (find(cit->second.routedMessages.begin(), cit->second.routedMessages.end(), RoutedMessage(msg)) !=
                   cit->second.routedMessages.end()) {
            didRoute = true;
        } else {
            cit->second.routedMessages.push_back(RoutedMessage(msg));
        }
    }
    if (didRoute) {
        lock.Unlock();
        router.UnlockNameTable();
        return true;
    }

    /*
     * Check to see if this session ID is for a catchup. If it is, we'll route
     * it below.
     */
    map<uint32_t, CatchupState>::iterator cuit = catchupMap.find(sid);
    if (cuit != catchupMap.end()) {
        bool isMatch = false;
        BusEndpoint ep = router.FindEndpoint(cuit->second.epName);
        if (ep->IsValid() && ep->AllowRemoteMessages() && cuit->second.rule.IsMatch(msg)) {
            isMatch = true;
        }
        lock.Unlock();
        router.UnlockNameTable();
        if (isMatch) {
            SendThroughEndpoint(msg, ep, sid);
        }
        return true;
    }

    /*
     * Not a catchup so multiple receivers may be interested in this message.
     */
    RuleIterator rit = rules.begin();
    while (rit != rules.end()) {
        bool isMatch = false;
        /*
         * Only apply the rule if it was added before we started the request,
         * otherwise there is a possibilty of duplicate messages received.  A
         * rule added while in progress will trigger a catchup request.
         */
        String epName = rit->first;
        BusEndpoint ep = router.FindEndpoint(epName);
        if (rit->second.timestamp < cit->second.inProgressTimestamp && ep->IsValid() && ep->AllowRemoteMessages()) {
            if (rit->second.IsMatch(msg)) {
                isMatch = true;
            } else if (rit->second == legacyRule) {
                /*
                 * Legacy clients will add the "type='error',sessionless='t'"
                 * rule.  In that case the expected behavior is that incoming
                 * sessionless signals will route through the daemon router's
                 * rule table.
                 */
                router.GetRuleTable().Lock();
                for (ajn::RuleIterator drit = router.GetRuleTable().FindRulesForEndpoint(ep);
                     !isMatch && (drit != router.GetRuleTable().End()) && (drit->first == ep);
                     ++drit) {
                    isMatch = drit->second.IsMatch(msg);
                }
                router.GetRuleTable().Unlock();
            }
        }
        if (isMatch) {
            lock.Unlock();
            router.UnlockNameTable();
            SendThroughEndpoint(msg, ep, sid);
            router.LockNameTable();
            lock.Lock();
            rit = rules.upper_bound(epName);
        } else {
            ++rit;
        }
    }
    lock.Unlock();
    router.UnlockNameTable();
    return true;
}

QStatus SessionlessObj::CancelMessage(const qcc::String& sender, uint32_t serialNum)
{
    QStatus status = ER_BUS_NO_SUCH_MESSAGE;
    bool messageErased = false;

    QCC_DbgTrace(("SessionlessObj::CancelMessage(%s, 0x%x)", sender.c_str(), serialNum));

    lock.Lock();
    SessionlessMessageKey key(sender.c_str(), "", "", "");
    LocalQueue::iterator it = localQueue.lower_bound(key);
    while ((it != localQueue.end()) && (sender == it->second.second->GetSender())) {
        if (it->second.second->GetCallSerial() == serialNum) {
            if (!it->second.second->IsExpired()) {
                status = ER_OK;
            }
            localQueue.erase(it);
            messageErased = true;
            break;
        }
        ++it;
    }
    lock.Unlock();

    /* Alert the advertiser worker */
    if (messageErased) {
        uint32_t zero = 0;
        SessionlessObj* slObj = this;
        status = timer.AddAlarm(Alarm(zero, slObj));
    }

    return status;
}

QStatus SessionlessObj::RereceiveMessages(const qcc::String& epName, const Rule& rule)
{
    QStatus status = ER_OK;
    QCC_DbgTrace(("RereceiveMessages(epName=%s,rule=%s)", epName.c_str(), rule.ToString().c_str()));
    uint64_t now = GetTimestamp64();
    const uint64_t timeoutValue = 18000;
    String selfGuid = bus.GetGlobalGUIDShortString();
    lock.Lock();

    RemoteQueues::iterator cit = remoteQueues.begin();
    while ((status == ER_OK) && (cit != remoteQueues.end())) {
        RemoteQueueKey lastKey = cit->first;

        /* Skip self */
        if (cit->second.guid == selfGuid) {
            ++cit;
            continue;
        }

        /* Skip remote queues that don't match */
        if ((cit->second.version != 0) && (rule.iface != cit->second.iface)) {
            ++cit;
            continue;
        }

        /* Wait for inProgress to be cleared */
        while ((cit != remoteQueues.end()) && cit->second.IsReceiveInProgress() && (GetTimestamp64() < (now + timeoutValue))) {
            lock.Unlock();
            qcc::Sleep(5);
            lock.Lock();
            cit = remoteQueues.lower_bound(lastKey);
            if (cit != remoteQueues.end()) {
                lastKey = cit->first;
            }
        }

        /* Process this guid */
        if (GetTimestamp64() >= (now + timeoutValue)) {
            status = ER_TIMEOUT;
        } else if (cit != remoteQueues.end()) {
            assert(!cit->second.IsReceiveInProgress());

            /* Add new catchup state */
            if (cit->second.lastReceivedChangeId != numeric_limits<uint32_t>::max()) {
                uint32_t beginState = cit->second.lastReceivedChangeId - (numeric_limits<uint32_t>::max() >> 1);
                cit->second.catchupList.push(CatchupState(epName, rule, beginState));
            }

            /* Get the sessions rolling */
            ScheduleTry(cit->second);

            cit = remoteQueues.lower_bound(lastKey);
        }

        /* Continue with other guids if guid is empty. (empty means all) */
        if ((cit != remoteQueues.end()) && (cit->first == lastKey)) {
            ++cit;
        }
    }

    uint32_t fromChangeId = curChangeId - (numeric_limits<uint32_t>::max() >> 1);
    uint32_t toChangeId = curChangeId + 1;
    lock.Unlock();

    /* Retrieve from our own cache */
    HandleRangeRequest(epName.c_str(), 0, fromChangeId, toChangeId);

    return status;
}

void SessionlessObj::NameOwnerChanged(const String& name,
                                      const String* oldOwner,
                                      const String* newOwner)
{
    QCC_DbgTrace(("SessionlessObj::NameOwnerChanged(%s, %s, %s)", name.c_str(), oldOwner ? oldOwner->c_str() : "(null)", newOwner ? newOwner->c_str() : "(null)"));

    /* Remove entries from rules for names exiting from the bus */
    if (oldOwner && !newOwner) {
        router.LockNameTable();
        lock.Lock();
        std::pair<RuleIterator, RuleIterator> range = rules.equal_range(name);
        if (range.first != rules.end()) {
            rules.erase(range.first, range.second);
        }

        /* Remove stored sessionless messages sent by the old owner */
        SessionlessMessageKey key(oldOwner->c_str(), "", "", "");
        LocalQueue::iterator mit = localQueue.lower_bound(key);
        while ((mit != localQueue.end()) && (::strcmp(oldOwner->c_str(), mit->second.second->GetSender()) == 0)) {
            localQueue.erase(mit++);
        }
        /* Alert the advertiser worker if the local queue is empty */
        if (localQueue.empty()) {
            uint32_t zero = 0;
            SessionlessObj* slObj = this;
            QStatus status = timer.AddAlarm(Alarm(zero, slObj));
            if (status != ER_OK) {
                /*
                 * When daemon is closing the daemon will receive multiple error
                 * because the timer is exiting.  print a high level debug message
                 * not a log error since this is expected behaver and should not
                 * be presented to the user if they don't want to see it.
                 */
                if (ER_TIMER_EXITING == status) {
                    QCC_DbgHLPrintf(("Timer::AddAlarm failed : %s", QCC_StatusText(status)));
                } else {
                    QCC_LogError(status, ("Timer::AddAlarm failed"));
                }
            }
        }

        /* Stop discovery if nobody is looking for sessionless signals */
        if (rules.empty()) {
            CancelFindAdvertisedNames();
        }
        lock.Unlock();
        router.UnlockNameTable();
    }
}

void SessionlessObj::FoundAdvertisedNameSignalHandler(const InterfaceDescription::Member* member,
                                                      const char* sourcePath,
                                                      Message& msg)
{
    /* Parse the args */
    const char* name;
    TransportMask transport;
    const char* prefix;
    QStatus status = msg->GetArgs("sqs", &name, &transport, &prefix);
    if (status != ER_OK) {
        QCC_LogError(status, ("GetArgs failed"));
        return;
    }

    /* Examine found name to see if we need to connect to it */
    String guid, iface;
    uint32_t version, changeId;
    status = ParseAdvertisedName(name, &version, &guid, &iface, &changeId);
    if (status != ER_OK) {
        QCC_LogError(status, ("Found invalid name \"%s\"", name));
        return;
    }
    QCC_DbgPrintf(("FoundAdvertisedName(name=%s,transport=0x%x,...) guid=%s,version=%u,iface=%s,changeId=%u",
                   name, transport, guid.c_str(), version, iface.c_str(), changeId));

    /* Add/replace sessionless adv name for remote daemon */
    busController->GetAllJoynObj().SetAdvNameAlias(guid, transport, name);

    /* Join session if we need signals from this advertiser and we aren't already getting them */
    bool doJoin = false;
    lock.Lock();
    RemoteQueueKey key(guid, iface);
    RemoteQueues::iterator it = remoteQueues.find(key);
    if (it == remoteQueues.end()) {
        it = remoteQueues.insert(pair<RemoteQueueKey, RemoteQueue>(key, RemoteQueue(version, guid, iface, changeId, transport))).first;
        doJoin = true;
    } else if (IS_GREATER(uint32_t, changeId, it->second.lastReceivedChangeId)) {
        it->second.changeId = changeId;
        it->second.transport = transport;
        it->second.retries = 0;
        doJoin = true;
    }
    if (doJoin) {
        ScheduleTry(it->second);
    }
    lock.Unlock();
}

bool SessionlessObj::AcceptSessionJoiner(SessionPort port,
                                         const char* joiner,
                                         const SessionOpts& opts)
{
    QCC_DbgTrace(("SessionlessObj::AcceptSessionJoiner(%d, %s, ...)", port, joiner));
    return true;
}

void SessionlessObj::SessionLostSignalHandler(const InterfaceDescription::Member* member,
                                              const char* sourcePath,
                                              Message& msg)
{
    uint32_t sid = 0;
    uint32_t reason = 0;
    msg->GetArgs("uu", &sid, &reason);
    QCC_DbgPrintf(("SessionLost(sid=%u,reason=%d)", sid, reason));
    DoSessionLost(sid, static_cast<SessionLostReason>(reason));
}

void SessionlessObj::DoSessionLost(SessionId sid, SessionLostReason reason)
{
    QCC_DbgTrace(("SessionlessObj::DoSessionLost(%u)", sid));

    lock.Lock();

    bool isCatchup = false;
    CatchupState catchup;
    map<uint32_t, CatchupState>::iterator it = catchupMap.find(sid);
    if (it != catchupMap.end()) {
        isCatchup = true;
        catchup = it->second;
        catchupMap.erase(it);
    }

    RemoteQueues::iterator cit = FindRemoteQueue(sid);
    if (cit != remoteQueues.end()) {
        /* Reset inProgress */
        uint32_t inProgressChangeId = cit->second.inProgressChangeId;
        cit->second.ReceiveCompleted();

        if (reason == ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION) {
            /* We got all the signals */
            if (!isCatchup) {
                cit->second.lastReceivedChangeId = inProgressChangeId;
            }
            cit->second.routedMessages.clear();

            /* Get the sessions rolling if necessary */
            if (cit->second.lastReceivedChangeId != cit->second.changeId) {
                ScheduleTry(cit->second);
            }
        } else {
            /* An error occurred while getting the signals, so retry */
            if (isCatchup) {
                cit->second.catchupList.push(catchup);
            }
            if (ScheduleRetry(cit->second) != ER_OK) {
                /* Retries exhausted. Clear state and wait for new advertisment */
                remoteQueues.erase(cit);
            }
        }
    }

    lock.Unlock();
}

void SessionlessObj::RequestSignalsSignalHandler(const InterfaceDescription::Member* member,
                                                 const char* sourcePath,
                                                 Message& msg)
{
    QCC_DbgTrace(("SessionlessObj::RequestSignalsHandler(%s, %s, ...)", member->name.c_str(), sourcePath));
    uint32_t fromId;
    QStatus status = msg->GetArgs("u", &fromId);
    if (status == ER_OK) {
        /* Send all signals in the range [fromId, curChangeId] */
        HandleRangeRequest(msg->GetSender(), msg->GetSessionId(), fromId, curChangeId + 1);
    } else {
        QCC_LogError(status, ("Message::GetArgs failed"));
    }
}

void SessionlessObj::RequestRangeSignalHandler(const InterfaceDescription::Member* member,
                                               const char* sourcePath,
                                               Message& msg)
{
    QCC_DbgTrace(("SessionlessObj::RequestRangeHandler(%s, %s, ...)", member->name.c_str(), sourcePath));
    uint32_t fromId, toId;
    QStatus status = msg->GetArgs("uu", &fromId, &toId);
    if (status == ER_OK) {
        HandleRangeRequest(msg->GetSender(), msg->GetSessionId(), fromId, toId);
    } else {
        QCC_LogError(status, ("Message::GetArgs failed"));
    }
}

void SessionlessObj::HandleRangeRequest(const char* sender, SessionId sid, uint32_t fromChangeId, uint32_t toChangeId)
{
    QStatus status = ER_OK;
    bool messageErased = false;
    QCC_DbgTrace(("SessionlessObj::HandleControlSignal(%d, %d)", fromChangeId, toChangeId));

    /* Enable concurrency since PushMessage could block */
    bus.EnableConcurrentCallbacks();

    /* Advance the curChangeId */
    lock.Lock();
    if (advanceChangeId) {
        ++curChangeId;
        advanceChangeId = false;
    }

    /* Send all messages in local queue in range [fromChangeId, toChangeId) */
    LocalQueue::iterator it = localQueue.begin();
    uint32_t rangeLen = toChangeId - fromChangeId;
    while (it != localQueue.end()) {
        if (IN_WINDOW(uint32_t, fromChangeId, rangeLen, it->second.first)) {
            SessionlessMessageKey key = it->first;
            if (it->second.second->IsExpired()) {
                /* Remove expired message without sending */
                localQueue.erase(it++);
                messageErased = true;
            } else {
                /* Send message */
                lock.Unlock();
                router.LockNameTable();
                BusEndpoint ep = router.FindEndpoint(sender);
                if (ep->IsValid()) {
                    router.UnlockNameTable();
                    SendThroughEndpoint(it->second.second, ep, sid);
                } else {
                    router.UnlockNameTable();
                }
                lock.Lock();
                it = localQueue.upper_bound(key);
            }
        } else {
            ++it;
        }
    }
    lock.Unlock();

    /* Alert the advertiser worker */
    if (messageErased) {
        uint32_t zero = 0;
        SessionlessObj* slObj = this;
        status = timer.AddAlarm(Alarm(zero, slObj));
    }

    /* Close the session */
    if (sid != 0) {
        status = bus.LeaveSession(sid);
        if (status != ER_OK) {
            QCC_LogError(status, ("LeaveSession failed"));
        }
    }
}


void SessionlessObj::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QCC_DbgTrace(("SessionlessObj::AlarmTriggered(alarm, %s)", QCC_StatusText(reason)));

    QStatus status = ER_OK;

    if (reason == ER_OK) {
        uint32_t tilExpire = ::numeric_limits<uint32_t>::max();
        uint32_t expire;

        /* Purge the local queue of expired messages */
        lock.Lock();
        LocalQueue::iterator it = localQueue.begin();
        while (it != localQueue.end()) {
            if (it->second.second->IsExpired(&expire)) {
                localQueue.erase(it++);
            } else {
                ++it;
            }
        }
        lock.Unlock();

        /* Change advertisment if needed */
        UpdateAdvertisements();

        /* Look for new/failed joinsessions to try/retry (after backoff) */
        router.LockNameTable();
        lock.Lock();
        for (RemoteQueues::iterator cit = remoteQueues.begin(); cit != remoteQueues.end(); ++cit) {
            RemoteQueue& queue = cit->second;
            uint64_t now = qcc::GetTimestamp64();
            bool doJoin =
                (queue.nextJoinTimestamp <= now) &&
                !queue.IsReceiveInProgress() &&
                ((queue.lastReceivedChangeId != queue.changeId) || !queue.catchupList.empty()) &&
                ((queue.version == 0) || IsMatch(queue));
            if (doJoin) {
                if (queue.retries++ < MAX_JOINSESSION_RETRIES) {
                    RemoteQueueSnapshot* ctx = new RemoteQueueSnapshot(queue);
                    queue.ReceiveStarted();
                    SessionOpts opts = sessionOpts;
                    opts.transports = queue.transport;
                    status = bus.JoinSessionAsync(ctx->name.c_str(), sessionPort, NULL, opts, this, reinterpret_cast<void*>(ctx));
                    if (status == ER_OK) {
                        QCC_DbgPrintf(("JoinSessionAsync(name=%s,...) pending", ctx->name.c_str()));
                    } else {
                        QCC_LogError(status, ("JoinSessionAsync to %s failed", ctx->name.c_str()));
                        queue.ReceiveCompleted();
                        delete ctx;
                        /* Retry the join session with random backoff */
                        uint32_t delay = qcc::Rand8();
                        queue.nextJoinTimestamp = qcc::GetTimestamp64() + delay;
                        tilExpire = min(tilExpire, delay + 1);
                    }
                } else {
                    QCC_LogError(ER_FAIL, ("Exhausted JoinSession retries to %s", queue.guid.c_str()));
                }
            }
        }

        lock.Unlock();
        router.UnlockNameTable();

        /* Rearm alarm */
        if (tilExpire != ::numeric_limits<uint32_t>::max()) {
            SessionlessObj* slObj = this;
            timer.AddAlarm(Alarm(tilExpire, slObj));
        }
    }
}

void SessionlessObj::JoinSessionCB(QStatus status, SessionId sid, const SessionOpts& opts, void* context)
{
    RemoteQueueSnapshot* ctx = reinterpret_cast<RemoteQueueSnapshot*>(context);

    QCC_DbgPrintf(("JoinSessionCB(status=%s,sid=%u) name=%s", QCC_StatusText(status), sid, ctx->name.c_str()));

    /* Send out RequestSignals or RequestRange message if join was successful. Otherwise retry. */
    router.LockNameTable();
    lock.Lock();
    RemoteQueues::iterator cit = remoteQueues.find(RemoteQueueKey(ctx->guid, ctx->iface));
    if (cit != remoteQueues.end()) {
        bool rangeCapable = false;
        bool isCatchup = false;
        CatchupState catchup;

        /* Check to see if there are any pending catch ups */
        uint32_t requestChangeId = cit->second.lastReceivedChangeId + 1;
        if (status == ER_OK) {
            /* Update session ID */
            cit->second.sid = sid;

            /* Check to see if session host is capable of handling RequestRange */
            BusEndpoint ep = router.FindEndpoint(ctx->name);
            if (ep->IsValid() && (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL)) {
                RemoteEndpoint rep = VirtualEndpoint::cast(ep)->GetBusToBusEndpoint(sid);
                if (rep->IsValid()) {
                    rangeCapable = (rep->GetRemoteProtocolVersion() >= 6);
                }
            }

            /*
             * Check first if routedMessages is empty.  If not it means we're
             * retrying a request in progress and want to continue retrying
             * before beginning a new catchup request.
             */
            if (cit->second.routedMessages.empty() && !cit->second.catchupList.empty()) {
                if (rangeCapable) {
                    /* Handle head of catchup list */
                    isCatchup = true;
                    catchup = cit->second.catchupList.front();
                    cit->second.catchupList.pop();
                    /* Put catchup on catchupMap */
                    catchupMap[sid] = catchup;
                } else {
                    /* This session cant be used for catchup because remote side doesn't support it */
                    /* Just clear the catchupList and move on as if it was the non-catchup case */
                    while (!cit->second.catchupList.empty()) {
                        cit->second.catchupList.pop();
                    }
                    bus.LeaveSession(sid);
                    DoSessionLost(sid, ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION);
                    status = ER_NONE;
                }
            }
        } else {
            /* Clear inProgress */
            cit->second.ReceiveCompleted();

            if (ScheduleRetry(cit->second) != ER_OK) {
                /* Retries exhausted. Clear state and wait for new advertisment */
                remoteQueues.erase(cit);
            }
        }
        lock.Unlock();
        router.UnlockNameTable();

        if (status == ER_OK) {
            /*
             * Send the request signal if join was successful.  Prefer
             * RequestRange since it may be possible to receive duplicates when
             * RequestSignals is used together with RequestRange.
             */
            if (isCatchup) {
                status = RequestRange(ctx->name.c_str(), sid, catchup.changeId, requestChangeId);
            } else if (rangeCapable) {
                status = RequestRange(ctx->name.c_str(), sid, requestChangeId, ctx->changeId + 1);
            } else {
                status = RequestSignals(ctx->name.c_str(), sid, requestChangeId);
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to send Request to %s", ctx->name.c_str()));
                bus.LeaveSession(sid);
                lock.Lock();
                if (isCatchup) {
                    catchupMap.erase(sid);
                    cit->second.catchupList.push(catchup);
                }

                /* Clear inProgress */
                cit->second.ReceiveCompleted();

                if (ScheduleRetry(cit->second) != ER_OK) {
                    /* Retries exhausted. Clear state and wait for new advertisment */
                    remoteQueues.erase(cit);
                }
                lock.Unlock();
            }
        }
    } else {
        lock.Unlock();
        router.UnlockNameTable();
        QCC_LogError(ER_FAIL, ("Missing queue in remoteQueues for %s", ctx->guid.c_str()));
    }

    delete ctx;
}

void SessionlessObj::ScheduleTry(RemoteQueue& queue)
{
    if (!queue.IsReceiveInProgress()) {
        ScheduleJoin(queue, qcc::Rand8());
    }
}

QStatus SessionlessObj::ScheduleRetry(RemoteQueue& queue)
{
    if (queue.retries < MAX_JOINSESSION_RETRIES) {
        ScheduleJoin(queue, 200 + (qcc::Rand16() >> 3));
        return ER_OK;
    } else {
        QCC_LogError(ER_FAIL, ("Exhausted JoinSession retries to %s", queue.guid.c_str()));
        return ER_FAIL;
    }
}

void SessionlessObj::ScheduleJoin(RemoteQueue& queue, uint32_t delayMs)
{
    queue.nextJoinTimestamp = GetTimestamp64() + delayMs;
    ++delayMs;
    SessionlessObj* slObj = this;
    QStatus status = timer.AddAlarm(Alarm(delayMs, slObj));
    if (status != ER_OK) {
        QCC_LogError(status, ("Timer::AddAlarm failed"));
    }
}

QStatus SessionlessObj::ParseAdvertisedName(const qcc::String& name, uint32_t* version, qcc::String* guid, qcc::String* iface, uint32_t* changeId)
{
    size_t guidPos = String::npos;
    size_t changePos = name.find_last_of('.');
    if (changePos != String::npos) {
        if (changeId) {
            *changeId = StringToU32(name.substr(changePos + 2), 16);
        }
        guidPos = name.find_last_of('.', changePos);
    }
    if (guidPos == String::npos) {
        return ER_FAIL;
    }
    if (guid) {
        *guid = name.substr(guidPos + 2, changePos - guidPos - 2);
    }
    if (version) {
        *version = name[guidPos + 1] - 'x';
    }
    if (iface && (guidPos > 3)) {
        *iface = name.substr(0, guidPos - 3);
        if (*iface == WildcardInterfaceName) {
            iface->clear();
        }
    }
    return ER_OK;
}

QStatus SessionlessObj::RequestSignals(const char* name, SessionId sid, uint32_t fromId)
{
    MsgArg args[1];
    args[0].Set("u", fromId);
    QCC_DbgPrintf(("RequestSignals(name=%s,sid=%u,fromId=%d)", name, sid, fromId));
    return Signal(name, sid, *requestSignalsSignal, args, ArraySize(args));
}

QStatus SessionlessObj::RequestRange(const char* name, SessionId sid, uint32_t fromId, uint32_t toId)
{
    MsgArg args[2];
    args[0].Set("u", fromId);
    args[1].Set("u", toId);
    QCC_DbgPrintf(("RequestRange(name=%s,sid=%u,fromId=%d,toId=%d)", name, sid, fromId, toId));
    return Signal(name, sid, *requestRangeSignal, args, ArraySize(args));
}

SessionlessObj::RemoteQueues::iterator SessionlessObj::FindRemoteQueue(SessionId sid)
{
    RemoteQueues::iterator cit;
    for (cit = remoteQueues.begin(); cit != remoteQueues.end(); ++cit) {
        if (cit->second.sid == sid) {
            break;
        }
    }
    return cit;
}

QStatus SessionlessObj::SendThroughEndpoint(Message& msg, BusEndpoint& ep, SessionId sid)
{
    QStatus status;
    if (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
        status = VirtualEndpoint::cast(ep)->PushMessage(msg, sid);
    } else {
        status = ep->PushMessage(msg);
    }
    if ((status != ER_OK) && (status != ER_BUS_ENDPOINT_CLOSING) && (status != ER_BUS_STOPPING)) {
        QCC_LogError(status, ("SendThroughEndpoint(dest=%s,ep=%s,sid=%u) failed",
                              msg->GetDestination(), ep->GetUniqueName().c_str(), sid));
    }
    return status;
}

void SessionlessObj::UpdateAdvertisements()
{
    QStatus status;

    /* Figure out what we need to advertise. */
    map<String, uint32_t> advertisements;
    lock.Lock();
    for (LocalQueue::iterator it = localQueue.begin(); it != localQueue.end(); ++it) {
        Message msg = it->second.second;
        advertisements[msg->GetInterface()] = max(advertisements[msg->GetInterface()], it->second.first);
        advertisements[WildcardInterfaceName] = max(advertisements[WildcardInterfaceName], it->second.first); /* The v0 advertisement */
    }
    lock.Unlock();

    /* First pass: cancel any names that don't need to be advertised anymore. */
    for (map<String, uint32_t>::iterator last = lastAdvertisements.begin(); last != lastAdvertisements.end();) {
        String name = last->first;
        if (advertisements.find(name) == advertisements.end()) {
            CancelAdvertisedName(AdvertisedName(name, last->second));
            lastAdvertisements.erase(last);
            last = lastAdvertisements.upper_bound(name);
        } else {
            ++last;
        }
    }

    /* Second pass: update/add any new advertisements. */
    for (map<String, uint32_t>::iterator it = advertisements.begin(); it != advertisements.end(); ++it) {
        map<String, uint32_t>::iterator last = lastAdvertisements.find(it->first);
        if (last == lastAdvertisements.end() || IS_GREATER(uint32_t, it->second, last->second)) {
            if (last != lastAdvertisements.end()) {
                CancelAdvertisedName(AdvertisedName(last->first, last->second));
                lastAdvertisements.erase(last);
            }
            status = AdvertiseName(AdvertisedName(it->first, it->second));
            if (status == ER_OK) {
                lastAdvertisements[it->first] = it->second;
            }
        }
    }
}

qcc::String SessionlessObj::AdvertisedName(const qcc::String& iface, uint32_t changeId)
{
    String name = iface;
    name.append(".sl.");
    name.append('x' + version);
    name.append(bus.GetGlobalGUIDShortString());
    name.append(".x");
    name.append(U32ToString(changeId, 16));
    return name;
}

QStatus SessionlessObj::AdvertiseName(const qcc::String& name)
{
    QStatus status = bus.RequestName(name.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (status == ER_OK) {
        status = bus.AdvertiseName(name.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
    }
    if (status == ER_OK) {
        QCC_DbgPrintf(("AdvertiseName(name=%s)", name.c_str()));
    } else {
        QCC_LogError(status, ("Failed to request/advertise \"%s\"", name.c_str()));
    }
    return status;
}

void SessionlessObj::CancelAdvertisedName(const qcc::String& name)
{
    QStatus status = bus.CancelAdvertiseName(name.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to cancel advertisment for \"%s\"", name.c_str()));
    }
    status = bus.ReleaseName(name.c_str());
    if (status == ER_OK) {
        QCC_DbgPrintf(("CancelAdvertiseName(name=%s)", name.c_str()));
    } else {
        QCC_LogError(status, ("Failed to release name \"%s\"", name.c_str()));
    }
}

void SessionlessObj::FindAdvertisedNames()
{
    for (RuleIterator rit = rules.begin(); rit != rules.end(); ++rit) {
        String name = (rit->second.iface.empty() ? WildcardInterfaceName : rit->second.iface) + ".sl.";
        pair<set<String>::iterator, bool> finding = findingNames.insert(name);
        if (finding.second) {
            QCC_DbgPrintf(("FindAdvertisedName(name=%s)", name.c_str()));
            QStatus status = bus.FindAdvertisedNameByTransport(name.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
            if (status != ER_OK) {
                QCC_LogError(status, ("FindAdvertisedNameByTransport failed"));
            }
        }
    }
    if (!findingNames.empty()) {
        /*
         * If we're finding anything, we always need to find the v0
         * advertisement for backwards compatibility.
         */
        pair<set<String>::iterator, bool> finding = findingNames.insert(WildcardAdvertisementPrefix);
        if (finding.second) {
            QCC_DbgPrintf(("FindAdvertisedName(name=%s)", WildcardAdvertisementPrefix));
            QStatus status = bus.FindAdvertisedNameByTransport(WildcardAdvertisementPrefix, TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
            if (status != ER_OK) {
                QCC_LogError(status, ("FindAdvertisedNameByTransport failed"));
            }
        }
    }
}

void SessionlessObj::CancelFindAdvertisedNames()
{
    set<String>::iterator it = findingNames.begin();
    while (it != findingNames.end()) {
        String name = *it;
        QStatus status = bus.CancelFindAdvertisedNameByTransport(name.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
        if (status == ER_OK) {
            QCC_DbgPrintf(("CancelFindAdvertisedName(%s)", name.c_str()));
        } else {
            QCC_LogError(status, ("CancelFindAdvertisedNameByTransport failed"));
        }
        findingNames.erase(it++);
    }
}

bool SessionlessObj::IsMatch(RemoteQueue& queue)
{
    bool matching = false;
    lock.Lock();
    for (RuleIterator rit = rules.begin(); rit != rules.end(); ++rit) {
        if (rit->second.iface == queue.iface) {
            matching = true;
            break;
        }
    }
    lock.Unlock();
    return matching;
}

}
