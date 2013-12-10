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

#include <qcc/Semaphore.h>

/** @internal */
#define QCC_MODULE "SEMAPHORE"

using namespace qcc;

Semaphore::Semaphore() : _initialized(false), _semaphore(INVALID_HANDLE_VALUE), _initial(-1),  _maximum(-1)
{
}

Semaphore::~Semaphore()
{
    Close();
}

void Semaphore::Close()
{
    if (_initialized) {
        _initialized = false;
        if (INVALID_HANDLE_VALUE != _semaphore) {
            CloseHandle(_semaphore);
            _semaphore = INVALID_HANDLE_VALUE;
        }
    }
}

QStatus Semaphore::Init(int32_t initial, int32_t maximum)
{
    QStatus result = ER_FAIL;

    while (true) {
        if (!_initialized) {
            _initial = initial;
            _maximum = maximum;
            _semaphore = CreateSemaphoreEx(NULL, initial, maximum,
                                           NULL, 0, SEMAPHORE_ALL_ACCESS);
            if (NULL == _semaphore) {
                result = ER_OS_ERROR;
                break;
            }
            result = ER_OK;
            _initialized = true;
        }

        break;
    }

    return result;
}

QStatus Semaphore::Wait(void)
{
    if (!_initialized) {
        return ER_INIT_FAILED;
    }
    return (WaitForSingleObjectEx(_semaphore, INFINITE, TRUE) == WAIT_OBJECT_0) ? ER_OK : ER_FAIL;
}

QStatus Semaphore::Release(void)
{
    if (!_initialized) {
        return ER_INIT_FAILED;
    }
    return (ReleaseSemaphore(_semaphore, 1, NULL) == TRUE) ? ER_OK : ER_FAIL;
}

QStatus Semaphore::Reset(void)
{
    if (!_initialized) {
        return ER_INIT_FAILED;
    }

    Close();

    return Init(_initial, _maximum);
}
