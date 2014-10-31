/**
 * @file
 *
 * This file implements qcc::Condition for Windows systems
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
#include <windows.h>

#include <qcc/Debug.h>
#include <qcc/Condition.h>

#define QCC_MODULE "CONDITION"

using namespace qcc;

Condition::Condition()
{
    InitializeConditionVariable(&c);
}

Condition::~Condition()
{
}

QStatus Condition::Wait(qcc::Mutex& m)
{
    return TimedWait(m, INFINITE);
}

QStatus Condition::TimedWait(qcc::Mutex& m, uint32_t ms)
{
    bool ret = SleepConditionVariableCS(&c, &m.mutex, ms);
    if (ret == true) {
        return ER_OK;
    }

    DWORD error = GetLastError();
    if (error == ERROR_TIMEOUT) {
        return ER_TIMEOUT;
    }

    QCC_LogError(ER_OS_ERROR, ("Condition::TimedWait(): Cannot wait on Windows condition variable (%d)", error));
    return ER_OS_ERROR;
}

QStatus Condition::Signal()
{
    WakeConditionVariable(&c);
    return ER_OK;
}

QStatus Condition::Broadcast()
{
    WakeAllConditionVariable(&c);
    return ER_OK;
}
