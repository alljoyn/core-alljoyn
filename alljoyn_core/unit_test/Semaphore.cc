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

#if defined(QCC_OS_DARWIN)
#include <sys/time.h>
#endif

#include "Semaphore.h"

Semaphore::Semaphore() :
    value(0)
{
}

Semaphore::Semaphore(int initial) :
    value(initial)
{
}

Semaphore::~Semaphore()
{
}

QStatus Semaphore::Post()
{
    QStatus status = ER_OK;

    mutex.Lock();
    value++;
    status = cond.Broadcast();
    mutex.Unlock();
    return status;
}

QStatus Semaphore::Wait()
{
    QStatus status = ER_OK;

    mutex.Lock();
    while (0 == value) {
        status = cond.Wait(mutex);
    }
    if (ER_OK == status) {
        value--;
    }
    mutex.Unlock();
    return status;
}

static struct timespec Now() {
    struct timespec now;

#if defined(QCC_OS_DARWIN)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    TIMEVAL_TO_TIMESPEC(&tv, &now);
#else
    clock_gettime(CLOCK_REALTIME, &now);
#endif
    return now;
}

/**
 * Returns the number of milliseconds passed since \a then.
 *
 * \pre \a then should come before now (timewise).
 */
static uint32_t ElapsedMillis(struct timespec& then)
{
    struct timespec now = Now();
    uint32_t millis;

    millis = ((now.tv_sec - then.tv_sec) * 1000) +
             ((now.tv_nsec - then.tv_nsec) / 1000000);
    return millis;
}

QStatus Semaphore::TimedWait(uint32_t ms)
{
    QStatus status = ER_OK;
    struct timespec start_time = Now();

    mutex.Lock();
    while ((0 == value) && (ER_OK == status)) {
        uint32_t elapsed = ElapsedMillis(start_time);
        if (elapsed < ms) {
            status = cond.TimedWait(mutex, ms - elapsed);
        } else {
            status = ER_TIMEOUT;
            break;
        }
    }
    if (ER_OK == status) {
        value--;
    }
    mutex.Unlock();
    return status;
}
