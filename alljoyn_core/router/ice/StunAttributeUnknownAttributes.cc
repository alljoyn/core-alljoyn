/**
 * @file
 *
 * This file implements the STUN Attribute Unknown Attributes class
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
#include <qcc/StringUtil.h>
#include <StunAttributeUnknownAttributes.h>
#include <alljoyn/Status.h>

using namespace qcc;

#define QCC_MODULE "STUN_ATTRIBUTE"


QStatus StunAttributeUnknownAttributes::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status;

    while (bufSize > 0) {
        uint16_t attr = 0;
        ReadNetToHost(buf, bufSize, attr);
        AddAttribute(attr);
    }

    status = StunAttribute::Parse(buf, bufSize);

    return status;
}



QStatus StunAttributeUnknownAttributes::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const

{
    std::vector<uint16_t>::const_iterator iter;
    QStatus status;
    QCC_DbgTrace(("StunAttributeUnknownAttributes::RenderBinary(*buf, bufSize = %u, sg = <>)", bufSize));

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    for (iter = Begin(); iter != End(); ++iter) {
        QCC_DbgPrintf(("Adding %04x (%u bytes - space: %u)...", *iter, sizeof(*iter), bufSize));
        WriteHostToNet(buf, bufSize, *iter, sg);
    }

    if ((attrTypes.size() & 0x1) == 1) {
        // pad with an empty bytes.
        WriteHostToNet(buf, bufSize, static_cast<uint16_t>(0), sg);
    }

exit:
    return status;
}


#if !defined(NDEBUG)
String StunAttributeUnknownAttributes::ToString(void) const
{
    std::vector<uint16_t>::const_iterator iter;
    String oss;

    oss.append(StunAttribute::ToString());
    oss.append(": ");

    iter = Begin();
    while (iter != End()) {
        oss.append(U32ToString(*iter, 16, 4, '0'));
        ++iter;
        if (iter != End()) {
            oss.append(", ");
        }
    }

    return oss;
}
#endif
