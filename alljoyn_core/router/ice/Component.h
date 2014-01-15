/**
 * @file Component.h
 *
 * Component contains the state for a single component of a media stream.
 *
 * From draft-ietf-mmusic-ice-19...
 * "A component is a piece of a media stream requiring a
 * single transport address (combination of IP address and transport
 * protocol - such as UDP or TCP - port); a media stream may require multiple
 * components, each of which has to work for the media stream as a
 * whole to work.  For media streams based on RTP, there are two
 * components per media stream - one for RTP, and one for RTCP."
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

#ifndef _COMPONENT_H
#define _COMPONENT_H

#include <string>
#include <list>
#include <qcc/IPAddress.h>
#include <ICECandidate.h>
#include <ICECandidatePair.h>
#include <StunRetry.h>
#include <Stun.h>
#include <StunActivity.h>
#include <alljoyn/Status.h>
#include "RendezvousServerInterface.h"

using namespace std;

/** @internal */
#define QCC_MODULE "COMPONENT"


static const uint16_t COMPONENT_ID_RTP = 1;    ///<
static const uint16_t COMPONENT_ID_RTCP = 2;   ///<

typedef uint16_t ComponentID;

namespace ajn {

// Forward Declaration
class ICEStream;

class Component {
  public:

    Component(ICEStream* stream, ComponentID id, const qcc::String& transport, qcc::AddressFamily af, STUNServerInfo stunInfo, const uint8_t* key, size_t keyLen) :
        stream(stream), id(id),
        transport(transport),
        stunActivityList(),
        candidateList(),
        af(af),
        defaultCandidate(),
        selectedPair(NULL),
        socketType(qcc::QCC_SOCK_DGRAM),
        hasValidPair(false),
        STUNInfo(stunInfo),
        hmacKey(key),
        hmacKeyLen(keyLen) { }

    ~Component(void);

    QStatus AddStun(const qcc::IPAddress& address, uint16_t& port, Stun*& stun, size_t mtu);

    QStatus AddCandidate(const ICECandidate& candidate);

    QStatus CreateHostCandidate(qcc::SocketType socketType, const qcc::IPAddress& addr, uint16_t port, size_t mtu);

    QStatus RemoveCandidate(ICECandidate& candidate);

    uint16_t GetId(void) { return id; }

    const list<StunActivity*>* GetStunActivityList(void) const
    {
        return &stunActivityList;
    }

    /// const_iterator typedef.
    typedef list<ICECandidate>::const_iterator const_iterator;
    typedef list<ICECandidate>::iterator iterator;

    const_iterator Begin(void) const { return candidateList.begin(); }
    const_iterator End(void) const { return candidateList.end(); }

    iterator Begin(void) { return candidateList.begin(); }
    iterator End(void) { return candidateList.end(); }

    typedef list<ICECandidatePair*>::const_iterator const_iterator_validList;

    const_iterator_validList BeginValidList(void) const { return validList.begin(); }
    const_iterator_validList EndValidList(void) const { return validList.end(); }

    // Used only during gathering...
    StunActivity* GetActivityFromStun(const Stun* stun) const
    {
        StunActivity* stunActivity = NULL;

        // iterate stunActivityList looking for activity
        list<StunActivity*>::const_iterator it;
        for (it = stunActivityList.begin(); it != stunActivityList.end(); ++it) {
            if ((*it)->stun == stun) {
                stunActivity = *it;
                break;
            }
        }

        return stunActivity;
    }

    CheckRetry* GetCheckRetryByTransaction(StunTransactionID tid) const;

    Retransmit* GetRetransmitByTransaction(const StunTransactionID& tid) const;

    void AddToStunActivityList(StunActivity* stunActivity);

    const qcc::String GetTransport(void) const { return transport; }

    ICEStream* GetICEStream(void) const { return stream; }

    const uint8_t* GetHmacKey(void) const;

    const size_t GetHmacKeyLength(void) const;

    ComponentID GetID(void) const { return id; }

    ICECandidate GetDefaultCandidate(void) const { return defaultCandidate; }

    void AssignDefaultCandidate(const ICECandidate& candidate);

    qcc::SocketType GetSocketType(void) const { return socketType; }

    qcc::AddressFamily GetAddressFamily(void) const { return af; }

    void AddToValidList(ICECandidatePair* validPair);

    bool HasValidPair(void) const { return hasValidPair; }

    bool FoundationMatchesValidPair(const qcc::String& foundation);

    QStatus GetSelectedCandidatePair(ICECandidatePair*& selectedPair);

    void SetSelectedIfHigherPriority(ICECandidatePair* pair);


  private:

    ICEStream* stream;         ///< ICEStream to which this component belongs. (An RTP component
                               //   and an RTCP component belong to the same stream.)
    ComponentID id;            ///< e.g. 1 for RTP, 2 for RTCP
    qcc::String transport;

    list<StunActivity*> stunActivityList;

    list<ICECandidate> candidateList;

    qcc::AddressFamily af;

    ICECandidate defaultCandidate;

    ICECandidatePair* selectedPair; ///< Highest priority nominated pair in 'valid' list
                                    //   if checkListState is 'Completed'

    qcc::SocketType socketType;

    bool hasValidPair;

    list<ICECandidatePair*> validList;

    qcc::Mutex mutex;

    STUNServerInfo STUNInfo;

    const uint8_t* hmacKey;

    size_t hmacKeyLen;

    void EmptyActivityList(void);

    void GetStunTurnServerAddress(qcc::String& address) const;

    uint16_t GetStunTurnServerPort(void) const;


};

} //namespace ajn

#undef QCC_MODULE

#endif
