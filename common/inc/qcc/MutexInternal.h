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

namespace qcc {

/**
 * Represents the non-public functionality of the Mutex class.
 */
class Mutex::Internal {
public:
    /**
     * Constructor.
     */
    Internal();
    
    /**
     * Called immediately after current thread acquired this Mutex.
     */
    void LockAcquired();

    /**
     * Called immediately before current thread releases this Mutex.
     */
    void ReleasingLock();

    /**
     * Assert that current thread owns this Mutex.
     */
    void AssertOwnedByCurrentThread() const;

private:
    /* Copy constructor is private */
    Internal(const Internal& other);

    /* Assignment operator is private */
    Internal& operator=(const Internal& other);

#ifndef NDEBUG
    /* Mutex owner thread ID */
    ThreadId m_ownerThread;

    /* How many times this Mutex has been acquired by its current owner thread */
    uint32_t m_recursionCount;
#endif 
};

} /* namespace */

#endif /* _QCC_MUTEXINTERNAL_H */
