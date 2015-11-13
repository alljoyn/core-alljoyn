/**
 * @file
 *
 * Define a class that abstracts mutexes.
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

Mutex::Mutex() : m_mutexInternal(new Internal)
{
}

Mutex::~Mutex()
{
    delete m_mutexInternal;
}

QStatus Mutex::Lock(const char* file, uint32_t line)
{
    return m_mutexInternal->Lock(file, line);
}

QStatus Mutex::Lock()
{
    return m_mutexInternal->Lock();
}

QStatus Mutex::Unlock(const char* file, uint32_t line)
{
    return m_mutexInternal->Unlock(file, line);
}

QStatus Mutex::Unlock()
{
    return m_mutexInternal->Unlock();
}

void Mutex::AssertOwnedByCurrentThread() const
{
    m_mutexInternal->AssertOwnedByCurrentThread();
}

Mutex::Internal::Internal()
{
#ifndef NDEBUG
    m_file = nullptr;
    m_line = static_cast<uint32_t>(-1);
    m_ownerThread = 0;
    m_recursionCount = 0;
#endif

    m_initialized = PlatformSpecificInit();
    QCC_ASSERT(m_initialized);
}

Mutex::Internal::~Internal()
{
    if (m_initialized) {
        PlatformSpecificDestroy();
        m_initialized = false;
    }
}

QStatus Mutex::Internal::Lock(const char* file, uint32_t line)
{
    QCC_ASSERT(m_initialized);

#ifdef NDEBUG
    QCC_UNUSED(file);
    QCC_UNUSED(line);
    return Lock();
#else
    QStatus status = Lock();
    if (status == ER_OK) {
        QCC_DbgPrintf(("Lock Acquired %s:%d", file, line));
        m_file = file;
        m_line = line;
    } else {
        QCC_LogError(status, ("Mutex::Lock %s:%u failed", file, line));
    }
    return status;
#endif
}

QStatus Mutex::Internal::Lock()
{
    QCC_ASSERT(m_initialized);
    if (!m_initialized) {
        return ER_INIT_FAILED;
    }

    QStatus status = PlatformSpecificLock();
    if (status == ER_OK) {
        LockAcquired();
    }

    return status;
}

QStatus Mutex::Internal::Unlock(const char* file, uint32_t line)
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

QStatus Mutex::Internal::Unlock()
{
    QCC_ASSERT(m_initialized);
    if (!m_initialized) {
        return ER_INIT_FAILED;
    }

    ReleasingLock();
    return PlatformSpecificUnlock();
}

bool Mutex::Internal::TryLock()
{
    QCC_ASSERT(m_initialized);
    if (!m_initialized) {
        return false;
    }

    bool locked = PlatformSpecificTryLock();
    if (locked) {
        LockAcquired();
    }
    return locked;
}

/**
 * Called immediately after current thread acquired this Mutex.
 */
void Mutex::Internal::LockAcquired()
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
#endif
}

/**
 * Called immediately before current thread releases this Mutex.
 */
void Mutex::Internal::ReleasingLock()
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
#endif
}

/**
 * Assert that current thread owns this Mutex.
 */
void Mutex::Internal::AssertOwnedByCurrentThread() const
{
#ifndef NDEBUG
    /* Use GetCurrentThreadId rather than GetThread, because GetThread acquires a Mutex */
    ThreadId currentThread = Thread::GetCurrentThreadId();
    QCC_ASSERT(currentThread != 0);
    QCC_ASSERT(m_ownerThread == currentThread);
    QCC_ASSERT(m_recursionCount != 0);
#endif
}
