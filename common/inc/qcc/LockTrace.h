#ifndef _QCC_LOCKTRACE_H
#define _QCC_LOCKTRACE_H
/**
 * @file
 *
 * This file defines a class for debugging thread deadlock problems
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
#include <qcc/String.h>
#include <qcc/Debug.h>
#include <deque>
#include <map>

namespace qcc {

class Thread;

class LockTrace {

  public:

    LockTrace(Thread* thread) : thread(thread) { }

    /**
     * Called when a mutex has been acquired
     *
     * @param mutex  The mutex that was acquired
     * @param file   The file that acquired the mutex
     * @param line   The line in the file that acquired the mutex
     */
    void Acquired(qcc::Mutex* mutex, qcc::String file, uint32_t line);

    /**
     * Called when a thread is waiting to acquire a mutex
     *
     * @param mutex  The mutex that was acquired
     * @param file   The file that acquired the mutex
     * @param line   The line in the file that acquired the mutex
     */
    void Waiting(qcc::Mutex* mutex, qcc::String file, uint32_t line);

    /**
     * Called when a mutex is about to be released
     *
     * @param mutex  The mutex that was released
     * @param file   The file that released the mutex
     * @param line   The line in the file that released the mutex
     */
    void Releasing(qcc::Mutex* mutex, qcc::String file, uint32_t line);

    /**
     * Dump lock trace information for a specific thread
     */
    void Dump();

  private:

    struct Info {
        Info(const qcc::Mutex* mutex, qcc::String file, uint32_t line) : mutex(mutex), file(file), line(line) { }
        const qcc::Mutex* mutex;
        qcc::String file;
        uint32_t line;
    };

    Thread* thread;

    std::deque<Info> queue;

};

};

#endif