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

#include <gtest/gtest.h>
#include <qcc/LockChecker.h>

using namespace qcc;

void AcquireAllLocks(LockChecker& lockChecker, Mutex** locks, uint32_t lockCount)
{
    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        lockChecker.AcquiringLock(locks[lockIndex]);
        lockChecker.LockAcquired(locks[lockIndex]);
    }
}

void ReleaseAllLocks(LockChecker& lockChecker, Mutex** locks, uint32_t lockCount)
{
    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        lockChecker.ReleasingLock(locks[lockCount - lockIndex - 1]);
    }
}

TEST(LockCheckerTest, StackGrowth)
{
    /* An arbitrary value that is not very small and not very large */
    const uint32_t lockCount = 30;

    Mutex** locks = new Mutex*[lockCount];
    ASSERT_NE(locks, nullptr);

    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        locks[lockIndex] = new Mutex(lockIndex + 1);
        ASSERT_NE(locks[lockIndex], nullptr);
    }

    /* Simulate acquiring all of these locks, in the correct order */
    LockChecker lockChecker;
    AcquireAllLocks(lockChecker, locks, lockCount);

    /* Simulate releasing all locks */
    ReleaseAllLocks(lockChecker, locks, lockCount);

    /* Clean-up */
    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        delete locks[lockIndex];
    }
    delete[] locks;
}

TEST(LockCheckerTest, OutOfOrderRelease1)
{
    const uint32_t lockCount = 2;

    Mutex** locks = new Mutex*[lockCount];
    ASSERT_NE(locks, nullptr);

    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        locks[lockIndex] = new Mutex((lockIndex + 1) * 10);
        ASSERT_NE(locks[lockIndex], nullptr);
    }

    /* Simulate acquiring all of these locks, in the correct order */
    LockChecker lockChecker;
    AcquireAllLocks(lockChecker, locks, lockCount);

    /* Simulate releasing locks in reverse order - as this is supported app behavior */
    lockChecker.ReleasingLock(locks[0]);
    lockChecker.ReleasingLock(locks[1]);

    /* Make sure re-acquire and release in the correct order still work */
    AcquireAllLocks(lockChecker, locks, lockCount);
    ReleaseAllLocks(lockChecker, locks, lockCount);

    /* Clean-up */
    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        delete locks[lockIndex];
    }
    delete[] locks;
}

TEST(LockCheckerTest, OutOfOrderRelease2)
{
    const uint32_t lockCount = 3;

    Mutex** locks = new Mutex*[lockCount];
    ASSERT_NE(locks, nullptr);

    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        locks[lockIndex] = new Mutex((lockIndex + 1) * 100);
        ASSERT_NE(locks[lockIndex], nullptr);
    }

    /* Simulate acquiring all of these locks, in the correct order */
    LockChecker lockChecker;
    AcquireAllLocks(lockChecker, locks, lockCount);

    /* Simulate releasing locks in reverse order - as this is supported app behavior */
    lockChecker.ReleasingLock(locks[1]);
    lockChecker.ReleasingLock(locks[0]);
    lockChecker.ReleasingLock(locks[2]);

    /* Make sure re-acquire and release in the correct order still work */
    AcquireAllLocks(lockChecker, locks, lockCount);
    ReleaseAllLocks(lockChecker, locks, lockCount);

    /* Clean-up */
    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        delete locks[lockIndex];
    }
    delete[] locks;
}

TEST(LockCheckerTest, RecursiveAcquire)
{
    const uint32_t lockCount = 3;

    Mutex** locks = new Mutex*[lockCount];
    ASSERT_NE(locks, nullptr);

    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        locks[lockIndex] = new Mutex((lockIndex + 1) * 1000);
        ASSERT_NE(locks[lockIndex], nullptr);
    }

    /* Simulate recursive acquires - as this is supported app behavior */
    LockChecker lockChecker;

    lockChecker.AcquiringLock(locks[0]);
    lockChecker.LockAcquired(locks[0]);

    lockChecker.AcquiringLock(locks[1]);
    lockChecker.LockAcquired(locks[1]);
    lockChecker.AcquiringLock(locks[1]);
    lockChecker.LockAcquired(locks[1]);

    lockChecker.AcquiringLock(locks[2]);
    lockChecker.LockAcquired(locks[2]);
    lockChecker.AcquiringLock(locks[2]);
    lockChecker.LockAcquired(locks[2]);
    lockChecker.AcquiringLock(locks[2]);
    lockChecker.LockAcquired(locks[2]);

    /* Simulate releasing all locks */
    lockChecker.ReleasingLock(locks[2]);
    lockChecker.ReleasingLock(locks[2]);
    lockChecker.ReleasingLock(locks[2]);

    lockChecker.ReleasingLock(locks[1]);
    lockChecker.ReleasingLock(locks[1]);

    lockChecker.ReleasingLock(locks[0]);

    /* Make sure re-acquire and release in the correct order still work */
    AcquireAllLocks(lockChecker, locks, lockCount);
    ReleaseAllLocks(lockChecker, locks, lockCount);

    /* Clean-up */
    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        delete locks[lockIndex];
    }
    delete[] locks;
}

TEST(LockCheckerTest, TryAcquire)
{
    const uint32_t lockCount = 4;

    Mutex** locks = new Mutex*[lockCount];
    ASSERT_NE(locks, nullptr);

    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        locks[lockIndex] = new Mutex((lockIndex + 1) * 1000);
        ASSERT_NE(locks[lockIndex], nullptr);
    }

    /* Simulate TryAcquire returning false for lock 1 & 2 */
    LockChecker lockChecker;

    lockChecker.AcquiringLock(locks[0]);
    lockChecker.LockAcquired(locks[0]);

    lockChecker.AcquiringLock(locks[1]);
    lockChecker.AcquiringLock(locks[1]);
    lockChecker.AcquiringLock(locks[1]);

    lockChecker.AcquiringLock(locks[2]);

    lockChecker.AcquiringLock(locks[3]);
    lockChecker.LockAcquired(locks[3]);
    lockChecker.AcquiringLock(locks[3]);
    lockChecker.LockAcquired(locks[3]);

    /* Simulate releasing all locks */
    lockChecker.ReleasingLock(locks[3]);
    lockChecker.ReleasingLock(locks[3]);

    lockChecker.ReleasingLock(locks[0]);

    /* Make sure re-acquire and release in the correct order still work */
    AcquireAllLocks(lockChecker, locks, lockCount);
    ReleaseAllLocks(lockChecker, locks, lockCount);

    /* Clean-up */
    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        delete locks[lockIndex];
    }
    delete[] locks;
}

TEST(LockCheckerTest, OutOfOrderRecursiveAcquire)
{
    const uint32_t lockCount = 3;

    Mutex** locks = new Mutex*[lockCount];
    ASSERT_NE(locks, nullptr);

    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        locks[lockIndex] = new Mutex((lockIndex + 1) * 1000);
        ASSERT_NE(locks[lockIndex], nullptr);
    }

    /* Simulate acquire pattern: lock0, lock1, then lock0 again - as this is supported app behavior */
    LockChecker lockChecker;

    lockChecker.AcquiringLock(locks[0]);
    lockChecker.LockAcquired(locks[0]);

    lockChecker.AcquiringLock(locks[1]);
    lockChecker.LockAcquired(locks[1]);

    lockChecker.AcquiringLock(locks[0]);
    lockChecker.LockAcquired(locks[0]);

    lockChecker.AcquiringLock(locks[2]);
    lockChecker.LockAcquired(locks[2]);

    /* Simulate releasing all locks */
    lockChecker.ReleasingLock(locks[2]);
    lockChecker.ReleasingLock(locks[0]);
    lockChecker.ReleasingLock(locks[1]);
    lockChecker.ReleasingLock(locks[0]);

    /* Make sure re-acquire and release in the correct order still work */
    AcquireAllLocks(lockChecker, locks, lockCount);
    ReleaseAllLocks(lockChecker, locks, lockCount);

    /* Clean-up */
    for (uint32_t lockIndex = 0; lockIndex < lockCount; lockIndex++) {
        delete locks[lockIndex];
    }
    delete[] locks;
}

#endif /* NDEBUG */
