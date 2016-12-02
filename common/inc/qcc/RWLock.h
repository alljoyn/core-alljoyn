/**
 * @file
 *
 * Define a class that abstracts rwlock's.
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
#ifndef _QCC_RWLOCK_H
#define _QCC_RWLOCK_H

#include <qcc/platform.h>

/*
 * Note: The Android NDK does not include support for pThread's rwlock
 * implementation, therefore we must fallback to using Mutexes instead.
 */

#if defined(QCC_OS_GROUP_POSIX) && !defined(QCC_OS_ANDROID)
#include <qcc/posix/RWLock.h>

#elif defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/RWLock.h>

#else
#include <qcc/Mutex.h>

namespace qcc {

/**
 * The simple fallback implementation of RWLock using Mutex.
 */
class RWLock {

  public:
    /**
     * The constructor initializes the underlying rwlock implementation.
     */
    RWLock() { }

    /**
     * The destructor will destroy the underlying rwlock.
     */
    ~RWLock() { }

    /**
     * Acquires a lock on the rwlock.  If another thread is holding the lock,
     * then this function will block until the other thread has released its
     * lock.
     *
     * @return  ER_OK if the lock was acquired, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus RDLock() { return mutex.Lock(); }
    QStatus WRLock() { return mutex.Lock(); }

    /**
     * Releases a lock on the rwlock.  This will only release a lock for the
     * current thread if that thread was the one that aquired the lock in the
     * first place.
     *
     * @return  ER_OK if the lock was released, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Unlock() { return mutex.Unlock(); }

    /**
     * Attempt to acquire a lock on a rwlock. If another thread is holding the lock
     * this function return false otherwise the lock is acquired and the function returns true.
     *
     * @return  True if the lock was acquired.
     */
    bool TryRDLock() { return mutex.TryLock(); }
    bool TryWRLock() { return mutex.TryLock(); }

    /**
     * Rwlock copy constructor creates a new rwlock.
     */
    RWLock(const RWLock& other) {  }

    /**
     * Rwlock assignment operator.
     */
    RWLock& operator=(const RWLock& other) { return *this; }

  private:
    qcc::Mutex mutex;  ///< Use a mutex.
};

}

#endif  // Default platform type.

#endif  // _QCC_RWLOCK_H