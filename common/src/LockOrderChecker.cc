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

/* Lock verification is enabled just on Debug builds */
#ifndef NDEBUG

#include <qcc/Debug.h>
#include <qcc/MutexInternal.h>
#include <qcc/LockLevel.h>
#include <qcc/LockOrderChecker.h>

#define QCC_MODULE "MUTEX"

namespace qcc {

/*
 * Default number of LockTrace objects allocated for each thread. Additional LockTrace
 * objects get allocated automatically if a thread acquires even more locks.
 */
const uint32_t LockOrderChecker::defaultMaximumStackDepth = 4;

/*
 * LOCKORDERCHECKER_OPTION_MISSING_LEVEL_ASSERT is disabled by default because
 * specifying lock level values from apps is not supported, but some of these
 * apps acquire their own locks during listener callbacks. Listener callbacks
 * commonly get called while owning one or more SCL locks.
 *
 * Another example of problematic MISSING_LEVEL_ASSERT behavior is: timer
 * callbacks get called with the reentrancy lock held, and they can go off
 * and execute app code.
 *
 * If you need to detect locks that don't have a proper level value:
 *  - Add the MISSING_LEVEL_ASSERT bit into LockOrderChecker::enabledOptions
 *  - Run your tests and look for failing assertions:
 *      - If an assertion points to a lock you care about, add a level
 *          value to that lock.
 *      - If an assertion points to a lock you want to ignore, mark that
 *          lock as LOCK_LEVEL_CHECKING_DISABLED.
 *  - Then re-run the tests, and repeat the above steps.
 */
int LockOrderChecker::enabledOptions = LOCKORDERCHECKER_OPTION_LOCK_ORDERING_ASSERT;

/**
 * Lock/Unlock file name is unknown on Release builds, and on Debug builds
 * if the caller didn't specify the MUTEX_CONTEXT parameter.
 */
const char* LockOrderChecker::s_unknownFile = nullptr;

/**
 * Lock/Unlock line number is unknown on Release builds, and on Debug builds
 * if the caller didn't specify the MUTEX_CONTEXT parameter.
 */
const uint32_t LockOrderChecker::s_unknownLineNumber = static_cast<uint32_t>(-1);

class LockOrderChecker::LockTrace {
  public:
    /* Default constructor */
    LockTrace() : m_lock(nullptr), m_level(LOCK_LEVEL_NOT_SPECIFIED), m_recursionCount(0)
    {
    }

    /* Copy constructor */
    LockTrace(const LockTrace& other)
    {
        *this = other;
    }

    /* Assignment operator */
    LockTrace& operator =(const LockTrace& other)
    {
        m_lock = other.m_lock;
        m_level = other.m_level;
        m_recursionCount = other.m_recursionCount;
        return *this;
    }

    /* Address of a lock acquired by current thread */
    const Mutex* m_lock;

    /*
     * Keep a copy of the lock's level here just in case someone decides to destroy
     * the lock while owning it, and therefore reaching inside the lock to get the
     * level would become incorrect.
     */
    LockLevel m_level;

    /* Number of times current thread acquired this lock, recursively */
    uint32_t m_recursionCount;
};

LockOrderChecker::LockOrderChecker()
    : m_currentDepth(0), m_maximumDepth(defaultMaximumStackDepth)
{
    m_lockStack = new LockTrace[m_maximumDepth];
}

LockOrderChecker::~LockOrderChecker()
{
    delete[] m_lockStack;
}

/*
 * Called when a thread is about to acquire a lock.
 *
 * @param lock    Lock being acquired by current thread.
 */
void LockOrderChecker::AcquiringLock(const Mutex* lock, const char* file, uint32_t line)
{
    /* Find the most-recently acquired lock that is being verified */
    bool foundRecentEntry = false;
    uint32_t recentEntry = m_currentDepth - 1;

    for (uint32_t stackEntry = 0; stackEntry < m_currentDepth; stackEntry++) {
        LockLevel previousLevel = m_lockStack[recentEntry].m_level;
        QCC_ASSERT(previousLevel != LOCK_LEVEL_CHECKING_DISABLED);

        if (previousLevel != LOCK_LEVEL_NOT_SPECIFIED) {
            foundRecentEntry = true;
            break;
        }

        recentEntry--;
    }

    /*
     * Nothing to check before this lock has been acquired if current thread doesn't
     * already own any other verified locks.
     */
    if (!foundRecentEntry) {
        return;
    }

    if (file == s_unknownFile) {
        /* Caller's location is unknown, so try to at least point to the previous owner of this lock */
        file = MutexInternal::GetLatestOwnerFileName(*lock);
    }
    if (line == s_unknownLineNumber) {
        /* Caller's location is unknown, so try to at least point to the previous owner of this lock */
        line = MutexInternal::GetLatestOwnerLineNumber(*lock);
    }

    LockLevel previousLevel = m_lockStack[recentEntry].m_level;
    LockLevel lockLevel = MutexInternal::GetLevel(*lock);
    QCC_ASSERT(lockLevel != LOCK_LEVEL_CHECKING_DISABLED);

    if (lockLevel == LOCK_LEVEL_NOT_SPECIFIED) {
        if (enabledOptions & LOCKORDERCHECKER_OPTION_MISSING_LEVEL_ASSERT) {
            LockTrace& previousTrace = m_lockStack[recentEntry];
            fprintf(stderr,
                    "Acquiring lock %p with unspecified level (%s:%d). Current thread already owns lock %p level %d (%s:%d).\n",
                    lock,
                    (file != nullptr) ? file : "unknown file",
                    line,
                    previousTrace.m_lock,
                    previousTrace.m_level,
                    MutexInternal::GetLatestOwnerFileName(*previousTrace.m_lock) ? MutexInternal::GetLatestOwnerFileName(*previousTrace.m_lock) : "unknown file",
                    MutexInternal::GetLatestOwnerLineNumber(*previousTrace.m_lock));
            fflush(stderr);
            QCC_ASSERT(false && "Please add a valid level to the lock being acquired");
        }
        return;
    }

    if (lockLevel >= previousLevel) {
        /* The order of acquiring this lock is correct */
        return;
    }

    /*
     * Check if current thread already owns this lock.
     */
    bool previouslyLocked = false;
    for (uint32_t stackEntry = 0; stackEntry < recentEntry; stackEntry++) {
        QCC_ASSERT(m_lockStack[stackEntry].m_level != LOCK_LEVEL_CHECKING_DISABLED);

        if (m_lockStack[stackEntry].m_lock == lock) {
            previouslyLocked = true;
            break;
        }
    }

    if (!previouslyLocked && (enabledOptions & LOCKORDERCHECKER_OPTION_LOCK_ORDERING_ASSERT)) {
        LockTrace& previousTrace = m_lockStack[recentEntry];
        fprintf(stderr,
                "Acquiring lock %p level %d (%s:%d). Current thread already owns lock %p level %d (%s:%d).\n",
                lock,
                lockLevel,
                (file != nullptr) ? file : "unknown file",
                line,
                previousTrace.m_lock,
                previousTrace.m_level,
                MutexInternal::GetLatestOwnerFileName(*previousTrace.m_lock) ? MutexInternal::GetLatestOwnerFileName(*previousTrace.m_lock) : "unknown file",
                MutexInternal::GetLatestOwnerLineNumber(*previousTrace.m_lock));
        fflush(stderr);
        QCC_ASSERT(false && "Detected out-of-order lock acquire");
    }
}

/*
 * Called when a thread has just acquired a lock.
 *
 * @param lock    Lock that has just been acquired by current thread.
 */
void LockOrderChecker::LockAcquired(const Mutex* lock)
{
    LockLevel lockLevel = MutexInternal::GetLevel(*lock);
    QCC_ASSERT(lockLevel != LOCK_LEVEL_CHECKING_DISABLED);

    /* Check if current thread already owns this lock */
    bool previouslyLocked = false;
    for (uint32_t stackEntry = 0; stackEntry < m_currentDepth; stackEntry++) {
        QCC_ASSERT(m_lockStack[stackEntry].m_level != LOCK_LEVEL_CHECKING_DISABLED);

        if (m_lockStack[stackEntry].m_lock == lock) {
            QCC_ASSERT(m_lockStack[stackEntry].m_level == lockLevel);
            QCC_ASSERT(m_lockStack[stackEntry].m_recursionCount > 0);
            m_lockStack[stackEntry].m_recursionCount++;
            previouslyLocked = true;
            break;
        }
    }

    if (previouslyLocked) {
        /* Lock is already present on the stack */
        return;
    }

    /* Add this lock to the stack */
    QCC_ASSERT(m_currentDepth <= m_maximumDepth);

    if (m_currentDepth == m_maximumDepth) {
        /* Grow the stack */
        uint32_t newDepth = m_maximumDepth + 1;
        LockTrace* newStack = new LockTrace[newDepth];
        QCC_ASSERT(newStack != nullptr);

        for (uint32_t stackEntry = 0; stackEntry < m_maximumDepth; stackEntry++) {
            newStack[stackEntry] = m_lockStack[stackEntry];
        }

        delete[] m_lockStack;
        m_lockStack = newStack;
        m_maximumDepth = newDepth;
    }

    m_lockStack[m_currentDepth].m_lock = lock;
    m_lockStack[m_currentDepth].m_level = lockLevel;
    m_lockStack[m_currentDepth].m_recursionCount = 1;
    m_currentDepth++;
}

/*
 * Called when a thread is about to release a lock.
 *
 * @param lock    Lock being released by current thread.
 */
void LockOrderChecker::ReleasingLock(const Mutex* lock)
{
    LockLevel lockLevel = MutexInternal::GetLevel(*lock);
    QCC_ASSERT(lockLevel != LOCK_LEVEL_CHECKING_DISABLED);

    /* Check if current thread owns this lock */
    bool previouslyLocked = false;
    for (uint32_t stackEntry1 = 0; stackEntry1 < m_currentDepth; stackEntry1++) {
        QCC_ASSERT(m_lockStack[stackEntry1].m_level != LOCK_LEVEL_CHECKING_DISABLED);

        if (m_lockStack[stackEntry1].m_lock == lock) {
            QCC_ASSERT(m_lockStack[stackEntry1].m_recursionCount > 0);
            m_lockStack[stackEntry1].m_recursionCount--;

            if (m_lockStack[stackEntry1].m_recursionCount == 0) {
                /* Current thread will no longer own this lock, so remove it from the stack */
                for (uint32_t stackEntry2 = stackEntry1; stackEntry2 < (m_currentDepth - 1); stackEntry2++) {
                    m_lockStack[stackEntry2] = m_lockStack[stackEntry2 + 1];
                }
                m_currentDepth--;
            }

            previouslyLocked = true;
            break;
        }
    }

    if (!previouslyLocked) {
        fprintf(stderr, "Current thread doesn't own lock %p level %d.\n", lock, lockLevel);
        fflush(stderr);
        QCC_ASSERT(false && "Current thread doesn't own the lock it is trying to release");
    }
}

} /* namespace */

#endif  /* #ifndef NDEBUG */
