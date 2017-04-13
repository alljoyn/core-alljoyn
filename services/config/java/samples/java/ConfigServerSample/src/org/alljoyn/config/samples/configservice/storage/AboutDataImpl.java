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

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.UUID;

import javax.xml.parsers.ParserConfigurationException;

import org.alljoyn.bus.AboutKeys;
import org.alljoyn.bus.AboutData;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.Variant;
import org.alljoyn.services.common.TransformUtil;
import org.xml.sax.SAXException;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

/**
 * Reads the factory defaults from assets/Config.xml and creates an AboutData object.
 * Stores custom configuration in local XML files.
 */
public class AboutDataImpl extends AboutData {

    public static final String TAG = AboutDataImpl.class.getSimpleName();

    protected static final String CONFIG_XML = "Config.xml";

    private String mDefaultLanguage = "en";
    private Set<String> mSupportedLanguages = new HashSet<String>();

    /**
     * configuration storage.
     */
    private final AssetManager mAssetMgr;
    private Context mContext;
    private Map<String, Property> mAboutConfigMap;

    public String getDefaultLanguage() {
        return mDefaultLanguage;
    }

    public Set<String> getLanguages() {
        return mSupportedLanguages;
    }

    public Context getContext() {
        return mContext;
    }

    public Map<String, Property> getAboutConfigMap() {
        return mAboutConfigMap;
    }

    public AboutDataImpl(Context context) {
        super();
        this.mContext = context;
        mAssetMgr = context.getAssets();
        mContext = context;

        // read factory defaults from assets/Config.xml
        loadFactoryDefaults();

        // figure out the available languages from the contents of Config.xml
        loadLanguages();

        // read modified-and-persisted configuration and override factory
        // defaults
        loadStoredConfiguration();

        // read the user's default language
        setDefaultLanguageFromProperties();

        // load the unique Id. Create if not exist
        loadAppId();

        // load the device Id. Create if not exist
        loadDeviceId();

        storeConfiguration();
    }

    protected void getConfiguration(String languageTag, Map<String, Object> configuration) {
        for (String key : mAboutConfigMap.keySet()) {
            Property property = mAboutConfigMap.get(key);
            if (!property.isPublic() || !property.isWritable()) {
                continue;
            }
            Object value = property.getValue(languageTag, mDefaultLanguage);
            if (value != null)
                configuration.put(key, value);
        }
    }

    protected void getAbout(String languageTag, Map<String, Object> about) {
        for (String key : mAboutConfigMap.keySet()) {
            Property property = mAboutConfigMap.get(key);
            if (!property.isPublic()) {
                continue;
            }
            Object value = property.getValue(languageTag, mDefaultLanguage);
            if (value != null)
                about.put(key, value);
        }
    }

    protected void getAnnouncement(String languageTag, Map<String, Object> announce) {
        for (String key : mAboutConfigMap.keySet()) {
            Property property = mAboutConfigMap.get(key);
            if (!property.isPublic() || !property.isAnnounced()) {
                continue;
            }
            Object value = property.getValue(languageTag, mDefaultLanguage);
            if (value != null)
                announce.put(key, value);
        }
    }

    /**
     * Set a value for a property
     *
     * @param key
     *            property name
     * @param value
     *            property value
     * @param languageTag
     *            the language for which to set the value
     * @see Property
     */
    public void setValue(String key, Object value, String languageTag) {
        Property property = mAboutConfigMap.get(key);
        if (property == null) {
            property = new Property(key, true, true, true);
            mAboutConfigMap.put(key, property);
        }
        property.setValue(languageTag, value);

    }

    protected void loadLanguages() {
        Set<String> languages = new HashSet<String>(3);
        for (String key : mAboutConfigMap.keySet()) {
            Property property = mAboutConfigMap.get(key);
            Set<String> langs = property.getLanguages();
            if (langs.size() != 0) {
                languages.addAll(langs);
            }
        }
        languages.remove(Property.NO_LANGUAGE);
        mSupportedLanguages = languages;
        Property property = new Property(AboutKeys.ABOUT_SUPPORTED_LANGUAGES, false, false, true);
        property.setValue(Property.NO_LANGUAGE, mSupportedLanguages);
        mAboutConfigMap.put(AboutKeys.ABOUT_SUPPORTED_LANGUAGES, property);
    }

    protected void loadFactoryDefaults() {
        try {
            InputStream is = mAssetMgr.open(CONFIG_XML);
            mAboutConfigMap = PropertyParser.readFromXML(is);
        } catch (IOException e) {
            Log.e(TAG, "Error loading file assets/" + CONFIG_XML, e);
            mAboutConfigMap = createCannedMap();
        } catch (ParserConfigurationException e) {
            Log.e(TAG, "Error parsing xml file assets/" + CONFIG_XML, e);
            mAboutConfigMap = createCannedMap();
        } catch (SAXException e) {
            Log.e(TAG, "Error parsing xml file assets/" + CONFIG_XML, e);
            mAboutConfigMap = createCannedMap();
        }
    }

    protected void loadStoredConfiguration() {
        try {
            if (new File(mContext.getFilesDir() + "/" + CONFIG_XML).exists()) {
                InputStream is = mContext.openFileInput(CONFIG_XML);
                Map<String, Property> storedConfiguration = PropertyParser.readFromXML(is);
                for (String key : storedConfiguration.keySet()) {
                    Property property = mAboutConfigMap.get(key);
                    Property storedProperty = storedConfiguration.get(key);

                    if (storedProperty != null) { // should never be null
                        if (property == null) {
                            mAboutConfigMap.put(key, storedProperty);
                        } else {
                            for (String language : storedProperty.getLanguages()) {
                                Object languageValue = storedProperty.getValue(language, language);
                                property.setValue(language, languageValue);
                            }
                        }
                    }
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Error loading file " + CONFIG_XML, e);
        } catch (ParserConfigurationException e) {
            Log.e(TAG, "Error parsing xml file " + CONFIG_XML, e);
        } catch (SAXException e) {
            Log.e(TAG, "Error parsing xml file " + CONFIG_XML, e);
        }
    }

    protected void storeConfiguration() {
        String localConfigFileName = CONFIG_XML;
        // Note: this one is on the app's folder, not in assets
        try {
            FileOutputStream openFileOutput = mContext.openFileOutput(localConfigFileName, Context.MODE_PRIVATE);
            PropertyParser.writeToXML(openFileOutput, mAboutConfigMap);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (IllegalStateException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    protected Map<String, Property> createCannedMap() {
        Map<String, Property> aboutMap = new HashMap<String, Property>(10);

        Property property = new Property(AboutKeys.ABOUT_APP_NAME, false, true, true);
        property.setValue(mDefaultLanguage, "Demo appname");
        aboutMap.put(AboutKeys.ABOUT_APP_NAME, property);

        property = new Property(AboutKeys.ABOUT_DEVICE_NAME, true, true, true);
        property.setValue(mDefaultLanguage, "Demo device");
        aboutMap.put(AboutKeys.ABOUT_DEVICE_NAME, property);

        property = new Property(AboutKeys.ABOUT_MANUFACTURER, false, true, true);
        property.setValue(mDefaultLanguage, "Demo manufacturer");
        aboutMap.put(AboutKeys.ABOUT_MANUFACTURER, property);

        property = new Property(AboutKeys.ABOUT_DESCRIPTION, false, false, true);
        property.setValue(mDefaultLanguage, "A default app that demonstrates the About/Config feature");
        aboutMap.put(AboutKeys.ABOUT_DESCRIPTION, property);

        property = new Property(AboutKeys.ABOUT_DEFAULT_LANGUAGE, true, true, true);
        property.setValue(mDefaultLanguage, mDefaultLanguage);
        aboutMap.put(AboutKeys.ABOUT_DEFAULT_LANGUAGE, property);

        property = new Property(AboutKeys.ABOUT_SOFTWARE_VERSION, false, false, true);
        property.setValue(mDefaultLanguage, "1.0.0.0");
        aboutMap.put(AboutKeys.ABOUT_SOFTWARE_VERSION, property);

        property = new Property(AboutKeys.ABOUT_AJ_SOFTWARE_VERSION, false, false, true);
        property.setValue(mDefaultLanguage, "3.3.1");
        aboutMap.put(AboutKeys.ABOUT_AJ_SOFTWARE_VERSION, property);

        property = new Property(AboutKeys.ABOUT_MODEL_NUMBER, false, true, true);
        property.setValue(mDefaultLanguage, "S100");
        aboutMap.put(AboutKeys.ABOUT_MODEL_NUMBER, property);

        property = new Property(AboutKeys.ABOUT_SUPPORTED_LANGUAGES, false, false, true);
        property.setValue(mDefaultLanguage, mSupportedLanguages);
        aboutMap.put(AboutKeys.ABOUT_SUPPORTED_LANGUAGES, property);

        return aboutMap;
    }

    protected void setDefaultLanguageFromProperties() {
        Property defaultLanguageProperty = mAboutConfigMap.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
        if (defaultLanguageProperty != null) {
            mDefaultLanguage = (String) defaultLanguageProperty.getValue(Property.NO_LANGUAGE, Property.NO_LANGUAGE);
        }
    }

    /**
     * If appId was not found in factory defaults or on persistent storage, generate it
     */
    public void loadAppId() {
        Property appIdProperty = mAboutConfigMap.get(AboutKeys.ABOUT_APP_ID);

        if (appIdProperty == null || appIdProperty.getValue(Property.NO_LANGUAGE, Property.NO_LANGUAGE) == null) {
            UUID defaultAppId = UUID.randomUUID();
            // String sAppId =
            // String.valueOf(TransformUtil.uuidToByteArray(defaultAppId));
            // here we take the stored about map, and fill gaps by default
            // values. We don't shrink the map - other existing values will remain.
            Property property = new Property(AboutKeys.ABOUT_APP_ID, false, true, true);
            property.setValue(Property.NO_LANGUAGE, defaultAppId);
            mAboutConfigMap.put(AboutKeys.ABOUT_APP_ID, property);
        }
    }

    /**
     * If uniqueId was not found in factory defaults or on persistent storage, generate it
     */
    public void loadDeviceId() {
        Property deviceIdProperty = mAboutConfigMap.get(AboutKeys.ABOUT_DEVICE_ID);

        if (deviceIdProperty == null || deviceIdProperty.getValue(Property.NO_LANGUAGE, Property.NO_LANGUAGE) == null) {
            String defaultDeviceId = String.valueOf("IoE" + System.currentTimeMillis());

            // Here we take the stored about map, and fill gaps by default values.
            // We don't shrink the map - other existing values will remain.
            Property property = new Property(AboutKeys.ABOUT_DEVICE_ID, false, true, true);
            property.setValue(Property.NO_LANGUAGE, defaultDeviceId);
            mAboutConfigMap.put(AboutKeys.ABOUT_DEVICE_ID, property);
        }
    }

    @Override
    public Map<String, Variant> getAboutData(String lang) throws ErrorReplyBusException {
        Map<String, Object> aboutData = new HashMap<String, Object>(3);
        getAbout(lang, aboutData);
        return TransformUtil.toVariantMap(aboutData);
    }

    @Override
    public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException {
        Map<String, Object> announceData = new HashMap<String, Object>(3);
        getAnnouncement(mDefaultLanguage, announceData);
        return TransformUtil.toVariantMap(announceData);
    }
}
