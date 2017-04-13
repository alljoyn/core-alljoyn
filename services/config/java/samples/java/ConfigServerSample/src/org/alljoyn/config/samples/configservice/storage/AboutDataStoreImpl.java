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

package org.alljoyn.config.samples.configservice.storage;

import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import org.alljoyn.bus.AboutKeys;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.config.AboutDataStore;

import android.content.Context;

/**
 * A default implementation of the AboutDataStore.
 */
public class AboutDataStoreImpl extends AboutDataImpl implements AboutDataStore {

    public static final String TAG = AboutDataStoreImpl.class.getSimpleName();

    public AboutDataStoreImpl(Context context) {
        super(context);
    }

    /**
     * Read all the properties for a given language, filtered by a criteria
     *
     * @param languageTag
     *            the language in which to retrieve the properties
     * @param filter
     *            filter the results by critreria: for announcement, for
     *            configuration, etc.
     * @param dataMap
     *            a map to fill with the result (to be compatible with the C++
     *            signature)
     * @throws ErrorReplyBusException
     *             if an unsupported language was given
     */
    @Override
    public void readAll(String languageTag, Filter filter, Map<String, Object> dataMap) throws ErrorReplyBusException {
        if (!Property.NO_LANGUAGE.equals(languageTag) && !getLanguages().contains(languageTag)) {
            throw new ErrorReplyBusException(AboutDataStore.UNSUPPORTED_LANGUAGE);
        }
        switch (filter) {
            case ANNOUNCE:
                getAnnouncement(languageTag, dataMap);
                break;
            case READ:
                getAbout(languageTag, dataMap);
                break;
            case WRITE:
                getConfiguration(languageTag, dataMap);
                break;
        }
    }

    /**
     * Update a property value
     *
     * @param key
     *            the property name
     * @param languageTag
     *            the language in which the value should be updated
     * @param newValue
     *            the neew value
     * @throws ErrorReplyBusException
     *             for the cases: UNSUPPORTED_LANGUAGE, INVALID_VALUE,
     *             UNSUPPORTED_KEY, ILLEGAL_ACCESS
     */
    @Override
    public void update(String key, String languageTag, Object newValue) throws ErrorReplyBusException {
        Property property = getAboutConfigMap().get(key);
        if (property == null) {
            throw new ErrorReplyBusException(AboutDataStore.UNSUPPORTED_KEY);
        }
        if (!property.isWritable()) {
            throw new ErrorReplyBusException(AboutDataStore.ILLEGAL_ACCESS);
        }
        if (AboutKeys.ABOUT_DEFAULT_LANGUAGE.equals(key) && !getLanguages().contains(newValue.toString())) {
            throw new ErrorReplyBusException(AboutDataStore.UNSUPPORTED_LANGUAGE);
        }

        languageTag = validateLanguageTag(languageTag, property);
        property.setValue(languageTag, newValue);

        setDefaultLanguageFromProperties();
        // save config map to persistent storage
        storeConfiguration();
    }

    /**
     * Reset the property value for a given language
     *
     * @param key
     *            the property key
     * @param languageTag
     *            the language in which to reset
     * @throws ErrorReplyBusException
     */
    @Override
    public void reset(String key, String languageTag) throws ErrorReplyBusException {
        if (!Property.NO_LANGUAGE.equals(languageTag) && !getLanguages().contains(languageTag)) {
            throw new ErrorReplyBusException(AboutDataStore.UNSUPPORTED_LANGUAGE);
        }

        Property property = getAboutConfigMap().get(key);
        if (property == null) {
            throw new ErrorReplyBusException(AboutDataStore.UNSUPPORTED_KEY);
        }

        languageTag = validateLanguageTag(languageTag, property);
        property.remove(languageTag);

        // save config map to persistent storage
        storeConfiguration();
        loadFactoryDefaults();
        loadStoredConfiguration();

        // since the default language may be reset
        if (AboutKeys.ABOUT_DEFAULT_LANGUAGE.equals(key)) {
            setDefaultLanguageFromProperties();
        }
    }

    /**
     * Reset all the properties in the store
     *
     * @throws ErrorReplyBusException
     */
    @Override
    public void resetAll() throws ErrorReplyBusException {
        // delete cache
        Property appId = getAboutConfigMap().get(AboutKeys.ABOUT_APP_ID);
        getAboutConfigMap().clear();
        // delete persistent storage
        getContext().deleteFile(CONFIG_XML);
        // load factory defaults
        loadFactoryDefaults();
        // TODO restart as a soft AP...
        getAboutConfigMap().put(AboutKeys.ABOUT_APP_ID, appId);
        loadLanguages();
    }

    /**
     * Checks if received languageTag is not {@link Property#NO_LANGUAGE} and
     * exists among supported languages, otherwise
     * {@link AboutDataStore#UNSUPPORTED_LANGUAGE} is thrown. If the
     * received languageTag is {@link Property#NO_LANGUAGE} then it will be set
     * to the default language. If languages attribute of the received property
     * has only one language and it's set to {@link Property#NO_LANGUAGE}, then
     * returned languageTag will be set to {@link Property#NO_LANGUAGE}.
     *
     * @param languageTag
     *            The language tag to be validates.
     * @param property
     *            The {@link Property} that the language tag is validated for.
     * @return The language tag to be used.
     * @throws ErrorReplyBusException
     *             of {@link AboutDataStore#UNSUPPORTED_LANGUAGE}
     */
    private String validateLanguageTag(String languageTag, Property property) throws ErrorReplyBusException {
        if (!Property.NO_LANGUAGE.equals(languageTag) && !getLanguages().contains(languageTag)) {
            throw new ErrorReplyBusException(AboutDataStore.UNSUPPORTED_LANGUAGE);
        }

        Set<String> langs = property.getLanguages();

        // If languageTag equals NO_LANGUAGE, set it to be defaultLanguage
        if (Property.NO_LANGUAGE.equals(languageTag)) {
            languageTag = getDefaultLanguage();
        }

        // In case the field has only one language and it equals NO_LANGUAGE, there
        // will be no possibility to set another value with a different language but NO_LANGUAGE.
        if (langs != null && langs.size() == 1) {
            Iterator<String> iterator = langs.iterator();
            String lang = iterator.next();

            if (Property.NO_LANGUAGE.equals(lang)) {
                // Override the original language tag with the NO_LANGUAGE
                languageTag = Property.NO_LANGUAGE;
            }
        }

        return languageTag;
    }
}
