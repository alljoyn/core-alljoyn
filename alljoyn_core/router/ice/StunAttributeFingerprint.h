#ifndef _STUNATTRIBUTEFINGERPRINT_H
#define _STUNATTRIBUTEFINGERPRINT_H
/**
 * @file
 *
 * This file defines the FINGERPRINT STUN message attribute.
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
#error Only include StunAttributeFingerprint.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <StunAttributeBase.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"

class StunMessage;

/**
 * Fingerprint STUN attribute class.
 */
class StunAttributeFingerprint : public StunAttribute {
  private:
    static const uint32_t CRC_TABLE[256];   ///< CRC look up table.
    const StunMessage& message;   ///< Reference to containing message.
    uint32_t fingerprint;         ///< CRC-32 value (XOR'd w/ 0x5354554e) for containing message.
    static const uint32_t MAGIC_XOR = 0x5354554e;    ///< Magic XOR value (see RFC 5389 sec. 15.5).

    /**
     * Compute the CRC-32 value.
     *
     * @param buf   Buffer to compute the CRC over.
     * @param len   Length of the buffer.
     * @param crc   Starting value for the CRC.
     *
     * @return  The result of the CRC-32 calculation.
     */
    static uint32_t ComputeCRC(const uint8_t* buf, size_t len, uint32_t crc = 0);


  public:
    /**
     * StunAttributeFingerprint constructor.  Fingerprint only works for the
     * message this instance is contained in.  Therefore, the message this
     * attribute belongs to must be provided to the constructor.
     *
     * @param msg Reference to the containing message.
     */
    StunAttributeFingerprint(const StunMessage& msg) :
        StunAttribute(STUN_ATTR_FINGERPRINT, "FINGERPRINT"), message(msg) { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const { return sizeof(fingerprint); }

    static const uint16_t ATTR_SIZE = sizeof(uint32_t);

    static const uint16_t ATTR_SIZE_WITH_HEADER = ((StunAttribute::ATTR_HEADER_SIZE + ATTR_SIZE + 3) & 0xfffc);
};


#undef QCC_MODULE
#endif
