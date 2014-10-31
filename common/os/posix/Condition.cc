/**
 * @file
 *
 * This file implements qcc::Condition for Posix systems
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

#include <assert.h>
#include <pthread.h>
#include <errno.h>

#if defined(QCC_OS_DARWIN)
#include <sys/time.h>
#endif

#include <qcc/Debug.h>
#include <qcc/Condition.h>

#define QCC_MODULE "CONDITION"

using namespace qcc;

Condition::Condition()
{
    int ret = pthread_cond_init(&c, NULL);
    if (ret != 0) {
        QCC_LogError(ER_OS_ERROR, ("Condition::Condition(): Cannot initialize pthread condition variable (%d)", ret));
    }
    assert(ret == 0 && "Condition::Condition(): Cannot initialize pthread condition variable");
}

Condition::~Condition()
{
    int ret = pthread_cond_destroy(&c);
    if (ret != 0) {
        QCC_LogError(ER_OS_ERROR, ("Condition::Condition(): Cannot destroy pthread condition variable (%d)", ret));
    }
    assert(ret == 0 && "Condition::Condition(): Cannot destroy pthread condition variable");
}

QStatus Condition::Wait(qcc::Mutex& m)
{
    int ret = pthread_cond_wait(&c, &m.mutex);
    if (ret != 0) {
        QCC_LogError(ER_OS_ERROR, ("Condition::Wait(): Cannot wait on pthread condition variable (%d)", ret));
        return ER_OS_ERROR;
    }
    return ER_OK;
}

QStatus Condition::TimedWait(qcc::Mutex& m, uint32_t ms)
{
    struct timespec tsTimeout;
    tsTimeout.tv_sec = ms / 1000;
    tsTimeout.tv_nsec = (ms % 1000) * 1000000;

    struct timespec tsNow;

#if defined(QCC_OS_DARWIN)
    struct timeval tvNow;
    gettimeofday(&tvNow, NULL);
    TIMEVAL_TO_TIMESPEC(&tvNow, &tsNow);
#else
    clock_gettime(CLOCK_REALTIME, &tsNow);
#endif

    tsTimeout.tv_nsec += tsNow.tv_nsec;
    tsTimeout.tv_sec += tsTimeout.tv_nsec / 1000000000;
    tsTimeout.tv_nsec %= 1000000000;
    tsTimeout.tv_sec += tsNow.tv_sec;

    int ret = pthread_cond_timedwait(&c, &m.mutex, &tsTimeout);
    if (ret == 0) {
        return ER_OK;
    }
    if (ret == ETIMEDOUT) {
        return ER_TIMEOUT;
    }
    QCC_LogError(ER_OS_ERROR, ("Condition::TimedWait(): Cannot wait on pthread condition variable (%d)", ret));
    return ER_OS_ERROR;
}

QStatus Condition::Signal()
{
    int ret = pthread_cond_signal(&c);
    if (ret != 0) {
        QCC_LogError(ER_OS_ERROR, ("Condition::Signal(): Cannot signal pthread condition variable (%d)", ret));
        return ER_OS_ERROR;
    }
    return ER_OK;
}

QStatus Condition::Broadcast()
{
    int ret = pthread_cond_broadcast(&c);
    if (ret != 0) {
        QCC_LogError(ER_OS_ERROR, ("Condition::Broadcast(): Cannot broadcast signal pthread condition variable (%d)", ret));
        return ER_OS_ERROR;
    }
    return ER_OK;
}
