/**
 * @file
 *
 * Define a class for accessing environment variables.
 */

/******************************************************************************
 *
 *
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
#ifndef _QCC_ENVIRON_H
#define _QCC_ENVIRON_H

#include <qcc/platform.h>
#include <map>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Stream.h>
#include <Status.h>

namespace qcc {

/**
 * Abstract encapsulation of the system environment variables.
 */
class Environ {
  public:

    /** Environment variable const_iterator */
    typedef std::map<qcc::String, qcc::String>::const_iterator const_iterator;

    /**
     * Create a new envionment (useful when launching other programs).
     */
    Environ(void) { }

    /**
     * Return a pointer to the Environ instance that applies to the running application.
     *
     * @return  Pointer to the environment variable singleton.
     */
    static Environ * AJ_CALL GetAppEnviron(void);

    /**
     * Return a specific environment variable
     */
    qcc::String Find(const qcc::String& key, const char* defaultValue = NULL);

    /**
     * Return a specific environment variable
     */
    qcc::String Find(const char* key, const char* defaultValue = NULL) { return Find(qcc::String(key), defaultValue); }

    /**
     * Preload environment variables with the specified prefix
     */
    void Preload(const char* keyPrefix);

    /**
     * Add an environment variable
     */
    void Add(const qcc::String& key, const qcc::String& value);

    /**
     * Parse a env settings file.
     * Each line is expected to be of the form <key> = <value>
     */
    QStatus Parse(Source& source);

    /**
     * Return an iterator to the first environment variable.
     *
     * @return  A const_iterator pointing to the beginning of the environment variables.
     */
    const_iterator Begin(void) const { return vars.begin(); }

    /**
     * Return an iterator to one past the last environment variable.
     *
     * @return  A const_iterator pointing to the end of the environment variables.
     */
    const_iterator End(void) const { return vars.end(); }

    /**
     * Return the number of entries in the environment.
     *
     * @return  Number of entries in the environment.
     */
    size_t Size(void) const { return vars.size(); }

  private:
    static void Init();
    static void Shutdown();
    friend class StaticGlobals;

    std::map<qcc::String, qcc::String> vars;    ///< Environment variable storage.
    qcc::Mutex lock;                            ///< Mutex to make operations thread-safe.
};

}

#endif
