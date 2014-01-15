#ifndef _STUNATTRIBUTEUNKNOWNATTRIBUTES_H
#define _STUNATTRIBUTEUNKNOWNATTRIBUTES_H
/**
 * @file
 *
 * This file defines the UNKNOWN-ATTRIBUTES STUN message attribute.
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
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"


/**
 * Unknown Attributes STUN attribute class.
 */
class StunAttributeUnknownAttributes : public StunAttribute {
  public:
    /// const_iterator typedef.
    typedef std::vector<uint16_t>::const_iterator const_iterator;

  private:
    /**
     * List of unknown attribute types.  (NOTE: Cannot be of type
     * StunAttrType because that enumerates all the known attribute types.)
     */
    std::vector<uint16_t> attrTypes;

  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_UNKNOWN_ATTRIBUTES.
     */
    StunAttributeUnknownAttributes(void) :
        StunAttribute(STUN_ATTR_UNKNOWN_ATTRIBUTES, "UNKNOWN-ATTRIBUTES") { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
#if !defined(NDEBUG)
    String ToString(void) const;
#endif
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const { return sizeof(uint16_t) * attrTypes.size(); }

    /**
     * Retrieve an iterator to beginning of the list of unknown attributes.
     *
     * @return An iterator to the first attribute.
     */
    const_iterator Begin(void) const { return attrTypes.begin(); }

    /**
     * Retrieve an iterator to the end of the list of unknown attributes.
     *
     * @return An iterator to the postion just past the last attribute.
     */
    const_iterator End(void) const { return attrTypes.end(); }

    /**
     * Add an unknown attribute to the list of unkown attributes.
     *
     * @param attr An unknown attribute.
     */
    void AddAttribute(uint16_t attr) { attrTypes.push_back(attr); }
};


#undef QCC_MODULE
#endif
