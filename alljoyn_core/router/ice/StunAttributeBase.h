#ifndef _STUNATTRIBUTEBASE_H
#define _STUNATTRIBUTEBASE_H
/**
 * @file
 *
 * This file defines the STUN Message Attribute base class.
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
#include <qcc/Debug.h>
#include <StunIOInterface.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"

/**
 * Base class for the various Stun Attributes.
 */
class StunAttribute : public StunIOInterface {
  private:
    const StunAttrType type;   ///< STUN Attribute Type.

    /// Raw buffer used when rendering to a scatter-gather list.
    uint8_t rawBuf[sizeof(uint16_t) +   // Size of attribute type.
                   sizeof(uint16_t)];   // Size of attribute length.


  protected:
    const char* const name;    ///< Human readable attribute name.

    bool parsed;  ///< @brief Flag indicating if attribute was parsed * (used
                  //   to prevent changing parsed attributes).

    /**
     * Constructor for the StunAttribute that sets the STUN Attribute
     * Type.  This constructor is protected so that only derivative classes
     * can call this constructor.
     *
     * @param attrType The STUN Attribute Type.
     * @param attrName The STUN Attribute name in human readable form.
     */
    StunAttribute(StunAttrType attrType, const char* const attrName) :
        type(attrType), name(attrName), parsed(false) { }

  public:

    static const uint16_t ATTR_HEADER_SIZE = (2 * sizeof(uint16_t));
    /**
     * This method in derivative classes of StunAttribute parses the contents
     * of the passed in buffer.  Derivative classes call the base class
     * StunAttribute::Parse() as a final step to verify that parsing of the
     * attribute consumed the entirity of the attribute.
     *
     * @param buf     IN:  A reference to pointer pointing to the beginning of
     *                     the buffer to be parsed.
     *                OUT: The pointer updated to point to octet after the last
     *                     octet consumed in the parsing.
     * @param bufSize IN:  Attribute size.
     *                OUT: Attribute size after parsing (should be 0).
     *
     * @return Indication of whether the parsing succeeded or not.
     *         ER_BUFFER_TOO_SMALL indicates that the buffer does not contain
     *         the whole message as expected.
     */
    QStatus Parse(const uint8_t*& buf, size_t& bufSize)
    {
        QStatus status = ER_OK;
        parsed = true;
        if (bufSize != 0) {
            status = ER_STUN_ATTR_SIZE_MISMATCH;
            QCC_LogError(status, ("Parsing %s attribute", name));
        }
        return status;
    }
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
    String ToString(void) const { return String(name); };
    size_t RenderSize(void) const { return 2 * sizeof(uint16_t); }

    /**
     * Returns the size of the attribute including the attribute header in
     * octets.  Derivative classes should provide their own implementation of
     * AttrSize rather than re-implement this function.
     *
     * @return Number of octets covering the entire binary representation of
     *         the object.
     */
    size_t Size(void) const
    {
        return (((2 * sizeof(uint16_t)) +   // Size of the attribute header.
                 AttrSize() + 3) & 0xfffc); // Size of the attribute with padding.
    }

    /**
     * Return just the size of the Attribute data.  This is happens to be the
     * same as the size field of the attribute TLV.  It is intended that
     * derivative classes provide their own implementation of this function
     * rather than modify Size().
     *
     * @return Number of octets in the attribute sans attribute header.
     */
    virtual uint16_t AttrSize(void) const { return 0; }


    /**
     * Simple accessor function to get the attribute type.
     *
     * @return the STUN message attribute type.
     */
    StunAttrType GetType(void) const { return type; }
};

#undef QCC_MODULE
#endif
