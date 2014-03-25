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

/** Internal context passed through JoinSessionAsync */
struct SessionlessJoinContext {
  public:
    SessionlessJoinContext(const qcc::String& name) : name(name) { }

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
    messageMap(),
    changeIdMap(),
    lock(),
    curChangeId(0),
    lastAdvChangeId(-1),
    isDiscoveryStarted(false),
    sessionOpts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY, SessionOpts::DAEMON_NAMES),
    sessionPort(SESSIONLESS_SESSION_PORT),
    advanceChangeId(false)
{
    /* Initialize findPrefix */
    findPrefix = WellKnownName;
    findPrefix.append('.');

    /* Initialize advPrefix */
    advPrefix = findPrefix;
    advPrefix.append('x');
    advPrefix.append(bus.GetGlobalGUIDShortString());
    advPrefix.append(".x");
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
        if (isNewRule && (!changeIdMap.empty() || !messageMap.empty())) {
            lock.Unlock();
            router.UnlockNameTable();
            RereceiveMessages(epName, rule);
            router.LockNameTable();
            lock.Lock();
        }

        if (!isDiscoveryStarted) {
            bus.EnableConcurrentCallbacks();
            QStatus status = bus.FindAdvertisedNameByTransport(findPrefix.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
            if (status != ER_OK) {
                QCC_LogError(status, ("FindAdvertisedNameByTransport failed"));
            } else {
                isDiscoveryStarted = true;
            }
        }
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

        if (isDiscoveryStarted && rules.empty()) {
            bus.EnableConcurrentCallbacks();
            QStatus status = bus.CancelFindAdvertisedNameByTransport(findPrefix.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
            if (status != ER_OK) {
                QCC_LogError(status, ("CancelFindAdvertisedNameByTransport failed"));
            }
            isDiscoveryStarted = false;
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

    /* Put the message in the map and kick the worker */
    MessageMapKey key(msg->GetSender(), msg->GetInterface(), msg->GetMemberName(), msg->GetObjectPath());
    lock.Lock();
    advanceChangeId = true;
    pair<uint32_t, Message> val(curChangeId, msg);
    map<MessageMapKey, pair<uint32_t, Message> >::iterator it = messageMap.find(key);
    if (it == messageMap.end()) {
        messageMap.insert(pair<MessageMapKey, pair<uint32_t, Message> >(key, val));
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
    map<String, ChangeIdEntry>::iterator cit = FindChangeIdEntry(sid);
    if (cit != changeIdMap.end()) {
        if (find(cit->second.routedMessages.begin(), cit->second.routedMessages.end(), RoutedMessage(msg)) !=
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
    MessageMapKey key(sender.c_str(), "", "", "");
    map<MessageMapKey, pair<uint32_t, Message> >::iterator it = messageMap.lower_bound(key);
    while ((it != messageMap.end()) && (sender == it->second.second->GetSender())) {
        if (it->second.second->GetCallSerial() == serialNum) {
            if (!it->second.second->IsExpired()) {
                status = ER_OK;
            }
            messageMap.erase(it);
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
    QCC_DbgTrace(("SessionlessObj::RereceiveMessages(%s)", epName.c_str()));
    uint64_t now = GetTimestamp64();
    const uint64_t timeoutValue = 18000;
    String selfGuid = bus.GetGlobalGUIDShortString();
    lock.Lock();

    map<String, ChangeIdEntry>::iterator it = changeIdMap.begin();
    while ((status == ER_OK) && (it != changeIdMap.end())) {
        String lastGuid = it->first;

        /* Skip self */
        if (lastGuid == selfGuid) {
            ++it;
            continue;
        }

        /* Wait for inProgress to be cleared */
        while ((it != changeIdMap.end()) && it->second.InProgress() && (GetTimestamp64() < (now + timeoutValue))) {
            lock.Unlock();
            qcc::Sleep(5);
            lock.Lock();
            it = changeIdMap.lower_bound(lastGuid);
            if (it != changeIdMap.end()) {
                lastGuid = it->first;
            }
        }

        /* Process this guid */
        if (GetTimestamp64() >= (now + timeoutValue)) {
            status = ER_TIMEOUT;
        } else if (it != changeIdMap.end()) {
            assert(!it->second.InProgress());

            /* Add new catchup state */
            uint32_t beginState = it->second.changeId - (numeric_limits<uint32_t>::max() >> 1);
            it->second.catchupList.push(CatchupState(epName, rule, beginState));

            /* Get the sessions rolling */
            ScheduleTry(it->second);

            it = changeIdMap.lower_bound(lastGuid);
        }

        /* Continue with other guids if guid is empty. (empty means all) */
        if ((it != changeIdMap.end()) && (it->first == lastGuid)) {
            ++it;
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

        /* Remove stored sessionless messages sent by toldOwner */
        MessageMapKey key(oldOwner->c_str(), "", "", "");
        map<MessageMapKey, pair<uint32_t, Message> >::iterator mit = messageMap.lower_bound(key);
        while ((mit != messageMap.end()) && (::strcmp(oldOwner->c_str(), mit->second.second->GetSender()) == 0)) {
            messageMap.erase(mit++);
        }
        /* Alert the advertiser worker if messageMap is empty */
        if (messageMap.empty()) {
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
        if (isDiscoveryStarted && rules.empty()) {
            QStatus status = bus.CancelFindAdvertisedNameByTransport(findPrefix.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
            if (status != ER_OK) {
                QCC_LogError(status, ("CancelFindAdvertisedNameByTransport failed"));
            }
            isDiscoveryStarted = false;
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
    QCC_DbgPrintf(("FoundAdvertisedName(name=%s,transport=0x%x,...)", name, transport));

    /* Examine found name to see if we need to connect to it */
    String guid;
    uint32_t changeId;
    status = ParseAdvertisedName(name, &guid, &changeId);
    if (status != ER_OK) {
        QCC_LogError(status, ("Found invalid name \"%s\"", name));
        return;
    }

    /* Add/replace sessionless adv name for remote daemon */
    busController->GetAllJoynObj().SetAdvNameAlias(guid, transport, name);

    /* Join session if we need signals from this advertiser and we aren't already getting them */
    bool doJoin = false;
    lock.Lock();
    map<String, ChangeIdEntry>::iterator it = changeIdMap.find(guid);
    if (it == changeIdMap.end()) {
        it = changeIdMap.insert(pair<String, ChangeIdEntry>(guid, ChangeIdEntry(name, transport, changeId))).first;
        doJoin = true;
    } else if (IS_GREATER(uint32_t, changeId, it->second.changeId)) {
        it->second.advName = name;
        it->second.advChangeId = changeId;
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

    map<String, ChangeIdEntry>::iterator cit = FindChangeIdEntry(sid);
    if (cit != changeIdMap.end()) {
        /* Reset inProgress */
        String inProgress = cit->second.inProgress;
        cit->second.Completed();

        if (reason == ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION) {
            /* We got all the signals */
            if (!isCatchup) {
                ParseAdvertisedName(inProgress, NULL, &cit->second.changeId);
            }
            cit->second.routedMessages.clear();

            /* Get the sessions rolling if necessary */
            if (cit->second.changeId != cit->second.advChangeId) {
                ScheduleTry(cit->second);
            }
        } else {
            /* An error occurred while getting the signals, so retry */
            if (isCatchup) {
                cit->second.catchupList.push(catchup);
            }
            if (ScheduleRetry(cit->second) != ER_OK) {
                /* Retries exhausted. Clear state and wait for new advertisment */
                changeIdMap.erase(cit);
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

    /* Send all messages in messageMap in range [fromChangeId, toChangeId) */
    map<MessageMapKey, pair<uint32_t, Message> >::iterator it = messageMap.begin();
    uint32_t rangeLen = toChangeId - fromChangeId;
    while (it != messageMap.end()) {
        if (IN_WINDOW(uint32_t, fromChangeId, rangeLen, it->second.first)) {
            MessageMapKey key = it->first;
            if (it->second.second->IsExpired()) {
                /* Remove expired message without sending */
                messageMap.erase(it++);
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
                it = messageMap.upper_bound(key);
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
        uint32_t maxChangeId = 0;
        bool mapIsEmpty = true;

        /* Purge the messageMap of expired messages */
        lock.Lock();
        map<MessageMapKey, pair<uint32_t, Message> >::iterator it = messageMap.begin();
        while (it != messageMap.end()) {
            if (it->second.second->IsExpired(&expire)) {
                messageMap.erase(it++);
            } else {
                maxChangeId = max(maxChangeId, it->second.first);
                mapIsEmpty = false;
                ++it;
            }
        }
        lock.Unlock();

        /* Change advertisment if map is empty or if maxChangeId > lastAdvChangeId */
        if (mapIsEmpty || IS_GREATER(uint32_t, maxChangeId, lastAdvChangeId)) {

            /* Cancel previous advertisment */
            if (!lastAdvName.empty()) {
                status = bus.CancelAdvertiseName(lastAdvName.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed to cancel advertisment for \"%s\"", lastAdvName.c_str()));
                }

                /* Cancel previous name */
                status = bus.ReleaseName(lastAdvName.c_str());
                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed to release name \"%s\"", lastAdvName.c_str()));
                }
            }

            /* Acqure new name and advertise */
            if (!mapIsEmpty) {
                lastAdvName = advPrefix + U32ToString(maxChangeId, 16);

                status = bus.RequestName(lastAdvName.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE);
                if (status == ER_OK) {
                    status = bus.AdvertiseName(lastAdvName.c_str(), TRANSPORT_ANY & ~TRANSPORT_ICE & ~TRANSPORT_LOCAL);
                }

                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed to request/advertise \"%s\"", lastAdvName.c_str()));
                    lastAdvName.clear();
                    lastAdvChangeId = -1;
                } else {
                    lastAdvChangeId = maxChangeId;
                }
            } else {
                /* Map is empty. No advertisment. */
                lastAdvName.clear();
                lastAdvChangeId = -1;
            }
        }

        /* Look for new/failed joinsessions to try/retry (after backoff) */
        router.LockNameTable();
        lock.Lock();
        for (map<String, ChangeIdEntry>::iterator cit = changeIdMap.begin(); cit != changeIdMap.end(); ++cit) {
            ChangeIdEntry& entry = cit->second;
            if ((entry.nextJoinTimestamp <= qcc::GetTimestamp64()) &&
                !entry.InProgress() &&
                ((entry.changeId != entry.advChangeId) || !entry.catchupList.empty())) {
                if (entry.retries++ < MAX_JOINSESSION_RETRIES) {
                    SessionlessJoinContext* ctx = new SessionlessJoinContext(entry.advName);
                    entry.Started();
                    SessionOpts opts = sessionOpts;
                    opts.transports = entry.transport;
                    status = bus.JoinSessionAsync(entry.advName.c_str(), sessionPort, NULL, opts, this, reinterpret_cast<void*>(ctx));
                    if (status == ER_OK) {
                        QCC_DbgPrintf(("JoinSessionAsync(name=%s,...) pending", entry.advName.c_str()));
                    } else {
                        QCC_LogError(status, ("JoinSessionAsync to %s failed", entry.advName.c_str()));
                        entry.Completed();
                        delete ctx;
                        /* Retry the join session with random backoff */
                        uint32_t delay = qcc::Rand8();
                        entry.nextJoinTimestamp = qcc::GetTimestamp64() + delay;
                        tilExpire = min(tilExpire, delay + 1);
                    }
                } else {
                    QCC_LogError(ER_FAIL, ("Exhausted JoinSession retries to %s", entry.advName.c_str()));
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
    SessionlessJoinContext* ctx = reinterpret_cast<SessionlessJoinContext*>(context);

    QCC_DbgPrintf(("JoinSessionCB(status=%s,sid=%u) name=%s", QCC_StatusText(status), sid, ctx->name.c_str()));

    /* Extract guid from creator name */
    String advName = ctx->name;
    String guid;
    uint32_t advChangeId;
    QStatus sts = ParseAdvertisedName(advName, &guid, &advChangeId);
    if (sts != ER_OK) {
        QCC_LogError(sts, ("Cant extract guid from name \"%s\"", advName.c_str()));
        if (status == ER_OK) {
            bus.LeaveSession(sid);
        }
        delete ctx;
        return;
    }

    /* Send out RequestSignals or RequestRange message if join was successful. Otherwise retry. */
    router.LockNameTable();
    lock.Lock();
    map<String, ChangeIdEntry>::iterator cit = changeIdMap.find(guid);
    if (cit != changeIdMap.end()) {
        bool rangeCapable = false;
        bool isCatchup = false;
        CatchupState catchup;

        /* Check to see if there are any pending catch ups */
        uint32_t requestChangeId = cit->second.changeId + 1;
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
            cit->second.Completed();

            if (ScheduleRetry(cit->second) != ER_OK) {
                /* Retries exhausted. Clear state and wait for new advertisment */
                changeIdMap.erase(cit);
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
                status = RequestRange(advName.c_str(), sid, catchup.changeId, requestChangeId);
            } else if (rangeCapable) {
                status = RequestRange(advName.c_str(), sid, requestChangeId, advChangeId + 1);
            } else {
                status = RequestSignals(advName.c_str(), sid, requestChangeId);
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to send Request to %s", advName.c_str()));
                bus.LeaveSession(sid);
                lock.Lock();
                if (isCatchup) {
                    catchupMap.erase(sid);
                    cit->second.catchupList.push(catchup);
                }

                /* Clear inProgress */
                cit->second.Completed();

                if (ScheduleRetry(cit->second) != ER_OK) {
                    /* Retries exhausted. Clear state and wait for new advertisment */
                    changeIdMap.erase(cit);
                }
                lock.Unlock();
            }
        }
    } else {
        lock.Unlock();
        router.UnlockNameTable();
        QCC_LogError(ER_FAIL, ("Missing entry in changeIdMap for %s", guid.c_str()));
    }

    delete ctx;
}

void SessionlessObj::ScheduleTry(ChangeIdEntry& entry)
{
    if (!entry.InProgress()) {
        ScheduleJoin(entry, qcc::Rand8());
    }
}

QStatus SessionlessObj::ScheduleRetry(ChangeIdEntry& entry)
{
    if (entry.retries < MAX_JOINSESSION_RETRIES) {
        ScheduleJoin(entry, 200 + (qcc::Rand16() >> 3));
        return ER_OK;
    } else {
        QCC_LogError(ER_FAIL, ("Exhausted JoinSession retries to %s", entry.advName.c_str()));
        return ER_FAIL;
    }
}

void SessionlessObj::ScheduleJoin(ChangeIdEntry& entry, uint32_t delayMs)
{
    entry.nextJoinTimestamp = GetTimestamp64() + delayMs;
    ++delayMs;
    SessionlessObj* slObj = this;
    QStatus status = timer.AddAlarm(Alarm(delayMs, slObj));
    if (status != ER_OK) {
        QCC_LogError(status, ("Timer::AddAlarm failed"));
    }
}

QStatus SessionlessObj::ParseAdvertisedName(const qcc::String& name, qcc::String* guid, uint32_t* changeId)
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

std::map<qcc::String, SessionlessObj::ChangeIdEntry>::iterator SessionlessObj::FindChangeIdEntry(SessionId sid)
{
    map<String, ChangeIdEntry>::iterator cit;
    for (cit = changeIdMap.begin(); cit != changeIdMap.end(); ++cit) {
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

}
