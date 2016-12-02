/**
 * @file
 *
 * This file implements qcc::Condition for Windows systems
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <windows.h>

#include <qcc/Debug.h>
#include <qcc/Condition.h>
#include <qcc/MutexInternal.h>

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
    MutexInternal::ReleasingLock(m);
    bool ret = SleepConditionVariableCS(&c, MutexInternal::GetPlatformSpecificMutex(m), ms);
    MutexInternal::LockAcquired(m);

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