/**
 * @file
 *
 * Source wrapper for std::string
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <cstring>

#include <qcc/Stream.h>
#include <qcc/StringSource.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "STREAM"

QStatus StringSource::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    QStatus status = ER_OK;
    actualBytes = (std::min)(reqBytes, str.size() - outIdx);
    if (0 < actualBytes) {
        memcpy(buf, str.data() + outIdx, actualBytes);
        outIdx += actualBytes;
    } else if (outIdx == str.size()) {
        status = ER_EOF;
    }
    return status;
}


