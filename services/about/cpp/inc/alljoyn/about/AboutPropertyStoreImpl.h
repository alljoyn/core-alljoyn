/******************************************************************************
 * Copyright (c) 2013 - 2014, AllSeen Alliance. All rights reserved.
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

typedef std::multimap<PropertyStoreKey, PropertyStoreProperty> PropertyMap;
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
     * @param[out] languageTag
     * @param[out] filter
     * @param[in] all
     * @return Status.OK if successful.
     */
    virtual QStatus ReadAll(const char* languageTag, Filter filter, ajn::MsgArg& all);

    /**
     * Update
     * @param[out] name
     * @param[out] languageTag
     * @param[out] value
     * @return Status.OK if successful.
     */
    virtual QStatus Update(const char* name, const char* languageTag, const ajn::MsgArg* value);

    /**
     * Delete
     * @param[out] name
     * @param[out] languageTag
     * @return Status.OK if successful.
     */
    virtual QStatus Delete(const char* name, const char* languageTag);

    /**
     * getProperty
     * @param[out] propertyKey
     * @return PropertyStoreProperty.
     */
    PropertyStoreProperty* getProperty(PropertyStoreKey propertyKey);

    /**
     * getProperty
     * @param[out] propertyKey
     * @param[out] language
     * @return PropertyStoreProperty.
     */
    PropertyStoreProperty* getProperty(PropertyStoreKey propertyKey, qcc::String const& language);

    /**
     * setDeviceId
     * @param[out] deviceId
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus
     */
    QStatus setDeviceId(qcc::String const& deviceId, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    /**
     * @deprecated
     *
     * setDeviceName
     * @param[out] deviceName
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setDeviceName(qcc::String const & deviceName, bool isPublic = true, bool isWritable = true, bool isAnnouncable = true)) {
        return setDeviceName(deviceName, "", isPublic, isWritable, isAnnouncable);
    }
    /**
     * setDeviceName
     * @param[out] deviceName
     * @param[out] language
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setDeviceName(qcc::String const& deviceName, qcc::String const& language, bool isPublic = true, bool isWritable = true, bool isAnnouncable = true);
    /**
     * setDeviceName
     * @param[out] deviceName
     * @param[out] language
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setDeviceName(qcc::String const& deviceName, const char* language, bool isPublic = true, bool isWritable = true, bool isAnnouncable = true);
    /**
     * setAppId
     * @param[out] appId
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setAppId(qcc::String const& appId, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);
    /**
     * setAppName
     * @param[out] appName
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setAppName(qcc::String const& appName, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);
    /**
     * setDefaultLang
     *
     * The default language must already be in the list of supported languages or
     * it will return status ER_LANGUAGE_NOT_SUPPORTED
     *
     * @param[out] defaultLang
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
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
     * @param[out] supportedLangs
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setSupportedLangs(std::vector<qcc::String> const& supportedLangs, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setDescription
     * @param[out] description
     * @param[out] language
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setDescription(qcc::String const& description, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setManufacturer
     * @param[out] manufacturer
     * @param[out] language
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setManufacturer(qcc::String const& manufacturer, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);
    /**
     * setDateOfManufacture
     *
     * Using format YYYY-MM-DD (Known as XML DateTime Format)
     * @param[out] dateOfManufacture
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setDateOfManufacture(qcc::String const& dateOfManufacture, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setSoftwareVersion
     * @param[out] softwareVersion
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setSoftwareVersion(qcc::String const& softwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setAjSoftwareVersion
     * @param[out] ajSoftwareVersion
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setAjSoftwareVersion(qcc::String const& ajSoftwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setHardwareVersion
     * @param[out] hardwareVersion
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setHardwareVersion(qcc::String const& hardwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * setModelNumber
     * @param[out] modelNumber
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setModelNumber(qcc::String const& modelNumber, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);
    /**
     * setSupportUrl
     * @param[out] supportUrl
     * @param[out] language
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus.
     */
    QStatus setSupportUrl(qcc::String const& supportUrl, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);
    /**
     * getPropertyStoreName
     * @param[out] propertyStoreKey
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
     * @param[out] language
     * @return QStatus
     */
    QStatus isLanguageSupported(const char* language);

    /**
     * m_SupportedLangs Stores the supported languages
     */
    std::vector<qcc::String> m_SupportedLangs;

    /**
     * removeExisting
     * @param[out] propertyKey
     * @return bool
     */
    bool removeExisting(PropertyStoreKey propertyKey);
    /**
     * removeExisting
     * @param[out] propertyKey
     * @param[out] language
     * @return bool
     */
    bool removeExisting(PropertyStoreKey propertyKey, qcc::String const& language);
    /**
     * validateValue
     * @param[out] propertyKey
     * @param[out] value
     * @param[out] languageTag
     * @return QStatus
     */
    QStatus validateValue(PropertyStoreKey propertyKey, ajn::MsgArg const& value, qcc::String const& languageTag = "");
    /**
     * setProperty
     * @param[out] propertyKey
     * @param[out] value
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus
     */
    QStatus setProperty(PropertyStoreKey propertyKey, const qcc::String& value, bool isPublic, bool isWritable, bool isAnnouncable);
    /**
     * setProperty
     * @param[out] propertyKey
     * @param[out] value
     * @param[out] language
     * @param[out] isPublic
     * @param[out] isWritable
     * @param[out] isAnnouncable
     * @return QStatus
     */
    QStatus setProperty(PropertyStoreKey propertyKey, const qcc::String& value, const qcc::String& language, bool isPublic,
                        bool isWritable, bool isAnnouncable);
};

} /* namespace services */
} /* namespace ajn */
#endif /* ABOUTPROPERTYSTOREIMPL_H_ */

