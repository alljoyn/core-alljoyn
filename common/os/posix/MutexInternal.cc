/**
 * @file
 *
 * Define a class that abstracts Linux mutex's.
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
#include <qcc/MutexInternal.h>
#include <qcc/Debug.h>

#define QCC_MODULE "MUTEX"

using namespace qcc;

bool MutexInternal::PlatformSpecificInit()
{
    bool success;
    pthread_mutexattr_t attr;
    // Can't use QCC_LogError() since it uses mutexes under the hood.
    QCC_VERIFY(success = (pthread_mutexattr_init(&attr) == 0));

    if (success) {
        // We want entities to be able to lock a mutex multiple times without deadlocking or reporting an error.
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        QCC_VERIFY(success = (pthread_mutex_init(&m_mutex, &attr) == 0));

        // Don't need the attribute once it has been assigned to a mutex.
        pthread_mutexattr_destroy(&attr);
    }

    return success;
}

void MutexInternal::PlatformSpecificDestroy()
{
    // Can't use QCC_LogError() since it uses mutexes under the hood.
    QCC_VERIFY(pthread_mutex_destroy(&m_mutex) == 0);
}

QStatus MutexInternal::PlatformSpecificLock()
{
    int ret = pthread_mutex_lock(&m_mutex);
    // Can't use QCC_LogError() since it uses mutexes under the hood.
    QCC_ASSERT(ret == 0);
    if (ret != 0) {
        return ER_OS_ERROR;
    }

    return ER_OK;
}

QStatus MutexInternal::PlatformSpecificUnlock()
{
    int ret = pthread_mutex_unlock(&m_mutex);
    // Can't use QCC_LogError() since it uses mutexes under the hood.
    QCC_ASSERT(ret == 0);
    if (ret != 0) {
        return ER_OS_ERROR;
    }
    return ER_OK;
}

bool MutexInternal::PlatformSpecificTryLock()
{
    return (pthread_mutex_trylock(&m_mutex) == 0);
}