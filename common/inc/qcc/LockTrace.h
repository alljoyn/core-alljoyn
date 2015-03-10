#ifndef _QCC_LOCKTRACE_H
#define _QCC_LOCKTRACE_H
/**
 * @file
 *
 * This file defines a class for debugging thread deadlock problems
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
