#ifndef _STUNATTRIBUTEMESSAGEINTEGRITY_H
#define _STUNATTRIBUTEMESSAGEINTEGRITY_H
/**
 * @file
 *
 * This file defines the MESSAGE-INTEGRITY STUN message attribute.
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
#error Only include StunAttributeBase.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <qcc/Crypto.h>
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"

class StunMessage;

/**
 * Message Integrity STUN attribute class.
 */
class StunAttributeMessageIntegrity : public StunAttribute {
  public:
    enum MessageIntegrityStatus {
        NOT_CHECKED,
        VALID,
        INVALID,
        NO_HMAC
    };

    /**
     * StunAttributeMessageIntegrity constructor.  Message integrity only
     * works for the message this instance is contained in.  Therefore, the
     * message this attribute belongs to must be provided to the constructor.
     *
     * @param msg Reference to the containing message.
     */
    StunAttributeMessageIntegrity(const StunMessage& msg) :
        StunAttribute(STUN_ATTR_MESSAGE_INTEGRITY, "MESSAGE-INTEGRITY"),
        message(msg),
        digest(NULL),
        miStatus(NOT_CHECKED)
    { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const { return static_cast<uint16_t>(Crypto_SHA1::DIGEST_SIZE); }

    MessageIntegrityStatus GetMessageIntegrityStatus(void) { return miStatus; }

    static const uint16_t ATTR_SIZE = static_cast<uint16_t>(Crypto_SHA1::DIGEST_SIZE);

    static const uint16_t ATTR_SIZE_WITH_HEADER = ((StunAttribute::ATTR_HEADER_SIZE + ATTR_SIZE + 3) & 0xfffc);

  private:
    const StunMessage& message;         ///< Reference to containing message.
    const uint8_t* digest;              ///< HMAC-SHA1 value for containing message.
    MessageIntegrityStatus miStatus;    ///< Parsed Message Integrity status.
};


#undef QCC_MODULE
#endif
