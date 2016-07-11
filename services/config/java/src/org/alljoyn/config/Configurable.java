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
package org.alljoyn.config;

import java.util.Map;
import org.alljoyn.bus.Status;

public interface Configurable {

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
        WRITE   //!< WRITE    Property that has  WRITE  enabled -- Config
    }

    /**
     * Reset all the properties in the store
     */
    void factoryReset();

    /**
     * Get all the properties that are stored for the given language
     * @param languageTag IETF language tag
     * @param filter classifies the properties to get: for announcement,
     *               for get-about, for get-config
     * @param dataMap the map to fill
     */
    Status readAll(String languageTag, Filter filter, Map<String, Object> dataMap);

    /**
     * Update a property value for a given language.
     * @param key the property key
     * @param languageTag the language in which this property should be set
     * @param newValue the property value
     */
    Status update(String key, String languageTag, Object newValue);

    /**
     * Reset a property for a given language
     * @param key the property name
     * @param languageTag the language in which this property should be set
     */
    Status reset(String key, String languageTag);
}

