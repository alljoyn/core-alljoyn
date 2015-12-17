/**
 * @file
 *
 * Class implementing sanity checks for Mutex objects.
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

#ifndef NDEBUG
#ifndef _QCC_LOCKORDERCHECKER_H
#define _QCC_LOCKORDERCHECKER_H

#include <qcc/platform.h>
#include <qcc/Mutex.h>
#include "LockLevel.h"

namespace qcc {

/**
 * Class implementing lock order verification for Mutex objects. Each Thread object
 * has an associated LockOrderChecker object on Debug builds.
 */
class LockOrderChecker {
  public:
    /* Default constructor */
    LockOrderChecker();

    /* Destructor */
    ~LockOrderChecker();

    /**
     * Called when a thread is about to acquire a lock.
     *
     * @param lock    Lock being acquired by current thread.
     */
    void AcquiringLock(const Mutex* lock);

    /**
     * Called when a thread has just acquired a lock.
     *
     * @param lock    Lock that has just been acquired by current thread.
     */
    void LockAcquired(Mutex* lock);

    /**
     * Called when a thread is about to release a lock.
     *
     * @param lock    Lock being released by current thread.
     */
    void ReleasingLock(const Mutex* lock);

  private:

    /**
     * @internal
     * Keep track of the locks acquired by current thread using a stack of LockTrace objects.
     * The goal is to always have this stack ordered by the lock level values - i.e. to acquire
     * locks in a well-defined order. This stack is implemented using a simple array instead of
     * a STL collection to make parsing the stack in a debugger easier.
     */
    uint32_t m_currentDepth;
    uint32_t m_maximumDepth;
    class LockTrace;
    LockTrace* m_lockStack;

    /**
     * @internal
     * Bit masks of lock verification options.
     */
    typedef enum {
        /* Trigger an assertion failed when locks are being used out of order */
        LOCKORDERCHECKER_OPTION_LOCK_ORDERING_ASSERT         = 0x1,

        /*
         * Trigger an assertion failed when a lock that IS NOT being verified
         * is aquired after a lock that IS being verified. The fix for this
         * kind of problem is to specify an apppropriate lock level value,
         * instead of the default value LOCK_LEVEL_NOT_SPECIFIED, as parameter
         * to the constructor of the lock that is not being verified yet.
         */
        LOCKORDERCHECKER_OPTION_MISSING_LEVEL_ASSERT         = 0x2,

    } LockOrderCheckerOptionBits;

    /**
     * @internal
     * Combination of LockOrderCheckerOptionBits, corresponding to LockOrderChecker features that are
     * currently enabled.
     */
    static int enabledOptions;

    /**
     * @internal
     * Initial lock stack maximum depth. The lock stack grows automatically if the
     * number of locks owned by a thread at a given time is larger than this default
     * maximum depth.
     */
    static const uint32_t defaultMaximumStackDepth;

    /* Copy constructor is private */
    LockOrderChecker(const LockOrderChecker& other);

    /* Assignment operator is private */
    LockOrderChecker& operator=(const LockOrderChecker& other);
};

} /* namespace */

#endif  /* _QCC_LOCKORDERCHECKER_H */
#endif  /* NDEBUG */
