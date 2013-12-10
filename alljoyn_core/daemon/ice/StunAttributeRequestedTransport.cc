/**
 * @file
 *
 * This file implements the STUN Attribute Requested Transport class
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

#include <string>
#include <qcc/platform.h>
#include <qcc/StringUtil.h>
#include <qcc/String.h>
#include <StunAttributeRequestedTransport.h>
#include <alljoyn/Status.h>

using namespace qcc;

#define QCC_MODULE "STUN_ATTRIBUTE"


QStatus StunAttributeRequestedTransport::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status;

    protocol = *buf;
    ++buf;
    --bufSize;

    // "Read" the RFFU (if included in the attribute size)
    buf += bufSize;
    bufSize = 0;

    status = StunAttribute::Parse(buf, bufSize);

    return status;
}


QStatus StunAttributeRequestedTransport::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status;

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }


    WriteHostToNet(buf, bufSize, static_cast<uint8_t>(protocol), sg);

    // Pad with empty bytes.
    WriteHostToNet(buf, bufSize, static_cast<uint8_t>(0), sg);
    WriteHostToNet(buf, bufSize, static_cast<uint16_t>(0), sg);

exit:
    return status;
}


#if !defined(NDEBUG)
String StunAttributeRequestedTransport::ToString(void) const
{
    String oss;

    oss.append(StunAttribute::ToString());
    oss.append(": ");
    oss.append(U32ToString(static_cast<uint32_t>(protocol), 10));

    return oss;
}
#endif
