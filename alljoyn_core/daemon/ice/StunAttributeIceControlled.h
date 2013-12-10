#ifndef _STUNATTRIBUTEICECONTROLLED_H
#define _STUNATTRIBUTEICECONTROLLED_H
/**
 * @file
 *
 * This file defines the ICE-CONTROLLED STUN message attribute.
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
#error Only include StunAttributeIceControlled.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"


/**
 * ICE Controlled STUN attribute class.
 */
class StunAttributeIceControlled : public StunAttribute {
  private:
    uint64_t value;

  public:
    /**
     * This constructor sets the attribute type to STUN_ATTR_ICE_CONTROLLED
     * and initializes the value.
     *
     * @param value The value value (defaults to 0).
     */
    StunAttributeIceControlled(uint64_t value = 0) :
        StunAttribute(STUN_ATTR_ICE_CONTROLLED, "ICE-CONTROLLED"),
        value(value)
    { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize)
    {
        ReadNetToHost(buf, bufSize, value);
        return StunAttribute::Parse(buf, bufSize);
    }

    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
    {
        QStatus status = StunAttribute::RenderBinary(buf, bufSize, sg);
        if (status == ER_OK) {
            WriteHostToNet(buf, bufSize, value, sg);
        }
        return status;
    }

#if !defined(NDEBUG)
    String ToString(void) const
    {
        String oss;

        oss.append(StunAttribute::ToString());
        oss.append(": ");

        oss.append(U32ToString(static_cast<uint32_t>(value >> 32), 16, 8, '0'));
        oss.push_back('-');
        oss.append(U32ToString(static_cast<uint32_t>(value & 0xffffffff), 16, 8, '0'));

        return oss;
    }
#endif
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const { return sizeof(value); }

    /**
     * Set the value.
     */
    void SetValue(uint64_t value) { this->value = value; }

    /**
     * Get the value
     */
    uint64_t GetValue(void) const { return value; }

};


#undef QCC_MODULE
#endif
