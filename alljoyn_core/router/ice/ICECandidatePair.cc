/**
 * @file ICECandidatePair.cc
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

#include <ICEStream.h>
#include <qcc/Config.h>
#include <ICECandidatePair.h>
#include <Component.h>
#include <StunAttribute.h>
#include <ICESession.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "ICECANDIDATEPAIR"

namespace ajn {

ICECandidatePair::ICECandidatePair(const ICECandidate& local, const ICECandidate& remote, bool isDefault, uint64_t priority) :
    local(local),
    remote(remote),
    state(Frozen),
    isValid(false),
    checkRetry(NULL),
    canceledRetry(NULL),
    priority(priority),
    isDefault(isDefault),
    isNominated(false),
    isNominatedContingent(false),
    foundation(),
    useAggressiveNomination(),
    regularlyNominated(),
    controlTieBreaker(),
    bindRequestPriority(),
    isTriggered()
{
    QCC_DbgTrace(("%s(%p): ", __FUNCTION__, this));

    // the colon disambiguates "12" + "345" from "123" + "45"
    foundation = local->GetFoundation() + ":" + remote->GetFoundation();
}

ICECandidatePair::~ICECandidatePair(void)
{
    QCC_DbgTrace(("%s(%p): ", __FUNCTION__, this));
    if (checkRetry) {
        delete checkRetry;
        checkRetry = NULL;
    }

    if (canceledRetry) {
        delete canceledRetry;
        canceledRetry = NULL;
    }

    // ICECandidate objects ('local' and 'remote') will be reclaimed from the Component dtor.
}

/* Just defined to make klocwork happy. Should never be used */
ICECandidatePair::ICECandidatePair(const ICECandidatePair& other) :
    local(other.local),
    remote(other.remote),
    state(Frozen),
    isValid(false),
    checkRetry(NULL),
    canceledRetry(NULL),
    priority(other.priority),
    isDefault(other.isDefault),
    isNominated(false),
    isNominatedContingent(false),
    foundation(other.foundation),
    useAggressiveNomination(other.useAggressiveNomination),
    regularlyNominated(other.regularlyNominated),
    controlTieBreaker(other.controlTieBreaker),
    bindRequestPriority(other.bindRequestPriority),
    isTriggered(other.isTriggered)
{
    /* This constructor should never be invoked */
    assert(false);
}

/* Just defined to make klocwork happy. Should neve r be used */
ICECandidatePair& ICECandidatePair::operator=(const ICECandidatePair& other) {
    /* This operator should never be invoked */
    assert(false);

    if (this != &other) {
        state = Frozen;
        isValid = false;
        checkRetry = NULL;
        canceledRetry = NULL;
        priority = other.priority;
        isDefault = other.isDefault;
        isNominated = false;
        isNominatedContingent = false;
        foundation = other.foundation;
        useAggressiveNomination = other.useAggressiveNomination;
        regularlyNominated = other.regularlyNominated;
        controlTieBreaker = other.controlTieBreaker;
        bindRequestPriority = other.bindRequestPriority;
        isTriggered = other.isTriggered;
    }

    return *this;
}


QStatus ICECandidatePair::InitChecker(ICECandidatePair& originalPair)
{
    return InitChecker(originalPair.controlTieBreaker,
                       originalPair.useAggressiveNomination,
                       originalPair.bindRequestPriority);
}


void ICECandidatePair::SetCanceled(void)
{
    // save TransactionID in case a latent response comes in.

    if (canceledRetry) {
        delete canceledRetry;
    }

    canceledRetry = checkRetry->Duplicate();
}


QStatus ICECandidatePair::InitChecker(const uint64_t& controlTieBreaker,
                                      bool useAggressiveNomination, const uint32_t& bindRequestPriority)
{
    QCC_DbgTrace(("%s(%p): checkRetry=%p", __FUNCTION__, this, checkRetry));
    QStatus status = ER_OK;

    this->useAggressiveNomination = useAggressiveNomination;
    this->controlTieBreaker = controlTieBreaker;
    this->bindRequestPriority = bindRequestPriority;

    if (checkRetry) {
        delete checkRetry;
    }
    checkRetry = new CheckRetry();

    if (canceledRetry) {
        delete canceledRetry;
        canceledRetry = NULL;
    }

    return status;
}


bool ICECandidatePair::IsWorkRemaining(void)
{
    bool isWorkRemaining = false;

    if (state != Failed && state != Succeeded) {
        if (checkRetry->AnyRetriesNotSent()) {
            isWorkRemaining = true;
        } else {
            // No more retries. See if last one has timed out yet.
            if (checkRetry->RetryTimedOut()) {
                // Notify the stream object
                IPEndpoint dummy;
                local->GetComponent()->GetICEStream()->ProcessCheckEvent(*this, CheckTimeout, dummy);
                local->GetComponent()->GetICEStream()->GetSession()->UpdateICEStreamStates();
            } else {
                // Last retry sent, but we need to wait around to process
                // the timeout.
                isWorkRemaining = true;
            }
        }
    }

    return isWorkRemaining;
}


ICECandidatePair* ICECandidatePair::IncrementRetryAttempt(void)
{
    // If we've run out of retries, return NULL.
    return (checkRetry->IncrementAttempts() ? this : NULL);
}

// Section 7.1.2.2.4 draft-ietf-mmusic-ice-19
void ICECandidatePair::UpdateNominatedFlag(void)
{
    QCC_DbgPrintf(("ICECandidatePair::UpdateNominatedFlag: IsControlling=%d, useAggressive=%d, regularlyNominated=%d",
                   local->GetComponent()->GetICEStream()->GetSession()->IsControllingAgent(),
                   useAggressiveNomination, regularlyNominated));

    if (local->GetComponent()->GetICEStream()->GetSession()->IsControllingAgent()) {
        if (useAggressiveNomination || regularlyNominated) {
            SetNominated();
            QCC_DbgPrintf(("SetNominated (CONTROLLING) local %s:%d remote %s:%d",
                           local->GetEndpoint().addr.ToString().c_str(),
                           local->GetEndpoint().port,
                           remote->GetEndpoint().addr.ToString().c_str(),
                           remote->GetEndpoint().port));
        } else {
            // Section 8.1.1.1 draft-ietf-mmusic-ice-19
            // Our criteria for stopping checks is to choose
            // first valid pair. So we repeat the check that just succeeded,
            // this time with the USE-CANDIDATE attribute.
            AddTriggered();
            regularlyNominated = true;
        }
    } else {
        // Section 7.2.1.5 draft-ietf-mmusic-ice-19
        if (isNominatedContingent) {
            SetNominated();
            QCC_DbgPrintf(("SetNominated (CONTROLLED) local %s:%d remote %s:%d",
                           local->GetEndpoint().addr.ToString().c_str(),
                           local->GetEndpoint().port,
                           remote->GetEndpoint().addr.ToString().c_str(),
                           remote->GetEndpoint().port));
        }

    }
}


// Section 7.1.1 draft-ietf-mmusic-ice-19
// send the STUN request
void ICECandidatePair::Check(void)
{
    StunTransactionID tid;
    StunMessage* msg;

    QCC_DbgTrace(("ICECandidatePair::Check: [local=%s:%d (%s)] [remote=%s:%d (%s)] priority=%ld",
                  local->GetEndpoint().addr.ToString().c_str(),
                  local->GetEndpoint().port,
                  local->GetTypeString().c_str(),
                  remote->GetEndpoint().addr.ToString().c_str(),
                  remote->GetEndpoint().port,
                  remote->GetTypeString().c_str(),
                  priority));

    if (!checkRetry->GetTransactionID(tid)) {
        // New transaction
        msg = new StunMessage(STUN_MSG_REQUEST_CLASS,
                              STUN_MSG_BINDING_METHOD,
                              local->GetComponent()->GetICEStream()->GetSession()->GetLocalInitiatedCheckHmacKey(),
                              local->GetComponent()->GetICEStream()->GetSession()->GetLocalInitiatedCheckHmacKeyLength());
        msg->GetTransactionID(tid);
        checkRetry->SetTransactionID(tid);
    } else {
        msg = new StunMessage(STUN_MSG_REQUEST_CLASS,
                              STUN_MSG_BINDING_METHOD,
                              local->GetComponent()->GetICEStream()->GetSession()->GetLocalInitiatedCheckHmacKey(),
                              local->GetComponent()->GetICEStream()->GetSession()->GetLocalInitiatedCheckHmacKeyLength(),
                              tid);
    }

    QCC_DbgPrintf(("SndChk TID %s from %s:%d remote %s:%d",
                   tid.ToString().c_str(),
                   local->GetEndpoint().addr.ToString().c_str(),
                   local->GetEndpoint().port,
                   remote->GetEndpoint().addr.ToString().c_str(),
                   remote->GetEndpoint().port));

    msg->AddAttribute(new StunAttributeUsername(local->GetComponent()->GetICEStream()->GetSession()->GetLocalInitiatedCheckUsername()));
    msg->AddAttribute(new StunAttributePriority(bindRequestPriority));

    if (local->GetComponent()->GetICEStream()->GetSession()->IsControllingAgent()) {
        msg->AddAttribute(new StunAttributeIceControlling(controlTieBreaker));

        if (useAggressiveNomination || regularlyNominated) {
            msg->AddAttribute(new StunAttributeUseCandidate());
        }
    } else {
        msg->AddAttribute(new StunAttributeIceControlled(controlTieBreaker));
    }

    msg->AddAttribute(new StunAttributeRequestedTransport(REQUESTED_TRANSPORT_TYPE_UDP));
    msg->AddAttribute(new StunAttributeMessageIntegrity(*msg));
    msg->AddAttribute(new StunAttributeFingerprint(*msg));

    // immediately send our request (without enqueuing, because we have already paced ourselves
    // via the check dispatcher thread.)

    if (remote->GetType() == _ICECandidate::Relayed_Candidate) {
        local->GetStunActivity()->stun->SetTurnAddr(remote->GetEndpoint().addr);
        local->GetStunActivity()->stun->SetTurnPort(remote->GetEndpoint().port);
        local->GetStunActivity()->stun->SendStunMessage(*msg,
                                                        remote->GetMappedAddress().addr,
                                                        remote->GetMappedAddress().port,
                                                        true);
    } else {
        local->GetStunActivity()->stun->SendStunMessage(*msg,
                                                        remote->GetEndpoint().addr,
                                                        remote->GetEndpoint().port,
                                                        local->GetType() == _ICECandidate::Relayed_Candidate);
    }

    delete msg;

}


void ICECandidatePair::AddTriggered(void)
{
    QCC_DbgTrace(("%s(%p): checkRetry=%p", __FUNCTION__, this, checkRetry));

    QCC_DbgPrintf(("AddTriggered: isTriggered (current) =%d, state (current) =%d", isTriggered, state));

    isTriggered = true;
    state = Waiting;

    checkRetry->Init();
}


void ICECandidatePair::SetNominated(void)
{
    isNominated = true;

    local->GetComponent()->SetSelectedIfHigherPriority(this);
}



String ICECandidatePair::CheckStatusToString(const CheckStatus status) const
{
    switch (status) {
    case CheckSucceeded:
        return String("CheckSucceeded");

    case CheckTimeout:
        return String("CheckTimeout");

    case CheckRoleConflict:
        return String("CheckRoleConflict");

    case CheckInconsistentTransactionID:
        return String("CheckInconsistentTransactionID");

    case CheckGenericFailed:
        return String("CheckGenericFailed");

    case CheckUnknownPair:
        return String("CheckUnknownPair");

    case CheckResponseSent:
        return String("CheckResponseSent");

    default:
        return String("<Unknown>");
    }
}


CheckRetry* ICECandidatePair::GetCheckRetryByTransaction(StunTransactionID& tid)
{
    CheckRetry* retransmit = NULL;

    if (NULL != checkRetry &&
        checkRetry->IsTransactionValid() && tid == checkRetry->GetTransactionID()) {
        retransmit = checkRetry;
    } else if (NULL != canceledRetry &&
               canceledRetry->IsTransactionValid() && tid == canceledRetry->GetTransactionID()) {
        retransmit = canceledRetry;
    }

    return retransmit;
}

} //namespace ajn
