#ifndef _STUNATTRIBUTEERRORCODE_H
#define _STUNATTRIBUTEERRORCODE_H
/**
 * @file
 *
 * This file defines the ERROR-CODE STUN message attribute.
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
#include <qcc/unicode.h>
#include <StunAttributeStringBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"


/**
 * Error Code STUN attribute class.
 */
class StunAttributeErrorCode : public StunAttributeStringBase {
  private:
    StunErrorCodes error;       ///< Error code number.

  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_ERROR_CODE.
     */
    StunAttributeErrorCode(void) : StunAttributeStringBase(STUN_ATTR_ERROR_CODE, "ERROR-CODE") { }

    /**
     * This constructor sets the attribute type to STUN_ATTR_ERROR_CODE and
     * initializes the error code and reason phrase.
     *
     * @param error     The error code.
     * @param reason    The reason phrase as a std::string.
     */
    StunAttributeErrorCode(StunErrorCodes error, const String& reason) :
        StunAttributeStringBase(STUN_ATTR_ERROR_CODE, "ERROR-CODE", reason),
        error(error)
    { }


    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
#if !defined(NDEBUG)
    String ToString(void) const;
#endif
    size_t RenderSize(void) const { return StunAttributeStringBase::RenderSize() + sizeof(uint32_t); }
    uint16_t AttrSize(void) const { return StunAttributeStringBase::AttrSize() + sizeof(uint32_t); }

    /**
     * Retreive the error information.
     *
     * @param error  OUT: A reference to where to copy the error code.
     * @param reason OUT: A reference to where to copy the reason std::string.
     */
    void GetError(StunErrorCodes& error, String& reason) const
    {
        error = this->error;
        GetStr(reason);
    }

    /**
     * Set the error information.
     *
     * @param error  The error code.
     * @param reason A reference to the reason std::string.
     */
    void SetError(StunErrorCodes error, const String& reason)
    {
        this->error = error;
        SetStr(reason);
    }
};

#undef QCC_MODULE
#endif
