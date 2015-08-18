/**
 * @file
 *
 * This file just provides static initialization for the OpenSSL libraries.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/MutexInternal.h>
#include <assert.h>

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
            locks[index].internal->SetLevel(LOCK_LEVEL_OPENSSL_LOCK);
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
    assert(mutex);
    mutex->Unlock();
}

void Crypto::Init() {
}

void Crypto::Shutdown() {
}

#endif
