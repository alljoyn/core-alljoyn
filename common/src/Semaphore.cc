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

#include <qcc/Semaphore.h>
#include <qcc/time.h>

Semaphore::Semaphore() :
    value(0)
{
}

Semaphore::Semaphore(unsigned int initial) :
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
    status = cond.Signal();
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

QStatus Semaphore::TimedWait(uint32_t ms)
{
    QStatus status = ER_OK;
    uint64_t start_time = qcc::GetTimestamp64();

    mutex.Lock();
    while ((0 == value) && (ER_OK == status)) {
        uint64_t elapsed = qcc::GetTimestamp64() - start_time;
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
