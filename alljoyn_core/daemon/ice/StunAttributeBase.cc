/**
 * @file
 *
 * This file implements the STUN Attribute base class
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
#include <StunAttribute.h>
#include <alljoyn/Status.h>

using namespace qcc;

#define QCC_MODULE "STUN_ATTRIBUTE"

QStatus StunAttribute::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status = ER_OK;

    uint16_t attrSize = AttrSize();

    QCC_DbgTrace(("StunAttribute::RenderBinary(buf, bufSize = %u, sg) [%s: %u/%u]", bufSize, name, RenderSize(), AttrSize()));

    assert(!parsed);

    if (bufSize < RenderSize()) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Rendering %s attribute (%u short)", name, RenderSize() - bufSize));
        goto exit;
    }

    WriteHostToNet(buf, bufSize, static_cast<uint16_t>(type), sg);
    WriteHostToNet(buf, bufSize, static_cast<uint16_t>(attrSize), sg);

exit:
    return status;
}
