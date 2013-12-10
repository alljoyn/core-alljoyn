/**
 * @file
 *
 * Define a class that abstracts WinRT mutexes.
 */

/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#include <qcc/platform.h>

#include <windows.h>
#include <stdio.h>

#include <qcc/Mutex.h>

/** @internal */
#define QCC_MODULE "MUTEX"

using namespace qcc;


void Mutex::Init()
{
    if (!initialized && InitializeCriticalSectionEx(&mutex, 100, 0)) {
        initialized = true;
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
    return Lock();
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
    return Unlock();
}

bool Mutex::TryLock(void)
{
    if (!initialized) {
        return false;
    }
    return TryEnterCriticalSection(&mutex);
}
