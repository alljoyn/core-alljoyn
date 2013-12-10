/**
 * @file
 *
 * This file just provides static initialization for the OpenSSL libraries.
 */

/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
#include <assert.h>

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
    if (mode & CRYPTO_LOCK) {
        locks[type].Lock();
    } else {
        locks[type].Unlock();
    }
}

static int openSslCounter = 0;

OpenSslInitializer::OpenSslInitializer()
{
    if (0 == openSslCounter++) {
        locks = new Mutex[CRYPTO_num_locks()];
        CRYPTO_set_locking_callback(LockingCb);
    }
}

OpenSslInitializer::~OpenSslInitializer()
{
    if (0 == --openSslCounter) {
        CRYPTO_set_locking_callback(NULL);
        delete[] locks;
    }
}

#else /* !OPENSSL_THREADS */

static Mutex* mutex = NULL;
static int32_t refCount = 0;

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

OpenSslInitializer::OpenSslInitializer() {
}
OpenSslInitializer::~OpenSslInitializer() {
}

#endif
