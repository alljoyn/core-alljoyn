/**
 * @file ICEStream.cc
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
#include <qcc/String.h>
#include <Component.h>
#include <ICESession.h>
#include "RendezvousServerInterface.h"

using namespace qcc;

/** @internal */
#define QCC_MODULE "ICESTREAM"

/** Uncomment this macro to enforce the selection of the relay candidate */
//#define ENFORCE_RELAY_CANDIDATE_SELECTION

namespace ajn {

#ifndef NDEBUG
void ICEStream::DumpChecklist(void)
{
    checkListIterator it;
    int count = 0;
    for (it = CheckListBegin(); it != CheckListEnd(); ++it, ++count) {

        QCC_DbgPrintf(("Pair %d: local %s:%d (%s) remote %s:%d (%s)", count,
                       (*it)->local->GetEndpoint().addr.ToString().c_str(),
                       (*it)->local->GetEndpoint().port,
                       (*it)->local->GetTypeString().c_str(),
                       (*it)->remote->GetEndpoint().addr.ToString().c_str(),
                       (*it)->remote->GetEndpoint().port,
                       (*it)->remote->GetTypeString().c_str()));
    }
}
#endif

ICEStream::~ICEStream(void)
{
    // Enter holding lock...

    CancelChecks();

    // Empty checkList
    while (!checkList.empty()) {
        ICECandidatePair* pair = checkList.back();
        delete pair;
        checkList.pop_back();
    }

    remoteCandidateList.clear();

    // Empty componentList
    while (!componentList.empty()) {
        Component* component = componentList.back();
        GetSession()->Unlock();
        delete component;
        GetSession()->Lock();
        componentList.pop_back();
    }
}


void ICEStream::CancelChecks(void)
{
    // Enter holding lock...

    terminating = true;

    // Wait on dispatcher thread
    if (NULL != checkListDispatcherThread &&
        Thread::GetThread() != checkListDispatcherThread) {

        // Ensure that it terminates
        checkListDispatcherThread->Stop();
        session->Unlock();
        checkListDispatcherThread->Join();
        session->Lock();

        delete checkListDispatcherThread;
        checkListDispatcherThread = NULL;
    }

    // In case we are asked to restart checks...
    checkListState = CheckStateInitial;
}

void ICEStream::AddRemoteCandidate(const ICECandidate& remoteCandidate)
{
    remoteCandidateList.push_back(remoteCandidate);
}

QStatus ICEStream::AddComponent(AddressFamily af, SocketType socketType,
                                Component*& component,
                                Component*& implicitComponent)

{
    QStatus status = ER_OK;

    QCC_DbgTrace(("ICEStream::AddComponent(af = %d, "
                  "socketType = %d, "
                  "*&component = <>, "
                  "*&implicitComponent = <>)",
                  af, socketType));


    component = NULL;
    implicitComponent = NULL;

    // must be first element 'inserted' into vector
    component = new Component(this, COMPONENT_ID_RTP, "RTP/AVP", af, STUNInfo, hmacKey, hmacKeyLen);
    componentList.push_back(component);

    return status;
}

void ICEStream::RemoveComponent(Component* component)
{
    iterator it = std::find(componentList.begin(), componentList.end(), component);
    if (it != componentList.end()) {
        componentList.erase(it);
    }
}


bool compareCandidatePairsByPriority(ICECandidatePair*first, ICECandidatePair*second)
{
    return first->GetPriority() > second->GetPriority();
}


// Section 5.7.3 draft-ietf-mmusic-ice-19
void ICEStream::SortAndPruneCandidatePairs(void)
{
    // Sort the candidate pairs by priority
    checkList.sort(compareCandidatePairsByPriority);

#ifndef NDEBUG
    QCC_DbgPrintf(("")); // make output line up nicely (Test harness forgets to leave us at newline.)
    QCC_DbgPrintf(("Before pruning stream %d...", streamNumber));
    DumpChecklist();
#endif

    // Prune the sorted list. Remove a pair if its local and remote
    // candidates are identical to those of a higher priority pair.
    // With our implementation, 'local' implies if server-reflexive, use its base.

    list<ICECandidatePair*> tempList;
    list<ICECandidatePair*>::iterator iter;

    tempList = checkList;   // work out of a temp list so as not to corrupt iterator

    ICECandidatePair* prev = NULL;

    for (iter = tempList.begin(); iter != tempList.end(); ++iter) {

#ifdef ENFORCE_RELAY_CANDIDATE_SELECTION
        if (((*iter)->local->GetType() != _ICECandidate::Relayed_Candidate) && ((*iter)->remote->GetType() != _ICECandidate::Relayed_Candidate)) {
            checkList.remove(*iter);
            continue;
        }
#endif

        if (prev == NULL) {
            prev = *iter;
            continue;
        }

        IPEndpoint prevLocalEndpoint = prev->local->GetEndpoint();
        IPEndpoint currLocalEndpoint = (*iter)->local->GetEndpoint();

        if (prev->local->GetType() == _ICECandidate::ServerReflexive_Candidate) {
            prevLocalEndpoint = prev->local->GetBase();
        }

        if ((*iter)->local->GetType() == _ICECandidate::ServerReflexive_Candidate) {
            currLocalEndpoint = (*iter)->local->GetBase();
        }

        if ((prevLocalEndpoint == currLocalEndpoint) &&
            (prev->remote->GetEndpoint() == (*iter)->remote->GetEndpoint())) {

            // This is guaranteed to be the lower priority candidate
            delete (*iter);
            checkList.remove(*iter);        // Note we are playing with real list here, not our temp.
        } else {
            prev = *iter;
        }
    }

#ifndef NDEBUG
    QCC_DbgPrintf(("After pruning stream %d...", streamNumber));
    DumpChecklist();
#endif

    // Limit the total number of checks across all check lists, by
    // impartially limiting each check list.
    uint16_t agentLimit = 100;
    uint16_t streamLimit = agentLimit / session->GetICEStreamCount();

    // Remove lowest priority pairs.
    while (checkList.size() > streamLimit) {
        delete checkList.back();
        checkList.pop_back();
    }
}


// Section 5.7.4 draft-ietf-mmusic-ice-19
bool compareByFoundationCompIDPriority(ICECandidatePair* first, ICECandidatePair* second)
{
    bool higher = false;

    String firstFoundation;
    String secondFoundation;
    first->GetFoundation(firstFoundation);
    second->GetFoundation(secondFoundation);

    // Arrange list so all identical foundations are adjacent, order doesn't matter
    if (firstFoundation < secondFoundation) {
        higher = true;
    } else if (firstFoundation == secondFoundation) {
        // We want components sorted descending
        if (first->local->GetComponent()->GetID() > second->local->GetComponent()->GetID()) {
            higher = true;
        } else if (first->local->GetComponent()->GetID() == second->local->GetComponent()->GetID()) {
            // We want priorities sorted ascending
            higher = first->GetPriority() < second->GetPriority();
        }
    }

    return higher;
}


// Section 5.7.4 draft-ietf-mmusic-ice-19

// Set state to 'Waiting' for each check list pair with a unique foundation.
// For all check list pairs with same foundation, set the state of the pair with
// lowest componentID to Waiting.  If more than one, use the one with highest priority.
QStatus ICEStream::ActivateCheckList(void)
{
    /* If the pruned check list is empty, then we have nothing to do.*/
    if (CheckListEmpty()) {
        return ER_FAIL;
    }

    SetPairsWaiting();

    return StartCheckListDispatcher();
}


// Section 7.1.2.2.3 (last bullet) draft-ietf-mmusic-ice-19

// Set state to 'Waiting' for each check list pair with a unique foundation.
// For all check list pairs with same foundation, set the state of the pair with
// lowest componentID to Waiting.  If more than one, use the one with highest priority.
void ICEStream::SetPairsWaiting(void)
{
    list<ICECandidatePair*> composedList;
    list<ICECandidatePair*>::iterator current;

    // Compose a temporary list and sort it to assist determining unique pairs.
    // (Note that our real checklist remains sorted by priority.)
    composedList = checkList;
    composedList.sort(compareByFoundationCompIDPriority);

    ICECandidatePair* prev = NULL;
    // Go thru list, doing pair-wise comparison ('previous' to 'current'). If different, set state
    // of 'previous'. This implies that we cannot set state of last element until we run past end of list.
    for (current = composedList.begin(); current != composedList.end(); ++current) {
        if (prev == NULL) {
            prev = *current;
            continue;
        }

        if (prev->GetFoundation() != (*current)->GetFoundation()) {
            // Because using references, this sets state of pair in real list
            prev->state = ICECandidatePair::Waiting;
        }
        prev = *current;
    }

    if (prev != NULL) {
        // Because using references, this sets state of pair in real list
        prev->state = ICECandidatePair::Waiting;
    }
}



void ICEStream::AddCandidatePair(ICECandidatePair* checkPair)
{
    checkList.push_back(checkPair);
}


void ICEStream::AddCandidatePairByPriority(ICECandidatePair* checkPair)
{
    AddCandidatePair(checkPair);

    // Sort the candidate pairs by priority
    checkList.sort(compareCandidatePairsByPriority);
}




void ICEStream::GetAllReadyTriggeredCheckPairs(list<ICECandidatePair*>& foundList)
{
    checkListIterator it;
    for (it = CheckListBegin(); it != CheckListEnd(); ++it) {
        if ((*it)->IsTriggered() &&
            (ICECandidatePair::Waiting == (*it)->state ||
             (ICECandidatePair::InProgress == (*it)->state &&
              // See if previous attempt timed out, and retry left
              (*it)->RetryAvailable()))) {
            foundList.push_back(*it);
        }
    }
}


void ICEStream::GetAllReadyOrdinaryCheckPairs(list<ICECandidatePair*>& foundList, bool& noWaitingPairs)
{
    noWaitingPairs = true;
    checkListIterator it;
    for (it = CheckListBegin(); it != CheckListEnd(); ++it) {
        if (ICECandidatePair::Waiting == (*it)->state ||
            ICECandidatePair::InProgress == (*it)->state) {
            noWaitingPairs = false;

            if (!(*it)->IsTriggered() &&
                (ICECandidatePair::Waiting == (*it)->state ||
                 (ICECandidatePair::InProgress == (*it)->state &&
                  // See if previous attempt timed out, and any retry left.
                  (*it)->RetryAvailable()))) {
                foundList.push_back(*it);
            }
        }
    }
}


// descending time since retransmit, descending priority
bool comparePairsByTransmitTimePriority(ICECandidatePair* first, ICECandidatePair* second)
{
    bool greater = false;

    // the smaller the queued time offset, the older (more stale) it is.
    if (first->GetQueuedTimeOffset() < second->GetQueuedTimeOffset()) {
        greater = true;
    } else if (first->GetQueuedTimeOffset() == second->GetQueuedTimeOffset()) {
        greater = (first->GetPriority() > second->GetPriority());
    }

    return greater;
}

// Section 5.8 draft-ietf-mmusic-ice-19
ICECandidatePair* ICEStream::GetNextCheckPair(void)
{
    ICECandidatePair* readyPair = NULL;

    checkListIterator it;
    list<ICECandidatePair*> triggeredList;

    // create list of all triggered pairs ready to transmit (or retransmit)
    GetAllReadyTriggeredCheckPairs(triggeredList);

    if (triggeredList.size() > 0) {

        // sort the list by time waiting to transmit/retransmit
        triggeredList.sort(comparePairsByTransmitTimePriority);

        readyPair = triggeredList.front();
        readyPair = readyPair->IncrementRetryAttempt();
    }


    // If no ready triggered check exists, look for ordinary check.
    if (!readyPair) {
        // Find non-Triggered pair ready to transmit (or retransmit)
        bool noWaitingPairs = true;
        list<ICECandidatePair*> ordinaryList;
        GetAllReadyOrdinaryCheckPairs(ordinaryList, noWaitingPairs);
        if (ordinaryList.size() > 0) {

            ordinaryList.sort(comparePairsByTransmitTimePriority);

            readyPair = ordinaryList.front();
            readyPair = readyPair->IncrementRetryAttempt();
        }

        if (!readyPair && noWaitingPairs) {
            // No Waiting (or InProgress) pairs. See if anything to unfreeze.
            for (it = CheckListBegin(); it != CheckListEnd(); ++it) {
                // List is already sorted.
                if (ICECandidatePair::Frozen == (*it)->state) {
                    readyPair = (*it);
                    break;
                }
            }
        }
    }

    if (readyPair) {
        // Set pair state to InProgress
        readyPair->state = ICECandidatePair::InProgress;
    }

    return readyPair;
}


bool ICEStream::ChecksFinished(void)
{
    bool checksFinished = true;

    if (checkListState != CheckStateFailed) {
        checkListIterator checkListIter;
        for (checkListIter = CheckListBegin(); checkListIter != CheckListEnd(); ++checkListIter) {

            // See if this pair has completed.  If all retries have timed out, it updates state
            if ((*checkListIter)->IsWorkRemaining()) {
                checksFinished = false;
            }
        }
    }

    return checksFinished;
}


void ICEStream::SetTerminate(void)
{
    terminating = true;
}

// Section 5.8 draft-ietf-mmusic-ice-19
void ICEStream::CheckListDispatcher(void)
{
    uint32_t activeCheckListCount;
    uint32_t pacingIntervalMsecs = 500;

    session->Lock();

    // Unless asynchronously told to terminate, see if there is more work
    // to do.  Implicitly process timeouts and notify app if necessary.
    while (!terminating && !ChecksFinished()) {
        // Get next pair from triggered queue (or ordinary list)
        ICECandidatePair* pair = GetNextCheckPair();
        if (pair) {
            // Send pair check.  Any response is handled elsewhere.
            pair->Check();
        }

        activeCheckListCount = session->GetActiveCheckListCount();

        // Pace ourselves
        session->Unlock();
        qcc::Sleep(pacingIntervalMsecs * max(1U, activeCheckListCount));
        //ToDo: 'max' is to accommodate improper semantics
        //of GetActiveCheckListCount
        session->Lock();
    }

    session->Unlock();

    QCC_DbgPrintf(("CheckListDispatcher terminating"));

    // Now CheckListDispatcher thread terminates
}

QStatus ICEStream::StartCheckListDispatcher(void)
{
    QStatus status = ER_OK;

    checkListState = CheckStateRunning;

    checkListDispatcherThread = new Thread("CheckListDispatcherThreadStub", CheckListDispatcherThreadStub);

    // Start the thread which will dispatch ICE pair checkers, at appropriate pace
    terminating = false;

    status = checkListDispatcherThread->Start(this);
    if (ER_OK != status) {
        checkListState = CheckStateFailed;
    }
    return status;
}


// Section 7.1.2.2.3 draft-ietf-mmusic-ice-19
void ICEStream::UnfreezeMatchingPairs(String foundation)
{
    checkListIterator checkListIter;
    for (checkListIter = CheckListBegin(); checkListIter != CheckListEnd(); ++checkListIter) {
        if ((*checkListIter)->state == ICECandidatePair::Frozen &&
            (*checkListIter)->GetFoundation() == foundation) {
            (*checkListIter)->state = ICECandidatePair::Waiting;
        }
    }
}


// Section 7.1.2.2.3 draft-ietf-mmusic-ice-19
void ICEStream::UnfreezeMatchingPairs(ICEStream* stream, Component* component)
{
    checkListIterator checkListIter;
    for (checkListIter = stream->CheckListBegin(); checkListIter != stream->CheckListEnd(); ++checkListIter) {
        if ((*checkListIter)->state == ICECandidatePair::Frozen &&
            component->FoundationMatchesValidPair((*checkListIter)->GetFoundation())) {
            (*checkListIter)->state = ICECandidatePair::Waiting;
        }
    }
}


bool ICEStream::AtLeastOneMatchingPair(ICEStream* stream, Component* component, vector<ICECandidatePair*>& matchingList)
{
    checkListIterator checkListIter;
    for (checkListIter = stream->CheckListBegin(); checkListIter != stream->CheckListEnd(); ++checkListIter) {
        if (component->FoundationMatchesValidPair((*checkListIter)->GetFoundation())) {
            matchingList.push_back((*checkListIter));
        }
    }

    return (!matchingList.empty());
}


// ToDo: Needs work. Users of this function have different expectations
// i.e. "if at least one pair is Waiting "
bool ICEStream::CheckListIsActive(void)
{
    bool isActive = false;

    checkListIterator checkListIter;
    for (checkListIter = CheckListBegin(); checkListIter != CheckListEnd(); ++checkListIter) {
        if ((*checkListIter)->state == ICECandidatePair::InProgress) {
            isActive = true;
            break;
        }
    }

    return isActive;
}


// all pairs frozen
bool ICEStream::CheckListIsFrozen(void)
{
    bool isFrozen = true;

    checkListIterator checkListIter;
    for (checkListIter = CheckListBegin(); checkListIter != CheckListEnd(); ++checkListIter) {
        if ((*checkListIter)->state != ICECandidatePair::Frozen) {
            isFrozen = false;
            break;
        }
    }

    return isFrozen;
}




// Section 7.1.2.2.3 draft-ietf-mmusic-ice-19
void ICEStream::UpdatePairStates(ICECandidatePair* pair)
{
    // See if there is a pair in the valid list for every component
    // of this stream.
    bool haveValidPairs = true;

    const_iterator componentIter;
    for (componentIter = Begin(); componentIter != End(); ++componentIter) {
        // Search valid list for this component.
        if (!(*componentIter)->HasValidPair()) {
            haveValidPairs = false;
            break;
        }
    }

    if (haveValidPairs) {
        // Examine check list for each other stream.
        ICESession::stream_iterator streamIter;
        for (streamIter = session->Begin(); streamIter != session->End(); ++streamIter) {
            if ((*streamIter) != this) {
                if ((*streamIter)->CheckListIsActive()) {
                    UnfreezeMatchingPairs(*streamIter, pair->local->GetComponent());
                } else {
                    // See if there is at least one pair in the check list whose
                    // foundation matches a pair in the valid list under consideration
                    vector<ICECandidatePair*> matchingList;
                    if (AtLeastOneMatchingPair(*streamIter, pair->local->GetComponent(), matchingList)) {
                        // Set state of _all_ matching pairs to Waiting
                        vector<ICECandidatePair*>::iterator matchListIt;
                        for (matchListIt = matchingList.begin(); matchListIt != matchingList.end(); ++matchListIt) {
                            (*matchListIt)->state = ICECandidatePair::Waiting;
                            // Activate this stream's check list
                            session->StartSubsequentCheckList(this);
                        }
                    } else {
                        SetPairsWaiting();
                    }
                }
            }
        }
    }
}


// Section 7.1.2.2.1 draft-ietf-mmusic-ice-19
bool ICEStream::DiscoverPeerReflexive(IPEndpoint& mappedAddress, ICECandidatePair* pair, ICECandidate& peerReflexiveCandidate)
{
    /* Invalidate candidate */
    peerReflexiveCandidate = ICECandidate();

    // Compare against known candidates.
    ICECandidate match;
    Component::const_iterator candidateIt;
    for (candidateIt = pair->local->GetComponent()->Begin(); candidateIt != pair->local->GetComponent()->End(); ++candidateIt) {
        if ((*candidateIt)->GetEndpoint() == mappedAddress) {
            match = *candidateIt;
            break;
        }
    }

    if (match->GetType() != _ICECandidate::Invalid_Candidate) {
        StunActivity* reflexiveCandidateStunActivity =
            new StunActivity(pair->local->GetStunActivity()->stun);

        pair->local->GetComponent()->AddToStunActivityList(reflexiveCandidateStunActivity);

        _ICECandidate::ICECandidateType type = _ICECandidate::PeerReflexive_Candidate;
        Component* component = pair->local->GetComponent();
        IPEndpoint base = pair->local->GetBase();
        SocketType sockType = pair->local->GetTransportProtocol();
        peerReflexiveCandidate = ICECandidate(type,
                                              mappedAddress,
                                              base,
                                              component,
                                              sockType,
                                              reflexiveCandidateStunActivity);
        peerReflexiveCandidate->SetPriority(pair->GetBindRequestPriority());
        String foundation;
        session->DeterminePeerReflexiveFoundation(mappedAddress.addr,
                                                  peerReflexiveCandidate->GetTransportProtocol(),
                                                  foundation);
        peerReflexiveCandidate->SetFoundation(foundation);

        // Add peer-reflexive candidate to our list.
        pair->local->GetComponent()->AddCandidate(peerReflexiveCandidate);
    }

    return (peerReflexiveCandidate->GetType() != _ICECandidate::Invalid_Candidate);
}



// Section 7.1.2 draft-ietf-mmusic-ice-19
void ICEStream::ProcessCheckEvent(ICECandidatePair& requestPair,
                                  ICECandidatePair::CheckStatus status,
                                  IPEndpoint& mappedAddress)
{
    QCC_DbgTrace(("ICEStream::ProcessCheckEvent(status=%s, local=%s:%d (%s), remote=%s:%d (%s)",
                  requestPair.CheckStatusToString(status).c_str(),
                  requestPair.local->GetEndpoint().addr.ToString().c_str(),
                  requestPair.local->GetEndpoint().port,
                  requestPair.local->GetTypeString().c_str(),
                  requestPair.remote->GetEndpoint().addr.ToString().c_str(),
                  requestPair.remote->GetEndpoint().port,
                  requestPair.remote->GetTypeString().c_str()));

    ICECandidate peerReflexiveCandidate;

    switch (status) {
    case ICECandidatePair::CheckRoleConflict:
        // We will try again at next opportunity, with roles reversed,
        // using same tie-breaker.
        session->SwapControllingAgent();

        requestPair.AddTriggered();
        break;

    case ICECandidatePair::CheckSucceeded:
    {
        ICECandidatePair* validPair = &requestPair;

        validPair->local->GetComponent()->AddToValidList(validPair);

        // Section 7.1.2.2.3 draft-ietf-mmusic-ice-19
        // This is ambiguous. Spec says 'pair that generated the check', which implies
        // the original request, not any peer-reflexive that may have just been added.
        requestPair.state = ICECandidatePair::Succeeded;

        UnfreezeMatchingPairs(requestPair.GetFoundation());
        UpdatePairStates(&requestPair);

        // Section 7.1.2.2.4 draft-ietf-mmusic-ice-19
        // This is less ambiguous, as it says "valid pair generated from that check..."
        validPair->UpdateNominatedFlag();

        break;
    }

    case ICECandidatePair::CheckTimeout:
        requestPair.state = ICECandidatePair::Failed;
        break;

    case ICECandidatePair::CheckGenericFailed:
    default:
        requestPair.state = ICECandidatePair::Failed;
        break;
    }

    UpdateCheckListAndTimerState();
}


// Section 7.1.2.3 draft-ietf-mmusic-ice-19
void ICEStream::UpdateCheckListAndTimerState(void)
{
    QCC_DbgTrace(("ICEStream::UpdateCheckListAndTimerState"));

    bool allFailedOrSucceeded = true;

    checkListIterator iter;
    for (iter = CheckListBegin(); iter != CheckListEnd(); ++iter) {
        if ((*iter)->state != ICECandidatePair::Failed &&
            (*iter)->state != ICECandidatePair::Succeeded) {
            QCC_DbgPrintf(("ICEStream::UpdateTimerState: local=%s:%d, remote=%s:%d is in state %d",
                           (*iter)->local->GetEndpoint().addr.ToString().c_str(),
                           (*iter)->local->GetEndpoint().port,
                           (*iter)->remote->GetEndpoint().addr.ToString().c_str(),
                           (*iter)->remote->GetEndpoint().port,
                           (*iter)->state));
            allFailedOrSucceeded = false;
            break;
        }
    }

    if (allFailedOrSucceeded) {
        bool allComponentsHaveValidPair = true;

        const_iterator componentIter;
        for (componentIter = Begin(); componentIter != End(); ++componentIter) {
            if (!(*componentIter)->HasValidPair()) {
                allComponentsHaveValidPair = false;
                break;
            }
        }

        if (!allComponentsHaveValidPair) {
            checkListState = CheckStateFailed;
        }

        // Examine check list for each other stream.
        ICESession::stream_iterator streamIter;
        for (streamIter = session->Begin(); streamIter != session->End(); ++streamIter) {
            if ((*streamIter)->CheckListIsFrozen()) {
                (*streamIter)->SetPairsWaiting();
            }
        }
    }
}


ICECandidatePair* ICEStream::MatchCheckListEndpoint(IPEndpoint& localEndpoint, IPEndpoint& remoteEndpoint)
{
    ICECandidatePair* pair = NULL;

    // Walk check list looking for pair with these endpoints.
    checkListIterator iter;

    for (iter = CheckListBegin(); iter != CheckListEnd(); ++iter) {
        // We should just be matching the IP address for the remote endpoint with the
        // one in our candidate pair remote candidate because if the NAT in between
        // is a symmetric NAT, the port number for server reflexive candidate will be
        // different than what was previously allocated.
        // TODO: Account for the case wherein the IP address can also be different
        // than the allocated one
        if ((*iter)->local->GetEndpoint() == localEndpoint &&
            (*iter)->remote->GetEndpoint().addr == remoteEndpoint.addr) {
            QCC_DbgPrintf(("%s: Matched %s:%d %s:%d  %s:%s",
                           __FUNCTION__,
                           (*iter)->local->GetEndpoint().addr.ToString().c_str(), (*iter)->local->GetEndpoint().port,
                           localEndpoint.addr.ToString().c_str(), localEndpoint.port,
                           (*iter)->remote->GetEndpoint().addr.ToString().c_str(), remoteEndpoint.addr.ToString().c_str()));
            pair = (*iter);
            break;
        }
    }

    return pair;
}

ICECandidatePair* ICEStream::MatchCheckList(IPEndpoint& remoteEndpoint, StunTransactionID& tid)
{
    ICECandidatePair* pair = NULL;

    // Walk check list looking for pair with this transaction ID and remote endpoint.
    checkListIterator iter;

    for (iter = CheckListBegin(); iter != CheckListEnd(); ++iter) {
        if ((tid == (*iter)->GetTransactionID()) || (*iter)->EqualsCanceledTransactionID(tid)) {
            pair = (*iter);
            break;
        }
    }

    return pair;
}


ICECandidate ICEStream::MatchRemoteCandidate(IPEndpoint& source, String& uniqueFoundation)
{
    ICECandidate remoteCandidate;
    uint32_t foundationID = 0;

    // Walk remote candidate list looking for this endpoint.
    constRemoteListIterator iter;
    for (iter = RemoteListBegin(); iter != RemoteListEnd(); ++iter) {
        foundationID = max(foundationID, StringToU32((*iter)->GetFoundation()));
        if ((*iter)->GetEndpoint() == source) {
            remoteCandidate = (*iter);
            break;
        }
    }

    if (remoteCandidate->GetType() == _ICECandidate::Invalid_Candidate) {
        uniqueFoundation = U32ToString((uint32_t)++ foundationID);
    }

    return remoteCandidate;
}


void ICEStream::RemoveWaitFrozenPairsForComponent(Component* component)
{
    checkListIterator checkListIter;
    bool removing = true;
    while (removing) {
        removing = false;
        for (checkListIter = CheckListBegin(); checkListIter != CheckListEnd(); ++checkListIter) {
            if (((*checkListIter)->state == ICECandidatePair::Frozen ||
                 (*checkListIter)->state == ICECandidatePair::Waiting) &&
                component == (*checkListIter)->local->GetComponent()) {
                ICECandidatePair* pair = *checkListIter;
                pair->RemoveTriggered();
                checkList.remove(pair);
                delete (pair);

                // kludge: STL 'remove' confounds iterators apparently
                removing = true;
                break;
            }
        }
    }
}


void ICEStream::CeaseRetransmissions(Component* component, uint64_t lowestPairPriority)
{
    checkListIterator checkListIter;
    bool removing = true;
    while (removing) {
        removing = false;
        for (checkListIter = CheckListBegin(); checkListIter != CheckListEnd(); ++checkListIter) {
            if ((*checkListIter)->state == ICECandidatePair::InProgress &&
                (*checkListIter)->local->GetComponent() == component &&
                (*checkListIter)->GetPriority() < lowestPairPriority) {

                // Implies that if/when the response arrives, it will be ignored.
                ICECandidatePair* pair = *checkListIter;
                pair->RemoveTriggered();
                checkList.remove(pair);
                delete (pair);

                // kludge: STL 'remove' confounds iterators apparently
                removing = true;
                break;
            }
        }
    }
}

} //namespace ajn
