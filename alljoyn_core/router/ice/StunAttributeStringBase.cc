/**
 * @file
 *
 * This file implements the STUN Attribute Str class
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
#include <qcc/String.h>
#include <StunAttributeStringBase.h>
#include <alljoyn/Status.h>

using namespace qcc;


#define QCC_MODULE "STUN_ATTRIBUTE"


QStatus StunAttributeStringBase::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status;
    QCC_DbgPrintf(("StunAttributeStringBase::Parse(*buf, bufSize = %u)", bufSize));
    QCC_DbgLocalData(&buf[0], bufSize);

    str = String(reinterpret_cast<const char*>(&buf[0]), bufSize);

    QCC_DbgPrintf(("str[%u] = '%s'", str.length(), str.c_str()));

    buf += bufSize;
    bufSize = 0;

    status = StunAttribute::Parse(buf, bufSize);

    return status;
}


void StunAttributeStringBase::RenderBinaryString(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QCC_DbgTrace(("StunAttributeStringBase::RenderBinaryString(*buf, bufSize = %u, sg)", bufSize));
    QCC_DbgPrintf(("str.data() = %p    str.length() = %u", str.data(), str.length()));
    QCC_DbgLocalData(str.data(), str.length());

    sg.AddBuffer(str.data(), str.length());
    sg.IncDataSize(str.length());

    if ((str.length() & 0x3) != 0) {
        // pad with empty bytes
        if ((str.length() & 0x3) < 3) {
            WriteHostToNet(buf, bufSize, static_cast<uint16_t>(0), sg);
        }
        if ((str.length() & 0x1) == 0x1) {
            WriteHostToNet(buf, bufSize, static_cast<uint8_t>(0), sg);
        }
    }
}

QStatus StunAttributeStringBase::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status;
    QCC_DbgTrace(("StunAttributeStringBase::RenderBinary(*buf, bufSize = %u, sg = <>)", bufSize));

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    RenderBinaryString(buf, bufSize, sg);

exit:
    return status;
}


#if !defined(NDEBUG)
String StunAttributeStringBase::ToString(void) const
{
    String oss;

    oss.append(StunAttribute::ToString());
    oss.append(": ");
    oss.append(str);

    return oss;
}
#endif
