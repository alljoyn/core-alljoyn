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
#include <qcc/platform.h>
#include <qcc/Mutex.h>
#include <qcc/MutexInternal.h>
#include <qcc/Debug.h>

#define QCC_MODULE "MUTEX"

using namespace qcc;

MutexInternal::MutexInternal(Mutex* ownerLock, int level)
{
#ifdef NDEBUG
    QCC_UNUSED(ownerLock);
    QCC_UNUSED(level);
#else
    m_file = nullptr;
    m_line = static_cast<uint32_t>(-1);
    m_ownerThread = 0;
    m_recursionCount = 0;
    m_level = static_cast<LockLevel>(level);
    m_ownerLock = ownerLock;
#endif

    m_initialized = PlatformSpecificInit();
    QCC_ASSERT(m_initialized);
}

MutexInternal::~MutexInternal()
{
    if (m_initialized) {
        PlatformSpecificDestroy();
        m_initialized = false;
    }
}

QStatus MutexInternal::Lock(const char* file, uint32_t line)
{
    QCC_ASSERT(m_initialized);

#ifdef NDEBUG
    QCC_UNUSED(file);
    QCC_UNUSED(line);
    return Lock();
#else
    // DMFIX
    m_file = file;
    m_line = line;
    // DMFIX

    QStatus status = Lock();
    if (status == ER_OK) {
        QCC_DbgPrintf(("Lock Acquired %s:%d", file, line));
        /* DMFIX
           m_file = file;
           m_line = line;
         */
    } else {
        QCC_LogError(status, ("Mutex::Lock %s:%u failed", file, line));
    }
    return status;
#endif
}

QStatus MutexInternal::Lock()
{
    QCC_ASSERT(m_initialized);
    if (!m_initialized) {
        return ER_INIT_FAILED;
    }

    AcquiringLock();
    QStatus status = PlatformSpecificLock();
    if (status == ER_OK) {
        LockAcquired();
    }

    return status;
}

QStatus MutexInternal::Unlock(const char* file, uint32_t line)
{
    QCC_ASSERT(m_initialized);

#ifdef NDEBUG
    QCC_UNUSED(file);
    QCC_UNUSED(line);
#else
    QCC_DbgPrintf(("Lock Released: %s:%u (acquired at %s:%u)", file, line, m_file, m_line));
    m_file = file;
    m_line = line;
#endif

    return Unlock();
}

QStatus MutexInternal::Unlock()
{
    QCC_ASSERT(m_initialized);
    if (!m_initialized) {
        return ER_INIT_FAILED;
    }

    ReleasingLock();
    return PlatformSpecificUnlock();
}

bool MutexInternal::TryLock()
{
    QCC_ASSERT(m_initialized);
    if (!m_initialized) {
        return false;
    }

    AcquiringLock();
    bool locked = PlatformSpecificTryLock();
    if (locked) {
        LockAcquired();
    }
    return locked;
}

/**
 * Called immediately before current thread tries to acquire this Mutex.
 */
void MutexInternal::AcquiringLock()
{
#ifndef NDEBUG
    /*
     * Perform lock order verification. Test LOCK_LEVEL_CHECKING_DISABLED before calling
     * GetThread, because GetThread uses a LOCK_LEVEL_CHECKING_DISABLED mutex internally.
     */
    if (Thread::initialized && m_level != LOCK_LEVEL_CHECKING_DISABLED) {
        Thread::GetThread()->lockChecker.AcquiringLock(m_ownerLock);
    }
#endif
}

/**
 * Called immediately after current thread acquired this Mutex.
 */
void MutexInternal::LockAcquired()
{
#ifndef NDEBUG
    /* Use GetCurrentThreadId rather than GetThread, because GetThread acquires a Mutex */
    ThreadId currentThread = Thread::GetCurrentThreadId();
    QCC_ASSERT(currentThread != 0);

    if (m_ownerThread == currentThread) {
        QCC_ASSERT(m_recursionCount != 0);
    } else {
        QCC_ASSERT(m_ownerThread == 0);
        QCC_ASSERT(m_recursionCount == 0);
        m_ownerThread = currentThread;
    }

    m_recursionCount++;

    /*
     * Perform lock order verification. Test LOCK_LEVEL_CHECKING_DISABLED before calling
     * GetThread, because GetThread uses a LOCK_LEVEL_CHECKING_DISABLED mutex internally.
     */
    if (Thread::initialized && m_level != LOCK_LEVEL_CHECKING_DISABLED) {
        Thread::GetThread()->lockChecker.LockAcquired(m_ownerLock);
    }
#endif
}

void MutexInternal::LockAcquired(Mutex& lock)
{
    lock.m_mutexInternal->LockAcquired();
}

/**
 * Called immediately before current thread releases this Mutex.
 */
void MutexInternal::ReleasingLock()
{
#ifndef NDEBUG
    /* Use GetCurrentThreadId rather than GetThread, because GetThread acquires a Mutex */
    ThreadId currentThread = Thread::GetCurrentThreadId();
    QCC_ASSERT(currentThread != 0);
    QCC_ASSERT(m_ownerThread == currentThread);
    QCC_ASSERT(m_recursionCount != 0);

    m_recursionCount--;

    if (m_recursionCount == 0) {
        m_ownerThread = 0;
    }

    /*
     * Perform lock order verification. Test LOCK_LEVEL_CHECKING_DISABLED before calling
     * GetThread, because GetThread uses a LOCK_LEVEL_CHECKING_DISABLED mutex internally.
     */
    if (Thread::initialized && m_level != LOCK_LEVEL_CHECKING_DISABLED) {
        Thread::GetThread()->lockChecker.ReleasingLock(m_ownerLock);
    }
#endif
}

void MutexInternal::ReleasingLock(Mutex& lock)
{
    lock.m_mutexInternal->ReleasingLock();
}

/**
 * Assert that current thread owns this Mutex.
 */
void MutexInternal::AssertOwnedByCurrentThread() const
{
#ifndef NDEBUG
    /* Use GetCurrentThreadId rather than GetThread, because GetThread acquires a Mutex */
    ThreadId currentThread = Thread::GetCurrentThreadId();
    QCC_ASSERT(currentThread != 0);
    QCC_ASSERT(m_ownerThread == currentThread);
    QCC_ASSERT(m_recursionCount != 0);
#endif
}

/**
 * Set the level value for locks that cannot get a proper level from their constructor -
 * because an entire array of locks has been allocated (e.g. locks = new Mutex[numLocks];).
 */
#ifndef NDEBUG
void MutexInternal::SetLevel(Mutex& lock, LockLevel level)
{
    QCC_ASSERT(lock.m_mutexInternal->m_level == LOCK_LEVEL_NOT_SPECIFIED);
    QCC_ASSERT(level != LOCK_LEVEL_NOT_SPECIFIED);
    lock.m_mutexInternal->m_level = level;
}
#endif
