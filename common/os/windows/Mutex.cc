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

#include <qcc/platform.h>

#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include <qcc/LockCheckerLevel.h>
#include <qcc/windows/utility.h>

/** @internal */
#define QCC_MODULE "MUTEX"

using namespace qcc;

void Mutex::Init()
{
    assert(!isInitialized);
#ifndef NDEBUG
    file = NULL;
    line = static_cast<uint32_t>(-1);
#endif
    InitializeCriticalSection(&mutex);
    isInitialized = true;
}

void Mutex::Destroy()
{
    if (isInitialized) {
        isInitialized = false;
        DeleteCriticalSection(&mutex);
    }
}

QStatus Mutex::Lock()
{
    assert(isInitialized);
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

#ifndef NDEBUG
    /*
     * Check for LOCK_LEVEL_CHECKING_DISABLED before calling GetThread,
     * because GetThread uses a LOCK_LEVEL_CHECKING_DISABLED internally.
     */
    if (Thread::initialized && level != LOCK_LEVEL_CHECKING_DISABLED) {
        Thread::GetThread()->lockChecker.AcquiringLock(this);
    }
#endif

    EnterCriticalSection(&mutex);

#ifndef NDEBUG
    if (Thread::initialized && level != LOCK_LEVEL_CHECKING_DISABLED) {
        Thread::GetThread()->lockChecker.LockAcquired(this);
    }
#endif

    return ER_OK;
}

QStatus Mutex::Unlock()
{
    assert(isInitialized);
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

#ifndef NDEBUG
    /*
     * Check for LOCK_LEVEL_CHECKING_DISABLED before calling GetThread,
     * because GetThread uses a LOCK_LEVEL_CHECKING_DISABLED internally.
     */
    if (Thread::initialized && level != LOCK_LEVEL_CHECKING_DISABLED) {
        Thread::GetThread()->lockChecker.ReleasingLock(this);
    }
#endif

    LeaveCriticalSection(&mutex);
    return ER_OK;
}

bool Mutex::TryLock()
{
    assert(isInitialized);
    if (!isInitialized) {
        return false;
    }

#ifndef NDEBUG
    /*
     * Check for LOCK_LEVEL_CHECKING_DISABLED before calling GetThread,
     * because GetThread uses a LOCK_LEVEL_CHECKING_DISABLED internally.
     */
    if (Thread::initialized && level != LOCK_LEVEL_CHECKING_DISABLED) {
        Thread::GetThread()->lockChecker.AcquiringLock(this);
    }
#endif

    BOOL acquired = TryEnterCriticalSection(&mutex);

    if (acquired) {
#ifndef NDEBUG
        if (Thread::initialized && level != LOCK_LEVEL_CHECKING_DISABLED) {
            Thread::GetThread()->lockChecker.LockAcquired(this);
        }
#endif
        return true;
    }

    return false;
}
