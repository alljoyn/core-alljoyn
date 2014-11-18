/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include <qcc/Condition.h>
#include <qcc/Mutex.h>

/**
 * \class Semaphore
 * \brief Implementation of a semaphore using a condition variable and a mutex.
 */
class Semaphore {
  public:
    /**
     * Construct a semaphore with an initial value of 0.
     */
    Semaphore();

    /**
     * Construct a semaphore with an initial value of \a initial.
     *
     * \param[in] initial The initial value for the semaphore.
     */
    Semaphore(unsigned int initial);

    /**
     * Destruct the semaphore.  Before calling this ensure that no one is
     * waiting on it anymore.
     */
    ~Semaphore();

    /**
     * Increment the value and signal all pending threads.  One will get
     * unblocked.
     *
     * \retval ER_OK on success
     * \retval other on failure.
     */
    QStatus Post();

    /**
     * Wait for the value to become positive and decrement it if it does.  Only
     * a single thread will be able to decrement the value and become
     * unblocked.
     *
     * \retval ER_OK on success
     * \retval other on failure.
     */
    QStatus Wait();

    /**
     * Wait a maximum amount of milliseconds for the value to become positive
     * and decrement it if it does.  Only a single thread will be able to
     * decrement the value and become unblocked.
     *
     * \param[in] ms The maximum number of milliseconds to wait.
     *
     * \retval ER_OK on success
     * \retval ER_TIMEOUT if not able to decrement in specified time period
     * \retval other on failure.
     */
    QStatus TimedWait(uint32_t ms);

  private:
    unsigned int value;
    qcc::Condition cond;
    qcc::Mutex mutex;

    // prevent copy by construction or assignment
    Semaphore(const Semaphore& other);
    Semaphore& operator=(const Semaphore& other);
};

#endif /* SEMAPHORE_H_ */
