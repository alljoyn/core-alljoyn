#ifndef _STUNATTRIBUTESTRINGBASE_H
#define _STUNATTRIBUTESTRINGBASE_H
/**
 * @file
 *
 * This file defines the base string message attribute.
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
#error Only include StunAttributeStringBase.h in C++ code.
#endif

#include <qcc/String.h>
#include <qcc/platform.h>
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>
#include "unicode.h"

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"

/**
 * Base String STUN attribute class.
 */
class StunAttributeStringBase : public StunAttribute {
  private:
    const static uint32_t MAX_LENGTH = 513; ///< Max str length as defined in RFC 5389.

    String str;  ///< String data

  protected:
    /**
     * Renders just the string portion of the attribute (not the header) into
     * the buffer/SG list.
     *
     * @param buf       Reference to memory where the rendering may be done.
     * @param bufSize   Amount of space left in the buffer.
     * @param sg        SG List where rendering may be done.
     */
    void RenderBinaryString(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;

    /**
     * Retrieves the parsed UTF-8 str.
     *
     * @param str OUT: A reference to where to copy the str.
     */
    void GetStr(String& str) const
    {
        str = this->str;
    }

    /**
     * Sets the UTF-8 str.
     *
     * @param str A reference the str.
     */
    void SetStr(const String& str)
    {
        QCC_DbgTrace(("StunAttributeStringBase::SetStr(string str = %s)", str.c_str()));
        this->str = str;
    }


  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_USERNAME.
     *
     * @param attrType  Attribute Type
     * @param attrName  Attribute Name
     */
    StunAttributeStringBase(StunAttrType attrType, const char* const attrName) :
        StunAttribute(attrType, attrName)
    { }

    /**
     * This constructor just sets the attribute type to STUN_ATTR_USERNAME.
     *
     * @param attrType  Attribute Type
     * @param attrName  Attribute Name
     * @param str       The str as std::string.
     */
    StunAttributeStringBase(StunAttrType attrType, const char* const attrName,
                            const String& str) :
        StunAttribute(attrType, attrName),
        str(str)
    {
        QCC_DbgTrace(("StunAttributeStringBase::StunAttributeStringBase(attrType, attrName = %s, string str = %s)", attrName, str.c_str()));
    }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
#ifndef NDEBUG
    String ToString(void) const;
#endif
    size_t RenderSize(void) const
    {
        return StunAttribute::RenderSize() + (static_cast<size_t>(-AttrSize() & 0x3));
    }
    uint16_t AttrSize(void) const { return str.length(); }
};


#undef QCC_MODULE
#endif
