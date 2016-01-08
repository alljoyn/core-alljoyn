/**
 * @file
 *
 * Define a class that abstracts mutexes.
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
