/**
 * @file
 * Router is responsible for taking inbound messages and routing them
 * to an appropriate set of endpoints.
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <assert.h>

#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/atomic.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Status.h>

#include "BusController.h"
#include "BusEndpoint.h"
#include "ConfigDB.h"
#include "DaemonRouter.h"
#include "EndpointHelper.h"
#ifdef ENABLE_POLICYDB
#include "PolicyDB.h"
#endif

#define QCC_MODULE "ROUTER"

using namespace std;
using namespace qcc;


namespace ajn {


DaemonRouter::DaemonRouter() : ruleTable(), nameTable(), busController(NULL)
{
#ifdef ENABLE_POLICYDB
    AddBusNameListener(ConfigDB::GetConfigDB());
#endif
}

DaemonRouter::~DaemonRouter()
{
}

static inline QStatus SendThroughEndpoint(Message& msg, BusEndpoint& ep, SessionId sessionId)
{
    QCC_DbgTrace(("SendThroughEndpoint(): Routing \"%s\" (%d) through \"%s\"", msg->Description().c_str(), msg->GetCallSerial(), ep->GetUniqueName().c_str()));
    QStatus status;
    if ((sessionId != 0) && (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL)) {
        status = VirtualEndpoint::cast(ep)->PushMessage(msg, sessionId);
    } else {
        status = ep->PushMessage(msg);
    }
    // if the bus is stopping or the endpoint is closing we don't expect to be able to send
    if ((status != ER_OK) && (status != ER_BUS_ENDPOINT_CLOSING) && (status != ER_BUS_STOPPING)) {
        QCC_DbgPrintf(("SendThroughEndpoint(dest=%s, ep=%s, id=%u) failed: %s", msg->GetDestination(), ep->GetUniqueName().c_str(), sessionId, QCC_StatusText(status)));
    }
    return status;
}

QStatus DaemonRouter::PushMessage(Message& msg, BusEndpoint& origSender)
{
    QCC_DbgTrace(("DaemonRouter::PushMessage(): Routing \"%s\" (%d) from \"%s\"", msg->Description().c_str(), msg->GetCallSerial(), origSender->GetUniqueName().c_str()));
    /*
     * Reference count protects local endpoint from being deregistered while in use.
     */
    if (!localEndpoint->IsValid()) {
        return ER_BUS_ENDPOINT_CLOSING;
    }

    QStatus status = ER_OK;
    BusEndpoint sender = origSender;
    bool replyExpected = (msg->GetType() == MESSAGE_METHOD_CALL) && ((msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED) == 0);
    const char* destination = msg->GetDestination();
    SessionId sessionId = msg->GetSessionId();
    bool isSessionless = msg->GetFlags() & ALLJOYN_FLAG_SESSIONLESS;
    bool destinationEmpty = destination[0] == '\0';
    BusEndpoint destEndpoint;

    if (!destinationEmpty) {
        nameTable.Lock();
        destEndpoint = nameTable.FindEndpoint(destination);
        nameTable.Unlock();
    }

#ifdef ENABLE_POLICYDB
    PolicyDB policyDB = ConfigDB::GetConfigDB()->GetPolicyDB();
    NormalizedMsgHdr nmh(msg, policyDB, origSender);

    if ((sender != localEndpoint) && !policyDB->OKToSend(nmh, destEndpoint)) {
        /* The sender is not allowed to send this message. */
        return ER_BUS_POLICY_VIOLATION;
    }
#endif

    /*
     * Sessionless messages don't have a session id even though they have a dedicated
     * session to travel over. Therefore, get the sessionId from the endpoint if possible.
     */
    if (isSessionless && (sender->GetEndpointType() == ENDPOINT_TYPE_BUS2BUS)) {
        QCC_DbgPrintf(("DaemonRouter::PushMessage(): RouteSessionlessMessage(): ENDPOINT_TYPE_BUS2BUS. Sender sessionId=%d.",
                       RemoteEndpoint::cast(sender)->GetSessionId()));

        /*
         * The bus controller is responsible for "some" routing of sessionless signals.
         * Specifically, sessionless signals that are received solely to "catch-up" a
         * newly connected local client are routed directly to that client by the BusController.
         * If RouteSessionlessSignal returns true, then the BusController has successfully
         * routed the sessionless signal to its destination and no further action should be
         * taken here. If it returns false, then the normal routing procedure should
         * attempt to deliver the message.
         */
        if (busController->GetSessionlessObj().RouteSessionlessMessage(RemoteEndpoint::cast(sender)->GetSessionId(), msg)) {
            return ER_OK;
        }
        QCC_DbgPrintf(("DaemonRouter::PushMessage(): Unable to RouteSessionlessMessage()"));
    }

    /*
     * If the message originated at the local endpoint check if the serial number needs to be
     * updated before queuing the message on a remote endpoint.
     */
    if (origSender == localEndpoint) {
        QCC_DbgPrintf(("DaemonRouter::PushMessage(): UpdateSerialNumber()"));
        LocalEndpoint::cast(origSender)->UpdateSerialNumber(msg);
    }

    if (!destinationEmpty) {
        QCC_DbgPrintf(("DaemonRouter::PushMessage(): destinationEmpty=false"));
        nameTable.Lock();
        if (destEndpoint->IsValid()) {
            QCC_DbgPrintf(("DaemonRouter::PushMessage(): Valid destEndpoint"));
            /* If this message is coming from a bus-to-bus ep, make sure the receiver is willing to receive it */
            if (!((sender->GetEndpointType() == ENDPOINT_TYPE_BUS2BUS) && !destEndpoint->AllowRemoteMessages())) {
                QCC_DbgPrintf(("DaemonRouter::PushMessage(): destEndpoint allows remote messages"));
                /*
                 * If the sender doesn't allow remote messages reject method
                 * calls that go off device and require a reply because the
                 * reply will be blocked and this is most definitely not what
                 * the sender expects.
                 */
                if ((destEndpoint->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) && replyExpected && !sender->AllowRemoteMessages()) {
                    QCC_DbgPrintf(("DaemonRouter::PushMessage(): Blocked method call from \"%s\" to \"%s\" (serial=%d). Caller does not allow remote messages",
                                   msg->GetSender(), destEndpoint->GetUniqueName().c_str(), msg->GetCallSerial()));
                    msg->ErrorMsg(msg, "org.alljoyn.Bus.Blocked", "Method reply would be blocked because caller does not allow remote messages");
                    BusEndpoint busEndpoint = BusEndpoint::cast(localEndpoint);
                    PushMessage(msg, busEndpoint);
#ifdef ENABLE_POLICYDB
                } else if ((destEndpoint != localEndpoint) && !policyDB->OKToReceive(nmh, destEndpoint)) {
                    /*
                     * The destination is not allowed to recieve the message.
                     * If a reply is expected, return an error to the sender.
                     */
                    QCC_DbgPrintf(("DaemonRouter::PushMessage(): Blocked method call from \"%s\" to \"%s\" (serial=%d). Caller does not allow remote messages",
                                   msg->GetSender(), destEndpoint->GetUniqueName().c_str(), msg->GetCallSerial()));
                    if (replyExpected) {
                        msg->ErrorMsg(msg, "org.alljoyn.Bus.Blocked", "Destination not allowed to receive method call");
                        BusEndpoint busEndpoint = BusEndpoint::cast(localEndpoint);
                        PushMessage(msg, busEndpoint);
                    }
                    status = ER_BUS_POLICY_VIOLATION;
#endif
                } else {
                    nameTable.Unlock();
                    QCC_DbgPrintf(("DaemonRouter::PushMessage(): SendThroughEndpoint()"));
                    status = SendThroughEndpoint(msg, destEndpoint, sessionId);
                    nameTable.Lock();
                }
            } else {
                QCC_DbgPrintf(("Blocked message from \"%s\" to \"%s\" (serial=%d). Receiver does not allow remote messages",
                               msg->GetSender(), destEndpoint->GetUniqueName().c_str(), msg->GetCallSerial()));
                /* If caller is expecting a response return an error indicating the method call was blocked */
                if (replyExpected) {
                    qcc::String description("Remote method calls blocked for bus name: ");
                    description += destination;
                    msg->ErrorMsg(msg, "org.alljoyn.Bus.Blocked", description.c_str());
                    BusEndpoint busEndpoint = BusEndpoint::cast(localEndpoint);
                    PushMessage(msg, busEndpoint);
                }
            }
            // if the bus is stopping or the endpoint is closing we can't push the message
            if ((ER_OK != status) && (ER_BUS_ENDPOINT_CLOSING != status) && (status != ER_BUS_STOPPING)) {
                QCC_DbgPrintf(("BusEndpoint::PushMessage failed: %s", QCC_StatusText(status)));
            }
            nameTable.Unlock();
        } else {
            nameTable.Unlock();
            if ((msg->GetFlags() & ALLJOYN_FLAG_AUTO_START) &&
                (sender->GetEndpointType() != ENDPOINT_TYPE_BUS2BUS) &&
                (sender->GetEndpointType() != ENDPOINT_TYPE_NULL)) {

                QCC_DbgPrintf(("DaemonRouter::PushMessage(): buscontroller->StartService()"));
                status = busController->StartService(msg, sender);
            } else {
                status = ER_BUS_NO_ROUTE;
            }
            if (status != ER_OK) {
                if (replyExpected) {
                    QCC_LogError(status, ("DaemonRouter::PushMessage(): Returning error \"%s\" no route to \"%s\"", msg->Description().c_str(), destination));
                    /* Need to let the sender know its reply message cannot be passed on. */
                    qcc::String description("Unknown bus name: ");
                    description += destination;
                    msg->ErrorMsg(msg, "org.freedesktop.DBus.Error.ServiceUnknown", description.c_str());
                    BusEndpoint busEndpoint = BusEndpoint::cast(localEndpoint);
                    PushMessage(msg, busEndpoint);
                } else {
                    /*
                     * The daemon may try and push a message when the program is
                     * shutting down this will result in a no route error. Although
                     * it is a valid error it is not a LogError. This is typically
                     * something like a NameLost signal because the program with the
                     * bundled router just shutdown.
                     */
                    if (ER_BUS_NO_ROUTE == status) {
                        QCC_DbgHLPrintf(("DaemonRouter::PushMessage(): Discarding \"%s\" no route to \"%s\"(%d): status=\"%s\"",
                                         msg->Description().c_str(), destination, sessionId, QCC_StatusText(status)));
                    } else {
                        QCC_LogError(status, ("DaemonRouter::PushMessage(): Discarding \"%s\" no route to \"%s\"(%d)",
                                              msg->Description().c_str(), destination, sessionId));
                    }
                }
            }
        }
    } else if (sessionId == 0) {
        /*
         * The message has an empty destination field and no session is specified so this is a
         * regular broadcast message.
         */
        QCC_DbgPrintf(("DaemonRouter::PushMessage(): broadcast messsage"));
        nameTable.Lock();
        ruleTable.Lock();
        RuleIterator it = ruleTable.Begin();
        while (it != ruleTable.End()) {
            if (it->second.IsMatch(msg)) {
                BusEndpoint dest = it->first;
                QCC_DbgPrintf(("DaemonRouter::PushMessage(): Routing \"%s\" (%d) to \"%s\"", msg->Description().c_str(), msg->GetCallSerial(), dest->GetUniqueName().c_str()));
                /*
                 * If the message originated locally or the destination allows remote messages
                 * forward the message, otherwise silently ignore it.
                 */
#ifdef ENABLE_POLICYDB
                if (!((sender->GetEndpointType() == ENDPOINT_TYPE_BUS2BUS) && !dest->AllowRemoteMessages()) &&
                    ((dest == localEndpoint) || policyDB->OKToReceive(nmh, dest))) {
#else
                if (!((sender->GetEndpointType() == ENDPOINT_TYPE_BUS2BUS) && !dest->AllowRemoteMessages())) {
#endif
                    ruleTable.Unlock();
                    nameTable.Unlock();
                    QCC_DbgPrintf(("DaemonRouter::PushMessage(): SendThroughEndpoint()"));
                    QStatus tStatus = SendThroughEndpoint(msg, dest, sessionId);
                    status = (status == ER_OK) ? tStatus : status;
                    nameTable.Lock();
                    ruleTable.Lock();
                }
                it = ruleTable.AdvanceToNextEndpoint(dest);
            } else {
                ++it;
            }
        }
        ruleTable.Unlock();
        nameTable.Unlock();

        if (msg->IsSessionless()) {
            /* Give "locally generated" sessionless message to SessionlessObj */
            if (sender->GetEndpointType() != ENDPOINT_TYPE_BUS2BUS) {
                status = busController->PushSessionlessMessage(msg);
            }
        } else if (msg->IsGlobalBroadcast()) {
            /*
             * A bit of explanation is needed here.  The AllJoyn daemon-to-daemon method
             * "DetachSession" is sent with session id == 0 (and NULL dest) rather than using the
             * session id being detached. This is because the receiver of the detach session message
             * is the bus controller of the session participant and is therefore not a member of the
             * session.  Unfortunately, because of the lack of a session id in this message, it was
             * discovered that the detach session message was capable of arriving at its destination
             * BEFORE the last user message(s) were received over the session. This caused
             * occasional loss of user data when there was more than one B2B endpoint that could
             * route to the receiving bus controller.
             *
             * The fix to this problem is unfortunately needed here. If the message is using
             * sessionId 0 and the message is a detach session message, then the session id in the
             * message body is used for selecting a B2B endpoint rather than the one that is in the
             * message header (0).
             */
            if ((sessionId == 0) && (::strcmp("DetachSession", msg->GetMemberName()) == 0) && (::strcmp(org::alljoyn::Daemon::InterfaceName, msg->GetInterface()) == 0)) {
                /* Clone the message since this message is unmarshalled by the LocalEndpoint too
                 * and the process of unmarshalling is not thread-safe.
                 */
                Message clone = Message(msg, true);
                QStatus status = clone->UnmarshalArgs("us");
                if (status == ER_OK) {
                    sessionId = clone->GetArg(0)->v_uint32;
                } else {
                    QCC_LogError(status, ("Failed to unmarshal args for DetachSession message"));
                }
            }

            /* Route global broadcast to all bus-to-bus endpoints that aren't the sender of the message */
            m_b2bEndpointsLock.Lock(MUTEX_CONTEXT);
            set<RemoteEndpoint>::iterator it = m_b2bEndpoints.begin();
            while (it != m_b2bEndpoints.end()) {
                RemoteEndpoint ep = *it;
                if ((ep != origSender) && ((sessionId == 0) || ep->GetSessionId() == sessionId)) {
                    BusEndpoint busEndpoint = BusEndpoint::cast(ep);
#ifdef ENABLE_POLICYDB
                    if ((busEndpoint == localEndpoint) || policyDB->OKToReceive(nmh, busEndpoint)) {
#else
                    {
#endif
                        m_b2bEndpointsLock.Unlock(MUTEX_CONTEXT);
                        QCC_DbgPrintf(("DaemonRouter::PushMessage(): SendThroughEndpoint()"));
                        QStatus tStatus = SendThroughEndpoint(msg, busEndpoint, sessionId);
                        status = (status == ER_OK) ? tStatus : status;
                        m_b2bEndpointsLock.Lock(MUTEX_CONTEXT);
                        it = m_b2bEndpoints.lower_bound(ep);
                    }
                }
                if (it != m_b2bEndpoints.end()) {
                    ++it;
                }
            }
            m_b2bEndpointsLock.Unlock(MUTEX_CONTEXT);
        }

    } else {
        /*
         * The message has an empty destination field and a session id was specified so this is a
         * session multicast message.
         */
        QCC_DbgPrintf(("DaemonRouter::PushMessage(): Session multicast message()"));
        sessionCastSetLock.Lock(MUTEX_CONTEXT);
        RemoteEndpoint lastB2b;
        /* We need to obtain the first entry in the sessionCastSet that has the id equal to 'sessionId'
         * and the src equal to 'msg->GetSender()'.
         * Note: sce.id has been set to sessionId - 1. Since the src is compared first, and session Ids
         * are integers, upper_bound will return the iterator to the first element with the desired or
         * greater src(if not present) and desired or greater id in most cases.
         */
        SessionCastEntry sce(sessionId - 1, msg->GetSender());
        set<SessionCastEntry>::iterator sit = sessionCastSet.upper_bound(sce);
        bool foundDest = false;
        bool okToReceive = true;

        /* In other cases, it may return the iterator to an element that has the desired src and
         * (sessionId - 1). In that case iterate, until the id is less than the desired one.
         */
        while ((sit != sessionCastSet.end()) && (sit->src == sce.src) && (sit->id < sessionId)) {
            sit++;
        }

        QCC_DbgPrintf(("DaemonRouter::PushMessage(): Sending to sessionCast subset"));
        while ((sit != sessionCastSet.end()) && (sit->id == sessionId) && (sit->src == sce.src)) {
            QCC_DbgPrintf(("DaemonRouter::PushMessage(): Trying \"%s\"", sit->destEp->GetUniqueName().c_str()));

            QCC_DbgPrintf(("DaemonRouter::PushMessage(): sit->b2bEp=\"%s\"", sit->b2bEp->GetUniqueName().c_str()));
            QCC_DbgPrintf(("DaemonRouter::PushMessage(): lastB2b=\"%s\"", lastB2b->GetUniqueName().c_str()));

            if (sit->b2bEp != lastB2b) {
                QCC_DbgPrintf(("DaemonRouter::PushMessage(): sit->b2bEp != lastB2b"));
                BusEndpoint ep = sit->destEp;

#ifdef ENABLE_POLICYDB
                okToReceive = (ep == localEndpoint) || policyDB->OKToReceive(nmh, ep);
#endif

                if (okToReceive) {
                    QCC_DbgPrintf(("DaemonRouter::PushMessage(): okToReceive"));
                    foundDest = true;
                    lastB2b = sit->b2bEp;
                    SessionCastEntry entry = *sit;
                    sessionCastSetLock.Unlock(MUTEX_CONTEXT);
                    QCC_DbgPrintf(("DaemonRouter::PushMessage(): SendThroughEndpoint(): ep=\"%s\", sessionId=%d", ep->GetUniqueName().c_str(), sessionId));
                    QStatus tStatus = SendThroughEndpoint(msg, ep, sessionId);
                    status = (status == ER_OK) ? tStatus : status;
                    sessionCastSetLock.Lock(MUTEX_CONTEXT);
                    sit = sessionCastSet.lower_bound(entry);
                }
            }
            if (sit != sessionCastSet.end()) {
                ++sit;
            }
        }
        if (!foundDest) {
            status = okToReceive ? ER_BUS_NO_ROUTE : ER_BUS_POLICY_VIOLATION;
        }
        sessionCastSetLock.Unlock(MUTEX_CONTEXT);
    }

    return status;
}

void DaemonRouter::GetBusNames(vector<qcc::String>& names) const
{
    nameTable.GetBusNames(names);
}

BusEndpoint DaemonRouter::FindEndpoint(const qcc::String& busName)
{
    BusEndpoint ep = nameTable.FindEndpoint(busName);
    if (!ep->IsValid()) {
        m_b2bEndpointsLock.Lock(MUTEX_CONTEXT);
        for (set<RemoteEndpoint>::const_iterator it = m_b2bEndpoints.begin(); it != m_b2bEndpoints.end(); ++it) {
            if ((*it)->GetUniqueName() == busName) {
                RemoteEndpoint rep = *it;
                ep = BusEndpoint::cast(rep);
                break;
            }
        }
        m_b2bEndpointsLock.Unlock(MUTEX_CONTEXT);
    }
    return ep;
}

QStatus DaemonRouter::AddRule(BusEndpoint& endpoint, Rule& rule)
{
    QStatus status = ruleTable.AddRule(endpoint, rule);

    /* Allow busController to examine this rule */
    if (status == ER_OK) {
        busController->AddRule(endpoint->GetUniqueName(), rule);
    }

    return status;
}

QStatus DaemonRouter::RemoveRule(BusEndpoint& endpoint, Rule& rule)
{
    QStatus status = ruleTable.RemoveRule(endpoint, rule);
    if (ER_OK == status) {
        /* Allow busController to examine rule being removed */
        busController->RemoveRule(endpoint->GetUniqueName(), rule);
    }
    return status;
}

QStatus DaemonRouter::RegisterEndpoint(BusEndpoint& endpoint)
{
    QCC_DbgTrace(("DaemonRouter::RegisterEndpoint(%s, %d)", endpoint->GetUniqueName().c_str(), endpoint->GetEndpointType()));
    QStatus status = ER_OK;

    /* Keep track of local endpoint */
    if (endpoint->GetEndpointType() == ENDPOINT_TYPE_LOCAL) {
        localEndpoint = LocalEndpoint::cast(endpoint);
    }

    if (endpoint->GetEndpointType() == ENDPOINT_TYPE_BUS2BUS) {
        /* AllJoynObj is in charge of managing bus-to-bus endpoints and their names */
        RemoteEndpoint busToBusEndpoint = RemoteEndpoint::cast(endpoint);
        status = busController->GetAllJoynObj().AddBusToBusEndpoint(busToBusEndpoint);

        /* Add to list of bus-to-bus endpoints */
        m_b2bEndpointsLock.Lock(MUTEX_CONTEXT);
        m_b2bEndpoints.insert(busToBusEndpoint);
        m_b2bEndpointsLock.Unlock(MUTEX_CONTEXT);
    } else {
        /* Bus-to-client endpoints appear directly on the bus */
        nameTable.AddUniqueName(endpoint);
    }

    /* Notify local endpoint that it is connected */
    if (endpoint == localEndpoint) {
        localEndpoint->OnBusConnected();
    }

    return status;
}

void DaemonRouter::UnregisterEndpoint(const qcc::String& epName, EndpointType epType)
{
    QCC_DbgTrace(("DaemonRouter::UnregisterEndpoint: %s", epName.c_str()));

    /* Attempt to get the endpoint */
    nameTable.Lock();
    BusEndpoint endpoint = FindEndpoint(epName);
    nameTable.Unlock();

    if (ENDPOINT_TYPE_BUS2BUS == endpoint->GetEndpointType()) {
        /* Inform bus controller of bus-to-bus endpoint removal */
        RemoteEndpoint busToBusEndpoint = RemoteEndpoint::cast(endpoint);

        busController->GetAllJoynObj().RemoveBusToBusEndpoint(busToBusEndpoint);

        /* Remove the bus2bus endpoint from the list */
        m_b2bEndpointsLock.Lock(MUTEX_CONTEXT);
        set<RemoteEndpoint>::iterator it = m_b2bEndpoints.begin();
        while (it != m_b2bEndpoints.end()) {
            RemoteEndpoint rep = *it;
            if (rep == busToBusEndpoint) {
                m_b2bEndpoints.erase(it);
                break;
            }
            ++it;
        }
        m_b2bEndpointsLock.Unlock(MUTEX_CONTEXT);

        /* Remove entries from sessionCastSet with same b2bEp */
        sessionCastSetLock.Lock(MUTEX_CONTEXT);
        set<SessionCastEntry>::iterator sit = sessionCastSet.begin();
        while (sit != sessionCastSet.end()) {
            set<SessionCastEntry>::iterator doomed = sit;
            ++sit;
            if (doomed->b2bEp == endpoint) {
                sessionCastSet.erase(doomed);
            }
        }
        sessionCastSetLock.Unlock(MUTEX_CONTEXT);
    } else {
        /* Remove any session routes */
        RemoveSessionRoutes(endpoint->GetUniqueName().c_str(), 0);
        /* Remove endpoint from names and rules */
        nameTable.RemoveUniqueName(endpoint->GetUniqueName());
        RemoveAllRules(endpoint);
        PermissionMgr::CleanPermissionCache(endpoint);
    }
    /*
     * If the local endpoint is being deregistered this indicates the router is being shut down.
     */
    if (endpoint == localEndpoint) {
        localEndpoint->Invalidate();
        localEndpoint = LocalEndpoint();
    }
}

QStatus DaemonRouter::AddSessionRoute(SessionId id, BusEndpoint& srcEp, RemoteEndpoint* srcB2bEp, BusEndpoint& destEp, RemoteEndpoint& destB2bEp, SessionOpts* optsHint)
{
    QCC_DbgTrace(("DaemonRouter::AddSessionRoute(%u, %s, %s, %s, %s, %s)", id, srcEp->GetUniqueName().c_str(), srcB2bEp ? (*srcB2bEp)->GetUniqueName().c_str() : "<none>", destEp->GetUniqueName().c_str(), destB2bEp->GetUniqueName().c_str(), optsHint ? "opts" : "NULL"));
    QStatus status = ER_OK;
    if (id == 0) {
        return ER_BUS_NO_SESSION;
    }
    if (destEp->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
        QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): destEp is ENDPOINT_TYPE_VIRTUAL)"));
        if (destB2bEp->IsValid()) {
            QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): AddSessionRef(id=%d., destB2bEp=\"%s\")", id, destB2bEp->GetUniqueName().c_str()));
            status = VirtualEndpoint::cast(destEp)->AddSessionRef(id, destB2bEp);
        } else if (optsHint) {
            QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): AddSessionRef(id=%d., optsHint, destB2bEp=\"%s\")", id, destB2bEp->GetUniqueName().c_str()));
            status = VirtualEndpoint::cast(destEp)->AddSessionRef(id, optsHint, destB2bEp);
        } else {
            status = ER_BUS_NO_SESSION;
        }
        if (status != ER_OK) {
            QCC_LogError(status, ("AddSessionRef(this=%s, %u, %s%s) failed", destEp->GetUniqueName().c_str(),
                                  id, destB2bEp->IsValid() ? "" : "opts, ", destB2bEp->GetUniqueName().c_str()));
        }
    }
    /*
     * srcB2bEp is only NULL when srcEP is non-virtual
     */
    if ((status == ER_OK) && srcB2bEp) {
        assert(srcEp->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL);
        QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): AddSessionRef(id=%d., optsHint, srcB2bEp=\"%s\")", id, (*srcB2bEp)->GetUniqueName().c_str()));
        status = VirtualEndpoint::cast(srcEp)->AddSessionRef(id, *srcB2bEp);
        if (status != ER_OK) {
            assert(destEp->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL);
            QCC_LogError(status, ("AddSessionRef(this=%s, %u, %s) failed", srcEp->GetUniqueName().c_str(), id, (*srcB2bEp)->GetUniqueName().c_str()));
            VirtualEndpoint::cast(destEp)->RemoveSessionRef(id);
            /* Need to hit NameTable here since name ownership of a destEp alias may have changed */
            nameTable.UpdateVirtualAliases(destEp->GetUniqueName());
        }
    }

    /* Set sessionId on B2B endpoints */
    if (status == ER_OK) {
        if (srcB2bEp) {
            QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): SetSessionId(%d.) on srcB2bEp \"%s\")", id, (*srcB2bEp)->GetUniqueName().c_str()));
            (*srcB2bEp)->SetSessionId(id);
        }
        QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): SetSessionId(%d.) on destB2bEp \"%s\")", id, destB2bEp->GetUniqueName().c_str()));
        destB2bEp->SetSessionId(id);
    }

    /* Add sessionCast entries */
    if (status == ER_OK) {
        sessionCastSetLock.Lock(MUTEX_CONTEXT);
        SessionCastEntry entry(id, srcEp->GetUniqueName(), destB2bEp, destEp);
        QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): sessionCastSet.insert(%d., \"%s\", \"%s\", \"%s\")", id, srcEp->GetUniqueName().c_str(),
                       destB2bEp->GetUniqueName().c_str(), destEp->GetUniqueName().c_str()));
        sessionCastSet.insert(entry);
        if (srcEp != destEp) { /* Prevent double entry for self-join */
            if (srcB2bEp) {
                QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): sessionCastSet.insert(%d., \"%s\", \"%s\", \"%s\")", id, destEp->GetUniqueName().c_str(),
                               (*srcB2bEp)->GetUniqueName().c_str(), srcEp->GetUniqueName().c_str()));
                sessionCastSet.insert(SessionCastEntry(id, destEp->GetUniqueName(), *srcB2bEp, srcEp));
            } else {
                RemoteEndpoint none;
                QCC_DbgPrintf(("DaemonRouter::AddSessionRoute(): sessionCastSet.insert(%d., \"%s\", \"%s\", \"%s\")", id, destEp->GetUniqueName().c_str(),
                               srcB2bEp ? (*srcB2bEp)->GetUniqueName().c_str() : "none", srcEp->GetUniqueName().c_str()));
                sessionCastSet.insert(SessionCastEntry(id, destEp->GetUniqueName(), none, srcEp));
            }
        }
        sessionCastSetLock.Unlock(MUTEX_CONTEXT);
    }
    return status;
}

void DaemonRouter::RemoveSelfJoinSessionRoute(const char* src, SessionId id)
{
    QCC_DbgTrace(("DaemonRouter::RemoveSelfJoinSessionRoute(\"%s\", %d.)", src, id));
    String srcStr = src;
    BusEndpoint ep = FindEndpoint(srcStr);

    sessionCastSetLock.Lock(MUTEX_CONTEXT);
    set<SessionCastEntry>::const_iterator it = sessionCastSet.begin();
    for (; it != sessionCastSet.end(); ++it) {
        if ((it->id == id) && (it->src == src) && (it->destEp == ep)) {
            sessionCastSet.erase(it);
            break;
        }
    }
    sessionCastSetLock.Unlock(MUTEX_CONTEXT);
}



void DaemonRouter::RemoveSessionRoutes(const char* src, SessionId id)
{
    QCC_DbgTrace(("DaemonRouter::RemoveSessionRoutes(\"%s\", %d.)", src, id));
    String srcStr = src;
    BusEndpoint ep = FindEndpoint(srcStr);

    sessionCastSetLock.Lock(MUTEX_CONTEXT);
    set<SessionCastEntry>::const_iterator it = sessionCastSet.begin();
    while (it != sessionCastSet.end()) {
        if (((it->id == id) || (id == 0)) && ((it->src == src) || (it->destEp == ep))) {
            SessionCastEntry entry = *it;
            sessionCastSet.erase(it);
            sessionCastSetLock.Unlock();
            if ((entry.id != 0) && (entry.destEp->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL)) {
                VirtualEndpoint::cast(entry.destEp)->RemoveSessionRef(entry.id);
                /* Need to hit NameTable here since name ownership of a destEp alias may have changed */
                nameTable.UpdateVirtualAliases(entry.destEp->GetUniqueName());
            }
            sessionCastSetLock.Lock();
            it = sessionCastSet.upper_bound(entry);
        } else {
            ++it;
        }
    }
    sessionCastSetLock.Unlock(MUTEX_CONTEXT);
}

}

