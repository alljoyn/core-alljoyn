/**
 * @file
 *
 * Define a class that abstracts WinRT mutexes.
 */

/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#include <qcc/CountDownLatch.h>
#include <qcc/atomic.h>

/** @internal */
#define QCC_MODULE "COUNTDOWNLATCH"

using namespace qcc;

_CountDownLatch::_CountDownLatch() : _count(0)
{
    _evt.SetEvent();
}

_CountDownLatch::~_CountDownLatch()
{
}

QStatus _CountDownLatch::Wait(void)
{
    return qcc::Event::Wait(_evt, (uint32_t)-1);
}

int32_t _CountDownLatch::Current(void)
{
    return _count;
}

int32_t _CountDownLatch::Increment(void)
{
    int32_t val = qcc::IncrementAndFetch(&_count);
    if (val == 1) {
        // 0 -> 1
        _evt.ResetEvent();
    }
    return val;
}

int32_t _CountDownLatch::Decrement(void)
{
    int32_t val = qcc::DecrementAndFetch(&_count);
    if (val == 0) {
        // 1 -> 0
        _evt.SetEvent();
    }
    return val;
}
