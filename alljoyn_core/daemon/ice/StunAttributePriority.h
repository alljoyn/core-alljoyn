#ifndef _STUNATTRIBUTEPRIORITY_H
#define _STUNATTRIBUTEPRIORITY_H
/**
 * @file
 *
 * This file defines the PRIORITY STUN message attribute.
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
#error Only include StunAttributePriority.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <qcc/StringUtil.h>
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"


/**
 * Priority STUN attribute class.
 */
class StunAttributePriority : public StunAttribute {
  private:
    uint32_t priority;   ///< Priority of peer reflexive address

  public:
    /**
     * This constructor sets the attribute type to STUN_ATTR_PRIORITY and
     * initializes the priority variable.
     *
     * @param priority  Priority level of the peer reflexive address.
     */
    StunAttributePriority(uint32_t priority = 0) :
        StunAttribute(STUN_ATTR_PRIORITY, "PRIORITY"),
        priority(priority)
    { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize)
    {
        ReadNetToHost(buf, bufSize, priority);
        return StunAttribute::Parse(buf, bufSize);
    }

    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
    {
        QStatus status = StunAttribute::RenderBinary(buf, bufSize, sg);
        if (status == ER_OK) {
            WriteHostToNet(buf, bufSize, priority, sg);
        }
        return status;
    }

#if !defined(NDEBUG)
    String ToString(void) const
    {
        String oss;

        oss.append(StunAttribute::ToString());
        oss.append(": ");
        oss.append(U32ToString(priority, 10));

        return oss;
    }
#endif
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const { return sizeof(priority); }

    /**
     * Get the priority value.
     *
     * @return  Number of seconds the server should maintain allocations in the
     *          absence of a refresh.
     */
    uint32_t GetPriority(void) const { return priority; }

    /**
     * Sets the priority value.
     *
     * @param priority  Number of seconds server should maintain allocations in
     *                  the absence of a refresh.
     */
    void SetPriority(uint32_t priority) { this->priority = priority; }
};


#undef QCC_MODULE
#endif
