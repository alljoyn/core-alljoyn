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

#include <qcc/MutexInternal.h>

namespace qcc {

#ifdef NDEBUG

Mutex::Internal::Internal()
{
}

void Mutex::Internal::LockAcquired()
{
}

void Mutex::Internal::ReleasingLock()
{
}

void Mutex::Internal::AssertOwnedByCurrentThread() const
{
}

#else /* NDEBUG */

Mutex::Internal::Internal() : m_ownerThread(0), m_recursionCount(0)
{
}

/**
 * Called immediately after current thread acquired this Mutex.
 */
void Mutex::Internal::LockAcquired()
{
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
}

/**
 * Called immediately before current thread releases this Mutex.
 */
void Mutex::Internal::ReleasingLock()
{
    /* Use GetCurrentThreadId rather than GetThread, because GetThread acquires a Mutex */
    ThreadId currentThread = Thread::GetCurrentThreadId();
    QCC_ASSERT(currentThread != 0);
    QCC_ASSERT(m_ownerThread == currentThread);
    QCC_ASSERT(m_recursionCount != 0);

    m_recursionCount--;

    if (m_recursionCount == 0) {
        m_ownerThread = 0;
    }
}

/**
 * Assert that current thread owns this Mutex.
 */
void Mutex::Internal::AssertOwnedByCurrentThread() const
{
    /* Use GetCurrentThreadId rather than GetThread, because GetThread acquires a Mutex */
    ThreadId currentThread = Thread::GetCurrentThreadId();
    QCC_ASSERT(currentThread != 0);
    QCC_ASSERT(m_ownerThread == currentThread);
    QCC_ASSERT(m_recursionCount != 0);
}

#endif /* NDEBUG */

} /* namespace */
