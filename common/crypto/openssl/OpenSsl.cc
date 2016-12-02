/**
 * @file
 *
 * This file just provides static initialization for the OpenSSL libraries.
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
#include <qcc/Thread.h>
#include <qcc/MutexInternal.h>

#include "Crypto.h"
#include "OpenSsl.h"
#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>

using namespace qcc;

#if defined(OPENSSL_THREADS)

OpenSsl_ScopedLock::OpenSsl_ScopedLock() {
}
OpenSsl_ScopedLock::~OpenSsl_ScopedLock() {
}

static Mutex* locks = 0;

static void LockingCb(int mode, int type, const char* file, int line)
{
    QCC_UNUSED(file);
    QCC_UNUSED(line);
    if (mode & CRYPTO_LOCK) {
        locks[type].Lock();
    } else {
        locks[type].Unlock();
    }
}

QStatus Crypto::Init()
{
    if (!locks) {
        int numLocks = CRYPTO_num_locks();
        locks = new Mutex[numLocks];
        CRYPTO_set_locking_callback(LockingCb);

#ifndef NDEBUG
        for (int index = 0; index < numLocks; index++) {
            MutexInternal::SetLevel(locks[index], LOCK_LEVEL_OPENSSL_LOCK);
        }
#endif
    }
    return ER_OK;
}

void Crypto::Shutdown()
{
    CRYPTO_set_locking_callback(NULL);
    delete[] locks;
    locks = NULL;
}

#else /* !OPENSSL_THREADS */

static Mutex* mutex = NULL;
static volatile int32_t refCount = 0;

OpenSsl_ScopedLock::OpenSsl_ScopedLock()
{
    if (IncrementAndFetch(&refCount) == 1) {
        mutex = new Mutex();
    } else {
        DecrementAndFetch(&refCount);
        while (!mutex) {
            qcc::Sleep(1);
        }
    }
    mutex->Lock();
}

OpenSsl_ScopedLock::~OpenSsl_ScopedLock()
{
    QCC_ASSERT(mutex);
    mutex->Unlock();
}

void Crypto::Init() {
}

void Crypto::Shutdown() {
}

#endif