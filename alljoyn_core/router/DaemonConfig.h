/**
 * @file
 * Daemon configuration
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#ifndef _DAEMONCONFIG_H
#define _DAEMONCONFIG_H

#include <vector>

#include <qcc/platform.h>
#include <qcc/XmlElement.h>
#include <qcc/String.h>


/**
 *
 */
namespace ajn {

class DaemonConfig {

  public:

    /**
     * Load a configuration creating the singleton if needed.
     *
     * @param configXml  Character string for the configuration xml
     */
    static DaemonConfig* Load(const char* configXml);

    /**
     * Load a configuration creating the singleton if needed.
     *
     * @param configXml  A source containing the configuration xml
     */
    static DaemonConfig* Load(qcc::Source& configSrc);

    /**
     * Return the configuration singleton
     */
    static DaemonConfig* Access() { assert(singleton); return singleton; }

    /**
     * Release the configuration singleton
     */
    static void Release() {
        delete singleton;
        singleton = NULL;
    }

    /**
     * Get an integer configuration value. See Get(const char* key) for more information about the
     * key.
     *
     * @param key         The key is a dotted path to a value in the XML
     * @param defaultVal  The default value if the key is not present
     */
    uint32_t Get(const char* key, uint32_t defaultVal);

    /**
     * Get a string configuration value. The key is a path name to the configuration value
     * expressed as dotted name for the nested tags with an optional attribute specifier
     * at the end separated from the dotted name by a '@' character.
     *
     * Given the configuration XML below Get("foo/value@first") will return "hello" and Get("foo/value@second")
     * returns "world". Note that the outermost tag (in this <config> is implicit and should not be specified.
     *
     * <config>
     *    <foo>
     *       <value first="hello"/>
     *       <value second="world"/>
     *    </foo>
     * </config>
     *
     * @param key   The key is a dotted path (with optional attribute) to a value in the XML
     */
    qcc::String Get(const char* key, const char* defaultVal = NULL);

    /**
     * Get a vector of configuration values that share the same key. The values are the tag
     * contents, attributes are not allowed in this case.
     *
     * @param key   The key is a dotted path to a value in the XML
     */
    std::vector<qcc::String> GetList(const char* key);

    /**
     * Check if the configuration has a specific key.
     */
    bool Has(const char* key);

  private:

    DaemonConfig();

    /*
     * Copy constructor private and undefined
     */
    DaemonConfig(const DaemonConfig& other);

    /*
     * Assignment operator private and undefined
     */
    DaemonConfig& operator=(const DaemonConfig& other);

    ~DaemonConfig();

    qcc::XmlElement* config;

    static DaemonConfig* singleton;

};

};

#endif
