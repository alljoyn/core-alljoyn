/**
 * @file
 *
 * Define a class that abstracts Linux rwlock's.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#ifndef _OS_QCC_RWLOCK_H
#define _OS_QCC_RWLOCK_H

#include <qcc/platform.h>

#include <pthread.h>

#include <Status.h>

namespace qcc {

/**
 * The Linux implementation of a Rwlock abstraction class.
 */
class RWLock {

  public:
    /**
     * The constructor initializes the underlying rwlock implementation.
     */
    RWLock() { Init(); }

    /**
     * The destructor will destroy the underlying rwlock.
     */
    ~RWLock();

    /**
     * Acquires a lock on the rwlock.  If another thread is holding the lock,
     * then this function will block until the other thread has released its
     * lock.
     *
     * @return  ER_OK if the lock was acquired, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus RDLock();
    QStatus WRLock();

    /**
     * Releases a lock on the rwlock.  This will only release a lock for the
     * current thread if that thread was the one that aquired the lock in the
     * first place.
     *
     * @return  ER_OK if the lock was released, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Unlock();

    /**
     * Attempt to acquire a lock on a rwlock. If another thread is holding the lock
     * this function return false otherwise the lock is acquired and the function returns true.
     *
     * @return  True if the lock was acquired.
     */
    bool TryRDLock();
    bool TryWRLock();

    /**
     * Rwlock copy constructor creates a new rwlock.
     */
    RWLock(const RWLock& other) { QCC_UNUSED(other); Init(); }

    /**
     * Rwlock assignment operator.
     */
    RWLock& operator=(const RWLock& other) { QCC_UNUSED(other); Init(); return *this; }

  private:
    pthread_rwlock_t rwlock;  ///< The Linux rwlock implementation uses pthread rwlock's.
    bool isInitialized;     ///< true iff rwlock was successfully initialized.
    void Init();            ///< Initialize underlying OS rwlock
};

} /* namespace */

#endif
