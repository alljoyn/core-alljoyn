#ifndef _STUNATTRIBUTEEVENPORT_H
#define _STUNATTRIBUTEEVENPORT_H
/**
 * @file
 *
 * This file defines the EVEN-PORT STUN message attribute.
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
#error Only include StunAttributeEvenPort.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <StunAttributeBase.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"


/**
 * Even Port STUN attribute class.
 */
class StunAttributeEvenPort : public StunAttribute {
  private:
    bool nextPort;   ///< @brief Flag indicating the next higher port should
                     //   be allocated as well.

  public:
    /**
     * This constructor sets the attribute type to STUN_ATTR_EVEN_PORT and
     * initializes the next port flag.
     *
     * @param np    Flag indicating if the next port should be reserved as well
     *              (defaults to 'false').
     */
    StunAttributeEvenPort(bool np = false) :
        StunAttribute(STUN_ATTR_EVEN_PORT, "EVEN-PORT"),
        nextPort(np)
    { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
#if !defined(NDEBUG)
    String ToString(void) const;
#endif
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const
    {
        // The TURN draft-13 spec section 14.6 only specifies 1 octet for the
        // attribute, so that will be the attribute size.
        // While RFC shows 1 byte, empirical testing against server shows 4 bytes
        return sizeof(uint32_t);
    }

    /**
     * Get the next port flag value.  "true" indicates that the next port
     * should be allocated.
     *
     * @return Value of the next port flag.
     */
    bool GetNextPort(void) const { return nextPort; }

    /**
     * Set the next port flag to "true" or "false" to indicate if the TURN server
     * should allocate the next higher port.
     *
     * @param v     "true" if the next port should be allocated.
     */
    void SetNextPort(bool v) { nextPort = v; }
};


class StunAttributeHexSeventeen : public StunAttribute {
  private:

  public:
    /**
     * This constructor sets the attribute type to 0x0017
     *
     */
    StunAttributeHexSeventeen() :
        StunAttribute((StunAttrType) 0x0017, "HEXSEVENTEEN")
    { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
#if !defined(NDEBUG)
    String ToString(void) const;
#endif
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const
    {
        return sizeof(uint32_t);
    }
};


#undef QCC_MODULE
#endif
