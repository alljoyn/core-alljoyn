/**
 * @file
 *
 * This file implements the STUN Attribute Even Port class
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
#include <StunAttributeEvenPort.h>
#include <alljoyn/Status.h>

using namespace qcc;

#define QCC_MODULE "STUN_ATTRIBUTE"


QStatus StunAttributeEvenPort::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status;

    nextPort = static_cast<bool>(*buf & 0x80);
    buf += AttrSize();
    bufSize -= AttrSize();

    status = StunAttribute::Parse(buf, bufSize);

    return status;
}


QStatus StunAttributeEvenPort::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status;

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    // While RFC indicates 1 byte, empirical testing against server shows 4 bytes
    WriteHostToNet(buf, bufSize, static_cast<uint32_t>(nextPort ? 0x80000000 : 0x00), sg);

    //WriteHostToNet(buf, bufSize, static_cast<uint8_t>(nextPort ? 0x80 : 0x00), sg);

    // pad with empty bytes
    //WriteHostToNet(buf, bufSize, static_cast<uint8_t>(0), sg);
    //WriteHostToNet(buf, bufSize, static_cast<uint16_t>(0), sg);

exit:
    return status;
}

#if !defined(NDEBUG)
String StunAttributeEvenPort::ToString(void) const
{
    String oss;

    oss.append(StunAttribute::ToString());
    if (nextPort) {
        oss.append(" (and next port)");
    }

    return oss;
}
#endif

QStatus StunAttributeHexSeventeen::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status;

    buf += AttrSize();
    bufSize -= AttrSize();

    status = StunAttribute::Parse(buf, bufSize);

    return status;
}


QStatus StunAttributeHexSeventeen::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status;

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    WriteHostToNet(buf, bufSize, static_cast<uint32_t>(0x01000000), sg);

exit:
    return status;
}

#if !defined(NDEBUG)
String StunAttributeHexSeventeen::ToString(void) const
{
    String oss;

    return StunAttribute::ToString();

}
#endif
