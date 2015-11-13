/**
 * @file
 *
 * Define a class that abstracts Windows mutexs.
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
#include <qcc/MutexInternal.h>

#define QCC_MODULE "MUTEX"

using namespace qcc;

bool Mutex::Internal::PlatformSpecificInit()
{
    InitializeCriticalSection(&m_mutex);
    return true;
}

void Mutex::Internal::PlatformSpecificDestroy()
{
    DeleteCriticalSection(&m_mutex);
}

QStatus Mutex::Internal::PlatformSpecificLock()
{
    EnterCriticalSection(&m_mutex);
    return ER_OK;
}

QStatus Mutex::Internal::PlatformSpecificUnlock()
{
    LeaveCriticalSection(&m_mutex);
    return ER_OK;
}

bool Mutex::Internal::PlatformSpecificTryLock()
{
    return (TryEnterCriticalSection(&m_mutex) != FALSE);
}
