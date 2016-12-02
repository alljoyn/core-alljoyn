#ifndef _ALLJOYN_SCOPEDMUTEXLOCK_H
#define _ALLJOYN_SCOPEDMUTEXLOCK_H
/**
 * @file
 *
 * This file defines a class that will ensure a mutex is unlocked when the method returns
 *
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