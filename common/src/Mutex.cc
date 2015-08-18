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
#include <assert.h>
#include <qcc/platform.h>
#include <qcc/Mutex.h>
#include <qcc/MutexInternal.h>
#include <qcc/Debug.h>
#include <qcc/LockCheckerLevel.h>

/** @internal */
#define QCC_MODULE "MUTEX"

namespace qcc {

#ifndef NDEBUG
Mutex::Internal::Internal(int level)
    : m_file(nullptr), m_line(-1), m_level(static_cast<LockCheckerLevel>(level)), m_maximumRecursionCount(0)
{
}

void Mutex::Internal::SetLevel(LockCheckerLevel level)
{
    m_level = level;
}
#endif /* NDEBUG */

Mutex::Mutex(int level) : isInitialized(false)
{
#ifndef NDEBUG
    internal = new Internal(level);
#else
    QCC_UNUSED(level);
#endif

    Init();
}

Mutex::Mutex(const Mutex& other) : isInitialized(false)
{
#ifndef NDEBUG
    internal = new Internal(other.internal->m_level);
#else
    QCC_UNUSED(other);
#endif

    Init();
}

Mutex::~Mutex()
{
#ifndef NDEBUG
    delete internal;
    internal = nullptr;
#endif

    Destroy();
}

Mutex& Mutex::operator=(const Mutex& other)
{
#ifndef NDEBUG
    internal = new Internal(other.internal->m_level);
#else
    QCC_UNUSED(other);
#endif

    Destroy();
    Init();
    return *this;
}

QStatus Mutex::Lock(const char* file, uint32_t line)
{
#ifdef NDEBUG
    QCC_UNUSED(file);
    QCC_UNUSED(line);
    return Lock();
#else
    assert(isInitialized);
    QStatus status;
    if (TryLock()) {
        status = ER_OK;
    } else {
        status = Lock();
    }
    if (status == ER_OK) {
        QCC_DbgPrintf(("Lock Acquired %s:%d", file, line));
        this->internal->m_file = file;
        this->internal->m_line = line;
    } else {
        QCC_LogError(status, ("Mutex::Lock %s:%d failed", file, line));
    }
    return status;
#endif
}

QStatus Mutex::Unlock(const char* file, uint32_t line)
{
#ifdef NDEBUG
    QCC_UNUSED(file);
    QCC_UNUSED(line);
    return Unlock();
#else
    assert(isInitialized);
    QCC_DbgPrintf(("Lock Released: %s:%d (acquired at %s:%d)", file, line, this->internal->m_file, this->internal->m_line));
    this->internal->m_file = file;
    this->internal->m_line = line;
    return Unlock();
#endif
}

} /* namespace */

