/**
 * @file StringStream.cc
 *
 * Sink/Source wrapper for std::string
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/Event.h>
#include <qcc/Pipe.h>
#include <qcc/Stream.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "STREAM"

QStatus Pipe::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    static const size_t cleanupSize = 4096;

    QStatus status = ER_OK;
    char* _buf = (char*)buf;

    /* Pipe has no network delay so doesn't need long timeouts */
    if (timeout != Event::WAIT_FOREVER) {
        timeout = min(timeout, (uint32_t)5);
    }

    lock.Lock();
    while ((ER_OK == status) && (0 < reqBytes)) {
        size_t b = min(str.size() - outIdx, reqBytes);
        if (0 < b) {
            memcpy(_buf, str.data() + outIdx, b);
            _buf += b;
            outIdx += b;
            reqBytes -= b;
        }
        /* If we haven't read anything yet block until data is available */
        if ((0 < reqBytes) && (_buf == (char*)buf)) {
            isWaiting = true;
            lock.Unlock();
            status = Event::Wait(event, timeout);
            lock.Lock();
            event.ResetEvent();
        } else {
            break;
        }
    }

    /* Perform clean-up if source is completely used up or if outIdx gets too large */
    if (outIdx >= str.size()) {
        str.clear();
        outIdx = 0;
    } else if (outIdx >= cleanupSize) {
        str = str.substr(outIdx);
        outIdx = 0;
    }
    lock.Unlock();

    actualBytes = _buf - (char*)buf;
    return status;
}

QStatus Pipe::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    QStatus status = ER_OK;

    str.append((const char*)buf, numBytes);
    numSent = numBytes;

    lock.Lock();
    if (isWaiting) {
        isWaiting = false;
        status = event.SetEvent();
    }
    lock.Unlock();

    return status;
}


