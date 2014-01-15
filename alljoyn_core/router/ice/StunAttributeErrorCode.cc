/**
 * @file
 *
 * This file implements the STUN Attribute Error Code class
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
#include <StunAttributeErrorCode.h>
#include <alljoyn/Status.h>

using namespace qcc;

#define QCC_MODULE "STUN_ATTRIBUTE"


QStatus StunAttributeErrorCode::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status;
    uint8_t errClass;
    uint8_t errNum;

    buf += sizeof(uint16_t);
    bufSize -= sizeof(uint16_t);

    errClass = *buf & 0x07;  // Ignore the upper bits per RFC 5389 sec. 15.6
    ++buf;
    --bufSize;

    errNum = *buf;
    ++buf;
    --bufSize;

    if (errClass < 3 || errClass > 6 || errNum > 99) {
        status = ER_STUN_INVALID_ERROR_CODE;
        QCC_LogError(status, ("Parsing %s (class: 3 =< %d =< 6  number: 0 =< %d =< 99)",
                              name, errClass, errNum));
        goto exit;
    }

    error = static_cast<StunErrorCodes>(errClass * 100 + errNum);

    status = StunAttributeStringBase::Parse(buf, bufSize);

exit:
    return status;
}


QStatus StunAttributeErrorCode::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status;

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    WriteHostToNet(buf, bufSize, static_cast<uint16_t>(0), sg);  // Fill reserved w/ 0.

    WriteHostToNet(buf, bufSize, static_cast<uint8_t>(error / 100), sg);
    WriteHostToNet(buf, bufSize, static_cast<uint8_t>(error % 100), sg);

    RenderBinaryString(buf, bufSize, sg);

exit:
    return status;
}


#if !defined(NDEBUG)
String StunAttributeErrorCode::ToString(void) const
{
    String oss;
    String reason;

    GetStr(reason);

    oss.append(StunAttribute::ToString());

    switch (error) {
    case STUN_ERR_CODE_TRY_ALTERNATE:
        oss.append(": TRY_ALTERNATE (");
        break;

    case STUN_ERR_CODE_BAD_REQUEST:
        oss.append(": BAD_REQUEST (");
        break;

    case STUN_ERR_CODE_UNAUTHORIZED:
        oss.append(": UNAUTHORIZED (");
        break;

    case STUN_ERR_CODE_UNKNOWN_ATTRIBUTE:
        oss.append(": UNKNOWN_ATTRIBUTE (");
        break;

    case STUN_ERR_CODE_SERVER_ERROR:
        oss.append(": SERVER_ERROR (");
        break;

    case STUN_ERR_CODE_FORBIDDEN:
        oss.append(": FORBIDDEN (");
        break;

    case STUN_ERR_CODE_ALLOCATION_MISMATCH:
        oss.append(": ALLOCATION_MISMATCH (");
        break;

    case STUN_ERR_CODE_WRONG_CREDENTIALS:
        oss.append(": WRONG_CREDENTIALS (");
        break;

    case STUN_ERR_CODE_UNSUPPORTED_TRANSPORT_PROTOCOL:
        oss.append(": UNSUPPORTED_TRANSPORT_PROTOCOL (");
        break;

    case STUN_ERR_CODE_ALLOCATION_QUOTA_REACHED:
        oss.append(": ALLOCATION_QUOTA_REACHED (");
        break;

    case STUN_ERR_CODE_INSUFFICIENT_CAPACITY:
        oss.append(": INSUFFICIENT_CAPACITY (");
        break;

    default:
        oss.append(": <Unknow error code> (");
    }

    oss.append(U32ToString(error, 10));
    oss.append("): ");
    oss.append(reason);

    return oss;
}
#endif
