/**
 * @file
 *
 * Define a class that abstracts WinRT count down latches.
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
#ifndef _OS__QCC_COUNTDOWNLATCH_H
#define _OS__QCC_COUNTDOWNLATCH_H

#include <windows.h>
#include <qcc/platform.h>
#include <qcc/Event.h>
#include <qcc/ManagedObj.h>
#include <Status.h>

namespace qcc {

/**
 * The WinRT implementation of a count down latch abstraction class.
 */
class _CountDownLatch {
  public:

    /**
     * Constructor
     */
    _CountDownLatch();

    /**
     * Destructor
     */
    ~_CountDownLatch();

    /**
     * Blocks the currently executing thread while the latch count
     * is non-zero.
     */
    QStatus Wait(void);

    /**
     * Gets the current latch count
     *
     * @return  The current latch count.
     */
    int32_t Current(void);

    /**
     * Gets the current latch count
     *
     * @return  The post incremented latch count.
     */
    int32_t Increment(void);

    /**
     * Gets the current latch count
     *
     * @return  The post decremeneted latch count.
     */
    int32_t Decrement(void);

  private:
    volatile int32_t _count;
    qcc::Event _evt;
};

/**
 * CountDownLatch is a reference counted (managed) version of _CountDownLatch
 */
typedef qcc::ManagedObj<_CountDownLatch> CountDownLatch;

} /* namespace */

#endif
