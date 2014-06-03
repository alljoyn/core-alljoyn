/**
 * @file
 *
 * Define a class that abstracts Linux mutex's.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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
#include <assert.h>

#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include <qcc/Debug.h>

#include <Status.h>

/** @internal */
#define QCC_MODULE "MUTEX"

using namespace qcc;

void Mutex::Init()
{
    isInitialized = false;
    int ret;
    pthread_mutexattr_t attr;
    ret = pthread_mutexattr_init(&attr);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexs under the hood.
        printf("***** Mutex attribute initialization failure: %d - %s\n", ret, strerror(ret));
        goto cleanup;
    }
    // We want entities to be able to lock a mutex multiple times without deadlocking or reporting an error.
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    ret = pthread_mutex_init(&mutex, &attr);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexs under the hood.
        printf("***** Mutex initialization failure: %d - %s\n", ret, strerror(ret));
        goto cleanup;
    }

    isInitialized = true;
    file = NULL;
    line = -1;

cleanup:
    // Don't need the attribute once it has been assigned to a mutex.
    pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex()
{
    if (!isInitialized) {
        return;
    }

    int ret;
    ret = pthread_mutex_destroy(&mutex);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexs under the hood.
        printf("***** Mutex destruction failure: %d - %s\n", ret, strerror(ret));
        assert(false);
    }
}

QStatus Mutex::Lock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    int ret = pthread_mutex_lock(&mutex);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexes under the hood.
        printf("***** Mutex lock failure: %d - %s\n", ret, strerror(ret));
        assert(false);
        return ER_OS_ERROR;
    }
    return ER_OK;
}

QStatus Mutex::Lock(const char* file, uint32_t line)
{
#ifdef NDEBUG
    return Lock();
#else
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    QStatus status;
    if (TryLock()) {
        status = ER_OK;
    } else {
        status = Lock();
        if (status == ER_OK) {
            QCC_DbgPrintf(("Lock Acquired %s:%d", file, line));
        } else {
            QCC_LogError(status, ("Mutex::Lock %s:%d failed", file, line));
        }
    }
    if (status == ER_OK) {
        this->file = reinterpret_cast<const char*>(file);
        this->line = line;
    }
    return status;
#endif
}

QStatus Mutex::Unlock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    int ret = pthread_mutex_unlock(&mutex);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexes under the hood.
        printf("***** Mutex unlock failure: %d - %s\n", ret, strerror(ret));
        assert(false);
        return ER_OS_ERROR;
    }
    return ER_OK;
}

QStatus Mutex::Unlock(const char* file, uint32_t line)
{
#ifdef NDEBUG
    return Unlock();
#else
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }
    this->file = NULL;
    this->line = -1;
    int ret = pthread_mutex_unlock(&mutex);
    if (ret != 0) {
        fflush(stdout);
        printf("***** Mutex unlock failure: %s:%d %d - %s\n", file, line, ret, strerror(ret));
        assert(false);
        return ER_OS_ERROR;
    }
    return ER_OK;
#endif
}

bool Mutex::TryLock(void)
{
    if (!isInitialized) {
        return false;
    }
    return pthread_mutex_trylock(&mutex) == 0;
}
