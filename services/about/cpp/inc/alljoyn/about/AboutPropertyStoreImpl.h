/**
 * @file
 * Default implementation of the Property store
 */
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

#ifndef ABOUTPROPERTYSTOREIMPL_H_
#define ABOUTPROPERTYSTOREIMPL_H_

#include <stdio.h>
#include <iostream>

#include <qcc/platform.h>
#include <alljoyn/about/AboutServiceApi.h>
#include <alljoyn/about/PropertyStore.h>
#include <alljoyn/about/PropertyStoreProperty.h>

namespace ajn {
namespace services {

/**
 * enumerated list used to retrieve property store Field Names
 */
typedef enum {
    DEVICE_ID = 0,
    DEVICE_NAME = 1,
    APP_ID = 2,
    APP_NAME = 3,
    DEFAULT_LANG = 4,
    SUPPORTED_LANGS = 5,
    DESCRIPTION = 6,
    MANUFACTURER = 7,
    DATE_OF_MANUFACTURE = 8,
    MODEL_NUMBER = 9,
    SOFTWARE_VERSION = 10,
    AJ_SOFTWARE_VERSION = 11,
    HARDWARE_VERSION = 12,
    SUPPORT_URL = 13,
    NUMBER_OF_KEYS = 14
} PropertyStoreKey;

/**
 * multimap that will hold PropertyStoreProperties for each PropertyStoreKey
 * Used to hold properties that are localizable
 */
typedef std::multimap<PropertyStoreKey, PropertyStoreProperty> PropertyMap;

/**
 * Relate PropertyStoreKey with its PropertyStorePoperty
 * Used to hold properties that are not localizable
 */
typedef std::pair<PropertyStoreKey, PropertyStoreProperty> PropertyPair;

/**
 * AboutPropertyStoreImpl
 * About property store implementation
 */
class AboutPropertyStoreImpl : public PropertyStore {
  public:

    /**
     * AboutPropertyStoreImpl
     */
    AboutPropertyStoreImpl();

    /**
     * ~AboutPropertyStoreImpl
     */
    virtual ~AboutPropertyStoreImpl();

    /**
     * ReadAll
     * @param[in] languageTag
     * @param[in] filter
     * @param[out] all
     * @return
     *  - ER_OK if successful.
     *  - an error status if otherwise
     */
    virtual QStatus ReadAll(const char* languageTag, Filter filter, ajn::MsgArg& all);

    /**
     * Update
     * @param[in] name
     * @param[in] languageTag
     * @param[in] value
     * @return
     *  - ER_OK if successful.
     *  - an error status if otherwise
     */
    virtual QStatus Update(const char* name, const char* languageTag, const ajn::MsgArg* value);

    /**
     * Delete
     * @param[in] name
     * @param[in] languageTag
     * @return
     *  - ER_OK if successful.
     *  - an error status if otherwise
     */
    virtual QStatus Delete(const char* name, const char* languageTag);

    /**
     * getProperty
     * @param[in] propertyKey
     * @return PropertyStoreProperty.
     */
    PropertyStoreProperty* getProperty(PropertyStoreKey propertyKey);

    /**
     * getProperty
     * @param[in] propertyKey
     * @param[in] language
     * @return PropertyStoreProperty.
     */
    PropertyStoreProperty* getProperty(PropertyStoreKey propertyKey, qcc::String const& language);

    /**
     * setDeviceId
     * @param[in] deviceId
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus
     */
    QStatus setDeviceId(qcc::String const& deviceId, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    /**
     * setDeviceName
     * @param[in] deviceName
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setDeviceName(qcc::String const& deviceName, qcc::String const& language = "", bool isPublic = true, bool isWritable = true, bool isAnnouncable = true);

    /**
     * setAppId
     * @param[in] appId
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setAppId(qcc::String const& appId, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    /**
     * setAppName
     * @param[in] appName
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setAppName(qcc::String const& appName, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    /**
     * setDefaultLang
     *
     * The default language must already be in the list of supported languages or
     * it will return status ER_LANGUAGE_NOT_SUPPORTED
     *
     * @param[in] defaultLang
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus
     *
     *   - ER_OK on success
     *   - ER_LANGUAGE_NOT_SUPPORTED if language is not in supporte languages
     *   - status indicating failure
     *
     */
    QStatus setDefaultLang(qcc::String const& defaultLang, bool isPublic = true, bool isWritable = true, bool isAnnouncable = true);
    /**
     * setSupportedLangs
     * @param[in] supportedLangs
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setSupportedLangs(std::vector<qcc::String> const& supportedLangs, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setDescription
     * @param[in] description
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setDescription(qcc::String const& description, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setManufacturer
     * @param[in] manufacturer
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setManufacturer(qcc::String const& manufacturer, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);
    /**
     * setDateOfManufacture
     *
     * Using format YYYY-MM-DD (Known as XML DateTime Format)
     * @param[in] dateOfManufacture
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setDateOfManufacture(qcc::String const& dateOfManufacture, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setSoftwareVersion
     * @param[in] softwareVersion
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setSoftwareVersion(qcc::String const& softwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setAjSoftwareVersion
     * @param[in] ajSoftwareVersion
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setAjSoftwareVersion(qcc::String const& ajSoftwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setHardwareVersion
     * @param[in] hardwareVersion
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setHardwareVersion(qcc::String const& hardwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setModelNumber
     * @param[in] modelNumber
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setModelNumber(qcc::String const& modelNumber, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);
    /**
     * setSupportUrl
     * @param[in] supportUrl
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QStatus setSupportUrl(qcc::String const& supportUrl, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * getPropertyStoreName
     * @param[in] propertyStoreKey
     * @return static qcc::String const&
     */
    static qcc::String const& getPropertyStoreName(PropertyStoreKey propertyStoreKey);

  protected:

    /**
     * m_Properties
     */
    PropertyMap m_Properties;

    /**
     * m_PropertyStoreName
     */
    static qcc::String PropertyStoreName[NUMBER_OF_KEYS + 1];

    /**
     * isLanguageSupported
     * @param[in] language
     * @return
     *   - ER_OK if language is supported
     *   - ER_LANGUAGE_NOT_SUPPORTED if language is not supported
     */
    QStatus isLanguageSupported(const char* language);

    /**
     * m_SupportedLangs Stores the supported languages
     */
    std::vector<qcc::String> m_SupportedLangs;

    /**
     * removeExisting
     * @param[in] propertyKey the value to be removed from the PropertyStore
     * @return true if property was found and removed.
     * returning false most likely means the property key was not found
     */
    bool removeExisting(PropertyStoreKey propertyKey);
    /**
     * removeExisting
     * @param[in] propertyKey
     * @param[in] language
     * @return true if property was found and removed.
     * returning false most likely means the property key was nto found or the was
     * not a key for the specified language
     */
    bool removeExisting(PropertyStoreKey propertyKey, qcc::String const& language);
    /**
     * validateValue
     * @param[in] propertyKey
     * @param[in] value
     * @param[in] languageTag
     * @return QStatus
     */
    QStatus validateValue(PropertyStoreKey propertyKey, ajn::MsgArg const& value, qcc::String const& languageTag = "");
    /**
     * setProperty
     * @param[in] propertyKey
     * @param[in] value
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus
     */
    QStatus setProperty(PropertyStoreKey propertyKey, const qcc::String& value, bool isPublic, bool isWritable, bool isAnnouncable);
    /**
     * setProperty
     * @param[in] propertyKey
     * @param[in] value
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus
     */
    QStatus setProperty(PropertyStoreKey propertyKey, const qcc::String& value, const qcc::String& language, bool isPublic,
                        bool isWritable, bool isAnnouncable);
};

} /* namespace services */
} /* namespace ajn */
#endif /* ABOUTPROPERTYSTOREIMPL_H_ */

