#ifndef _STUNATTRIBUTEREQUESTEDTRANSPORT_H
#define _STUNATTRIBUTEREQUESTEDTRANSPORT_H
/**
 * @file
 *
 * This file defines the REQUESTED-TRANSPORT STUN message attribute.
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

#ifndef __cplusplus
#error Only include StunAttributeRequestedTransport.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"


/**
 * Requested Transport STUN attribute class.
 */
class StunAttributeRequestedTransport : public StunAttribute {
  private:
    uint8_t protocol;   ///< IP protocol.

  public:
    /**
     * This constructor sets the attribute type to STUN_ATTR_REQUESTED_TRANSPORT
     * and initializes the protocol.
     *
     * @param protocol  The protocol (defaults to 0).
     */
    StunAttributeRequestedTransport(uint8_t protocol = 0) :
        StunAttribute(STUN_ATTR_REQUESTED_TRANSPORT, "REQUESTED-TRANSPORT"),
        protocol(protocol)
    { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
#if !defined(NDEBUG)
    String ToString(void) const;
#endif
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const
    {
        // The TURN draft-13 spec section 14.7 specifies the RFFU as part of
        // the attribute so include it in the size.
        return (sizeof(uint8_t) +     // size of protocol
                3 * sizeof(uint8_t)); // size of RFFU;
    }

    /**
     * Retrieve the protocol.
     *
     * @return The requested protocol.
     */
    uint8_t GetProtocol(void) const { return protocol; }

    /**
     * Set the protocol.
     *
     * @param protocol  The requested protocol.
     */
    void SetProtocol(uint8_t protocol) { this->protocol = protocol; }
};


#undef QCC_MODULE
#endif
