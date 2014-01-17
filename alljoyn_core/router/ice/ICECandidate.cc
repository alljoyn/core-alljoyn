/**
 * @file ICECandidate.cc
 *
 *
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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
#include <qcc/atomic.h>

#include <ICEStream.h>
#include <ICECandidate.h>
#include <StunActivity.h>
#include <Component.h>
#include <StunAttribute.h>
#include <ICESession.h>

#include <map>

using namespace qcc;

/** @internal */
#define QCC_MODULE "ICECANDIDATE"

namespace ajn {

_ICECandidate::_ICECandidate(_ICECandidate::ICECandidateType type,
                             IPEndpoint endPoint,
                             IPEndpoint base,
                             Component* component,
                             SocketType transportProtocol,
                             StunActivity* stunActivity) :
    type(type),
    priority(),
    endPoint(endPoint),
    base(base),
    mappedAddress(),
    grantedAllocationLifetimeSecs(),
    foundation(),
    component(component),
    transportProtocol(transportProtocol),
    stunActivity(stunActivity),
    permissionStunActivity(),
    terminating(false),
    sharedStunRelayedCandidate(NULL),
    sharedStunServerReflexiveCandidate(NULL),
    candidateThread(NULL)
{
    QCC_DbgTrace(("ICECandidate::ICECandidate1(%p, type=%d)", this, type));
    stunActivity->SetCandidate(ICECandidate::wrap(this));
}

_ICECandidate::_ICECandidate(IPEndpoint endPoint,
                             IPEndpoint base,
                             IPEndpoint mappedAddress,
                             uint32_t grantedAllocationLifetimeSecs,
                             Component* component,
                             SocketType transportProtocol,
                             StunActivity* stunActivity,
                             StunActivity* permissionStunActivity) :
    type(_ICECandidate::Relayed_Candidate),
    priority(),
    endPoint(endPoint),
    base(base),
    mappedAddress(mappedAddress),
    grantedAllocationLifetimeSecs(grantedAllocationLifetimeSecs),
    foundation(),
    component(component),
    transportProtocol(transportProtocol),
    stunActivity(stunActivity),
    permissionStunActivity(permissionStunActivity),
    terminating(false),
    sharedStunRelayedCandidate(NULL),
    sharedStunServerReflexiveCandidate(NULL),
    candidateThread(NULL)
{
    QCC_DbgTrace(("ICECandidate::ICECandidate2(%p, relayed)", this));
    stunActivity->SetCandidate(ICECandidate::wrap(this));
    permissionStunActivity->SetCandidate(ICECandidate::wrap(this));
}

_ICECandidate::_ICECandidate(_ICECandidate::ICECandidateType type,
                             IPEndpoint endPoint,
                             Component* component,
                             SocketType transportProtocol,
                             uint32_t priority,
                             String foundation) :
    type(type),
    priority(priority),
    endPoint(endPoint),
    base(),
    mappedAddress(),
    grantedAllocationLifetimeSecs(),
    foundation(foundation),
    component(component),
    transportProtocol(transportProtocol),
    stunActivity(),
    permissionStunActivity(),
    terminating(false),
    sharedStunRelayedCandidate(NULL),
    sharedStunServerReflexiveCandidate(NULL),
    candidateThread(NULL)
{
    QCC_DbgTrace(("ICECandidate::ICECandidate3(%p, type=%d)", this, type));
    if (stunActivity) {
        stunActivity->SetCandidate(ICECandidate::wrap(this));
    }
}

_ICECandidate::_ICECandidate() :
    type(Invalid_Candidate),
    priority(),
    endPoint(),
    base(),
    mappedAddress(),
    grantedAllocationLifetimeSecs(),
    foundation(),
    component(),
    transportProtocol(),
    stunActivity(),
    permissionStunActivity(),
    terminating(false),
    sharedStunRelayedCandidate(NULL),
    sharedStunServerReflexiveCandidate(NULL),
    candidateThread(NULL)
{
    QCC_DbgTrace(("ICECandidate::ICECandidate(%p, INVALID)", this));
}

_ICECandidate::~_ICECandidate(void)
{
    QCC_DbgTrace(("ICECandidate::~ICECandidate(%p)", this));
    terminating = true;

    StopCheckListener();

    if (sharedStunRelayedCandidate) {
        delete sharedStunRelayedCandidate;
    }
    if (sharedStunServerReflexiveCandidate) {
        delete sharedStunServerReflexiveCandidate;
    }
    if (candidateThread) {
        delete candidateThread;
        candidateThread = NULL;
    }
}

QStatus _ICECandidate::StartListener()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(!candidateThread);
    candidateThread = new ICECandidateThread(this);

    // Start the thread which will listen for responses, ICE checks
    return candidateThread->Start();
}

QStatus _ICECandidate::StopCheckListener()
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("ICECandidate::StopCheckListener()"));

    // Notify checkListener thread to quit.
    terminating = true;

    /* Signal the candidateThread to stop and wait for it it die */
    if (candidateThread) {
        candidateThread->Stop();
        candidateThread->Join();
    }

    return status;
}

void _ICECandidate::AwaitRequestsAndResponses()
{
    // This method runs only in the instance of a Host Candidate, but
    // because the host candidate's stun object is shared by reflexive and
    // relayed candidates for a component, we need to be prepared
    // to receive a message for any of these.
    assert(NULL != stunActivity);
    assert(NULL != stunActivity->stun);

    // This timer provides an upper limit on the time to shutdown.
    // Decreasing it makes shutdown more responsive, at the expense
    // of busy loop polling. It has nothing to do with
    // retransmit timeouts, pacing, etc.
    uint32_t worstCaseShudownTimeoutMsec = 5000;
    bool signaledStop = false;

    QStatus status = ER_OK;
    Thread* thisThread = Thread::GetThread();
    while (!terminating && !signaledStop && !thisThread->IsStopping()) {

        ICESession* session = component->GetICEStream()->GetSession();
        if (session) {
            session->Lock();
            uint64_t startTime = GetTimestamp64();
            while ((session->GetState() == ICESession::ICECandidatesGathered) &&
                   (session->ChecksStarted() == false) &&
                   (!thisThread->IsStopping()) &&
                   (GetTimestamp64() < (startTime + 10000))) {
                session->Unlock();
                qcc::Sleep(100);
                session->Lock();
            }
            session->Unlock();
        }

        // block until receive data is ready, or timeout
        switch (status = ReadReceivedMessage(worstCaseShudownTimeoutMsec)) {
        case ER_TIMEOUT:
            // Timer has expired. Go around for another try.
            break;

        case ER_OK:
            // Message was processed. Go around for next one.
            break;

        case ER_STOPPING_THREAD:
            signaledStop = true;
            break;

        default:
            QCC_LogError(status, ("ReadReceivedMessage"));
            signaledStop = true;
            break;
        }
    }

    QCC_DbgPrintf(("AwaitCheckRequestsAndResponses terminating"));
    // Thread terminates.
}

// Section 7.2.1 draft-ietf-mmusic-ice-19
QStatus _ICECandidate::ReadReceivedMessage(uint32_t timeoutMsec)
{
    // This method runs in the instance of a Host Candidate, but
    // because the host candidate's stun object is shared by server
    // reflexive and relayed candidates for a component, we need to be prepared
    // to receive a message for any of these.

    QStatus status = ER_OK;
    ICECandidatePair::CheckStatus checkStatus = ICECandidatePair::CheckGenericFailed;   // Assume the worst.
    String username;

    IPEndpoint mappedAddress;
    IPEndpoint remote;
    bool receivedMsgWasRelayed;

    StunMessage::const_iterator stunAttrIt;
    StunTransactionID requestTransID;
    bool ICEcontrollingRequest = false;
    bool useCandidateRequest = false;
    //uint64_t controlTieBreaker = 0;
    //uint32_t requestPriority = 0;
    StunTransactionID tid;

    ICESession* session = component->GetICEStream()->GetSession();
    session->Lock();

    StunMessage msg(session->GetRemoteInitiatedCheckUsername(),
                    session->GetRemoteInitiatedCheckHmacKey(),
                    session->GetRemoteInitiatedCheckHmacKeyLength());

    session->Unlock();

    status = stunActivity->stun->RecvStunMessage(msg, remote.addr, remote.port, receivedMsgWasRelayed, timeoutMsec);

    if (status != ER_OK) {
        if (status != ER_STOPPING_THREAD) {
            QCC_LogError(status, ("ReadReceivedMessage failed (ICECandidate=%p)", this));
        }
        return status;
    }
#if !defined(NDEBUG)
    QCC_DbgPrintf(("ReadRxMsg status=%d, class=%s,  base %s:%d %s from %s:%d",
                   status, msg.MessageClassToString(msg.GetTypeClass()).c_str(),
                   base.addr.ToString().c_str(), base.port,
                   receivedMsgWasRelayed ? "relayed" : "",
                   remote.addr.ToString().c_str(), remote.port));
#endif
    component->GetICEStream()->GetSession()->Lock();

    // ToDo: If the message is a request and does not contain both a MESSAGE-INTEGRITY and a
    // USERNAME attribute: reject the request with an error response.  This response MUST use an error code
    // of 400 (Bad Request).

    //ToDo: (RFC 5389 10.1.2) If the message is a request and fails MESSAGE-INTEGRITY
    // the server MUST reject the request with error response 401 (Unauthorized).

    msg.GetTransactionID(tid);

    // We don't know if this is a request or response yet, but assume it is a response
    // and try to match up transaction in list of checks that we have sent.
    CheckRetry* checkRetry = component->GetCheckRetryByTransaction(tid);
    if (NULL != checkRetry) {
        if (msg.GetTypeClass() == STUN_MSG_RESPONSE_CLASS) {
            QCC_DbgPrintf(("TID: %s, Check Response matches", tid.ToString().c_str()));
        } else {
            QCC_DbgPrintf(("TID: %s, Expected STUN Response to check but got %s with matching tid instead", tid.ToString().c_str(), msg.MessageClassToString(msg.GetTypeClass()).c_str()));
            checkRetry = NULL;
        }
    }

    // We don't know if this is a request or response yet, but assume it is a response
    // and try to match up transaction in list of non-checks (Allocate refresh, etc) that we have sent.
    Retransmit* retransmit = component->GetRetransmitByTransaction(tid);
    if (NULL != retransmit) {
        if ((msg.GetTypeClass() == STUN_MSG_RESPONSE_CLASS) || (msg.GetTypeClass() == STUN_MSG_ERROR_CLASS)) {
            QCC_DbgPrintf(("TID: %s, Found matching NonCheck %s", tid.ToString().c_str(), msg.MessageClassToString(msg.GetTypeClass()).c_str()));
        } else {
            QCC_DbgPrintf(("TID: %s, Expected STUN Response to nonCheck but got %s with matching tid instead", tid.ToString().c_str(), msg.MessageClassToString(msg.GetTypeClass()).c_str()));
            retransmit = NULL;
        }
    }

    if (NULL == checkRetry && NULL == retransmit) {
        QCC_DbgPrintf(("TID: %s, Unknown %s", tid.ToString().c_str(), msg.MessageClassToString(msg.GetTypeClass()).c_str()));
    }

    ICECandidate relayedCandidate;
    IPEndpoint reflexive;
    IPEndpoint relayed;
    uint32_t grantedAllocationLifetimeSecs = 0;

    // iterate thru message looking for attributes
    for (stunAttrIt = msg.Begin(); stunAttrIt != msg.End(); ++stunAttrIt) {
        switch ((*stunAttrIt)->GetType()) {
        case STUN_ATTR_XOR_MAPPED_ADDRESS: {
                const StunAttributeXorMappedAddress& sa = *reinterpret_cast<StunAttributeXorMappedAddress*>(*stunAttrIt);

                IPEndpoint base;
                stunActivity->stun->GetLocalAddress(base.addr, base.port);

                // Should only appear in a response to our earlier (outbound) check.
                // To later determine peer-reflexive candidate...
                sa.GetAddress(mappedAddress.addr, mappedAddress.port);

                sa.GetAddress(reflexive.addr, reflexive.port);

                // Set the local Server reflexive candidate in the associated STUN object. We don't
                // care if the returned Server reflexive candidate is same as the local host
                // candidate for this setting
                stunActivity->stun->SetLocalSrflxCandidate(reflexive);

                if (ICESession::ICEGatheringCandidates ==
                    component->GetICEStream()->GetSession()->GetState()) {
                    if (relayedCandidate->GetType() == _ICECandidate::Relayed_Candidate) {
                        relayedCandidate->SetMappedAddress(reflexive);
                    }

                    // Discard if reflexive is identical to host
                    if (base.addr != reflexive.addr) {
                        StunActivity* reflexiveCandidateStunActivity =
                            new StunActivity(stunActivity->stun);

                        stunActivity->stun->GetComponent()->AddToStunActivityList(
                            reflexiveCandidateStunActivity);

                        _ICECandidate::ICECandidateType type = _ICECandidate::ServerReflexive_Candidate;
                        SocketType sockType = stunActivity->stun->GetSocketType();
                        Component* component = stunActivity->stun->GetComponent();
                        ICECandidate reflexiveCandidate = ICECandidate(type,
                                                                       reflexive,
                                                                       base,
                                                                       component,
                                                                       sockType,
                                                                       reflexiveCandidateStunActivity);

                        // store server_reflexive candidate (reuse host candidate's stun object)
                        stunActivity->stun->GetComponent()->AddCandidate(reflexiveCandidate);
                        if (sharedStunServerReflexiveCandidate) {
                            delete sharedStunServerReflexiveCandidate;
                        }
                        sharedStunServerReflexiveCandidate = new ICECandidate(reflexiveCandidate);
                    }
                }
                // cease retries
                if (NULL != retransmit) {
                    retransmit->SetState(Retransmit::ReceivedSuccessResponse);
                }
                break;
            }

        case STUN_ATTR_XOR_PEER_ADDRESS:
            {
                break;
            }

        case STUN_ATTR_ALLOCATED_XOR_SERVER_REFLEXIVE_ADDRESS:
            {
                break;
            }

        case STUN_ATTR_XOR_RELAYED_ADDRESS: {
                const StunAttributeXorRelayedAddress& sa = *reinterpret_cast<StunAttributeXorRelayedAddress*>(*stunAttrIt);

                IPEndpoint host;
                stunActivity->stun->GetLocalAddress(host.addr, host.port);
                sa.GetAddress(relayed.addr, relayed.port);

                if (ICESession::ICEGatheringCandidates ==
                    component->GetICEStream()->GetSession()->GetState()) {
                    // Discard if relayed is identical to host
                    if (relayed.addr != host.addr) {
                        // (Reuse host candidate's stun object.)

                        //  Maintain count of retries and timeouts as we perform Allocate
                        // _refresh_ requests to the TURN server for this relayed candidate.
                        StunActivity* relayedCandidateStunActivity = new StunActivity(stunActivity->stun);

                        stunActivity->stun->GetComponent()->AddToStunActivityList(
                            relayedCandidateStunActivity);

                        //  Maintain count of retries and timeouts as we perform Permission
                        // _refresh_ requests to the TURN server for this relayed candidate.
                        StunActivity* permissionStunActivity = new StunActivity(stunActivity->stun);

                        stunActivity->stun->GetComponent()->AddToStunActivityList(
                            permissionStunActivity);

                        SocketType sockType = stunActivity->stun->GetSocketType();
                        Component* component = stunActivity->stun->GetComponent();
                        relayedCandidate = ICECandidate(relayed,
                                                        relayed,
                                                        reflexive,
                                                        grantedAllocationLifetimeSecs,
                                                        component,
                                                        sockType,
                                                        relayedCandidateStunActivity,
                                                        permissionStunActivity);

                        // store relayed candidate
                        stunActivity->stun->GetComponent()->AddCandidate(relayedCandidate);

                        // Set the relay IP and port in the STUN object
                        stunActivity->stun->SetTurnAddr(relayed.addr);
                        stunActivity->stun->SetTurnPort(relayed.port);
                        QCC_DbgPrintf(("Setting Relay address %s and port %d in STUN object", relayed.addr.ToString().c_str(), relayed.port));

                        if (sharedStunRelayedCandidate) {
                            delete sharedStunRelayedCandidate;
                        }

                        sharedStunRelayedCandidate = new ICECandidate(relayedCandidate); // to demux received check messages later
                    }
                }
                // cease retries
                if (NULL != retransmit) {
                    retransmit->SetState(Retransmit::ReceivedSuccessResponse);
                }
                break;
            }

        case STUN_ATTR_LIFETIME: {
                const StunAttributeLifetime& sa = *reinterpret_cast<StunAttributeLifetime*>(*stunAttrIt);
                grantedAllocationLifetimeSecs = sa.GetLifetime();
                if (relayedCandidate->GetType() == _ICECandidate::Relayed_Candidate) {
                    relayedCandidate->SetAllocationLifetimeSeconds(grantedAllocationLifetimeSecs);
                }

                break;
            }

        case STUN_ATTR_PRIORITY: {
                //const StunAttributePriority& sa = *reinterpret_cast<StunAttributePriority*>(*stunAttrIt);
                //requestPriority = sa.GetPriority();
                break;
            }

        case STUN_ATTR_USE_CANDIDATE:
            useCandidateRequest = true;
            break;

        case STUN_ATTR_ICE_CONTROLLING: {
                //const StunAttributeIceControlling& sa = *reinterpret_cast<StunAttributeIceControlling*>(*stunAttrIt);
                ICEcontrollingRequest = true;
                //controlTieBreaker = sa.GetValue();
                break;
            }

        case STUN_ATTR_ICE_CONTROLLED: {
                //const StunAttributeIceControlled& sa = *reinterpret_cast<StunAttributeIceControlled*>(*stunAttrIt);
                ICEcontrollingRequest = false;
                //controlTieBreaker = sa.GetValue();
                break;
            }


        case STUN_ATTR_ERROR_CODE: {
                const StunAttributeErrorCode& sa = *reinterpret_cast<StunAttributeErrorCode*>(*stunAttrIt);
                StunErrorCodes error;
                String reason;
                sa.GetError(error, reason);

                if (NULL != retransmit) {
                    retransmit->SetState(Retransmit::ReceivedErrorResponse);

                    switch (error) {
                    case STUN_ERR_CODE_UNAUTHORIZED:
                        // Handle this special case of STUN 'error' by retrying with credentials
                        retransmit->SetState(Retransmit::ReceivedAuthenticateResponse);
                        retransmit->SetErrorCode(ER_STUN_AUTH_CHALLENGE);
                        break;

                    case STUN_ERR_CODE_INSUFFICIENT_CAPACITY:
                        retransmit->SetErrorCode(ER_ICE_ALLOCATE_REJECTED_NO_RESOURCES);
                        //todo notify app
                        break;

                    case STUN_ERR_CODE_ALLOCATION_QUOTA_REACHED:
                        retransmit->SetErrorCode(ER_ICE_ALLOCATION_QUOTA_REACHED);
                        break;

                    case STUN_ERR_CODE_ALLOCATION_MISMATCH:
                        retransmit->SetErrorCode(ER_ICE_ALLOCATION_MISMATCH);
                        break;

                    case STUN_ERR_CODE_ROLE_CONFLICT:
                        // Handle this special case of STUN 'error' by retrying with reversed role
                        checkStatus = ICECandidatePair::CheckRoleConflict;
                        break;

                    default:
                        status = ER_ICE_STUN_ERROR;
                        checkStatus = ICECandidatePair::CheckGenericFailed;
                        retransmit->SetErrorCode(status); //ToDo make these unique!!!
                        break;
                    }
                }

                break;
            }

        case STUN_ATTR_USERNAME: {
                const StunAttributeUsername& sa = *reinterpret_cast<StunAttributeUsername*>(*stunAttrIt);
                sa.GetUsername(username);
                break;
            }

        default:
            break;
        }
    }

    switch (msg.GetTypeClass()) {
    case STUN_MSG_ERROR_CLASS:
        // ToDo report which errors?
        // component->GetICEStream()->GetSession()->UpdateICEStreamStates();
        break;

    case STUN_MSG_INDICATION_CLASS:
        // ignore
        QCC_DbgPrintf(("Unexpected STUN_MSG_INDICATION_CLASS"));
        break;

    case STUN_MSG_REQUEST_CLASS:
        {
            if (msg.GetTypeMethod() != STUN_MSG_BINDING_METHOD) {
                goto exit;
            }

            if (!component->GetICEStream()->GetSession()->ChecksStarted()) {
                // We haven't received peer's candidates yet via offer/answer.
                // Simply respond with no further state change.
                SendResponse(checkStatus, remote, receivedMsgWasRelayed, tid);
                goto exit;
            }

            if (component->GetICEStream()->GetSession()->GetRemoteInitiatedCheckUsername() != username) {
                // Username fragment does not match.

                // ToDo: the server MUST reject the request
                // with an error response.  This response MUST use an error code
                // of 401 (Unauthorized).
                goto exit;
            }

            // Section 7.2.1.1 draft-ietf-mmusic-ice-19
            checkStatus = ICECandidatePair::CheckResponseSent; // assume everything is ok

            if (component->GetICEStream()->GetSession()->IsControllingAgent()) {
                if (ICEcontrollingRequest) {
/* ToDo: determine tieBreaker by matching intended pair (Need to know if this is a Relayed request.)
                if (constructedPair->GetControlTieBreaker() < controlTieBreaker) {
                    // switch to controlled role
                    component->GetICEStream()->GetSession()->SwapControllingAgent();
                }
                else {
                    checkStatus = ICECandidatePair::CheckRoleConflict;
                }
 */
                }
            } else { // agent is in controlled role
                if (!ICEcontrollingRequest) { //ICE-CONTROLLED present in request
/* ToDo: determine tieBreaker by matching intended pair (Need to know if this is a Relayed request.)
                if ( constructedPair->GetControlTieBreaker() < controlTieBreaker) {
                    checkStatus = ICECandidatePair::CheckRoleConflict;
                }
                else {
                    // switch to controlled role
                    component->GetICEStream()->GetSession()->SwapControllingAgent();
                }
 */
                }
            }

            status = SendResponse(checkStatus, remote, receivedMsgWasRelayed, tid);

            if (ER_OK == status &&
                checkStatus == ICECandidatePair::CheckResponseSent) {

                // Section 7.2.1.4 draft-ietf-mmusic-ice-19
                // 'Construct' a pair, meaning find a pair whose local candidate is equal to
                // the transport address on which the STUN request was received, and a
                // remote candidate equal to the source transport address where the
                // request came from (which may be peer-reflexive remote candidate that was just learned).
                // Recall that this Stun object may be shared by multiple local
                // candidates (host, server-reflexive, relayed,) each belonging to perhaps multiple candidate pairs.
                ICECandidatePair* constructedPair = NULL;
                if (receivedMsgWasRelayed && sharedStunRelayedCandidate) {
                    QCC_DbgPrintf(("%s: receivedMsgWasRelayed && sharedStunRelayedCandidate", __FUNCTION__));
                    constructedPair = component->GetICEStream()->MatchCheckListEndpoint((*sharedStunRelayedCandidate)->endPoint, remote);
                } else {
                    QCC_DbgPrintf(("%s: !(receivedMsgWasRelayed && sharedStunRelayedCandidate)", __FUNCTION__));
                    constructedPair = component->GetICEStream()->MatchCheckListEndpoint(endPoint, remote);
                    if ((NULL == constructedPair) && sharedStunServerReflexiveCandidate) {
                        QCC_DbgPrintf(("%s: ((NULL == constructedPair) && sharedStunServerReflexiveCandidate)", __FUNCTION__));
                        constructedPair = component->GetICEStream()->MatchCheckListEndpoint((*sharedStunServerReflexiveCandidate)->endPoint, remote);
                    }
                }
                if (!constructedPair) {
                    break;
                } else {
                    // Pair is on check list
                    switch (constructedPair->state) {
                    case ICECandidatePair::Waiting:
                        constructedPair->AddTriggered();
                        break;

                    case ICECandidatePair::Frozen:
                        constructedPair->AddTriggered(); // ToDo: spec doesn't explicitly say to, but
                        // we change to Waiting here. Correct?
                        break;

                    case ICECandidatePair::InProgress:
                        // Cancel the in-progress transaction and any remaining retransmits,
                        // and trigger a new check.
                        constructedPair->SetCanceled();
                        constructedPair->AddTriggered();
                        break;

                    case ICECandidatePair::Failed:
                        constructedPair->AddTriggered();
                        break;

                    case ICECandidatePair::Succeeded:
                        // do nothing
                        break;
                    }
                }

                // Section 7.2.1.5 draft-ietf-mmusic-ice-19
                if (useCandidateRequest && !component->GetICEStream()->GetSession()->IsControllingAgent()) {
                    if (constructedPair->state == ICECandidatePair::Succeeded) {
                        constructedPair->SetNominated();
                        QCC_DbgPrintf(("SetNominated (CONTROLLED) local %s:%d remote %s:%d",
                                       constructedPair->local->endPoint.addr.ToString().c_str(),
                                       constructedPair->local->endPoint.port,
                                       constructedPair->remote->endPoint.addr.ToString().c_str(),
                                       constructedPair->remote->endPoint.port));

                    } else if (constructedPair->state == ICECandidatePair::InProgress ||
                               constructedPair->state == ICECandidatePair::Waiting) { //EqualsCanceledTransactionID??
                        constructedPair->SetNominatedContingent();
                        QCC_DbgPrintf(("SetNominatedContingent (CONTROLLED) local %s:%d remote %s:%d",
                                       constructedPair->local->endPoint.addr.ToString().c_str(),
                                       constructedPair->local->endPoint.port,
                                       constructedPair->remote->endPoint.addr.ToString().c_str(),
                                       constructedPair->remote->endPoint.port));
                    }
                }

                component->GetICEStream()->GetSession()->UpdateICEStreamStates();
            }

            break;
        }

    case STUN_MSG_RESPONSE_CLASS:
        {
            // Is this an Allocate/CreatePermission/refresh response?
            if (NULL != retransmit) {
                if (retransmit->GetState() == Retransmit::AwaitingResponse) {
                    retransmit->SetState(Retransmit::ReceivedSuccessResponse);
                    QCC_DbgPrintf(("ReceivedSuccessResponse"));
                }
            } else {
                // Check if this is a response to our request.
                ICECandidatePair* intendedPair = component->GetICEStream()->MatchCheckList(remote, tid);
                if (intendedPair) {
                    if ((ICECandidatePair::InProgress == intendedPair->state) ||
                        ((ICECandidatePair::Waiting == intendedPair->state) && // canceled?
                         intendedPair->EqualsCanceledTransactionID(tid))) {

                        QCC_DbgPrintf(("CheckSucceeded"));

                        // Notify the stream object
                        component->GetICEStream()->ProcessCheckEvent(*intendedPair,
                                                                     ICECandidatePair::CheckSucceeded, mappedAddress);
                    }

                    component->GetICEStream()->GetSession()->UpdateICEStreamStates();
                }
            }
            break;
        }

    default:
        break;
    }


exit:
    component->GetICEStream()->GetSession()->Unlock();

    return status;
}

IPAddress _ICECandidate::GetServer(void) const
{
    return stunActivity->stun->GetSTUNServerInfo().address;
}

String _ICECandidate::GetTURNUserName(void) const
{
    return stunActivity->stun->GetSTUNServerInfo().acct;
}

QStatus _ICECandidate::SendResponse(uint16_t checkStatus, IPEndpoint& dest,
                                    bool usingTurn, StunTransactionID tid)
{
    QStatus status = ER_OK;
    StunMessage*msg = NULL;

    if (checkStatus == ICECandidatePair::CheckRoleConflict) {
        msg = new StunMessage(STUN_MSG_ERROR_CLASS,
                              STUN_MSG_BINDING_METHOD,
                              component->GetICEStream()->GetSession()->GetRemoteInitiatedCheckHmacKey(),
                              component->GetICEStream()->GetSession()->GetRemoteInitiatedCheckHmacKeyLength(), tid);
        msg->AddAttribute(new StunAttributeErrorCode(STUN_ERR_CODE_ROLE_CONFLICT, "Role Conflict"));
    } else {
        msg = new StunMessage(STUN_MSG_RESPONSE_CLASS,
                              STUN_MSG_BINDING_METHOD,
                              component->GetICEStream()->GetSession()->GetRemoteInitiatedCheckHmacKey(),
                              component->GetICEStream()->GetSession()->GetRemoteInitiatedCheckHmacKeyLength(), tid);
    }

    QCC_DbgPrintf(("Send Response: class %s, TID %s dest %s:%d",
                   msg->MessageClassToString(msg->GetTypeClass()).c_str(),
                   tid.ToString().c_str(),
                   dest.addr.ToString().c_str(), dest.port));

    /*
     * We don't need to include the XOR_MAPPED_ADDRESS attribute in binding responses as this
     * attribute is not used in any way by either the Server or the daemon. This attribute
     * may be required if the support for peer reflexive candidates are enabled.
     */
    //msg->AddAttribute(new StunAttributeXorMappedAddress(*msg, dest.addr, dest.port));
    msg->AddAttribute(new StunAttributeRequestedTransport(REQUESTED_TRANSPORT_TYPE_UDP));
    msg->AddAttribute(new StunAttributeMessageIntegrity(*msg));
    msg->AddAttribute(new StunAttributeFingerprint(*msg));

    // send our response
    status = stunActivity->stun->SendStunMessage(*msg, dest.addr, dest.port, usingTurn);

    delete msg;

    return status;
}

String _ICECandidate::GetPriorityString(void) const
{
    return U32ToString((uint32_t)priority, 10);
}

String _ICECandidate::GetTypeString(void) const
{
    String s;

    switch (type) {
    case Host_Candidate:
        s = "host";
        break;

    case ServerReflexive_Candidate:
        s = "srflx";
        break;

    case Relayed_Candidate:
        s = "relay";
        break;

    case PeerReflexive_Candidate:
        s = "prflx";
        break;

    default:
        s = "unk";
        break;
    }

    return s;
}

} //namespace ajn
