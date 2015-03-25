#ifndef _ALLJOYN_SCOPEDMUTEXLOCK_H
#define _ALLJOYN_SCOPEDMUTEXLOCK_H
/**
 * @file
 *
 * This file defines a class that will ensure a mutex is unlocked when the method returns
 *
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

#include <qcc/Mutex.h>

namespace qcc {

/**
 * An implementation of a scoped mutex lock class.
 */
class ScopedMutexLock {
  public:

    /**
     * Constructor
     *
     * @param lock The lock we want to manage
     */
    ScopedMutexLock(Mutex& lock)
        : lock(lock),
        file(NULL),
        line(0)
    {
        lock.Lock();
    }

    /**
     * Constructor
     *
     * @param lock The lock we want to manage
     * @param file The file for the lock trace
     * @param file The line for the lock trace
     */
    ScopedMutexLock(Mutex& lock, const char* file, uint32_t line)
        : lock(lock),
        file(file),
        line(line)
    {
        lock.Lock(file, line);
    }

    ~ScopedMutexLock()
    {
        if (file) {
            lock.Unlock(file, line);
        } else {
            lock.Unlock();
        }
    }

  private:
    /* Private assigment operator - does nothing */
    ScopedMutexLock& operator=(const ScopedMutexLock&);
    Mutex& lock;
    const char* file;
    const uint32_t line;
};

}

#endif
