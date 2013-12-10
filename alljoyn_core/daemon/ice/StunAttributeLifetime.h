#ifndef _STUNATTRIBUTELIFETIME_H
#define _STUNATTRIBUTELIFETIME_H
/**
 * @file
 *
 * This file defines the LIFETIME STUN message attribute.
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
#error Only include StunAttributeLifetime.h in C++ code.
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
 * Lifetime STUN attribute class.
 */
class StunAttributeLifetime : public StunAttribute {
  private:
    uint32_t lifetime;   ///< Lifetime in seconds.

  public:
    /**
     * This constructor sets the attribute type to STUN_ATTR_LIFETIME and
     * initializes the lifetime variable.
     *
     * @param lifetime  Number of seconds the server should maintain allocations
     *                  in the absence of a refresh (defaults to 0).
     */
    StunAttributeLifetime(uint32_t lifetime = 0) :
        StunAttribute(STUN_ATTR_LIFETIME, "LIFETIME"),
        lifetime(lifetime)
    { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize)
    {
        ReadNetToHost(buf, bufSize, lifetime);
        return StunAttribute::Parse(buf, bufSize);
    }

    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
    {
        QStatus status = StunAttribute::RenderBinary(buf, bufSize, sg);
        if (status == ER_OK) {
            WriteHostToNet(buf, bufSize, lifetime, sg);
        }
        return status;
    }

#if !defined(NDEBUG)
    String ToString(void) const
    {
        String oss;

        oss.append(StunAttribute::ToString());
        oss.append(": ");
        oss.append(U32ToString(lifetime, 10));
        oss.append(" seconds");

        return oss;
    }
#endif
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const { return sizeof(lifetime); }

    /**
     * Get the lifetime value.
     *
     * @return  Number of seconds the server should maintain allocations in the
     *          absence of a refresh.
     */
    uint32_t GetLifetime(void) const { return lifetime; }

    /**
     * Sets the lifetime value.
     *
     * @param lifetime  Number of seconds server should maintain allocations in
     *                  the absence of a refresh.
     */
    void SetLifetime(uint32_t lifetime) { this->lifetime = lifetime; }
};


#undef QCC_MODULE
#endif
