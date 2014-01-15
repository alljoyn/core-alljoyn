/**
 * @file
 *
 * This file implements the STUN Attribute Generic Address base class
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
#include <qcc/Socket.h>
#include <alljoyn/Status.h>
#include <StunTransactionID.h>

using namespace qcc;

#define QCC_MODULE "STUN_TRANSACTION_ID"

QStatus StunTransactionID::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("StunTransactionID::Parse(*buf, bufSize = %u)", bufSize));

    if (bufSize < Size()) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Parsing Transaction (missing %u)", Size() - bufSize));
        goto exit;
    }

    memcpy(id, buf, sizeof(id));

    buf += Size();
    bufSize -= Size();

exit:
    return status;
}


QStatus StunTransactionID::RenderBinary(uint8_t*& buf, size_t& bufSize,
                                        ScatterGatherList& sg) const
{
    memcpy(buf, id, sizeof(id));

    sg.AddBuffer(&buf[0], Size());
    sg.IncDataSize(Size());

    buf += Size();
    bufSize -= Size();

    return ER_OK;
}

String StunTransactionID::ToString(void) const
{
    if (value.empty()) {
        value = BytesToHexString(id, SIZE, true);
    }
    return value;
}

void StunTransactionID::SetValue(StunTransactionID& other)
{
    memcpy(id, other.id, sizeof(id));
    value.clear();
}



