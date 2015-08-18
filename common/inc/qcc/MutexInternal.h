/**
 * @file
 *
 * Internal functionality of the Mutex class.
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
#ifndef NDEBUG
#ifndef _QCC_MUTEXINTERNAL_H
#define _QCC_MUTEXINTERNAL_H

#include <qcc/platform.h>
#include <qcc/Mutex.h>
#include <qcc/LockCheckerLevel.h>

namespace qcc {

/**
 * Represents the non-public functionality of a Mutex abstraction class.
 */
class Mutex::Internal {
  public:
    /* Default constructor */
    Internal(int level);

    /**
     * Set the LockChecker level value after the Mutex has been constructed.
     *
     * @param _level Lock level used on Debug builds to detect out-of-order lock acquires.
     */
    void SetLevel(LockCheckerLevel level);

    /* 
     * Source code file where this lock has been acquired or released last time,
     * if MUTEX_CONTEXT has been used for Lock/Unlock.
     */
    const char* m_file;

    /* 
     * Source code line number where this lock has been acquired or released last time,
     * if MUTEX_CONTEXT has been used for Lock/Unlock.
     */
    int32_t m_line;

    /* If the level value is not 0 or -1, LockChecker verification is enabled for this lock */
    LockCheckerLevel m_level;

    /* Maximum number of recursive acquires, from any thread using this lock */
    uint32_t m_maximumRecursionCount;

private:
    /* Copy constructor is private */
    Internal(const Mutex& other);

    /* Assignment operator is private */
    Internal& operator=(const Internal& other);
};

} /* namespace */

#endif /* _QCC_MUTEXINTERNAL_H */
#endif /* NDEBUG */
