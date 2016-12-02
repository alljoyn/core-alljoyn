/**
 * @file
 *
 * Define a class that abstracts Windows mutexs.
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

#define QCC_MODULE "MUTEX"

using namespace qcc;

bool MutexInternal::PlatformSpecificInit()
{
    InitializeCriticalSection(&m_mutex);
    return true;
}

void MutexInternal::PlatformSpecificDestroy()
{
    DeleteCriticalSection(&m_mutex);
}

QStatus MutexInternal::PlatformSpecificLock()
{
    EnterCriticalSection(&m_mutex);
    return ER_OK;
}

QStatus MutexInternal::PlatformSpecificUnlock()
{
    LeaveCriticalSection(&m_mutex);
    return ER_OK;
}

bool MutexInternal::PlatformSpecificTryLock()
{
    return (TryEnterCriticalSection(&m_mutex) != FALSE);
}