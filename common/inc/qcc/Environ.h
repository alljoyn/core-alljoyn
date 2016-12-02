/**
 * @file
 *
 * Define a class for accessing environment variables.
 */

/******************************************************************************
 *
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _QCC_ENVIRON_H
#define _QCC_ENVIRON_H

#include <qcc/platform.h>
#include <map>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Stream.h>
#include <qcc/LockLevel.h>
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
     * Create a new envionment (useful when launching other programs). Lock checking
     * is disabled to allow lock order checking to use Environ.
     */
    Environ() : lock(LOCK_LEVEL_CHECKING_DISABLED) { }

    /**
     * Return a pointer to the Environ instance that applies to the running application.
     *
     * @return  Pointer to the environment variable singleton.
     */
    static Environ * AJ_CALL GetAppEnviron();

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
    const_iterator Begin() const { return vars.begin(); }

    /**
     * Return an iterator to one past the last environment variable.
     *
     * @return  A const_iterator pointing to the end of the environment variables.
     */
    const_iterator End() const { return vars.end(); }

    /**
     * Return the number of entries in the environment.
     *
     * @return  Number of entries in the environment.
     */
    size_t Size() const { return vars.size(); }

  private:
    static void Init();
    static void Shutdown();
    friend class StaticGlobals;

    std::map<qcc::String, qcc::String> vars;    ///< Environment variable storage.
    qcc::Mutex lock;                            ///< Mutex to make operations thread-safe.
};

}

#endif