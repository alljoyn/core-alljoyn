/**
 * @file
 *
 * Define a class that abstracts Windows "slim reader/writer" locks.
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
#include <qcc/platform.h>
#include <qcc/RWLock.h>
#include <Status.h>

/** @internal */
#define QCC_MODULE "RWLOCK"

using namespace qcc;

void RWLock::Init()
{
    isInitialized = false;
    isWriteLock = false;

    InitializeSRWLock(&rwlock);
    isInitialized = true;
}

RWLock::~RWLock()
{
    if (!isInitialized) {
        return;
    }

    isInitialized = false;
    isWriteLock = false;
}

QStatus RWLock::RDLock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    AcquireSRWLockShared(&rwlock);

    return ER_OK;
}

QStatus RWLock::WRLock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    AcquireSRWLockExclusive(&rwlock);
    isWriteLock = true;

    return ER_OK;
}

QStatus RWLock::Unlock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    if (isWriteLock) {
        isWriteLock = false;
        ReleaseSRWLockExclusive(&rwlock);
    } else {
        ReleaseSRWLockShared(&rwlock);
    }

    return ER_OK;
}

bool RWLock::TryRDLock(void)
{
    if (!isInitialized) {
        return false;
    }

    return TryAcquireSRWLockShared(&rwlock) != 0;
}

bool RWLock::TryWRLock(void)
{
    if (!isInitialized) {
        return false;
    }

    return TryAcquireSRWLockExclusive(&rwlock) != 0;
}