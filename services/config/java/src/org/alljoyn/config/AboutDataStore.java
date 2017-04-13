/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

package org.alljoyn.config;

import org.alljoyn.bus.AboutDataListener;
import org.alljoyn.bus.ErrorReplyBusException;

import java.util.Map;

/**
 * A generic About data store definition. Needs to be implemented per platform.
 * The application creates this store and passes it to the Config service that
 * uses it for Get/Set data support.
 * The application is responsible for the persistence of this store.
 */
public interface AboutDataStore extends AboutDataListener {

    /**
     * The given key is not supported. Used in ErrorReplyBusException error name.
     */
    public final static String UNSUPPORTED_KEY = "org.alljoyn.Error.UnsupportedKey";

    /**
     * The language specified is not supported. Used in ErrorReplyBusException error name.
     */
    public final static String UNSUPPORTED_LANGUAGE = "org.alljoyn.Error.LanguageNotSupported";

    /**
     * Trying to set a read-only field. Used in ErrorReplyBusException error name.
     */
    public final static String ILLEGAL_ACCESS = "org.alljoyn.Error.IllegalAccess";

    /**
     * Trying to set a field to an invalid value. Used in ErrorReplyBusException error name.
     */
    public final static String INVALID_VALUE = "org.alljoyn.Error.InvalidValue";

    /**
     * Property filter has three possible values ANNOUNCE, READ, WRITE.
     */
    enum Filter {
        /**
         * Property has announce enabled
         */
        ANNOUNCE,
        /**
         * Property is read-only
         */
        READ,
        /**
         * Property is read-write
         */
        WRITE
    }

    /**
     * Get all the properties that are stored for the given language.
     * @param languageTag IETF language tag
     * @param filter classifies the properties to get: announced, read-only, read-write
     * @param dataMap the map to fill
     * @throws ErrorReplyBusException if language is not supported. See ErrorReplyBusException#getErrorName().
     */
    void readAll(String languageTag, Filter filter, Map<String, Object> dataMap) throws ErrorReplyBusException;

    /**
     * Update a property value for a given language.
     * @param key the property name
     * @param languageTag the language in which this property should be set
     * @param newValue the property value
     * @throws ErrorReplyBusException if language isn't supported, key not found, illegal value, or illegal access. See ErrorReplyBusException#getErrorName().
     */
    void update(String key, String languageTag, Object newValue) throws ErrorReplyBusException;

    /**
     * Reset a property for a given language.
     * @param key the property name
     * @param languageTag the language in which this property should be set
     * @throws ErrorReplyBusException if language isn't supported, or key not found. See ErrorReplyBusException#getErrorName().
     */
    void reset(String key, String languageTag) throws ErrorReplyBusException;

    /**
     * Reset all the properties in the store.
     * @throws ErrorReplyBusException indicating failure to reset all the properties in the store
     */
    void resetAll() throws ErrorReplyBusException;

}
