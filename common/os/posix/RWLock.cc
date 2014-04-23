/**
 * @file
 *
 * Define a class that abstracts Linux rwlock's.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

/*
 * When building for Android, the Mutex fallback implementation in
 * qcc/RWLock.h is used instead of the pThread based version since the rwlock
 * functionality is missing from the Android version of pThread.
 */
#if !defined(QCC_OS_ANDROID)

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <qcc/RWLock.h>

#include <Status.h>

/** @internal */
#define QCC_MODULE "RWLOCK"

using namespace qcc;

void RWLock::Init()
{
    isInitialized = false;
    int ret;

    ret = pthread_rwlock_init(&rwlock, NULL);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexes under the hood.
        printf("***** RWLock initialization failure: %d - %s\n", ret, strerror(ret));
        goto cleanup;
    }

    isInitialized = true;

cleanup:
    return;
}

RWLock::~RWLock()
{
    if (!isInitialized) {
        return;
    }

    int ret;
    ret = pthread_rwlock_destroy(&rwlock);
    if (ret != 0) {
        fflush(stdout);
        // Can't use QCC_LogError() since it uses mutexes under the hood.
        printf("***** RWLock destruction failure: %d - %s\n", ret, strerror(ret));
        assert(false);
    }
}

QStatus RWLock::RDLock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    int ret = pthread_rwlock_rdlock(&rwlock);
    if (ret != 0) {
        fflush(stdout);
        // Can't use QCC_LogError() since it uses mutexes under the hood.
        printf("***** RWLock lock failure: %d - %s\n", ret, strerror(ret));
        assert(false);
        return ER_OS_ERROR;
    }
    return ER_OK;
}

QStatus RWLock::WRLock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    int ret = pthread_rwlock_wrlock(&rwlock);
    if (ret != 0) {
        fflush(stdout);
        // Can't use QCC_LogError() since it uses mutexes under the hood.
        printf("***** RWLock lock failure: %d - %s\n", ret, strerror(ret));
        assert(false);
        return ER_OS_ERROR;
    }
    return ER_OK;
}

QStatus RWLock::Unlock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    int ret = pthread_rwlock_unlock(&rwlock);
    if (ret != 0) {
        fflush(stdout);
        // Can't use QCC_LogError() since it uses mutexes under the hood.
        printf("***** RWLock unlock failure: %d - %s\n", ret, strerror(ret));
        assert(false);
        return ER_OS_ERROR;
    }
    return ER_OK;
}

bool RWLock::TryRDLock(void)
{
    if (!isInitialized) {
        return false;
    }
    return pthread_rwlock_tryrdlock(&rwlock) == 0;
}

bool RWLock::TryWRLock(void)
{
    if (!isInitialized) {
        return false;
    }
    return pthread_rwlock_trywrlock(&rwlock) == 0;
}

#endif
