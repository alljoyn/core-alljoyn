/**
 * @file ICECandidatePair.h
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

#ifndef _ICECANDIDATEPAIR_H
#define _ICECANDIDATEPAIR_H

#include <qcc/Thread.h>
#include <ICECandidate.h>
#include <Stun.h>
#include <StunRetry.h>

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE "ICECANDIDATEPAIR"

namespace ajn {

/**
 * An ICECandidatePair describes a local and a remote ICECandidate used during an ICE Connection check.
 */
class ICECandidatePair {
  public:
    /** Connection check state for candidate pair */
    typedef enum {
        Frozen,          /**< Connection check is deferred until another check completes */
        Waiting,         /**< Connection check is ready to be performed but has not been started */
        InProgress,      /**< Connection check is in progress */
        Failed,          /**< Connection check has failed */
        Succeeded        /**< Connection check has succeeded */
    } ICEPairConnectionState;

    /** Error Code for check result (Keep CheckStatusToString consistent) */
    typedef enum {
        CheckSucceeded,
        CheckTimeout,
        CheckRoleConflict,
        CheckInconsistentTransactionID,
        CheckGenericFailed,
        CheckUnknownPair,
        CheckResponseSent
    } CheckStatus;

    String CheckStatusToString(const CheckStatus status) const;


    /**
     * Constructor
     *
     * @param local     Local candidate used in pair.
     * @param remote    Remote candidate used in pair.
     */
    ICECandidatePair(const ICECandidate& local, const ICECandidate& remote, bool isDefault, uint64_t priority);

    ~ICECandidatePair(void);

    uint64_t GetPriority(void) const { return priority; };
    uint32_t GetBindRequestPriority(void) const { return bindRequestPriority; };

    void GetFoundation(String& foundation) const { foundation = this->foundation; }

    String GetFoundation(void) const { return foundation; }

    QStatus InitChecker(const uint64_t& controlTieBreaker,
                        bool useAggressiveNomination, const uint32_t& bindRequestPriority);

    QStatus InitChecker(ICECandidatePair& originalPair);

    void Check(void);

    StunTransactionID GetTransactionID(void) const { return checkRetry->GetTransactionID(); }

    bool EqualsCanceledTransactionID(const StunTransactionID& tid) const
    {
        return (NULL != canceledRetry &&
                canceledRetry->IsTransactionValid() && (tid == canceledRetry->GetTransactionID()));
    }

    uint64_t GetControlTieBreaker(void) const { return controlTieBreaker; }

    void SetNominated(void);

    void SetNominatedContingent(void) { isNominatedContingent = true; }

    bool IsNominated(void) const { return isNominated; }

    void AddTriggered(void);

    bool IsTriggered(void) { return isTriggered; }

    void RemoveTriggered(void) { isTriggered = false; }

    bool IsWorkRemaining(void);

    bool RetryTimedOut(void) { return checkRetry->RetryTimedOut(); }

    bool RetryAvailable(void) { return checkRetry->RetryAvailable(); }

    uint32_t GetQueuedTimeOffset() { return checkRetry->GetQueuedTimeOffset(); }

    ICECandidatePair* IncrementRetryAttempt(void);

    void UpdateNominatedFlag(void);

    void SetCanceled(void);

    CheckRetry* GetCheckRetryByTransaction(StunTransactionID& tid);


    /** Local ICECandidate */
    const ICECandidate local;

    /** Remote ICECandidate */
    const ICECandidate remote;

    /** Connection check state for this ICECandidatePair */
    ICEPairConnectionState state;

    bool isValid;

  private:

    /* Just defined to make klocwork happy. Should never be used */
    ICECandidatePair(const ICECandidatePair& other);

    /* Just defined to make klocwork happy. Should never be used */
    ICECandidatePair& operator=(const ICECandidatePair& other);

    CheckRetry* checkRetry;

    CheckRetry* canceledRetry;

    /** Connection check priority of this ICECandidatePair */
    uint64_t priority;

    bool isDefault;

    bool isNominated;

    bool isNominatedContingent;

    String foundation;

    bool useAggressiveNomination;

    bool regularlyNominated;

    uint64_t controlTieBreaker;

    uint32_t bindRequestPriority;

    bool isTriggered;

};

} //namespace ajn

#undef QCC_MODULE
#endif
