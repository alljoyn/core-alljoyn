/**
 * @file
 *
 * Define a class that abstracts WinRT semaphores.
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
#ifndef _OS_QCC_SEMAPHORE_H
#define _OS_QCC_SEMAPHORE_H

#include <windows.h>
#include <qcc/platform.h>
#include <Status.h>

namespace qcc {

/**
 * The Windows implementation of a Semaphore abstraction class.
 */
class Semaphore {
  public:

    /**
     * Constructor
     */
    Semaphore();

    /**
     * Destructor
     */
    ~Semaphore();

    /**
     * Terminates the semaphore and unblocks any waiters
     */
    void Close();

    /**
     * Initializes the default state of the semaphore.
     *
     * @return  ER_OK if the initialization was successful, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Init(int32_t initial, int32_t maximum);

    /**
     * Blocks the currently executing thread until a resource can be
     * acquired.
     *
     * @return  ER_OK if the lock was acquired, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Wait(void);

    /**
     * Adds a resource to the semaphore. A blocking thread will return if
     * waiting on a resource.
     *
     * @return  ER_OK if the lock was acquired, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Release(void);

    /**
     * Resets the semaphore state to the default values specfied during Init()
     *
     * @return  ER_OK if the lock was acquired, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Reset(void);

  private:
    bool _initialized;
    int32_t _initial;
    int32_t _maximum;
    HANDLE _semaphore;
};

} /* namespace */

#endif
