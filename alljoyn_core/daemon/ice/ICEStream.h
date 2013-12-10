#ifndef _ICESTREAM_H
#define _ICESTREAM_H
/**
 * @file ICEStream.h
 *
 * ICEStream contains the state for a single media stream
 * for a session.  (Each session contains one or more streams)
 * A stream contains one or more components. e.g. RTP and RTCP
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

#include <list>
#include <queue>
#include <qcc/IPAddress.h>
#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include "ICECandidatePair.h"
#include <alljoyn/Status.h>
#include "RendezvousServerInterface.h"

using namespace qcc;

/** @internal */
#define QCC_MODULE "ICESTREAM"

using namespace std;

namespace ajn {

// Forward Declaration
class ICESession;

class ICEStream {
  public:

    /** ICE checks state for stream */
    typedef enum {
        CheckStateInitial,         /**< Checks have not yet begun.      */
        CheckStateRunning,         /**< Checks are still in progress.   */
        CheckStateCompleted,       /**< Checks have produced nominated pair(s) for
                                   **  each component of the stream. ICE has
                                   **  succeeded and media can be sent. */
        CheckStateFailed           /**< Checks have finished unsuccessfully. */
    } ICEStreamCheckListState;

    /** RTP 'RR and RS' Bandwidth values */
    typedef enum {
        Unspecified,
        BothAreZero,
        NotBothAreZero
    } BandwidthSpecifierType;   /**< Bandwidth specifier for RTP stream */


    ICEStream(uint16_t streamNumber, ICESession* session, STUNServerInfo stunInfo, const uint8_t* key, size_t keyLen, BandwidthSpecifierType bwSpec = Unspecified) :
        streamNumber(streamNumber),
        componentList(),
        session(session),
        bandwidthSpecifier(bwSpec),
        checkListState(CheckStateInitial),
        checkList(),
        checkListDispatcherThread(NULL),
        terminating(false),
        STUNInfo(stunInfo),
        hmacKey(key),
        hmacKeyLen(keyLen)
    { }


    ~ICEStream(void);

    QStatus AddComponent(AddressFamily af, SocketType socketType,
                         Component*& component,
                         Component*& implicitComponent);

    void RemoveComponent(Component* component);

    /// const_iterator typedef.
    typedef vector<Component*>::const_iterator const_iterator;
    typedef vector<Component*>::iterator iterator;

    const_iterator Begin(void) const { return componentList.begin(); }
    const_iterator End(void) const { return componentList.end(); }

    uint16_t GetICEStreamNumber(void) { return streamNumber; }

    ICESession* GetSession(void) const { return session; }

    void SortAndPruneCandidatePairs(void);

    QStatus ActivateCheckList(void);

    void AddCandidatePair(ICECandidatePair* checkPair);

    void AddCandidatePairByPriority(ICECandidatePair* checkPair);

    void AddRemoteCandidate(const ICECandidate& remoteCandidate);

    void ProcessCheckEvent(ICECandidatePair& requestPair,
                           ICECandidatePair::CheckStatus status,
                           IPEndpoint& mappedAddress);

    /*A check list with at least one pair that is Waiting is
       called an active check list, and a check list with all pairs frozen
       is called a frozen check list.
     */
    bool CheckListIsActive(void);

    bool CheckListIsFrozen(void);

    void CancelChecks(void);

    ICECandidate MatchRemoteCandidate(IPEndpoint& source, String& uniqueFoundation);

    ICECandidatePair* MatchCheckList(IPEndpoint& remoteEndpoint, StunTransactionID& tid);

    ICECandidatePair* MatchCheckListEndpoint(IPEndpoint& localBase, IPEndpoint& remoteEndpoint);

    ICEStreamCheckListState GetCheckListState(void) const { return checkListState; }

    void SetCheckListState(ICEStreamCheckListState state) { checkListState = state; }

    void RemoveWaitFrozenPairsForComponent(Component* component);

    void CeaseRetransmissions(Component* component, uint64_t lowestPairPriority);

    void SetTerminate(void);

    /// const_iterator typedef.
    typedef list<ICECandidate>::const_iterator constRemoteListIterator;
    constRemoteListIterator RemoteListBegin(void) const { return remoteCandidateList.begin(); }
    constRemoteListIterator RemoteListEnd(void) const { return remoteCandidateList.end(); }

    typedef list<ICECandidatePair*>::iterator checkListIterator;

    checkListIterator CheckListBegin(void) { return checkList.begin(); }
    checkListIterator CheckListEnd(void) { return checkList.end(); }
    bool CheckListEmpty(void) { return checkList.empty(); };

  private:

    bool ChecksFinished(void);

    void UpdateCheckListAndTimerState(void);

    QStatus StartCheckListDispatcher(void);

    void CheckListDispatcher(void);

    // Each active check list will have a thread with this
    // function as its entry point.
    static ThreadReturn STDCALL CheckListDispatcherThreadStub(void* pThis)
    {
        ICEStream* thisPtr = (ICEStream*)pThis;
        thisPtr->CheckListDispatcher();

        return 0;
    }

    ICECandidatePair* GetNextCheckPair(void);

    void UpdatePairStates(ICECandidatePair* pair);

    bool DiscoverPeerReflexive(IPEndpoint& mappedAddress, ICECandidatePair* pair, ICECandidate& peerReflexiveCandidate);

    void UnfreezeMatchingPairs(String foundation);

    void UnfreezeMatchingPairs(ICEStream* stream, Component* component);

    bool AtLeastOneMatchingPair(ICEStream* stream, Component* component, vector<ICECandidatePair*>& matchingList);

    void SetPairsWaiting(void);

    void GetAllReadyTriggeredCheckPairs(list<ICECandidatePair*>& foundList);

    void GetAllReadyOrdinaryCheckPairs(list<ICECandidatePair*>& foundList, bool& noWaitingPairs);

#ifndef NDEBUG
    void DumpChecklist(void);
#endif

    uint16_t streamNumber;

    vector<Component*> componentList;

    ICESession* session;

    BandwidthSpecifierType bandwidthSpecifier;

    ICEStreamCheckListState checkListState;

    list<ICECandidatePair*> checkList;

    Thread* checkListDispatcherThread;

    bool terminating;

    Mutex lock;

    list<ICECandidate> remoteCandidateList;

    typedef list<ICECandidate>::iterator remoteListIterator;

    STUNServerInfo STUNInfo;

    const uint8_t* hmacKey;

    size_t hmacKeyLen;
};

} //namespace ajn

#undef QCC_MODULE

#endif
