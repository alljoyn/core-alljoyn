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
    ruleCountMap(),
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

    /* Create the org.alljoyn.Sessionless interface */
    InterfaceDescription* intf = NULL;
    status = bus.CreateInterface(InterfaceName, intf);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to create interface %s", InterfaceName));
        return status;
    }
    intf->AddSignal("RequestSignals", "u", NULL, 0);
    intf->AddSignal("RequestRange", "uu", NULL, 0);
    intf->Activate();

    /* Make this object implement org.alljoyn.Sessionless */
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

    /* Acquire org.alljoyn.Sessionless name */
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

    /* Add a broadcast Rule rule to receive org.alljoyn.Sessionless signals */
    if (status == ER_OK) {
        status = bus.AddMatch("type='signal',interface='org.alljoyn.Sessionless'");
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to add match rule for org.alljoyn.Sessionless"));
        }
    }

    /* Must call base class */
    BusObject::ObjectRegistered();

    /* Notify parent */
    busController->ObjectRegistered(this);
}

void SessionlessObj::AddRule(const qcc::String& epName, Rule& rule)
{
    QCC_DbgTrace(("SessionlessObj::AddRule(%s, ...)", epName.c_str()));

    if (rule.sessionless == Rule::SESSIONLESS_TRUE) {
        router.LockNameTable();
        lock.Lock();
        map<String, uint32_t>::iterator it = ruleCountMap.find(epName);
        if (it == ruleCountMap.end()) {
            ruleCountMap.insert(pair<String, int>(epName, 1));
            /*
             * Since this is the first addMatch that specifies sessionless='t'
             * from this client, we need to re-receive previous signals for
             * this client (implicitly) if this daemon has previously received
             * sessionless signals for any client.
             */
            if (!changeIdMap.empty() || !messageMap.empty()) {
                lock.Unlock();
                router.UnlockNameTable();
                RereceiveMessages(epName, "");
                router.LockNameTable();
                lock.Lock();
            }
        } else {
            it->second++;
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
        map<String, uint32_t>::iterator it = ruleCountMap.find(epName);
        if (it != ruleCountMap.end() && it->second) {
            --it->second;
        }

        if (isDiscoveryStarted && RuleCount() == 0) {
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

    QCC_DbgPrintf(("RouteSessionlessMessage(sid=%d,msg={sender='%s',interface='%s',member='%s',path='%s'})",
                   sid, msg->GetSender(), msg->GetInterface(), msg->GetMemberName(), msg->GetObjectPath()));

    /*
     * Sessionless messages first get passed here to handle.  If we return
     * false, then the message will get routed (in the standard way) by the
     * DaemonRouter.
     */
    bool didRoute = false;

    String catchupEpName;
    lock.Lock();
    /*
     * Check if we've already routed this message.  This may occur if we are
     * retrying and only received a subset of the sessionless signals during our
     * most recent attempt.
     */
    map<String, ChangeIdEntry>::iterator cit = FindChangeIdEntry(sid);
    if (cit != changeIdMap.end()) {
        if (find(cit->second.routedMessages.begin(), cit->second.routedMessages.end(), RoutedMessage(msg)) !=
            cit->second.routedMessages.end()) {
            didRoute = true;
        } else {
            cit->second.routedMessages.push_back(RoutedMessage(msg));
        }
    }
    /*
     * Check to see if this session ID is for a catchup. If it is, we'll route
     * it below.
     */
    map<uint32_t, CatchupState>::const_iterator it = catchupMap.find(sid);
    if (it != catchupMap.end()) {
        catchupEpName = it->second.sender;
    }
    lock.Unlock();

    if (!didRoute && !catchupEpName.empty()) {
        router.LockNameTable();
        BusEndpoint ep = router.FindEndpoint(catchupEpName);
        if (ep->IsValid()) {
            QStatus status;
            router.UnlockNameTable();
            if (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
                status = VirtualEndpoint::cast(ep)->PushMessage(msg, sid);
            } else {
                status = ep->PushMessage(msg);
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("PushMessage to %s failed", catchupEpName.c_str()));
            }
        } else {
            router.UnlockNameTable();
        }
        didRoute = true;
    }

    return didRoute;
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

QStatus SessionlessObj::RereceiveMessages(const qcc::String& sender, const qcc::String& guid)
{
    QStatus status = ER_OK;
    QCC_DbgTrace(("SessionlessObj::RereceiveMessages(%s, %s)", sender.c_str(), guid.c_str()));
    uint64_t now = GetTimestamp64();
    const uint64_t timeoutValue = 18000;
    String selfGuid = bus.GetGlobalGUIDShortString();
    lock.Lock();

    map<String, ChangeIdEntry>::iterator it = guid.empty() ? changeIdMap.begin() : changeIdMap.find(guid);
    while ((status == ER_OK) && (it != changeIdMap.end())) {
        String lastGuid = it->first;

        /* Skip self */
        if (lastGuid == selfGuid) {
            ++it;
            continue;
        }

        /* Wait for inProgress to be cleared */
        while ((it != changeIdMap.end()) && !it->second.inProgress.empty() && (GetTimestamp64() < (now + timeoutValue))) {
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
            assert(it->second.inProgress.empty());

            /* Add new catchup state */
            uint32_t beginState = it->second.changeId - (numeric_limits<uint32_t>::max() >> 1);
            it->second.catchupList.push(CatchupState(sender, it->first, beginState, 0));

            /* Trigger an artificial FoundAdvertisedName to get the sessions rolling */
            String advName = it->second.advName;
            TransportMask mask = it->second.transport;
            lock.Unlock();
            HandleFoundAdvertisedName(advName.c_str(), mask, false);
            lock.Lock();
            it = changeIdMap.lower_bound(lastGuid);
        }

        /* Continue with other guids if guid is empty. (empty means all) */
        if (!guid.empty()) {
            break;
        } else if ((it != changeIdMap.end()) && (it->first == lastGuid)) {
            ++it;
        }
    }

    /* If all guids or self guid, retrieve from our own cache */
    if (guid.empty() || (guid == selfGuid)) {
        HandleRangeRequest(sender.c_str(), 0, curChangeId - (numeric_limits<uint32_t>::max() >> 1), curChangeId + 1);
    }

    lock.Unlock();

    return status;
}

void SessionlessObj::NameOwnerChanged(const String& name,
                                      const String* oldOwner,
                                      const String* newOwner)
{
    QCC_DbgTrace(("SessionlessObj::NameOwnerChanged(%s, %s, %s)", name.c_str(), oldOwner ? oldOwner->c_str() : "(null)", newOwner ? newOwner->c_str() : "(null)"));

    /* Remove entries from ruleCountMap for names exiting from the bus */
    if (oldOwner && !newOwner) {
        router.LockNameTable();
        lock.Lock();
        map<String, uint32_t>::iterator it = ruleCountMap.find(name);
        if (it != ruleCountMap.end()) {
            ruleCountMap.erase(it);
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
        if (isDiscoveryStarted && RuleCount() == 0) {
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
    if (status == ER_OK) {
        QCC_DbgPrintf(("FoundAdvertisedName(name=%s,transport=0x%x,...)", name, transport));
        HandleFoundAdvertisedName(name, transport, true);
    } else {
        QCC_LogError(status, ("GetArgs failed"));
    }
}

QStatus SessionlessObj::HandleFoundAdvertisedName(const char* name, TransportMask transport, bool isNewAdv)
{
    QStatus status = ER_OK;

    /* Examine found name to see if we need to connect to it */
    String guid;
    uint32_t changeId;
    status = ParseAdvertisedName(name, &guid, &changeId);
    if (status != ER_OK) {
        QCC_LogError(status, ("Found invalid name \"%s\"", name));
        return status;
    }

    /* Join session if we need signals from this advertiser and we aren't already getting them */
    lock.Lock();
    map<String, ChangeIdEntry>::iterator it = changeIdMap.find(guid);
    bool updateChangeIdMap = (it == changeIdMap.end()) || IS_GREATER(uint32_t, changeId, it->second.changeId);
    if (updateChangeIdMap || !isNewAdv) {
        SessionlessObj* slObj = this;

        /* Setup for JoinSession at random time between now and 256ms from now */
        uint32_t delay = qcc::Rand8();
        if (it == changeIdMap.end()) {
            changeIdMap.insert(pair<String, ChangeIdEntry>(guid, ChangeIdEntry(name, transport, numeric_limits<uint32_t>::max(), changeId, qcc::GetTimestamp64() + delay)));
            ++delay;
            status = timer.AddAlarm(Alarm(delay, slObj));
        } else {
            if (isNewAdv) {
                it->second.advName = name;
                it->second.advChangeId = changeId;
                it->second.transport = transport;
                it->second.retries = 0;
            }
            if (it->second.inProgress.empty()) {
                it->second.nextJoinTimestamp = qcc::GetTimestamp64() + delay;
                ++delay;
                status = timer.AddAlarm(Alarm(delay, slObj));
            }
        }
    }
    lock.Unlock();
    return status;
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
    QCC_DbgPrintf(("SessionLost(sid=%d,reason=%d)", sid, reason));
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
        cit->second.inProgress.clear();
        cit->second.sid = 0;

        if (reason == ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION) {
            /* We got all the signals, so update the ChangeIdEntry */
            ParseAdvertisedName(inProgress, NULL, &cit->second.changeId);
            cit->second.routedMessages.clear();

            /* Retrigger FoundAdvName if necessary */
            if (cit->second.changeId != cit->second.advChangeId) {
                String advName = cit->second.advName;
                TransportMask transport = cit->second.transport;
                lock.Unlock();
                HandleFoundAdvertisedName(advName.c_str(), transport, false);
                lock.Lock();
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
                    if (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
                        status = VirtualEndpoint::cast(ep)->PushMessage(it->second.second, sid);
                    } else {
                        status = ep->PushMessage(it->second.second);
                    }
                } else {
                    router.UnlockNameTable();
                }
                lock.Lock();
                it = messageMap.upper_bound(key);
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to push sessionless signal to %s", sender));
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
        map<String, ChangeIdEntry>::iterator cit = changeIdMap.begin();
        while (cit != changeIdMap.end()) {
            if ((cit->second.nextJoinTimestamp <= qcc::GetTimestamp64()) && cit->second.inProgress.empty() && ((cit->second.changeId != cit->second.advChangeId) || !cit->second.catchupList.empty())) {
                if (cit->second.retries++ < MAX_JOINSESSION_RETRIES) {
                    SessionlessJoinContext* ctx = new SessionlessJoinContext(cit->second.advName);
                    cit->second.inProgress = cit->second.advName;
                    SessionOpts opts = sessionOpts;
                    opts.transports = cit->second.transport;
                    status = bus.JoinSessionAsync(cit->second.advName.c_str(), sessionPort, NULL, opts, this, reinterpret_cast<void*>(ctx));
                    if (status == ER_OK) {
                        QCC_DbgPrintf(("JoinSessionAsync(name=%s,...) pending", cit->second.advName.c_str()));
                    } else {
                        QCC_LogError(status, ("JoinSessionAsync to %s failed", cit->second.advName.c_str()));
                        cit->second.inProgress.clear();
                        delete ctx;
                        /* Retry the join session with random backoff */
                        uint32_t delay = qcc::Rand8();
                        cit->second.nextJoinTimestamp = qcc::GetTimestamp64() + delay;
                        tilExpire = min(tilExpire, delay + 1);
                    }
                } else {
                    QCC_LogError(ER_FAIL, ("Exhausted JoinSession retries to %s", cit->second.advName.c_str()));
                }
            }
            ++cit;
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

    QCC_DbgPrintf(("JoinSessionCB(status=%s,sid=%d) name=%s", QCC_StatusText(status), sid, ctx->name.c_str()));

    /* Extract guid from creator name */
    String guid;
    QStatus sts = ParseAdvertisedName(ctx->name, &guid, NULL);
    if (sts != ER_OK) {
        QCC_LogError(sts, ("Cant extract guid from name \"%s\"", ctx->name.c_str()));
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
        String advName = cit->second.advName;
        bool isCatchup = false;
        CatchupState catchup;

        /* Check to see if there are any pending catch ups */
        uint32_t requestChangeId = cit->second.changeId + 1;
        if (status == ER_OK) {
            /* Update session ID */
            cit->second.sid = sid;

            if (!cit->second.catchupList.empty()) {
                /* Check to see if session host is capable of handling RequestSignalRange */
                bool rangeCapable = false;
                BusEndpoint ep = router.FindEndpoint(ctx->name);
                if (ep->IsValid() && (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL)) {
                    RemoteEndpoint rep = VirtualEndpoint::cast(ep)->GetBusToBusEndpoint(sid);
                    if (rep->IsValid()) {
                        rangeCapable = (rep->GetRemoteProtocolVersion() >= 6);
                    }
                }
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
            cit->second.inProgress.clear();
            cit->second.sid = 0;

            if (ScheduleRetry(cit->second) != ER_OK) {
                /* Retries exhausted. Clear state and wait for new advertisment */
                changeIdMap.erase(cit);
            }
        }
        lock.Unlock();
        router.UnlockNameTable();

        if (status == ER_OK) {
            /* Add/replace sessionless adv name for remote daemon */
            String guid = advName.substr(::strlen(WellKnownName) + 2, qcc::GUID128::SHORT_SIZE);
            busController->GetAllJoynObj().SetAdvNameAlias(guid, opts.transports, advName);

            /* Send the signal if join was successful */
            if (isCatchup) {
                status = RequestRange(advName.c_str(), sid, catchup.changeId, requestChangeId);
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
                cit->second.inProgress.clear();
                cit->second.sid = 0;

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

uint32_t SessionlessObj::RuleCount() const
{
    uint32_t count = 0;
    for (map<String, uint32_t>::const_iterator it = ruleCountMap.begin(); it != ruleCountMap.end(); ++it) {
        count += it->second;
    }
    return count;
}

QStatus SessionlessObj::ScheduleRetry(ChangeIdEntry& entry)
{
    if (entry.retries < MAX_JOINSESSION_RETRIES) {
        uint32_t delay = 200 + (qcc::Rand16() >> 3);
        entry.nextJoinTimestamp = GetTimestamp64() + delay;
        ++delay;
        SessionlessObj* slObj = this;
        QStatus status = timer.AddAlarm(Alarm(delay, slObj));
        if (status != ER_OK) {
            QCC_LogError(status, ("Timer::AddAlarm failed"));
        }
        return ER_OK;
    } else {
        QCC_LogError(ER_FAIL, ("Exhausted JoinSession retries to %s", entry.advName.c_str()));
        return ER_FAIL;
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
    QCC_DbgPrintf(("RequestSignals(name=%s,sid=%d,fromId=%d)", name, sid, fromId));
    return Signal(name, sid, *requestSignalsSignal, args, ArraySize(args));
}

QStatus SessionlessObj::RequestRange(const char* name, SessionId sid, uint32_t fromId, uint32_t toId)
{
    MsgArg args[2];
    args[0].Set("u", fromId);
    args[1].Set("u", toId);
    QCC_DbgPrintf(("RequestRange(name=%s,sid=%d,fromId=%d,toId=%d) to %s", name, sid, fromId, toId));
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

}
