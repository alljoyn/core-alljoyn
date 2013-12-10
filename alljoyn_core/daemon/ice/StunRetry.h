#ifndef _STUNRETRY_H
#define _STUNRETRY_H
/**
 * @file StunRetry.h
 *
 */

/******************************************************************************
 * Copyright (c) 2009,2012-2013 AllSeen Alliance. All rights reserved.
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
#include <qcc/time.h>
#include <StunTransactionID.h>
#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include <alljoyn/Status.h>

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE "STUNRETRY"

static const uint8_t MAX_SEND_ATTEMPTS = 9;

class CheckRetry {
  public:
    CheckRetry() : sendAttempt(),
        queuedTime(),
        transactionValid(false),
        transaction()
    {
        /* Set the response reception time interval for all attempts to 500ms */
        for (uint8_t i = 0; i < MAX_SEND_ATTEMPTS; i++) {
            maxReceiveWaitMsec[i] = 500;
        }

        /* Set the response reception time interval for the first attempt to 200ms
         * and the second attempt to 400ms */
        if (MAX_SEND_ATTEMPTS >= 2) {
            maxReceiveWaitMsec[0] = 200;
            maxReceiveWaitMsec[1] = 400;
        }
    }

    ~CheckRetry(void) { }


    CheckRetry* Duplicate(void)
    {
        CheckRetry* duplicate = new CheckRetry();

        duplicate->sendAttempt = sendAttempt;
        duplicate->queuedTime = queuedTime;
        duplicate->transactionValid = transactionValid;
        duplicate->transaction = transaction;

        return duplicate;
    }



    void Init(void)
    {
        sendAttempt = 0;
        queuedTime = 0;
        transactionValid = false;
    }

    void SetTransactionID(StunTransactionID& tid) { transaction = tid; transactionValid = true; }

    StunTransactionID GetTransactionID(void) const { return transaction; }

    bool GetTransactionID(StunTransactionID& tid) const
    {
        if (transactionValid) {
            tid = transaction;
        }
        return transactionValid;
    }

    bool IsTransactionValid(void) const { return transactionValid; }

    bool AnyRetriesNotSent(void);

    bool RetryTimedOut(void);

    bool RetryAvailable(void);

    bool IncrementAttempts(void);

    uint32_t GetQueuedTimeOffset(void);

  private:

    uint8_t sendAttempt;
    uint32_t queuedTime;
    bool transactionValid;
    StunTransactionID transaction;

    uint16_t maxReceiveWaitMsec[MAX_SEND_ATTEMPTS];
};




class Retransmit {
  public:
    Retransmit() : sendAttempt(), receivedErrorCode(ER_OK),
        retransmitState(AwaitingTransmitSlot),
        queuedTime(0), transactionValid(), transaction()
    {
        /* Set the response reception time interval for all attempts to 500ms */
        for (uint8_t i = 0; i < MAX_SEND_ATTEMPTS; i++) {
            maxReceiveWaitMsec[i] = 500;
        }

        /* Set the response reception time interval for the first attempt to 200ms
         * and the second attempt to 400ms */
        if (MAX_SEND_ATTEMPTS >= 2) {
            maxReceiveWaitMsec[0] = 200;
            maxReceiveWaitMsec[1] = 400;
        }
    }

    ~Retransmit(void);

    typedef enum {
        AwaitingTransmitSlot,       /**< Awaiting pacing slot for transmit (or retransmit) */
        AwaitingResponse,           /**< Awaiting response from server */
        NoResponseToAllRetries,     /**< All retries sent with no successful response */
        ReceivedAuthenticateResponse,   /**< Received a authentication response. */
        ReceivedErrorResponse,      /**< Received an error response. */
        ReceivedSuccessResponse,    /**< Received a successful response. StunTurn completed */
        Error                       /**< Failed in send or receive. StunTurn completed */
    } RetransmitState;              /**< State of this Stun process object */


    /**
     * Get the state
     *
     * @return  The state of the Stun object.
     */
    void SetState(RetransmitState state);

    RetransmitState GetState(void);

    void SetErrorCode(QStatus errorCode);

    QStatus GetErrorCode(void) const;

    void SetTransactionID(StunTransactionID& tid) { transaction = tid; transactionValid = true; }

    bool GetTransactionID(StunTransactionID& tid) const { tid = transaction; return transactionValid; }

    void IncrementAttempts();

    void RecordKeepaliveTime(void);

    // Make it appear this has been waiting for longest time
    void PrematurelyAge(void) { queuedTime = 0; }

    uint16_t GetMaxReceiveWaitMsec(void);

    uint32_t GetAwaitingTransmitTimeMsecs(void);

    bool AnyRetriesNotSent(void);

    bool RetryTimedOut(void);

    bool RetryAvailable(void);

  private:

    uint8_t sendAttempt;
    QStatus receivedErrorCode;
    RetransmitState retransmitState;
    uint32_t queuedTime;
    bool transactionValid;
    StunTransactionID transaction;

    uint16_t maxReceiveWaitMsec[MAX_SEND_ATTEMPTS];
};

#undef QCC_MODULE
#endif
