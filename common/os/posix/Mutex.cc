/**
 * @file
 *
 * Define a class that abstracts Linux mutex's.
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

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include <qcc/MutexInternal.h>
#include <qcc/Debug.h>

#include <Status.h>

/** @internal */
#define QCC_MODULE "MUTEX"

using namespace qcc;

void Mutex::Init()
{
    QCC_ASSERT(!isInitialized);
    mutexInternal = new Internal;
    file = nullptr;
    line = static_cast<uint32_t>(-1);

    pthread_mutexattr_t attr;
    int ret = pthread_mutexattr_init(&attr);
    // Can't use QCC_LogError() since it uses mutexes under the hood.
    QCC_ASSERT(ret == 0);
    if (ret != 0) {
        return;
    }

    // We want entities to be able to lock a mutex multiple times without deadlocking or reporting an error.
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    isInitialized = (pthread_mutex_init(&mutex, &attr) == 0);
    // Can't use QCC_LogError() since it uses mutexes under the hood.
    QCC_ASSERT(isInitialized);

    // Don't need the attribute once it has been assigned to a mutex.
    pthread_mutexattr_destroy(&attr);
}

void Mutex::Destroy()
{
    if (isInitialized) {
        isInitialized = false;
        // Can't use QCC_LogError() since it uses mutexes under the hood.
        QCC_VERIFY(pthread_mutex_destroy(&mutex) == 0);
        delete mutexInternal;
        mutexInternal = nullptr;
    }
}

QStatus Mutex::Lock()
{
    QCC_ASSERT(isInitialized);
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    int ret = pthread_mutex_lock(&mutex);
    // Can't use QCC_LogError() since it uses mutexes under the hood.
    QCC_ASSERT(ret == 0);
    if (ret != 0) {
        return ER_OS_ERROR;
    }

    mutexInternal->LockAcquired();
    return ER_OK;
}

QStatus Mutex::Unlock()
{
    QCC_ASSERT(isInitialized);
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    mutexInternal->ReleasingLock();
    int ret = pthread_mutex_unlock(&mutex);
    // Can't use QCC_LogError() since it uses mutexes under the hood.
    QCC_ASSERT(ret == 0);
    if (ret != 0) {
        return ER_OS_ERROR;
    }
    return ER_OK;
}

bool Mutex::TryLock(void)
{
    bool locked = false;
    QCC_ASSERT(isInitialized);
    if (isInitialized) {
        locked = (pthread_mutex_trylock(&mutex) == 0);
        if (locked) {
            mutexInternal->LockAcquired();
        }
    }

    return locked;
}
