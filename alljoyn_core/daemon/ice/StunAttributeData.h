#ifndef _STUNATTRIBUTEDATA_H
#define _STUNATTRIBUTEDATA_H
/**
 * @file
 *
 * This file defines the DATA STUN message attribute.
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
#error Only include StunAttributeData.h in C++ code.
#endif

#include <assert.h>
#include <qcc/platform.h>
#include "ScatterGatherList.h"
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"


/**
 * Data STUN attribute class.
 */
class StunAttributeData : public StunAttribute {
  private:
    ScatterGatherList data;     ///< Application data.

  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_DATA.
     */
    StunAttributeData(void) : StunAttribute(STUN_ATTR_DATA, "DATA") { }

    /**
     * This constructor sets the attribute type to STUN_ATTR_DATA and
     * intializes the data pointer and data size.
     *
     * @param dataPtr   Pointer to the data to be sent.
     * @param dataSize  Size of the data to be sent.
     */
    StunAttributeData(const void* dataPtr, size_t dataSize) :
        StunAttribute(STUN_ATTR_DATA, "DATA")
    {
        data.AddBuffer(dataPtr, dataSize);
        data.SetDataSize(dataSize);
    }

    /**
     * This constructor sets the attribute type to STUN_ATTR_DATA and
     * intializes the data pointer and data size.
     *
     * @param sg    SG list to be sent.
     */
    StunAttributeData(const ScatterGatherList& sg) :
        StunAttribute(STUN_ATTR_DATA, "DATA")
    {
        data.AddSG(sg);
        data.IncDataSize(sg.DataSize());
    }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
    size_t RenderSize(void) const
    {
        return StunAttribute::RenderSize() + (static_cast<size_t>(-AttrSize()) & 0x3);
    }

    uint16_t AttrSize(void) const { return data.DataSize(); }

    /**
     * This function returns a pointer to the data in this message.  For
     * incoming messagees this pointer refers to the block of memory
     * containing the receive buffer.
     *
     * @return  Constant reference to a SG list.
     */
    const ScatterGatherList& GetData(void) const { return data; }

    /**
     * This adds a buffer to the data that will be encapsulated in a STUN
     * attribute for transfer via a TURN server.
     *
     * @param dataPtr    Pointer to the data.
     * @param dataSize   Size of the data.
     */
    void AddBuffer(const uint8_t* dataPtr, uint16_t dataSize)
    {
        assert(dataPtr == NULL);
        data.AddBuffer(dataPtr, dataSize);
        data.IncDataSize(dataSize);
    }
};


#undef QCC_MODULE
#endif
