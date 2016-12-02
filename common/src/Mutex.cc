/**
 * @file
 *
 * Define a class that abstracts mutexes.
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
#include <qcc/Mutex.h>
#include <qcc/MutexInternal.h>
#include <qcc/Debug.h>

#define QCC_MODULE "MUTEX"

using namespace qcc;

Mutex::Mutex(int level) : m_mutexInternal(new MutexInternal(this, static_cast<LockLevel>(level)))
{
}

Mutex::~Mutex()
{
    delete m_mutexInternal;
}

QStatus Mutex::Lock(const char* file, uint32_t line)
{
    return m_mutexInternal->Lock(file, line);
}

QStatus Mutex::Lock()
{
    return m_mutexInternal->Lock();
}

QStatus Mutex::Unlock(const char* file, uint32_t line)
{
    return m_mutexInternal->Unlock(file, line);
}

QStatus Mutex::Unlock()
{
    return m_mutexInternal->Unlock();
}

void Mutex::AssertOwnedByCurrentThread() const
{
    m_mutexInternal->AssertOwnedByCurrentThread();
}

bool Mutex::TryLock()
{
    return m_mutexInternal->TryLock();
}