/**
 * @file
 *
 * Define a class that abstracts Windows mutexs.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

/** @internal */
#define QCC_MODULE "MUTEX"

#ifdef NDEBUG
#define NO_LOCK_TRACE 1
#else
#define NO_LOCK_TRACE 1 // disabled
#endif

using namespace qcc;

void Mutex::Init()
{
    if (!initialized) {
        // Starting with Vista this always returns non-zero so this test will be less and less important
        // in the future (http://msdn.microsoft.com/en-us/library/windows/desktop/ms683476.aspx)
        if (InitializeCriticalSectionAndSpinCount(&mutex, 100)) {
            initialized = true;
        } else {
            char buf[80];
            uint32_t ret = GetLastError();
            strerror_r(ret, buf, sizeof(buf));
            // Can't use ER_LogError() since it uses mutexs under the hood.
            printf("**** Mutex initialization failure: %u - %s", ret, buf);
        }
    }
}

Mutex::~Mutex()
{
    if (initialized) {
        initialized = false;
        DeleteCriticalSection(&mutex);
    }
}

QStatus Mutex::Lock(void)
{
    if (!initialized) {
        return ER_INIT_FAILED;
    }
    EnterCriticalSection(&mutex);
    return ER_OK;
}

QStatus Mutex::Lock(const char* file, uint32_t line)
{
#if NO_LOCK_TRACE
    return Lock();
#else
    QStatus status;
    if (TryLock()) {
        status = ER_OK;
    } else {
        Thread::GetThread()->lockTrace.Waiting(this, file, line);
        status = Lock();
    }
    if (status == ER_OK) {
        Thread::GetThread()->lockTrace.Acquired(this, file, line);
    } else {
        QCC_LogError(status, ("Mutex::Lock %s:%d failed", file, line));
    }
    return status;
#endif
}

QStatus Mutex::Unlock(void)
{
    if (!initialized) {
        return ER_INIT_FAILED;
    }
    LeaveCriticalSection(&mutex);
    return ER_OK;
}

QStatus Mutex::Unlock(const char* file, uint32_t line)
{
#if NO_LOCK_TRACE
    return Unlock();
#else
    if (!initialized) {
        return ER_INIT_FAILED;
    }
    Thread::GetThread()->lockTrace.Releasing(this, file, line);
    return Unlock();
#endif
}

bool Mutex::TryLock(void)
{
    if (!initialized) {
        return false;
    }
    return TryEnterCriticalSection(&mutex);
}
