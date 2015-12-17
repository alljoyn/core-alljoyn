/**
 * @file
 *
 * Internal functionality of the Mutex class.
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
#ifndef _QCC_MUTEXINTERNAL_H
#define _QCC_MUTEXINTERNAL_H

#include <qcc/Thread.h>
#include <qcc/LockLevel.h>

namespace qcc {

#if defined(QCC_OS_GROUP_POSIX)

typedef pthread_mutex_t QccPlatformSpecificMutex;

#elif defined(QCC_OS_GROUP_WINDOWS)

typedef CRITICAL_SECTION QccPlatformSpecificMutex;

#else
#error No OS GROUP defined.
#endif

/**
 * Represents the non-public functionality of the Mutex class.
 */
class MutexInternal {
public:
    /**
     * Constructor.
     *
     * @param mutex Pointer back to the mutex object that owns this MutexInternal object.
     * @param level Lock level used on Debug builds to detect out-of-order lock acquires.
     */
    MutexInternal(Mutex *mutex, int level);

    /**
     * Destructor.
     */
    ~MutexInternal();

    /**
     * Acquires a lock on the mutex.  If another thread is holding the lock,
     * then this function will block until the other thread has released its
     * lock.
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
     * @return
     * - #ER_OK if the lock was acquired.
     * - #ER_OS_ERROR if the underlying OS reports an error.
     */
    QStatus Lock();

    /**
     * Releases a lock on the mutex.  This will only release a lock for the
     * current thread if that thread was the one that aquired the lock in the
     * first place.
     *
     * @param file the name of the file this Mutex unlock was called from
     * @param line the line number of the file this Mutex unlock was called from
     *
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
     * @return
     * - #ER_OK if the lock was acquired.
     * - #ER_OS_ERROR if the underlying OS reports an error.
     */
    QStatus Unlock();

    /**
     * Attempt to acquire a lock on a mutex. If another thread is holding the lock
     * this function return false otherwise the lock is acquired and the function returns true.
     *
     * @return  True if the lock was acquired.
     */
    bool TryLock();

    /**
     * Assert that current thread owns this Mutex.
     */
    void AssertOwnedByCurrentThread() const;

    /* Called immediately before current thread tries to acquire this Mutex */
    void AcquiringLock();

    /* Called immediately after current thread acquired this Mutex */
    void LockAcquired();

    /* Called immediately before current thread releases this Mutex */
    void ReleasingLock();

    QccPlatformSpecificMutex *GetPlatformSpecificMutex() { return &m_mutex; }

#ifndef NDEBUG
    LockLevel GetLevel() { return m_level; }
    const char* GetLatestOwnerFileName() { return m_file; }
    uint32_t GetLatestOwnerLineNumber() { return m_line; }
#endif 

private:
    bool PlatformSpecificInit();
    void PlatformSpecificDestroy();
    QStatus PlatformSpecificLock();
    QStatus PlatformSpecificUnlock();
    bool PlatformSpecificTryLock();

    /* Copy constructor is private */
    MutexInternal(const MutexInternal& other);

    /* Assignment operator is private */
    MutexInternal& operator=(const MutexInternal& other);

    /* Underlying platform-specific lock */
    QccPlatformSpecificMutex m_mutex;

    /* true if mutex was successfully initialized */
    bool m_initialized;

#ifndef NDEBUG
    /* Pointer back to the mutex object that owns this MutexInternal object */
    Mutex *m_ownerLock;

    /* Source code file name where this Mutex has been acquired */
    const char* m_file;

    /* Source code line number where this Mutex has been acquired */
    uint32_t m_line;

    /* Mutex owner thread ID */
    ThreadId m_ownerThread;

    /* How many times this Mutex has been acquired by its current owner thread */
    uint32_t m_recursionCount;

    /* If the m_level value is not 0 or -1, lock order verification is enabled for this lock */
    LockLevel m_level;
#endif
};

} /* namespace */

#endif /* _QCC_MUTEXINTERNAL_H */
