/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.services.common;

import java.util.Map;

/**
 * A generic Property store definition. Needs to be implemented per platform.
 * The application creates this store and passes it to the About and Config
 * services that use it for Announcements and for Get/Set data support.
 * The application is responsible for the persistence of this store.
 */
public interface PropertyStore {

    /**
     * Classifies the properties by purpose: for announcement, for get-about, for get-config
     */
    enum Filter {
        /**
         * Properties that should be announced
         */
        ANNOUNCE,

        /**
         * Read only properties that should be retrieved by 'About' requests
         */
        READ,

        /**
         * Read-Write properties that are available through the 'Config' interface
         */
        WRITE,   //!< WRITE    Property that has  WRITE  enabled -- Config
    }

    /**
     * Get all the properties that are stored for the given language
     * @param languageTag IETF language tag
     * @param filter classifies the properties to get: for announcement,
     *               for get-about, for get-config
     * @param dataMap the map to fill
     * @throws PropertyStoreException if language is not supported.
     */
    void readAll(String languageTag, Filter filter, Map<String, Object> dataMap) throws PropertyStoreException;

    /**
     * Update a property value for a given language.
     * @param key the property name
     * @param languageTag the language in which this property should be set
     * @param newValue the property value
     * @throws PropertyStoreException if language isn't supported, key not found, or illegal value
     */
    void update(String key, String languageTag, Object newValue) throws PropertyStoreException;

    /**
     * Reset a property for a given language
     * @param key the property name
     * @param languageTag the language in which this property should be set
     * @throws PropertyStoreException if language isn't supported, or key not found
     */
    void reset(String key, String languageTag) throws PropertyStoreException;

    /**
     * Reset all the properties in the store
     * @throws PropertyStoreException indicating failure to reset all the properties in the store
     */
    void resetAll() throws PropertyStoreException;

}
