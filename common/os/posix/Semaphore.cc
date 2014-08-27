/**
 * @file
 *
 * Define a class that abstracts Posix semaphores.
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
 *****************************************************************************/

#include <qcc/Semaphore.h>
#include <time.h>
#include <errno.h>

/** @internal */
#define QCC_MODULE "SEMAPHORE"

using namespace qcc;

Semaphore::Semaphore() : _initialized(false), _initial(-1), _semaphore()
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
        sem_close(&_semaphore);
    }
}

QStatus Semaphore::Init(int32_t initial, int32_t maximum)
{
    QStatus result = ER_FAIL;

    while (true) {
        if (!_initialized) {
            _initial = initial;
            if (0 != sem_init(&_semaphore, 0, initial)) {
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
    return (0 == sem_wait(&_semaphore)) ? ER_OK : ER_FAIL;
}

QStatus Semaphore::WaitFor(uint32_t sec, uint32_t nsec)
{
    struct timespec ts;
    QStatus status = ER_FAIL;
    const uint32_t billion = 1000000000L;

    if (0 != clock_gettime(CLOCK_REALTIME, &ts)) {
        return status;
    }
    ts.tv_sec = ts.tv_sec + sec + (ts.tv_nsec + nsec) / billion;
    ts.tv_nsec = (ts.tv_nsec + nsec) % billion;
    status = (0 == sem_timedwait(&_semaphore, &ts)) ? ER_OK : ER_FAIL;
    if (errno == ETIMEDOUT) {
        status = ER_TIMEOUT;
    }
    return status;
}

QStatus Semaphore::Release(void)
{
    if (!_initialized) {
        return ER_INIT_FAILED;
    }
    return (0 == sem_post(&_semaphore)) ? ER_OK : ER_FAIL;
}

QStatus Semaphore::Reset(void)
{
    if (!_initialized) {
        return ER_INIT_FAILED;
    }

    Close();

    return Init(_initial, 0);
}
