/**
 * @file StunRetry.cc
 *
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

#include <qcc/time.h>
#include <StunRetry.h>

/** @internal */
#define QCC_MODULE "STUNRETRY"

using namespace qcc;

bool CheckRetry::AnyRetriesNotSent(void)
{
    return (sendAttempt < MAX_SEND_ATTEMPTS - 1);
}


bool CheckRetry::RetryTimedOut(void)
{
    return ((GetTimestamp() - queuedTime) >= maxReceiveWaitMsec[sendAttempt]);
}


bool CheckRetry::RetryAvailable(void)
{
    return (AnyRetriesNotSent() && RetryTimedOut());
}

uint32_t CheckRetry::GetQueuedTimeOffset(void)
{
    return (queuedTime + maxReceiveWaitMsec[sendAttempt]);
}


bool CheckRetry::IncrementAttempts(void)
{
    bool attemptsRemaining = false;

    if (sendAttempt < MAX_SEND_ATTEMPTS - 1) {
        sendAttempt++;
        // record time of this attempt
        queuedTime = GetTimestamp();
        attemptsRemaining = true;
    } else {
        attemptsRemaining = false;
    }

    return attemptsRemaining;
}





/*
 *****************************************************************************************
 *****************************************************************************************
 *****************************************************************************************
 */

Retransmit::~Retransmit(void)
{
}


Retransmit::RetransmitState Retransmit::GetState(void)
{
    RetransmitState state;

    state = retransmitState;

    return state;
}

void Retransmit::SetState(RetransmitState state)
{
    retransmitState = state;
}

QStatus Retransmit::GetErrorCode(void) const {
    return receivedErrorCode;
}

void Retransmit::SetErrorCode(QStatus errorCode) {
    receivedErrorCode = errorCode;
}

void Retransmit::IncrementAttempts()
{
    if (sendAttempt < MAX_SEND_ATTEMPTS) {
        sendAttempt++;
    }

    // record time of this attempt
    queuedTime = GetTimestamp();

    retransmitState = AwaitingResponse;
}

uint16_t Retransmit::GetMaxReceiveWaitMsec(void)
{
    uint16_t wait = 0;

    // ToDo RFC 5389 7.2.1, and draft-ietf-mmusic-ice-?

    // because we pre-increment, index is off by 1
    if (sendAttempt - 1 < MAX_SEND_ATTEMPTS) {
        wait = maxReceiveWaitMsec[sendAttempt - 1];
    }

    return wait;
}

uint32_t Retransmit::GetAwaitingTransmitTimeMsecs(void)
{
    uint32_t transmitTime;

    transmitTime = GetTimestamp() - queuedTime;

    return transmitTime;
}

void Retransmit::RecordKeepaliveTime(void)
{
    // record time of this attempt
    queuedTime = GetTimestamp();
}

bool Retransmit::AnyRetriesNotSent(void)
{
    return (sendAttempt < MAX_SEND_ATTEMPTS - 1);
}


bool Retransmit::RetryTimedOut(void)
{
    return ((GetTimestamp() - queuedTime) >= maxReceiveWaitMsec[sendAttempt]);
}


bool Retransmit::RetryAvailable(void)
{
    return (AnyRetriesNotSent() && RetryTimedOut());
}
