/**
 * @file Component.cc
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

#include <qcc/Config.h>
#include <Stun.h>
#include <ICECandidate.h>
#include <StunActivity.h>
#include <Component.h>
#include <StunCredential.h>
#include <ICEStream.h>
#include "RendezvousServerInterface.h"

#define QCC_MODULE "COMPONENT"

namespace ajn {

Component::~Component(void)
{
    /* Disable the ICEComponents' threads that are managed by this component */
    list<ICECandidate>::iterator it = candidateList.begin();
    while (it != candidateList.end()) {
        (*it)->StopCheckListener();
        ++it;
    }

    EmptyActivityList();
}

void Component::EmptyActivityList(void)
{
    while (!stunActivityList.empty()) {
        StunActivity* stunActivity = stunActivityList.front();

        // Because a stun object can be shared among candidates,
        // we only delete it once - here, from the Host_Candidate that allocated it.
        if (_ICECandidate::Host_Candidate == stunActivity->candidate->GetType()) {

            if (stunActivity->stun) {
                delete stunActivity->stun;
                stunActivity->stun = NULL;
            }
        }

        delete stunActivity;
        stunActivityList.pop_front();
    }
}


QStatus Component::AddCandidate(const ICECandidate& candidate)
{
    QStatus status = ER_OK;

    candidateList.push_back(candidate);

    return status;
}


QStatus Component::RemoveCandidate(ICECandidate& candidate)
{
    QStatus status = ER_FAIL;

    iterator iter;
    for (iter = candidateList.begin(); iter != candidateList.end(); ++iter) {
        if (candidate == (*iter)) {
            candidateList.erase(iter);
            status = ER_OK;
            break;
        }
    }

    return status;
}


void Component::AddToStunActivityList(StunActivity* stunActivity)
{
    stunActivityList.push_back(stunActivity);
}


QStatus Component::CreateHostCandidate(qcc::SocketType socketType, const qcc::IPAddress& addr, uint16_t port, size_t mtu)
{
    QStatus status = ER_OK;
    QCC_DbgTrace(("Component::CreateHostCandidate(socketType = %d, &addr = %s, port = %d, mtu = %d)", socketType, addr.ToString().c_str(), port, mtu));

    this->socketType = socketType;

    port = 0;

    Stun* stun = NULL;
    status = AddStun(addr, port, stun, mtu);

    if (ER_OK == status) {
        qcc::IPEndpoint host;
        host.addr = addr;
        host.port = port;

        StunActivity* stunActivity = new StunActivity(stun);
        AddToStunActivityList(stunActivity);

        _ICECandidate::ICECandidateType type = _ICECandidate::Host_Candidate;
        Component* pComponent = this;
        ICECandidate candidate(type, host, host, pComponent, socketType, stunActivity);
        status = AddCandidate(candidate);

        if (ER_OK == status) {
            candidate->StartListener();
        }
    }

    return status;
}


void Component::AssignDefaultCandidate(const ICECandidate& candidate)
{
    if (candidate->GetType() > defaultCandidate->GetType()) {
        defaultCandidate = candidate;
    }
}


QStatus Component::AddStun(const qcc::IPAddress& address, uint16_t& port, Stun*& stun, size_t mtu)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("Component::AddStun(&address = %s, &port = %d, *&stun = <>)", address.ToString().c_str(), port));

    stun = new Stun(socketType, this, STUNInfo, hmacKey, hmacKeyLen, mtu);

    status = stun->OpenSocket(af);
    if (ER_OK == status) {
        status = stun->Bind(address, port);
        if (ER_OK == status) {
            // see what port we were assigned
            qcc::IPAddress ignored;
            status = stun->GetLocalAddress(ignored, port);
        }
    }

    if (ER_OK == status) {
        QCC_DbgPrintf(("Add Stun: %s:%d", address.ToString().c_str(), port));
    } else {
        delete stun;
    }
    return status;
}

void Component::GetStunTurnServerAddress(qcc::String& address) const
{
    address = STUNInfo.address.ToString();
}


uint16_t Component::GetStunTurnServerPort(void) const
{
    uint16_t port = 0;

    port = STUNInfo.port;

    return port;
}


void Component::AddToValidList(ICECandidatePair* validPair)
{
    QCC_DbgPrintf(("AddToValidList isValid(current): %s, hasValidPair(current): %s, [local addr = %s port = %d], [remote addr = %s port = %d]",
                   (validPair->isValid ? "true" : "false"),
                   (hasValidPair ? "true" : "false"),
                   validPair->local->GetEndpoint().addr.ToString().c_str(),
                   validPair->local->GetEndpoint().port,
                   validPair->remote->GetEndpoint().addr.ToString().c_str(),
                   validPair->remote->GetEndpoint().port));

    validPair->isValid = true;

    // ensure exactly one instance
    validList.remove(validPair);
    validList.push_back(validPair);

    hasValidPair = true;
}

bool Component::FoundationMatchesValidPair(const qcc::String& foundation)
{
    bool match = false;
    const_iterator_validList iter;

    for (iter = validList.begin(); iter != validList.end(); ++iter) {
        if ((*iter)->GetFoundation() == foundation) {
            match = true;
            break;
        }
    }

    return match;
}


const uint8_t* Component::GetHmacKey(void) const {
    return hmacKey;
}

const size_t Component::GetHmacKeyLength(void) const {
    return hmacKeyLen;
}


void Component::SetSelectedIfHigherPriority(ICECandidatePair* pair)
{
    if (NULL == selectedPair ||
        (pair->GetPriority() > selectedPair->GetPriority())) {
        selectedPair = pair;
    }
}


QStatus Component::GetSelectedCandidatePair(ICECandidatePair*& pair)
{
    QStatus status = ER_OK;

    if (stream->GetCheckListState() != ICEStream::CheckStateCompleted) {
        status = ER_ICE_CHECKS_INCOMPLETE;
        pair = NULL;
    } else {
        // By definition, this is the highest priority nominated pair from the valid list;
        pair = selectedPair;
    }

    return status;
}


Retransmit* Component::GetRetransmitByTransaction(const StunTransactionID& tid) const
{
    Retransmit* retransmit = NULL;

    // iterate stunActivityList looking for activity
    list<StunActivity*>::const_iterator it;
    for (it = stunActivityList.begin(); it != stunActivityList.end(); ++it) {
        StunTransactionID transaction;
        if ((*it)->retransmit.GetTransactionID(transaction) &&
            transaction == tid) {
            retransmit = &(*it)->retransmit;
            break;
        }
    }

    return retransmit;
}


CheckRetry* Component::GetCheckRetryByTransaction(StunTransactionID tid) const
{
    CheckRetry* retransmit = NULL;

    // iterate checkLists looking for activity
    ICEStream::checkListIterator checkListIter;
    for (checkListIter = stream->CheckListBegin(); checkListIter != stream->CheckListEnd(); ++checkListIter) {
        if (NULL != (retransmit = (*checkListIter)->GetCheckRetryByTransaction(tid))) {
            break;
        }
    }

    return retransmit;
}

} //namespace ajn
