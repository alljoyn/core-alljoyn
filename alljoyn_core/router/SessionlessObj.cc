/**
 * @file
 * This file implements the org.alljoyn.sl interfaces.
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
#include "ConfigDB.h"

#ifdef ENABLE_POLICYDB
#include "PolicyDB.h"
#endif

#define QCC_MODULE "SESSIONLESS"

using namespace std;
using namespace qcc;

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

const Rule SessionlessObj::legacyRule = Rule("type='error',sessionless='t'");

/*
 * The context for the implements query response.  It must be delivered on
 * a separate thread than the Query callback to avoid deadlock.
 */
struct ResponseContext {
    TransportMask transport;
    String name;
    IPEndpoint ns4;
    ResponseContext(TransportMask transport, const qcc::String& name, const qcc::IPEndpoint& ns4)
        : transport(transport), name(name), ns4(ns4) { }
};

/*
 * Internal context passed through JoinSessionAsync.  This holds a snapshot of
 * the remote cache state at the time we issue the JoinSessionAsync.
 */
struct RemoteCacheSnapshot {
    RemoteCacheSnapshot(SessionlessObj::RemoteCache& cache) :
        name(cache.name), guid(cache.guid) { }

    qcc::String name;
    qcc::String guid;
};

SessionlessObj::SessionlessObj(Bus& bus, BusController* busController) :
    BusObject(ObjectPath, false),
    bus(bus),
    busController(busController),
    router(reinterpret_cast<DaemonRouter&>(bus.GetInternal().GetRouter())),
    sessionlessIface(NULL),
    requestSignalsSignal(NULL),
    requestRangeSignal(NULL),
    requestRangeMatchSignal(NULL),
    timer("sessionless"),
    curChangeId(0),
    sessionOpts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY, SessionOpts::DAEMON_NAMES),
    sessionPort(SESSIONLESS_SESSION_PORT),
    advanceChangeId(false),
    nextRulesId(0),
    backoff(ConfigDB::GetConfigDB()->GetLimit("sls_backoff", 1500),
            ConfigDB::GetConfigDB()->GetLimit("sls_backoff_linear", 4),
            ConfigDB::GetConfigDB()->GetLimit("sls_backoff_exponential", 32),
            ConfigDB::GetConfigDB()->GetLimit("sls_backoff_max", 15 * 60))
{
    sessionOpts.transports = ConfigDB::GetConfigDB()->GetLimit("sls_preferred_transports", TRANSPORT_ANY & ~TRANSPORT_UDP);
}

SessionlessObj::~SessionlessObj()
{
    IpNameService::Instance().UnregisterListener(*this);

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
    intf->AddSignal("RequestRangeMatch", "uuas", NULL, 0);
    intf->Activate();

    /* Make this object implement org.alljoyn.sl */
    const InterfaceDescription* sessionlessIntf = bus.GetInterface(InterfaceName);
    if (!sessionlessIntf) {
        status = ER_BUS_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Failed to get %s interface", InterfaceName));
        return status;
    }

    /* Cache requestSignals, requestRange, and requestRangeMatch interface members */
    requestSignalsSignal = sessionlessIntf->GetMember("RequestSignals");
    assert(requestSignalsSignal);
    requestRangeSignal = sessionlessIntf->GetMember("RequestRange");
    assert(requestRangeSignal);
    requestRangeMatchSignal = sessionlessIntf->GetMember("RequestRangeMatch");
    assert(requestRangeMatchSignal);

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

    /* Register a signal handler for requestRangeMatch */
    status = bus.RegisterSignalHandler(this,
                                       static_cast<MessageReceiver::SignalHandler>(&SessionlessObj::RequestRangeMatchSignalHandler),
                                       requestRangeMatchSignal,
                                       NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to register RequestRangeMatch signal handler"));
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

    /* Register signal handler for SessionLostWithReasonAndDisposition */
    /* (If we werent in the daemon, we could just use SessionListener, but it doesnt work without the full BusAttachment implementation */
    status = bus.RegisterSignalHandler(this,
                                       static_cast<MessageReceiver::SignalHandler>(&SessionlessObj::SessionLostSignalHandler),
                                       ajIntf->GetMember("SessionLostWithReasonAndDisposition"),
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

    IpNameService::Instance().RegisterListener(*this);

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

        uint32_t fromRulesId = nextRulesId;
        bool isNewRule = true;
        uint32_t ruleId = nextRulesId;
        for (std::pair<RuleIterator, RuleIterator> range = rules.equal_range(epName); range.first != range.second; ++range.first) {
            if (range.first->second == rule) {
                isNewRule = false;
                ruleId = range.first->second.id;
                break;
            }
        }
        rules.insert(std::pair<String, TimestampedRule>(epName, TimestampedRule(rule, ruleId)));
        if (isNewRule) {
            ++nextRulesId;
        }

        ScheduleWork();

        /* For retrieving from our own cache after releasing the locks below */
        uint32_t fromChangeId = curChangeId - (numeric_limits<uint32_t>::max() >> 1);
        uint32_t toChangeId = curChangeId + 1;
        uint32_t toRulesId = nextRulesId;

        FindAdvertisedNames();

        lock.Unlock();
        router.UnlockNameTable();

        /* Retrieve from our own cache */
        HandleRangeRequest(epName.c_str(), 0, fromChangeId, toChangeId, fromRulesId, toRulesId);
    }
}

void SessionlessObj::RemoveRule(const qcc::String& epName, Rule& rule)
{
    if (rule.sessionless == Rule::SESSIONLESS_TRUE) {
        QCC_DbgPrintf(("RemoveRule(epName=%s,rule=%s)", epName.c_str(), rule.ToString().c_str()));
        router.LockNameTable();
        lock.Lock();

        std::pair<RuleIterator, RuleIterator> range = rules.equal_range(epName);
        while (range.first != range.second) {
            if (range.first->second == rule) {
                RemoveImplicitRules(range.first);
                rules.erase(range.first);
                break;
            }
            ++range.first;
        }

        if (rules.empty()) {
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

    router.LockNameTable();
    lock.Lock();

    /* Match the message against any existing implicit rules */
    uint32_t fromRulesId = nextRulesId - (numeric_limits<uint32_t>::max() >> 1);
    uint32_t toRulesId = nextRulesId;
    SendMatchingThroughEndpoint(0, msg, fromRulesId, toRulesId, true);

    /* Put the message in the local cache */
    SessionlessMessageKey key(msg->GetSender(), msg->GetInterface(), msg->GetMemberName(), msg->GetObjectPath());
    advanceChangeId = true;
    SessionlessMessage val(curChangeId, msg);
    LocalCache::iterator it = localCache.find(key);
    if (it == localCache.end()) {
        localCache.insert(pair<SessionlessMessageKey, SessionlessMessage>(key, val));
    } else {
        it->second = val;
    }

    lock.Unlock();
    router.UnlockNameTable();

    /* Kick the worker */
    uint32_t zero = 0;
    SessionlessObj* slObj = this;
    QStatus status = timer.AddAlarm(Alarm(zero, slObj));
    if (ER_OK != status) {
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

    return ER_OK;
}

bool SessionlessObj::RouteSessionlessMessage(SessionId sid, Message& msg)
{
    QCC_DbgPrintf(("RouteSessionlessMessage(sid=%u,msg={sender='%s',interface='%s',member='%s',path='%s'})",
                   sid, msg->GetSender(), msg->GetInterface(), msg->GetMemberName(), msg->GetObjectPath()));

    router.LockNameTable();
    lock.Lock();

    RemoteCaches::iterator cit = FindRemoteCache(sid);
    if (cit == remoteCaches.end()) {
        QCC_LogError(ER_WARNING, ("Received message on unknown sid %u, ignoring", sid));
        lock.Unlock();
        router.UnlockNameTable();
        return true;
    }
    RemoteCache& cache = cit->second;

    if (find(cache.routedMessages.begin(), cache.routedMessages.end(), RoutedMessage(msg)) != cache.routedMessages.end()) {
        /* We are retrying and have already routed this message, ignore it */
        lock.Unlock();
        router.UnlockNameTable();
        return true;
    } else {
        cache.routedMessages.push_back(RoutedMessage(msg));
    }

    SendMatchingThroughEndpoint(sid, msg, cache.fromRulesId, cache.toRulesId);

    lock.Unlock();
    router.UnlockNameTable();
    return true;
}

void SessionlessObj::SendMatchingThroughEndpoint(SessionId sid, Message msg, uint32_t fromRulesId, uint32_t toRulesId,
                                                 bool onlySendIfImplicit)
{
    uint32_t rulesRangeLen = toRulesId - fromRulesId;
    RuleIterator rit = rules.begin();
    bool isAnnounce = (0 == strcmp(msg->GetInterface(), "org.alljoyn.About")) && (0 == strcmp(msg->GetMemberName(), "Announce"));
    while (rit != rules.end()) {
        bool isExplicitMatch = false;
        String epName = rit->first;
        BusEndpoint ep = router.FindEndpoint(epName);
        RuleIterator end = rules.upper_bound(epName);
        for (; rit != end; ++rit) {
            if (IN_WINDOW(uint32_t, fromRulesId, rulesRangeLen, rit->second.id) && ep->IsValid() && ep->AllowRemoteMessages()) {
                if (rit->second.IsMatch(msg)) {
                    isExplicitMatch = true;
                    if (isAnnounce && !rit->second.implements.empty()) {
                        /*
                         * Add an implicit rule so that we will receive Announce
                         * signals if the interface of interest is removed from the
                         * Announce signal.
                         */
                        String ruleStr = String("sender='") + msg->GetSender() + "',interface='org.alljoyn.About',member='Announce'";
                        Rule rule(ruleStr.c_str());
                        AddImplicitRule(rule, rit);
                    }
                } else if (rit->second == legacyRule) {
                    /*
                     * Legacy clients will add the "type='error',sessionless='t'"
                     * rule.  In that case the expected behavior is that incoming
                     * sessionless signals will route through the daemon router's
                     * rule table.
                     */
                    router.GetRuleTable().Lock();
                    for (ajn::RuleIterator drit = router.GetRuleTable().FindRulesForEndpoint(ep);
                         !isExplicitMatch && (drit != router.GetRuleTable().End()) && (drit->first == ep);
                         ++drit) {
                        isExplicitMatch = drit->second.IsMatch(msg);
                    }
                    router.GetRuleTable().Unlock();
                }
            }
        }

        bool isImplicitMatch = false;
        if (isAnnounce && !isExplicitMatch && ep->IsValid() && ep->AllowRemoteMessages()) {
            /* The message did not match any rules for this endpoint.
             * Check if it matches (only) an implicit rule. */
            isImplicitMatch = IsOnlyImplicitMatch(epName, msg);
        }

        if ((onlySendIfImplicit && !isExplicitMatch && isImplicitMatch) ||
            (!onlySendIfImplicit && (isExplicitMatch || isImplicitMatch))) {
            lock.Unlock();
            router.UnlockNameTable();
            SendThroughEndpoint(msg, ep, sid);
            router.LockNameTable();
            lock.Lock();
            rit = rules.upper_bound(epName);
        }
    }
}

QStatus SessionlessObj::CancelMessage(const qcc::String& sender, uint32_t serialNum)
{
    QStatus status = ER_BUS_NO_SUCH_MESSAGE;
    bool messageErased = false;

    QCC_DbgTrace(("SessionlessObj::CancelMessage(%s, 0x%x)", sender.c_str(), serialNum));

    lock.Lock();
    SessionlessMessageKey key(sender.c_str(), "", "", "");
    LocalCache::iterator it = localCache.lower_bound(key);
    while ((it != localCache.end()) && (sender == it->second.second->GetSender())) {
        if (it->second.second->GetCallSerial() == serialNum) {
            if (!it->second.second->IsExpired()) {
                status = ER_OK;
            }
            localCache.erase(it);
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

void SessionlessObj::NameOwnerChanged(const String& name,
                                      const String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                                      const String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer)
{
    QCC_DbgTrace(("SessionlessObj::NameOwnerChanged(%s, %s, %s)", name.c_str(), oldOwner ? oldOwner->c_str() : "(null)", newOwner ? newOwner->c_str() : "(null)"));

    /* When newOwner and oldOwner are the same, only the name transfer changed. */
    if (newOwner == oldOwner) {
        return;
    }

    /* Remove entries from rules for names exiting from the bus */
    if (oldOwner && !newOwner) {
        router.LockNameTable();
        lock.Lock();
        std::pair<RuleIterator, RuleIterator> range = rules.equal_range(name);
        if (range.first != rules.end()) {
            RemoveImplicitRules(name);
            rules.erase(range.first, range.second);
        }

        /* Remove stored sessionless messages sent by the old owner */
        SessionlessMessageKey key(oldOwner->c_str(), "", "", "");
        LocalCache::iterator mit = localCache.lower_bound(key);
        while ((mit != localCache.end()) && (::strcmp(oldOwner->c_str(), mit->second.second->GetSender()) == 0)) {
            localCache.erase(mit++);
        }
        /* Alert the advertiser worker if the local cache is empty */
        if (localCache.empty()) {
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

    FoundAdvertisedNameHandler(name, transport, prefix);
}

void SessionlessObj::FoundAdvertisedNameHandler(const char* name, TransportMask transport, const char* prefix, bool doInitialBackoff)
{
    /* Examine found name to see if we need to connect to it */
    if ((transport & sessionOpts.transports) == 0) {
        QCC_DbgPrintf(("FoundAdvertisedName(name=%s,transport=0x%x,...): Transport not preferred", name, transport));
        return;
    }

    String guid, iface;
    uint32_t version, changeId;
    QStatus status = ParseAdvertisedName(name, &version, &guid, &iface, &changeId);
    if (status != ER_OK) {
        QCC_LogError(status, ("Found invalid name \"%s\"", name));
        return;
    }
    QCC_DbgPrintf(("FoundAdvertisedName(name=%s,transport=0x%x,...) guid=%s,version=%u,iface=%s,changeId=%u",
                   name, transport, guid.c_str(), version, iface.c_str(), changeId));

    /* Add/replace sessionless adv name for remote daemon */
    busController->GetAllJoynObj().AddAdvNameAlias(guid, transport, name);

    /* Join session if we need signals from this advertiser and we aren't already getting them */
    lock.Lock();
    RemoteCaches::iterator cit = remoteCaches.find(guid);
    if (cit == remoteCaches.end()) {
        remoteCaches.insert(pair<String, RemoteCache>(guid, RemoteCache(name, version, guid, iface, changeId, transport)));
    } else {
        cit->second.name = name;
        cit->second.ifaces.insert(iface);
        if (IS_GREATER(uint32_t, changeId, cit->second.changeId)) {
            cit->second.changeId = changeId;
            cit->second.retries = 0; /* Reset the backoff schedule when new signals are available. */
        }
        cit->second.transport = transport;
    }
    ScheduleWork(doInitialBackoff);
    lock.Unlock();
}

bool SessionlessObj::AcceptSessionJoiner(SessionPort port,
                                         const char* joiner,
                                         const SessionOpts& opts)
{
    QCC_DbgPrintf(("AcceptSessionJoiner(port=%d,joiner=%s,...)", port, joiner));
    return true;
}

void SessionlessObj::SessionJoined(SessionPort port,
                                   SessionId sid,
                                   const char* joiner)
{
    QCC_DbgPrintf(("SessionJoined(port=%d,sid=%u,joiner=%s)", port, sid, joiner));
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

    RemoteCaches::iterator cit = FindRemoteCache(sid);
    if (cit != remoteCaches.end()) {
        RemoteCache& cache = cit->second;
        /* Reset in progress */
        cache.state = RemoteCache::IDLE;
        cache.sid = 0;

        if (reason == ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION) {
            /* We got all the signals */
            cache.retries = 0;
            cache.routedMessages.clear();
            if (IS_GREATER(uint32_t, cache.toRulesId - 1, cache.appliedRulesId)) {
                cache.appliedRulesId = cache.toRulesId - 1;
            }
            if (IS_GREATER(uint32_t, cache.toChangeId - 1, cache.receivedChangeId)) {
                cache.receivedChangeId = cache.toChangeId - 1;
                cache.haveReceived = true;
            }

            /* Get the sessions rolling if necessary */
            ScheduleWork();
        } else {
            /* An error occurred while getting the signals, so retry */
            if (ScheduleWork(cache) != ER_OK) {
                /* Retries exhausted. Clear state and wait for new advertisment */
                EraseRemoteCache(cit);
            }
        }
    }

    lock.Unlock();
}

void SessionlessObj::RequestSignalsSignalHandler(const InterfaceDescription::Member* member,
                                                 const char* sourcePath,
                                                 Message& msg)
{
    uint32_t fromId;
    QStatus status = msg->GetArgs("u", &fromId);
    if (status == ER_OK) {
        /* Send all signals in the range [fromId, curChangeId] */
        QCC_DbgPrintf(("RequestSignals(sender=%s,sid=%u,fromId=%u)", msg->GetSender(), msg->GetSessionId(), fromId));
        HandleRangeRequest(msg->GetSender(), msg->GetSessionId(), fromId, curChangeId + 1);
    } else {
        QCC_LogError(status, ("Message::GetArgs failed"));
    }
}

void SessionlessObj::RequestRangeSignalHandler(const InterfaceDescription::Member* member,
                                               const char* sourcePath,
                                               Message& msg)
{
    uint32_t fromId, toId;
    QStatus status = msg->GetArgs("uu", &fromId, &toId);
    if (status == ER_OK) {
        QCC_DbgPrintf(("RequestRange(sender=%s,sid=%u,fromId=%u,toId=%u)", msg->GetSender(), msg->GetSessionId(), fromId, toId));
        HandleRangeRequest(msg->GetSender(), msg->GetSessionId(), fromId, toId);
    } else {
        QCC_LogError(status, ("Message::GetArgs failed"));
    }
}

void SessionlessObj::RequestRangeMatchSignalHandler(const InterfaceDescription::Member* member,
                                                    const char* sourcePath,
                                                    Message& msg)
{
    uint32_t fromId, toId;
    size_t numMatchRuleArgs;
    const MsgArg* matchRuleArgs;
    QStatus status = msg->GetArgs("uuas", &fromId, &toId, &numMatchRuleArgs, &matchRuleArgs);
    if (status == ER_OK) {
        QCC_DbgPrintf(("RequestRangeMatch(sender=%s,sid=%u,fromId=%u,toId=%u,numMatchRules=%d)",
                       msg->GetSender(), msg->GetSessionId(), fromId, toId, numMatchRuleArgs));
        vector<String> matchRules;
        for (size_t i = 0; i < numMatchRuleArgs; ++i) {
            char* matchRule;
            matchRuleArgs[i].Get("s", &matchRule);
            QCC_DbgPrintf(("  [%d] %s", i, matchRule));
            matchRules.push_back(matchRule);
        }
        HandleRangeRequest(msg->GetSender(), msg->GetSessionId(), fromId, toId, 0, 0, matchRules);
    } else {
        QCC_LogError(status, ("Message::GetArgs failed"));
    }
}

void SessionlessObj::HandleRangeRequest(const char* sender, SessionId sid,
                                        uint32_t fromChangeId, uint32_t toChangeId,
                                        uint32_t fromLocalRulesId, uint32_t toLocalRulesId,
                                        std::vector<qcc::String> remoteRules)
{
    QStatus status = ER_OK;
    bool messageErased = false;
    QCC_DbgTrace(("SessionlessObj::HandleControlSignal(%d, %d)", fromChangeId, toChangeId));

    /* Enable concurrency since PushMessage could block */
    bus.EnableConcurrentCallbacks();

    /* Advance the curChangeId */
    router.LockNameTable();
    lock.Lock();
    if (advanceChangeId) {
        ++curChangeId;
        advanceChangeId = false;
    }

    /* Send all messages in local cache in range [fromChangeId, toChangeId) */
    LocalCache::iterator it = localCache.begin();
    uint32_t rangeLen = toChangeId - fromChangeId;
    while (it != localCache.end()) {
        if (IN_WINDOW(uint32_t, fromChangeId, rangeLen, it->second.first)) {
            SessionlessMessageKey key = it->first;
            if (it->second.second->IsExpired()) {
                /* Remove expired message without sending */
                localCache.erase(it++);
                messageErased = true;
            } else if (sid != 0) {
                /* Send message to remote destination */
                bool isMatch = remoteRules.empty();
                for (vector<String>::iterator rit = remoteRules.begin(); !isMatch && (rit != remoteRules.end()); ++rit) {
                    Rule rule(rit->c_str());
                    isMatch = rule.IsMatch(it->second.second) || (rule == legacyRule);
                }
                if (isMatch) {
                    BusEndpoint ep = router.FindEndpoint(sender);
                    if (ep->IsValid()) {
                        lock.Unlock();
                        router.UnlockNameTable();
                        QCC_DbgPrintf(("Send cid=%u,serialNum=%u to sid=%u", it->second.first, it->second.second->GetCallSerial(), sid));
                        SendThroughEndpoint(it->second.second, ep, sid);
                        router.LockNameTable();
                        lock.Lock();
                    }
                }
                it = localCache.upper_bound(key);
            } else {
                /* Send message to local destination */
                SendMatchingThroughEndpoint(sid, it->second.second, fromLocalRulesId, toLocalRulesId);
                it = localCache.upper_bound(key);
            }
        } else {
            ++it;
        }
    }
    lock.Unlock();
    router.UnlockNameTable();

    /* Alert the advertiser worker */
    if (messageErased) {
        uint32_t zero = 0;
        SessionlessObj* slObj = this;
        status = timer.AddAlarm(Alarm(zero, slObj));
    }

    /* Close the session */
    if (sid != 0) {
        status = bus.LeaveSession(sid);
        if (status == ER_OK) {
            QCC_DbgPrintf(("LeaveSession(sid=%u)", sid));
        } else {
            QCC_LogError(status, ("LeaveSession sid=%u failed", sid));
        }
    }
}

void SessionlessObj::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QCC_DbgTrace(("SessionlessObj::AlarmTriggered(alarm, %s)", QCC_StatusText(reason)));

    QStatus status = ER_OK;

    if (reason == ER_OK) {
        Timespec tilExpire;
        uint32_t expire;

        /* Send name service responses if needed */
        ResponseContext* ctx = static_cast<ResponseContext*>(alarm->GetContext());
        if (ctx) {
            MDNSPacket response;
            response->SetDestination(ctx->ns4);
            MDNSAdvertiseRData advRData;
            advRData.SetTransport(sessionOpts.transports & TRANSPORT_IP);
            advRData.SetValue("name", ctx->name);
            String guid = bus.GetInternal().GetGlobalGUID().ToString();
            MDNSResourceRecord advertiseRecord("advertise." + guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &advRData);
            response->AddAdditionalRecord(advertiseRecord);
            QStatus status = IpNameService::Instance().Response(ctx->transport, 120, response);
            if (ER_OK == status) {
                QCC_DbgPrintf(("Sent implements response for name=%s", ctx->name.c_str()));
            } else {
                QCC_LogError(status, ("Response failed"));
            }
            delete ctx;
        }

        /* Purge the local cache of expired messages */
        lock.Lock();
        LocalCache::iterator it = localCache.begin();
        while (it != localCache.end()) {
            if (it->second.second->IsExpired(&expire)) {
                localCache.erase(it++);
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
        Timespec now;
        GetTimeNow(&now);
        for (RemoteCaches::iterator cit = remoteCaches.begin(); cit != remoteCaches.end(); ++cit) {
            RemoteCache& cache = cit->second;
            WorkType pendingWork = PendingWork(cache);
            if ((cache.nextJoinTime <= now) && (cache.state == RemoteCache::IDLE) && pendingWork) {
                RemoteCacheSnapshot* ctx = new RemoteCacheSnapshot(cache);
                cache.state = RemoteCache::IN_PROGRESS;
                if (pendingWork == SessionlessObj::APPLY_NEW_RULES) {
                    cache.fromChangeId = cache.receivedChangeId - (numeric_limits<uint32_t>::max() >> 1);
                    cache.toChangeId = cache.receivedChangeId + 1;
                    cache.fromRulesId = cache.appliedRulesId + 1;
                    cache.toRulesId = nextRulesId;
                } else if (pendingWork == SessionlessObj::REQUEST_NEW_SIGNALS) {
                    cache.fromChangeId = cache.receivedChangeId + 1;
                    cache.toChangeId = cache.changeId + 1;
                    cache.fromRulesId = cache.appliedRulesId - (numeric_limits<uint32_t>::max() >> 1);
                    cache.toRulesId = nextRulesId;
                }
                SessionOpts opts = sessionOpts;
                opts.transports = cache.transport;
                status = bus.JoinSessionAsync(cache.name.c_str(), sessionPort, NULL, opts, this, reinterpret_cast<void*>(ctx));
                if (status == ER_OK) {
                    QCC_DbgPrintf(("JoinSessionAsync(name=%s,...) pending", cache.name.c_str()));
                    ++cache.retries;
                } else {
                    QCC_LogError(status, ("JoinSessionAsync to %s failed", cache.name.c_str()));
                    cache.state = RemoteCache::IDLE;
                    delete ctx;
                    /* Retry with a random backoff */
                    ScheduleWork(cache, false);
                    if ((tilExpire == Timespec::Zero) || (cache.nextJoinTime < tilExpire)) {
                        tilExpire = cache.nextJoinTime;
                    }
                }
            }
        }

        lock.Unlock();
        router.UnlockNameTable();

        /* Rearm alarm */
        if (tilExpire != Timespec::Zero) {
            SessionlessObj* slObj = this;
            timer.AddAlarm(Alarm(tilExpire, slObj));
        }
    }
}

void SessionlessObj::JoinSessionCB(QStatus status, SessionId sid, const SessionOpts& opts, void* context)
{
    RemoteCacheSnapshot* ctx = reinterpret_cast<RemoteCacheSnapshot*>(context);

    QCC_DbgPrintf(("JoinSessionCB(status=%s,sid=%u) name=%s", QCC_StatusText(status), sid, ctx->name.c_str()));

    /* Send out RequestRange message if join was successful. Otherwise retry. */
    router.LockNameTable();
    lock.Lock();
    RemoteCaches::iterator cit = remoteCaches.find(ctx->guid);
    if (cit != remoteCaches.end()) {
        RemoteCache& cache = cit->second;
        uint32_t fromId = cache.fromChangeId;
        uint32_t toId = cache.toChangeId;
        bool rangeCapable = false;
        bool matchCapable = false;
        vector<String> matchRules;
        if (status == ER_OK) {
            /* Update session ID */
            cache.sid = sid;

            /* Check to see if session host is capable of handling RequestRange */
            BusEndpoint ep = router.FindEndpoint(ctx->name);
            if (ep->IsValid() && (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL)) {
                RemoteEndpoint rep = VirtualEndpoint::cast(ep)->GetBusToBusEndpoint(sid);
                if (rep->IsValid()) {
                    rangeCapable = (rep->GetRemoteProtocolVersion() >= 6);
                    matchCapable = (rep->GetRemoteProtocolVersion() >= 10);
                }
            }
            if (!rangeCapable && (toId != cache.changeId + 1)) {
                /* This session can't be used because the remote side doesn't support RequestRange */
                status = bus.LeaveSession(sid);
                QCC_LogError(status, ("Failed to leave session %u", sid));
                DoSessionLost(sid, ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION);
                status = ER_FAIL;
            }
            if (matchCapable) {
                uint32_t rulesRangeLen = cache.toRulesId - cache.fromRulesId;
                for (RuleIterator rit = rules.begin(); rit != rules.end(); ++rit) {
                    if (IN_WINDOW(uint32_t, cache.fromRulesId, rulesRangeLen, rit->second.id)) {
                        matchRules.push_back(rit->second.ToString());
                    }
                }
                for (ImplicitRuleIterator irit = implicitRules.begin(); irit != implicitRules.end(); ++irit) {
                    qcc::String sender = irit->sender;
                    if (sender.substr(1, sender.find_last_of('.') - 1) == cache.guid) {
                        matchRules.push_back(irit->ToString());
                    }
                }
            }
        } else {
            QCC_LogError(status, ("JoinSessionAsync to %s failed", cache.name.c_str()));

            /* Clear in progress */
            cache.state = RemoteCache::IDLE;
            cache.sid = 0;

            if (ScheduleWork(cache) != ER_OK) {
                /* Retries exhausted. Clear state and wait for new advertisment */
                EraseRemoteCache(cit);
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
            if (matchCapable) {
                status = RequestRangeMatch(ctx->name.c_str(), sid, fromId, toId, matchRules);
            } else if (rangeCapable) {
                status = RequestRange(ctx->name.c_str(), sid, fromId, toId);
            } else {
                status = RequestSignals(ctx->name.c_str(), sid, fromId);
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to send Request to %s", ctx->name.c_str()));
                status = bus.LeaveSession(sid);
                QCC_LogError(status, ("Failed to leave session %u", sid));

                lock.Lock();
                cit = remoteCaches.find(ctx->guid);
                if (cit != remoteCaches.end()) {
                    cache = cit->second;

                    /* Clear in progress */
                    cache.state = RemoteCache::IDLE;
                    cache.sid = 0;

                    if (ScheduleWork(cache) != ER_OK) {
                        /* Retries exhausted. Clear state and wait for new advertisment */
                        EraseRemoteCache(cit);
                    }
                }
                lock.Unlock();
            }
        }
    } else {
        lock.Unlock();
        router.UnlockNameTable();
        QCC_LogError(ER_FAIL, ("Missing cache in remoteCaches for %s", ctx->guid.c_str()));
    }

    delete ctx;
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

QStatus SessionlessObj::RequestRangeMatch(const char* name, SessionId sid, uint32_t fromId, uint32_t toId,
                                          std::vector<qcc::String>& matchRules)
{
    MsgArg args[3];
    args[0].Set("u", fromId);
    args[1].Set("u", toId);
    args[2].Set("a$", matchRules.size(), matchRules.empty() ? NULL : &matchRules[0]);
    QCC_DbgPrintf(("RequestRangeMatch(name=%s,sid=%u,fromId=%d,toId=%d,numRules=%d)", name, sid, fromId, toId, matchRules.size()));
    return Signal(name, sid, *requestRangeMatchSignal, args, ArraySize(args));
}

QStatus SessionlessObj::SendThroughEndpoint(Message& msg, BusEndpoint& ep, SessionId sid)
{
    QStatus status;

#ifdef ENABLE_POLICYDB
    PolicyDB policyDB = ConfigDB::GetConfigDB()->GetPolicyDB();
    BusEndpoint dummy;
    NormalizedMsgHdr nmh(msg, policyDB, dummy);
    bool okToReceive = policyDB->OKToReceive(nmh, ep);
#else
    bool okToReceive = true;
#endif

    if (!okToReceive) {
        status = ER_BUS_POLICY_VIOLATION;
    } else if (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
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
    /* Figure out what we need to advertise. */
    map<String, uint32_t> advertisements;
    lock.Lock();
    for (LocalCache::iterator it = localCache.begin(); it != localCache.end(); ++it) {
        Message msg = it->second.second;
        advertisements[msg->GetInterface()] = max(advertisements[msg->GetInterface()], it->second.first);
        advertisements[WildcardInterfaceName] = max(advertisements[WildcardInterfaceName], it->second.first); /* The v0 advertisement */
    }

    /* First pass: cancel any names that don't need to be advertised anymore. */
    vector<String> cancelNames;
    for (map<String, uint32_t>::iterator last = lastAdvertisements.begin(); last != lastAdvertisements.end();) {
        String name = last->first;
        if (advertisements.find(name) == advertisements.end()) {
            cancelNames.push_back(AdvertisedName(name, last->second));
            lastAdvertisements.erase(last);
            last = lastAdvertisements.upper_bound(name);
        } else {
            ++last;
        }
    }

    /* Second pass: update/add any new advertisements. */
    vector<String> advertiseNames;
    for (map<String, uint32_t>::iterator it = advertisements.begin(); it != advertisements.end(); ++it) {
        map<String, uint32_t>::iterator last = lastAdvertisements.find(it->first);
        if (last == lastAdvertisements.end() || IS_GREATER(uint32_t, it->second, last->second)) {
            if (last != lastAdvertisements.end()) {
                cancelNames.push_back(AdvertisedName(last->first, last->second));
                lastAdvertisements.erase(last);
            }
            advertiseNames.push_back(AdvertisedName(it->first, it->second));
            lastAdvertisements[it->first] = it->second;
        }
    }

    lock.Unlock();

    for (vector<String>::iterator it = cancelNames.begin(); it != cancelNames.end(); ++it) {
        CancelAdvertisedName(*it);
    }
    for (vector<String>::iterator it = advertiseNames.begin(); it != advertiseNames.end(); ++it) {
        AdvertiseName(*it);
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
        status = bus.AdvertiseName(name.c_str(), sessionOpts.transports & ~TRANSPORT_LOCAL);
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
    QStatus status = bus.CancelAdvertiseName(name.c_str(), sessionOpts.transports & ~TRANSPORT_LOCAL);
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
    set<String> names;
    for (RuleIterator rit = rules.begin(); rit != rules.end(); ++rit) {
        String name;
        if (rit->second.implements.empty()) {
            name = "name='" + (rit->second.iface.empty() ? WildcardInterfaceName : rit->second.iface) + ".sl.*'";
        } else {
            for (set<String>::const_iterator iit = rit->second.implements.begin(); iit != rit->second.implements.end(); ++iit) {
                if (!name.empty()) {
                    name += ",";
                }
                name += "implements='" + *iit + "'";
            }
        }
        if (name.empty()) {
            continue;
        }
        if (findingNames.insert(name).second) {
            names.insert(name);
        }
    }
    if (!findingNames.empty()) {
        /*
         * If we're finding anything, we always need to find the v0
         * advertisement for backwards compatibility.
         */
        String name = String("name='") + WildcardInterfaceName + ".sl.*'";
        if (findingNames.insert(name).second) {
            names.insert(name);
        }
    }

    lock.Unlock();
    router.UnlockNameTable();
    set<String>::iterator it = names.begin();
    while (it != names.end()) {
        String name = *it;
        QCC_DbgPrintf(("FindAdvertisement(%s)", name.c_str()));
        QStatus status = FindAdvertisementByTransport(name.c_str(), sessionOpts.transports & ~TRANSPORT_LOCAL);
        if (status != ER_OK) {
            QCC_LogError(status, ("FindAdvertisementByTransport failed for name %s :", name.c_str()));
        }
        it++;
    }
    router.LockNameTable();
    lock.Lock();
}

void SessionlessObj::CancelFindAdvertisedNames()
{
    set<String> names = findingNames;
    findingNames.clear();

    lock.Unlock();
    router.UnlockNameTable();
    set<String>::iterator it = names.begin();
    while (it != names.end()) {
        String name = *it;
        QStatus status = CancelFindAdvertisementByTransport(name.c_str(), sessionOpts.transports & ~TRANSPORT_LOCAL);
        if (status == ER_OK) {
            QCC_DbgPrintf(("CancelFindAdvertisement(%s)", name.c_str()));
        } else {
            QCC_LogError(status, ("CancelFindAdvertisementByTransport failed for name %s :", name.c_str()));
        }
        it++;
    }
    router.LockNameTable();
    lock.Lock();

}

SessionlessObj::RemoteCaches::iterator SessionlessObj::FindRemoteCache(SessionId sid)
{
    RemoteCaches::iterator cit;
    for (cit = remoteCaches.begin(); cit != remoteCaches.end(); ++cit) {
        if (cit->second.sid == sid) {
            break;
        }
    }
    return cit;
}

void SessionlessObj::ScheduleWork(bool doInitialBackoff)
{
    RemoteCaches::iterator cit = remoteCaches.begin();
    while (cit != remoteCaches.end()) {
        RemoteCache& cache = cit->second;
        String guid = cache.guid;
        if (PendingWork(cache) && ScheduleWork(cache, true, doInitialBackoff) != ER_OK) {
            /* Retries exhausted. Clear state and wait for new advertisment */
            EraseRemoteCache(cit);
            cit = remoteCaches.upper_bound(guid);
        } else {
            ++cit;
        }
    }
}

QStatus SessionlessObj::ScheduleWork(RemoteCache& cache, bool addAlarm, bool doInitialBackoff)
{
    if (cache.state != RemoteCache::IDLE) {
        return ER_OK;
    }

    QStatus status = GetNextJoinTime(backoff, doInitialBackoff,
                                     cache.retries, cache.firstJoinTime, cache.nextJoinTime);
    if (status != ER_OK) {
        QCC_LogError(ER_FAIL, ("Exhausted JoinSession retries to %s", cache.guid.c_str()));
        return ER_FAIL;
    }

    if (addAlarm) {
        SessionlessObj* slObj = this;
        QStatus status = timer.AddAlarm(Alarm(cache.nextJoinTime, slObj));
        if (status != ER_OK) {
            QCC_LogError(status, ("Timer::AddAlarm failed"));
        }
    }
    return ER_OK;
}

/*
 * The backoff schedule looks like this:
 *
 * Initial
 * |    Linear
 * |    |                    Exponential
 * |    |                    |                 Constant
 * |    |                    |                 |
 * v    v                    v                 v
 * | T || 1T | 2T | 3T | 4T || 8T | 16T | 32T || 32T | 32T ...
 *
 * The actual join time is randomly distributed over the retry interval above.
 */
QStatus SessionlessObj::GetNextJoinTime(const BackoffLimits& backoff, bool doInitialBackoff,
                                        uint32_t retries, qcc::Timespec& firstJoinTime, qcc::Timespec& nextJoinTime)
{
    if (retries == 0) {
        GetTimeNow(&firstJoinTime);
    }

    qcc::Timespec startTime;
    uint32_t delayMs = 1;
    for (uint32_t m = 0, i = 0; i <= retries; ++i) {
        if (i == 0) {
            /* Initial backoff */
            startTime = firstJoinTime;
            delayMs = doInitialBackoff ? backoff.periodMs : 1;
        } else if (i <= backoff.linear) {
            /* Linear backoff */
            m = m + 1;
            startTime += delayMs;
            delayMs = m * backoff.periodMs;
        } else if (m < backoff.exponential) {
            /* Exponential backoff */
            m = m << 1;
            startTime += delayMs;
            delayMs = m * backoff.periodMs;
        } else {
            /* Constant backoff */
            startTime += delayMs;
        }
    }
    nextJoinTime = (startTime + qcc::Rand32() % delayMs);
    if ((nextJoinTime - firstJoinTime) < (backoff.maxSecs * 1000)) {
        return ER_OK;
    } else {
        return ER_FAIL;
    }
}

SessionlessObj::WorkType SessionlessObj::PendingWork(RemoteCache& cache)
{
    if (cache.haveReceived && IS_GREATER(uint32_t, nextRulesId - 1, cache.appliedRulesId)) {
        if (IsMatch(cache, cache.appliedRulesId + 1, nextRulesId)) {
            return APPLY_NEW_RULES;
        }
    } else if (IS_GREATER(uint32_t, cache.changeId, cache.receivedChangeId)) {
        if (IsMatch(cache, cache.appliedRulesId - (numeric_limits<uint32_t>::max() >> 1), nextRulesId)) {
            return REQUEST_NEW_SIGNALS;
        }
    }
    return NONE;
}

bool SessionlessObj::IsMatch(RemoteCache& cache, uint32_t fromRulesId, uint32_t toRulesId)
{
    if (cache.version == 0) {
        return true;
    }
    uint32_t rulesRangeLen = toRulesId - fromRulesId;
    for (RuleIterator rit = rules.begin(); rit != rules.end(); ++rit) {
        if (IN_WINDOW(uint32_t, fromRulesId, rulesRangeLen, rit->second.id) &&
            (cache.ifaces.find(rit->second.iface) != cache.ifaces.end())) {
            return true;
        }
    }
    return false;
}

bool SessionlessObj::QueryHandler(TransportMask transport, MDNSPacket query, uint16_t recvPort,
                                  const qcc::IPEndpoint& ns4)
{
    MDNSResourceRecord* searchRecord;
    if (!query->GetAdditionalRecord("search.*", MDNSResourceRecord::TXT, &searchRecord)) {
        return false;
    }
    MDNSSearchRData* searchRData = static_cast<MDNSSearchRData*>(searchRecord->GetRData());
    if (!searchRData) {
        QCC_DbgPrintf(("Ignoring query with invalid search info"));
        return false;
    }

    bool sentResponse = false;
    String ruleStr;
    for (int i = 0; !sentResponse && i < searchRData->GetNumFields(); ++i) {
        pair<String, String> field = searchRData->GetFieldAt(i);
        if (field.first == "implements") {
            if (!ruleStr.empty()) {
                ruleStr += ",";
            }
            ruleStr += "implements='" + field.second + "'";
        } else if (field.first == ";") {
            sentResponse = SendResponseIfMatch(transport, ns4, ruleStr);
            ruleStr.clear();
        }
    }
    if (!sentResponse) {
        sentResponse = SendResponseIfMatch(transport, ns4, ruleStr);
    }
    return sentResponse;
}

bool SessionlessObj::SendResponseIfMatch(TransportMask transport, const qcc::IPEndpoint& ns4, const qcc::String& ruleStr)
{
    if (ruleStr.empty()) {
        return false;
    }

    bool sendResponse = false;
    Rule rule(ruleStr.c_str());
    String name;
    lock.Lock();
    for (LocalCache::iterator mit = localCache.begin(); mit != localCache.end(); ++mit) {
        Message& msg = mit->second.second;
        if (rule.IsMatch(msg)) {
            name = AdvertisedName(msg->GetInterface(), lastAdvertisements[msg->GetInterface()]);
            sendResponse = true;
            break;
        }
    }
    lock.Unlock();

    if (sendResponse) {
        ResponseContext* ctx = new ResponseContext(transport, name, ns4);
        const uint32_t timeout = 0;
        SessionlessObj* pObj = this;
        Alarm alarm(timeout, pObj, ctx);
        QStatus status = timer.AddAlarm(alarm);
        if (ER_OK != status) {
            QCC_LogError(status, ("Response failed"));
            delete ctx;
        }
    }

    return sendResponse;
}

bool SessionlessObj::ResponseHandler(TransportMask transport, MDNSPacket response, uint16_t recvPort)
{
    MDNSResourceRecord* advRecord;
    if (!response->GetAdditionalRecord("advertise.*", MDNSResourceRecord::TXT, &advRecord)) {
        return false;
    }

    if (advRecord->GetRRttl() == 0) {
        return false;
    }

    MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
    if (!advRData) {
        QCC_DbgPrintf(("Ignoring response with invalid advertisement info"));
        return false;
    }
    /*
     * We always want to fetch advertisements received on the
     * multicast port (the unsolicited case).  We only want to fetch
     * advertisements received on the unicast port when they are a
     * response to our implements query.
     *
     * The check for only 1 name in the response is what determines
     * that the response is to our implements query.  The situation
     * is that a unicast response may include all the names
     * advertised by the responder, even ones we didn't ask for.  So
     * since we never explicitly ask for org.alljoyn.About.sl names
     * then if there's more than 1 name in the response the response
     * must have been for a different query.
     */
    bool unsolicited = (recvPort == IpNameService::MULTICAST_MDNS_PORT);
    bool solicited = (advRData->GetNumNames() == 1);
    if (!unsolicited && !solicited) {
        return false;
    }

    /*
     * Next step is to see if the response matches any of our rules.  If it
     * does, then report the name as found.
     */
    router.LockNameTable();
    lock.Lock();
    for (RuleIterator rit = rules.begin(); rit != rules.end(); ++rit) {
        Rule& rule = rit->second;
        if (rule.iface != "org.alljoyn.About") {
            continue;
        }

        String name;
        for (int i = 0; i < advRData->GetNumFields(); ++i) {
            pair<String, String> field = advRData->GetFieldAt(i);
            if ((field.first == "name") && (field.second.find(rule.iface) == 0)) {
                name = field.second;
            } else if (field.first == "transport") {
                transport = StringToU32(field.second, 16);
                if (!name.empty()) {
                    QCC_DbgPrintf(("Received %s implements response (name=%s) ttl %d",
                                   (unsolicited ? "unsolicited" : "solicited"),
                                   name.c_str(), advRecord->GetRRttl()));
                    FoundAdvertisedNameHandler(name.c_str(), transport, name.c_str(), unsolicited);
                }
                name.clear();
            }
        }

        if (!name.empty()) {
            QCC_DbgPrintf(("Received %s implements response (name=%s) ttl %d",
                           (unsolicited ? "unsolicited" : "solicited"),
                           name.c_str(), advRecord->GetRRttl()));
            FoundAdvertisedNameHandler(name.c_str(), transport, name.c_str(), unsolicited);
        }
    }
    lock.Unlock();
    router.UnlockNameTable();

    /* Always return false since we're just sniffing for org.alljoyn.About.sl. advertisements */
    return false;
}

QStatus SessionlessObj::FindAdvertisementByTransport(const char* matching, TransportMask transports)
{
    Message reply(bus);
    MsgArg args[2];
    size_t numArgs = ArraySize(args);

    MsgArg::Set(args, numArgs, "sq", matching, transports);

    const ProxyBusObject& alljoynObj = bus.GetAllJoynProxyObj();
    QStatus status = alljoynObj.MethodCall(org::alljoyn::Bus::InterfaceName, "FindAdvertisementByTransport", args, numArgs, reply);
    if (ER_OK == status) {
        uint32_t disposition;
        status = reply->GetArgs("u", &disposition);
        if (ER_OK == status) {
            switch (disposition) {
            case ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS:
                break;

            case ALLJOYN_FINDADVERTISEDNAME_REPLY_ALREADY_DISCOVERING:
                status = ER_ALLJOYN_FINDADVERTISEDNAME_REPLY_ALREADY_DISCOVERING;
                break;

            case ALLJOYN_FINDADVERTISEDNAME_REPLY_FAILED:
                status = ER_ALLJOYN_FINDADVERTISEDNAME_REPLY_FAILED;
                break;

            default:
                status = ER_BUS_UNEXPECTED_DISPOSITION;
                break;
            }
        }
    } else {
        QCC_LogError(status, ("%s.FindAdvertisement returned ERROR_MESSAGE (error=%s)", org::alljoyn::Bus::InterfaceName, reply->GetErrorDescription().c_str()));
    }
    return status;
}

QStatus SessionlessObj::CancelFindAdvertisementByTransport(const char* matching, TransportMask transports)
{
    Message reply(bus);
    MsgArg args[2];
    size_t numArgs = ArraySize(args);

    MsgArg::Set(args, numArgs, "sq", matching, transports);

    const ProxyBusObject& alljoynObj = bus.GetAllJoynProxyObj();
    QStatus status = alljoynObj.MethodCall(org::alljoyn::Bus::InterfaceName, "CancelFindAdvertisementByTransport", args, numArgs, reply);
    if (ER_OK == status) {
        uint32_t disposition;
        status = reply->GetArgs("u", &disposition);
        if (ER_OK == status) {
            switch (disposition) {
            case ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_SUCCESS:
                break;

            case ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_FAILED:
                status = ER_ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_FAILED;
                break;

            default:
                status = ER_BUS_UNEXPECTED_DISPOSITION;
                break;
            }
        }
    } else {
        QCC_LogError(status, ("%s.CancelFindAdvertisement returned ERROR_MESSAGE (error=%s)", org::alljoyn::Bus::InterfaceName, reply->GetErrorDescription().c_str()));
    }
    return status;
}

void SessionlessObj::EraseRemoteCache(RemoteCaches::iterator cit)
{
    RemoveImplicitRules(cit->second);
    remoteCaches.erase(cit);
}

void SessionlessObj::AddImplicitRule(const Rule& rule, const RuleIterator& explicitRule) {
    for (ImplicitRuleIterator irit = implicitRules.begin(); irit != implicitRules.end(); ++irit) {
        if (*irit == rule) {
            for (std::vector<RuleIterator>::iterator erit = irit->explicitRules.begin(); erit != irit->explicitRules.end(); ++erit) {
                if (*erit == explicitRule) {
                    return;
                }
            }
            irit->explicitRules.push_back(explicitRule);
            return;
        }
    }
    implicitRules.push_back(ImplicitRule(rule, explicitRule));
}

void SessionlessObj::RemoveImplicitRules(const qcc::String& epName) {
    QCC_DbgTrace(("SessionlessObj::RemoveImplicitRules(epName=%s)", epName.c_str()));
    ImplicitRuleIterator irit = implicitRules.begin();
    while (irit != implicitRules.end()) {
        std::vector<RuleIterator>::iterator erit = irit->explicitRules.begin();
        while (erit != irit->explicitRules.end()) {
            if ((*erit)->first == epName) {
                irit->explicitRules.erase(erit);
                erit = irit->explicitRules.begin();
            } else {
                ++erit;
            }
        }
        if (irit->explicitRules.empty()) {
            implicitRules.erase(irit);
            irit = implicitRules.begin();
        } else {
            ++irit;
        }
    }
}

void SessionlessObj::RemoveImplicitRules(const RuleIterator& explicitRule) {
    QCC_DbgTrace(("SessionlessObj::RemoveImplicitRules(explicitrule=%s for endpoint %s)", explicitRule->second.ToString().c_str(), explicitRule->first.c_str()));
    ImplicitRuleIterator irit = implicitRules.begin();
    while (irit != implicitRules.end()) {
        bool deleted = false;
        for (std::vector<RuleIterator>::iterator erit = irit->explicitRules.begin(); erit != irit->explicitRules.end(); ++erit) {
            if (*erit == explicitRule) {
                irit->explicitRules.erase(erit);
                if (irit->explicitRules.empty()) {
                    implicitRules.erase(irit);
                    deleted = true;
                }
                break;
            }
        }
        if (deleted) {
            irit = implicitRules.begin();
        } else {
            irit++;
        }
    }
}

void SessionlessObj::RemoveImplicitRules(const RemoteCache& cache) {
    QCC_DbgTrace(("SessionlessObj::RemoveImplicitRules(remotecache=%s)", cache.guid.c_str()));
    String guid = cache.guid;
    ImplicitRuleIterator irit = implicitRules.begin();
    while (irit != implicitRules.end()) {
        String sender = irit->sender;
        if (sender.substr(1, sender.find_last_of('.') - 1) == guid) {
            implicitRules.erase(irit);
            irit = implicitRules.begin();
        } else {
            irit++;
        }
    }
}

bool SessionlessObj::IsOnlyImplicitMatch(const qcc::String& epName, Message& msg)
{
    QCC_DbgTrace(("IsOnlyImplicitMatch(epName=%s, msg.sender=%s)", epName.c_str(), msg->GetSender()));

    /* Find the implicit rule that matches the message sender, check all associated
     * explicit rules that originate from epName. If none of those matches, the match is
     * purely implicit, and the implicit match rule should be removed for this epName.
     */
    for (ImplicitRuleIterator irit = implicitRules.begin(); irit != implicitRules.end(); ++irit) {
        if (irit->IsMatch(msg)) {
            bool hasExplicitMatch = false;
            std::pair<RuleIterator, RuleIterator> range = rules.equal_range(epName);
            bool hasExplicitRules = (range.first != range.second);
            for (; range.first != range.second; range.first++) {
                if (range.first->second.IsMatch(msg)) {
                    hasExplicitMatch = true;
                    break;
                }
            }
            if (hasExplicitRules && !hasExplicitMatch) {
                /* remove all explicit rules related to epName */
                vector<RuleIterator>::iterator erit = irit->explicitRules.begin();
                while (erit != irit->explicitRules.end()) {
                    if ((*erit)->first == epName) {
                        irit->explicitRules.erase(erit);
                        erit = irit->explicitRules.begin();
                    } else {
                        erit++;
                    }
                }
                if (irit->explicitRules.empty()) {
                    implicitRules.erase(irit);
                }
                return true;
            }
            return false;
        }
    }
    return false;
}

}
