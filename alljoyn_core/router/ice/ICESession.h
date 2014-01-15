#ifndef _ICESESSION_H
#define _ICESESSION_H

/**
 * @file ICESession.h
 *
 *  Implements ICE per draft-ietf-mmusic-ice-19 (no IETF RFC as of
 *  this writing.)
 *
 *      - This is Full-Implementation (not Lite.)
 *      - This inter operates only with an ICE-aware (Full Implementation) peer.
 *      - ICE Restart is not explicitly supported in this layer. App layer can effect
 *        Restart by another call to AllocateSession.
 *      - does not inter operate with SIP (at least not directly)
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

#include <vector>
#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include <alljoyn/Status.h>
#include "ICECandidate.h"
#include "ICECandidatePair.h"
#include "ICESessionListener.h"
#include "Component.h"
#include "ICEStream.h"
#include "StunRetry.h"
#include "ICEManager.h"
#include "RendezvousServerInterface.h"
#include "StunCredential.h"
#include "NetworkInterface.h"

using namespace qcc;

//* For friend
class ICESession;

/** @internal */
#define QCC_MODULE "ICESESSION"

namespace ajn {

void Tokenize(const String& str,
              vector<String>& tokens,
              const char* delimiters = " ");


struct StunWork {
    StunWork(Stun* stun, StunMessage* msg, IPEndpoint& destination)
        : stun(stun), msg(msg), destination(destination) { }

    Stun* stun;
    StunMessage* msg;
    IPEndpoint destination;
};

// per draft-ietf-mmusic-ice-19 Section 15.4  (Our encoding allows for 8 bits
// of randomness per character)
static const int ICE_CREDENTIAL_UFRAG_CHAR_LENGTH = (24 / 8);
static const int ICE_CREDENTIAL_PWD_CHAR_LENGTH = (128 / 8);

// per http://tools.ietf.org/html/draft-ietf-behave-turn-16, Section 2.3
/*PPN - Review duration*/
static const uint32_t TURN_PERMISSION_REFRESH_PERIOD_SECS = 300;

// Refresh a little before the expiration.
static const uint32_t TURN_REFRESH_WARNING_PERIOD_SECS = 15;

// Interval at which to send the NAT keepalives
static const uint32_t STUN_KEEP_ALIVE_INTERVAL_IN_MILLISECS = 15000;

const uint8_t REQUESTED_TRANSPORT_TYPE_UDP = 17;
const uint8_t REQUESTED_TRANSPORT_TYPE_TCP = 6;

/**
 * ICESession contains the state for a single ICE session.
 * The session may contain one or more media streams (each of which may have several components.)
 */
class ICESession {
  public: ~ICESession(void);

    /** ICESession states */
    typedef enum {
        ICEUninitialized,
        ICEGatheringCandidates,
        ICECandidatesGathered,
        ICEChecksRunning,
        ICEChecksSucceeded,
        ICEProcessingFailed
    } ICESessionState;   /**< State of this ICESession */

    /**
     * Execute ICE Connection checks.
     *
     * Begin ICE connection check processing between the given local ICESession (CONTROLLING AGENT)
     * and the remote peer (CONTROLLED AGENT)
     *
     * This method may also be called to restart ICE connection checks for a currently connected session.
     *
     * @param peerCandidates            Remote ICE candidates to be checked.
     *
     * @param ice_frag                  Remote frag
     *
     * @param useAggressiveNomination   selects Aggressive vs Regular nomination
     *
     * @param ice_pwd                  Remote pwd
     *
     *
     * @return ER_OK if successful.
     */
    QStatus StartChecks(list<ICECandidates>& peerCandidates, bool useAggressiveNomination, String ice_frag, String ice_pwd);


    /**
     * Execute ICE Connection checks.
     *
     * Begin ICE connection check processing between the given local ICESession (CONTROLLED AGENT)
     * and the remote peer (CONTROLLING AGENT)
     *
     * This method may also be called to restart ICE connection checks for a currently connected session.
     *
     * @param peerCandidates  Remote ICE candidates to be checked.
     *
     * @return ER_OK if successful.
     */
    QStatus StartChecks(list<ICECandidates>& peerCandidates, String ice_frag, String ice_pwd);

    /**
     * Cancel ICE Connection checks.
     *
     * Cancel any in-progress connection checks for a session.
     *
     * @return   OI_OK if successful.
     */
    QStatus CancelChecks();

    ICESessionState GetState(void);

    QStatus GetErrorCode(void);

    QStatus GetLocalICECandidates(list<ICECandidates>& iceCandidates, String& session_ufrag, String& session_pwd);

    void GetSelectedCandidatePairList(vector<ICECandidatePair*>& selectedCandidatePairList);

    uint16_t GetActiveCheckListCount(void);

    void DeterminePeerReflexiveFoundation(IPAddress addr,
                                          SocketType transportProtocol,
                                          String& foundation);

    typedef vector<ICEStream*>::iterator stream_iterator;
    stream_iterator Begin(void) { return streamList.begin(); }
    stream_iterator End(void) { return streamList.end(); }

    void StartSubsequentCheckList(ICEStream* stream);

    bool ChecksStarted(void) const { return checksStarted; }

    String GetLocalInitiatedCheckUsername(void) const { return localInitiatedCheckUsername; }

    String GetRemoteInitiatedCheckUsername(void) const { return remoteInitiatedCheckUsername; }

    uint8_t* GetLocalInitiatedCheckHmacKey(void) const;

    size_t GetLocalInitiatedCheckHmacKeyLength(void) const;

    size_t GetRemoteInitiatedCheckHmacKeyLength(void) const;

    uint8_t* GetRemoteInitiatedCheckHmacKey(void) const;

    void Lock(void) { lock.Lock(); }

    void Unlock(void) { lock.Unlock(); }

    uint64_t ComputePairPriority(bool isControllingAgent, uint32_t localPriority, uint32_t remotePriority);

    bool IsControllingAgent(void) const { return isControllingAgent; }

    void SwapControllingAgent(void);

    void UpdateICEStreamStates(void);

    uint16_t GetICEStreamCount(void) const { return streamList.size(); }

    uint32_t GetTURNRefreshPeriod(void) { return((TURN_PERMISSION_REFRESH_PERIOD_SECS - TURN_REFRESH_WARNING_PERIOD_SECS) * 1000); };

    uint32_t GetSTUNKeepAlivePeriod(void) { return STUN_KEEP_ALIVE_INTERVAL_IN_MILLISECS; };

    friend class ICEManager;

    void StopPacingThreadAndClearStunQueue(void);

    String GetusernameForShortTermCredential() { return usernameForShortTermCredential; };

    /**
     * Return the address of the relay server
     */
    IPAddress GetRelayServerAddr() { return TurnServer.addr; };

    /**
     * Return the port of the relay server
     */
    uint16_t GetRelayServerPort() { return TurnServer.port; };

  private:

    /* Just defined to make klocwork happy. Should never be used */
    ICESession(const ICESession& other) {
        assert(false);
    }

    /* Just defined to make klocwork happy. Should never be used */
    ICESession& operator=(const ICESession& other) {
        assert(false);
        return *this;
    }

    uint8_t* hmacKey;

    size_t hmacKeyLen;

    IPEndpoint StunServer;

    bool TurnServerAvailable;

    IPEndpoint TurnServer;

    list<StunWork*> stunQueue;  // Queue of Stun messages to send, at pacing interval
                                // (currently queue size never greater than one.)

    bool terminating;

    uint8_t* shortTermHmacKey;

    size_t shortTermHmacKeyLength;

    uint8_t* remoteShortTermHmacKey;

    size_t remoteShortTermHmacKeyLength;

    String ufrag;

    String pwd;

    String usernameForShortTermCredential;

    String localInitiatedCheckUsername;

    String remoteInitiatedCheckUsername;

    String peerPwd;

    /**
     * The array of ICE address candidates
     */
    list<ICECandidates> candidates;

    /** Selected candidate pair per component.
     *  Implicit RTCP for a component (if present)
     *  follows its associated RTP component.
     *
     *  The selected pair for a component of a media stream is equal to
     *  the highest priority nominated pair for that component in the valid
     *  list if the state of the check list for that component is 'Completed',
     *  NULL otherwise.
     */
    vector<ICECandidatePair*>  selectedCandidatePairList;

    ICESessionState sessionState;

    Mutex lock;                    ///< Synchronizes multiple threads

    IPAddress defaultConnectionAddress;

    std::vector<ICEStream*> streamList;

    /// const_iterator typedef.
    typedef vector<ICEStream*>::const_iterator stream_const_iterator;

    ICESessionListener* sessionListener;

    bool addHostCandidates;

    bool addRelayedCandidates;

    Thread* pacingThread;

    QStatus errorCode;

    bool isControllingAgent;

    bool useAggressiveNomination;

    uint16_t foundationID;

    bool checksStarted;

    bool listenerNotifiedOnSuccessOrFailure;

    STUNServerInfo STUNInfo;

    IPAddress OnDemandAddress;

    IPAddress PersistentAddress;

    bool EnableIPv6;

    NetworkInterface networkInterface;

    // Private ctor, used only by friend ICEManager
    ICESession(bool addHostCandidates,
               bool addRelayedCandidates,
               ICESessionListener* listener,
               STUNServerInfo stunInfo,
               IPAddress onDemandAddress,
               IPAddress persistentAddress,
               bool enableIPv6) :
        hmacKeyLen(0),
        TurnServerAvailable(false),
        terminating(false),
        shortTermHmacKey(NULL),
        shortTermHmacKeyLength(0),
        remoteShortTermHmacKey(NULL),
        remoteShortTermHmacKeyLength(0),
        sessionState(ICEUninitialized),
        sessionListener(listener),
        addHostCandidates(addHostCandidates),
        addRelayedCandidates(addRelayedCandidates),
        pacingThread(),
        errorCode(ER_OK),
        isControllingAgent(false),
        useAggressiveNomination(false),
        foundationID(0),
        checksStarted(false),
        listenerNotifiedOnSuccessOrFailure(false),
        STUNInfo(stunInfo),
        OnDemandAddress(onDemandAddress),
        PersistentAddress(persistentAddress),
        EnableIPv6(enableIPv6),
        networkInterface(enableIPv6) {

        usernameForShortTermCredential = STUNInfo.acct;

        StunServer.addr = STUNInfo.address;
        StunServer.port = STUNInfo.port;

        if (STUNInfo.relayInfoPresent) {
            TurnServerAvailable = true;
            TurnServer.addr = STUNInfo.relay.address;
            TurnServer.port = STUNInfo.relay.port;
        }

        // Get the short term credentials.
        StunCredential stunCredential(STUNInfo.pwd);

        // size buffer first
        stunCredential.GetKey(NULL, hmacKeyLen);

        hmacKey = new uint8_t[hmacKeyLen];
        if (!hmacKey) {
            QCC_LogError(ER_ICE_ALLOCATING_MEMORY, ("Allocating memory for HMAC key"));
        } else {
            // now get the real key
            stunCredential.GetKey(hmacKey, hmacKeyLen);
        }
    }

    // Private, used only by friend ICEManager
    QStatus Init(void);

    void SetState(ICESessionState state);

    void SetErrorCode(QStatus code);

    void EmptyICEStreamList(void);

    void EmptyLocalCandidatesList(void);


    QStatus GetIPAddressFromConnectionData(String& connectionData,
                                           IPAddress& IPAddressParm);

    QStatus GatherHostCandidates(bool enableIpv6);

    QStatus StartStunTurnPacingThread(void);

    // Gather STUN/TURN candidates, observing the pacing throttling, then perform keepalives.
    void StunTurnPacingWork(void);

    void FindPendingWork(void);

    void ComposeAndEnqueueStunRequest(Stun* stun, Retransmit* retransmit);

    QStatus UpdateLocalICECandidates(void);

    void GetAllReadyStunActivities(list<StunActivity*>& foundList);

    void ComposeCandidateList(list<ICECandidate>& composedList) const;

    void ComposeICEStreamCandidateList(list<ICECandidate>& composedList) const;

    void EliminateRedundantCandidates();

    void ChooseDefaultCandidates();

    void AssignFoundations(void);

    void AssignPriorities(void);

    uint32_t AssignPriority(uint16_t componentID, const ICECandidate& IceCandidate, _ICECandidate::ICECandidateType candidateType);

    void AssignPrioritiesPerICEStream(const ICEStream* stream);

    String GetTransport(const String& transport) const;

    // Each "ICE app" thread will create a gatherKeepalivePacing thread with this
    // function as its entry point.
    static ThreadReturn STDCALL GatheringKeepalivePacingThreadStub(void* pThis)
    {
        ICESession* thisPtr = (ICESession*)pThis;

        // There is a chance of memory corruption here in that if, by the time
        // the following instruction gets a chance to run, this object's destructor
        // has been called, 'thisPtr' will be invalid.  To mitigate the chances,
        // the parent of this thread sleeps for a while immediately after giving birth.
        thisPtr->Lock();
        thisPtr->StunTurnPacingWork();
        thisPtr->Unlock();

        return 0;
    }

    bool GetAddRelayedCandidates(void) const { return addRelayedCandidates; }

    void NotifyListenerIfNeeded(void);

    QStatus FormCheckLists(list<ICECandidates>& peerCandidates, String ice_frag, String ice_pwd);

    void SortAndPruneCandidatePairs(void);

    QStatus StartInitialCheckList(void);

    void SetTurnPermissions(void);

    QStatus ProcessCreatePermissionResponse(Stun* stun, StunTransactionID tid);

    void ComposeAndEnqueueNATKeepalive(Stun* stun, IPEndpoint& destination);

    void EnqueueTurnRefresh(StunActivity* stunActivity);

    void EnqueueTurnCreatePermissions(ICECandidate& candidate);

    QStatus DeallocateTurn(Stun* stun);

    bool RelayedCandidateActivityIsStale(StunActivity* stunActivity);
};

} //namespace ajn

#undef QCC_MODULE

#endif
