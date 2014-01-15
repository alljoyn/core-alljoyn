/**
 * @file StunActivity.cc
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

#include <ICECandidate.h>
#include <StunActivity.h>


/** @internal */
#define QCC_MODULE "STUNACTIVITY"

namespace ajn {

StunActivity::StunActivity(Stun* stun) :
    stun(stun),
    candidate(),
    retransmit()
{
}

StunActivity::~StunActivity()
{

}

void StunActivity::SetCandidate(const ICECandidate& candidate)
{
    QCC_DbgTrace(("%s(%p): ", __FUNCTION__, this));

    this->candidate = candidate;

    retransmit = Retransmit();

    switch (candidate->GetType()) {
    case _ICECandidate::Host_Candidate:
        // retransmit will maintain count of retries and timeouts as we perform the
        // (one and only) Bind/Allocate request to the STUN/TURN server for this local interface
        break;

    case _ICECandidate::ServerReflexive_Candidate:
    case _ICECandidate::PeerReflexive_Candidate:
        // Reflexive candidates use 'retransmit' only for NAT keepalive time stamping.
        // They do NOT timeout responses, nor perform retransmits.
        retransmit.RecordKeepaliveTime();  // stamp time now
        break;

    case _ICECandidate::Relayed_Candidate:
        // For refreshing Allocations/CreatePermissions on the TURN server.
        retransmit.RecordKeepaliveTime();  // stamp time now
        break;

    default:
        assert(false);
        break;
    }
}

} //namespace ajn

