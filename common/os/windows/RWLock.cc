/**
 * @file
 *
 * Define a class that abstracts Windows "slim reader/writer" locks.
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
