/**
 * @file
 *
 * This file implements the STUN Attribute Data class
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

#include <qcc/platform.h>
#include <StunAttributeData.h>
#include <alljoyn/Status.h>

using namespace qcc;

#define QCC_MODULE "STUN_ATTRIBUTE"


QStatus StunAttributeData::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status;
    QCC_DbgTrace(("StunAttributeData::Parse(*buf, bufSize = %u)", bufSize));
    QCC_DbgRemoteData(buf, bufSize);

    data.AddBuffer(&buf[0], bufSize);
    data.SetDataSize(bufSize);

    buf += bufSize;
    bufSize = 0;

    status = StunAttribute::Parse(buf, bufSize);

    return status;
}


QStatus StunAttributeData::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status;
    size_t dataLen;

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    dataLen = data.DataSize();
    sg.AddSG(data);
    sg.IncDataSize(dataLen);

    // Data does not end on a 32-bit boundary so add some 0 padding.
    if ((dataLen & 0x3) != 0) {
        // pad with empty bytes
        if ((dataLen & 0x3) < 3) {
            WriteHostToNet(buf, bufSize, static_cast<uint16_t>(0), sg);
        }
        if ((dataLen & 0x1) == 0x1) {
            WriteHostToNet(buf, bufSize, static_cast<uint8_t>(0), sg);
        }
    }

exit:
    return status;
}
