/**
 * @file
 *
 * Define a class that abstracts Windows mutexs.
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
#ifndef _OS_QCC_MUTEX_H
#define _OS_QCC_MUTEX_H

#include <qcc/platform.h>

#include <qcc/String.h>

#include <alljoyn/Status.h>

namespace qcc {

/**
 * a macro that can be passed into the Mutex::Lock/Unlock member functions to
 * help when debugging Mutex related issues. When running in debug mode, this
 * will cause the code to log the name of the file and the line number of that
 * file each time a Mutex lock is obtained and released. Logging must be turned
 * on to see this information.
 */
#ifndef NDEBUG
#define MUTEX_CONTEXT __FILE__, __LINE__
#else
#define MUTEX_CONTEXT
#endif

/**
 * The Windows implementation of a Mutex abstraction class.
 */
class Mutex {
  public:

    /**
     * Constructor
     */
    Mutex() : initialized(false) { Init(); }

    /**
     * Destructor
     */
    ~Mutex();

    /**
     * Acquires a lock on the mutex.  If another thread is holding the lock,
     * then this function will block until the other thread has released its
     * lock.
     *
     * NOTE: Best practice is to call `Mutex::Lock(MUTEX_CONTEXT)`
     *
     * @see MUTEX_CONTEXT
     *
     * @param file the name of the file this Mutex lock was called from
     * @param line the line number of the file this Mutex lock was called from
     *
     * @return
     *  - #ER_OK if the lock was acquired.
     *  - #ER_OS_ERROR if the underlying OS reports an error.
     */
    QStatus Lock(const char* file, uint32_t line);

    /**
     * Acquires a lock on the mutex.  If another thread is holding the lock,
     * then this function will block until the other thread has released its
     * lock.
     *
     * It is not safe to use static global data from within this function.
     *
     * @return
     *  - #ER_OK if the lock was acquired.
     *  - #ER_OS_ERROR if the underlying OS reports an error.
     */
    QStatus Lock(void);

    /**
     * Releases a lock on the mutex.  This will only release a lock for the
     * current thread if that thread was the one that aquired the lock in the
     * first place.
     *
     * NOTE: Best practice is to call `Mutex::Unlock(MUTEX_CONTEXT)`
     *
     * @see MUTEX_CONTEXT
     *
     * @param file the name of the file this Mutex unlock was called from
     * @param line the line number of the file this Mutex unlock was called from
     * @return
     *  - #ER_OK if the lock was acquired.
     *  - #ER_OS_ERROR if the underlying OS reports an error.
     */
    QStatus Unlock(const char* file, uint32_t line);

    /**
     * Releases a lock on the mutex.  This will only release a lock for the
     * current thread if that thread was the one that aquired the lock in the
     * first place.
     *
     * It is not safe to use static global data from within this function.
     *
     * @return
     *  - #ER_OK if the lock was acquired.
     *  - #ER_OS_ERROR if the underlying OS reports an error.
     */
    QStatus Unlock(void);

    /**
     * Attempt to acquire a lock on a mutex. If another thread is holding the lock
     * this function return false otherwise the lock is acquired and the function returns true.
     *
     * @return  True if the lock was acquired.
     */
    bool TryLock();

    /**
     * Mutex copy constructor creates a new mutex.
     */
    Mutex(const Mutex&) { Init(); }

    /**
     * Mutex assignment operator.
     */
    Mutex& operator=(const Mutex&) { Init(); return *this; }

  private:
    bool initialized;
    CRITICAL_SECTION mutex; ///< Mutex variable.
    void Init();            ///< initialize a mutex

    /**
     * Give the condition variable class access to the underlying critical
     * section so it can get the private CRITICAL_SECTION out of a qcc::Mutex
     * and use the Windows condition variable functions directly.
     */
    friend class Condition;

};

} /* namespace */

#endif
