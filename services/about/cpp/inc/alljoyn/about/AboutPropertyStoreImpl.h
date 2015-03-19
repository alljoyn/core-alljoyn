/**
 * @file
 * Default implementation of the Property store
 */
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
 *
 * @deprecated The AboutPropertyStoreImpl class has been deprecated please see the
 * AboutData class for similar functionality as the AboutPropertyStoreImpl class.
 */
class AboutPropertyStoreImpl : public PropertyStore {
  public:

    /**
     * AboutPropertyStoreImpl
     *
     * @deprecated The AboutPropertyStoreImpl class has been deprecated please see the
     * AboutData class.
     */
    QCC_DEPRECATED(AboutPropertyStoreImpl());

    /**
     * ~AboutPropertyStoreImpl
     *
     * @deprecated The AboutPropertyStoreImpl class has been deprecated please see the
     * AboutData class.
     */
    QCC_DEPRECATED(virtual ~AboutPropertyStoreImpl());

    /**
     * ReadAll
     * @param[in] languageTag
     * @param[in] filter
     * @param[out] all
     * @return
     *  - ER_OK if successful.
     *  - an error status if otherwise
     */
    QCC_DEPRECATED(virtual QStatus ReadAll(const char* languageTag, Filter filter, ajn::MsgArg & all));

    /**
     * Update
     *
     * @deprecated The AboutPropertyStoreImpl class has been deprecated please
     * see the AboutData class.
     *
     * @param[in] name
     * @param[in] languageTag
     * @param[in] value
     * @return
     *  - ER_OK if successful.
     *  - an error status if otherwise
     */
    QCC_DEPRECATED(virtual QStatus Update(const char* name, const char* languageTag, const ajn::MsgArg * value));

    /**
     * Delete
     *
     * @deprecated The AboutPropertyStoreImpl class has been deprecated please
     * see the AboutData class.
     *
     * @param[in] name
     * @param[in] languageTag
     * @return
     *  - ER_OK if successful.
     *  - an error status if otherwise
     */
    QCC_DEPRECATED(virtual QStatus Delete(const char* name, const char* languageTag));

    /**
     * getProperty
     *
     * @deprecated The AboutPropertyStoreImpl::getProperty function has been
     * deprecated please see the AboutData::GetField function.
     *
     * @param[in] propertyKey
     * @return PropertyStoreProperty.
     */
    QCC_DEPRECATED(PropertyStoreProperty * getProperty(PropertyStoreKey propertyKey));

    /**
     * getProperty
     *
     * @deprecated The AboutPropertyStoreImpl::getProperty function has been
     * deprecated please see the AboutData::GetField function.
     *
     * @param[in] propertyKey
     * @param[in] language
     * @return PropertyStoreProperty.
     */
    QCC_DEPRECATED(PropertyStoreProperty * getProperty(PropertyStoreKey propertyKey, qcc::String const & language));

    /**
     * setDeviceId
     *
     * @deprecated The AboutPropertyStoreImpl::setDeviceId function has been
     * deprecated please see the AboutData::SetDeviceId function.
     *
     * @param[in] deviceId
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus
     */
    QCC_DEPRECATED(QStatus setDeviceId(qcc::String const & deviceId, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true));

    /**
     * setDeviceName
     *
     * @deprecated The AboutPropertyStoreImpl::setDeviceName function has been
     * deprecated please see the AboutData::SetDeviceName function.
     *
     * @param[in] deviceName
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setDeviceName(qcc::String const & deviceName, qcc::String const& language = "", bool isPublic = true, bool isWritable = true, bool isAnnouncable = true));

    /**
     * setAppId
     *
     * @deprecated The AboutPropertyStoreImpl::setAppId function has been
     * deprecated please see the AboutData::SetAppId function.
     *
     * @param[in] appId
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setAppId(qcc::String const & appId, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true));

    /**
     * setAppName
     *
     * @deprecated The AboutPropertyStoreImpl::setAppName function has been
     * deprecated please see the AboutData::SetAppName function.
     *
     * @param[in] appName
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setAppName(qcc::String const & appName, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = true));

    /**
     * setDefaultLang
     *
     * The default language must already be in the list of supported languages or
     * it will return status ER_LANGUAGE_NOT_SUPPORTED
     *
     * @deprecated The AboutPropertyStoreImpl::setDefaultLang function has been
     * deprecated please see the AboutData::SetDefaultLanguage function.
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
    QCC_DEPRECATED(QStatus setDefaultLang(qcc::String const & defaultLang, bool isPublic = true, bool isWritable = true, bool isAnnouncable = true));

    /**
     * setSupportedLangs
     *
     * @deprecated The AboutPropertyStoreImpl::setSupportedLangs function has been
     * deprecated please see the AboutData::SetSupportedLanguage function.
     *
     * @param[in] supportedLangs
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setSupportedLangs(std::vector<qcc::String> const & supportedLangs, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false));

    /**
     * setDescription
     *
     * @deprecated The AboutPropertyStoreImpl::setDescription function has been
     * deprecated please see the AboutData::SetDescription function.
     *
     * @param[in] description
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setDescription(qcc::String const & description, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = false));

    /**
     * setManufacturer
     *
     * @deprecated The AboutPropertyStoreImpl::setManufacturer function has been
     * deprecated please see the AboutData::SetManufacturer function.
     *
     * @param[in] manufacturer
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setManufacturer(qcc::String const & manufacturer, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = true));

    /**
     * setDateOfManufacture
     *
     * @deprecated The AboutPropertyStoreImpl::setManufacturer function has been
     * deprecated please see the AboutData::SetManufacturer function.
     *
     * Using format YYYY-MM-DD (Known as XML DateTime Format)
     * @param[in] dateOfManufacture
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setDateOfManufacture(qcc::String const & dateOfManufacture, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false));

    /**
     * setSoftwareVersion
     *
     * @deprecated The AboutPropertyStoreImpl::setSoftwareVersion function has been
     * deprecated please see the AboutData::SetSoftwareVersion function.
     *
     * @param[in] softwareVersion
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setSoftwareVersion(qcc::String const & softwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false));

    /**
     * setAjSoftwareVersion
     *
     * @deprecated The AboutPropertyStoreImpl::setAjSoftwareVersion function has been
     * deprecated please see the AboutData class.
     *
     * @param[in] ajSoftwareVersion
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setAjSoftwareVersion(qcc::String const & ajSoftwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false));

    /**
     * setHardwareVersion
     *
     * @deprecated The AboutPropertyStoreImpl::setHardwareVersion function has been
     * deprecated please see the AboutData::SetHardwareVersion function.
     *
     * @param[in] hardwareVersion
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setHardwareVersion(qcc::String const & hardwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false));

    /**
     * setModelNumber
     *
     * @deprecated The AboutPropertyStoreImpl::setModelNumber function has been
     * deprecated please see the AboutData::SetModelNumber function.
     *
     * @param[in] modelNumber
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setModelNumber(qcc::String const & modelNumber, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true));

    /**
     * setSupportUrl
     *
     * @deprecated The AboutPropertyStoreImpl::setSupportUrl function has been
     * deprecated please see the AboutData::SetSupportUrl function.
     *
     * @param[in] supportUrl
     * @param[in] language
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus.
     */
    QCC_DEPRECATED(QStatus setSupportUrl(qcc::String const & supportUrl, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = false));

    /**
     * getPropertyStoreName
     *
     * @deprecated The AboutPropertyStoreImpl::getPropertyStoreName function has been
     * deprecated please see the AboutData class.
     *
     * @param[in] propertyStoreKey
     * @return static qcc::String const&
     */
    QCC_DEPRECATED(static qcc::String const & getPropertyStoreName(PropertyStoreKey propertyStoreKey));

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
    QCC_DEPRECATED(QStatus isLanguageSupported(const char* language));

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
    QCC_DEPRECATED(bool removeExisting(PropertyStoreKey propertyKey));
    /**
     * removeExisting
     * @param[in] propertyKey
     * @param[in] language
     * @return true if property was found and removed.
     * returning false most likely means the property key was nto found or the was
     * not a key for the specified language
     */
    QCC_DEPRECATED(bool removeExisting(PropertyStoreKey propertyKey, qcc::String const & language));
    /**
     * validateValue
     * @param[in] propertyKey
     * @param[in] value
     * @param[in] languageTag
     * @return QStatus
     */
    QCC_DEPRECATED(QStatus validateValue(PropertyStoreKey propertyKey, ajn::MsgArg const & value, qcc::String const& languageTag = ""));
    /**
     * setProperty
     * @param[in] propertyKey
     * @param[in] value
     * @param[in] isPublic
     * @param[in] isWritable
     * @param[in] isAnnouncable
     * @return QStatus
     */
    QCC_DEPRECATED(QStatus setProperty(PropertyStoreKey propertyKey, const qcc::String & value, bool isPublic, bool isWritable, bool isAnnouncable));
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
    QCC_DEPRECATED(QStatus setProperty(PropertyStoreKey propertyKey, const qcc::String & value, const qcc::String & language, bool isPublic,
                                       bool isWritable, bool isAnnouncable));
};

} /* namespace services */
} /* namespace ajn */
#endif /* ABOUTPROPERTYSTOREIMPL_H_ */

