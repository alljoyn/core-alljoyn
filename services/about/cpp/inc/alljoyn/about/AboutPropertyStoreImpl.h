/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

/*
 *
 */
class AboutPropertyStoreImpl : public PropertyStore {
  public:

    AboutPropertyStoreImpl();

    virtual ~AboutPropertyStoreImpl();

    virtual QStatus ReadAll(const char* languageTag, Filter filter, ajn::MsgArg& all);

    virtual QStatus Update(const char* name, const char* languageTag, const ajn::MsgArg* value);

    virtual QStatus Delete(const char* name, const char* languageTag);

    PropertyStoreProperty* getProperty(PropertyStoreKey propertyKey);

    PropertyStoreProperty* getProperty(PropertyStoreKey propertyKey, qcc::String const& language);

    QStatus setDeviceId(qcc::String const& deviceId, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    QStatus setDeviceName(qcc::String const& deviceName, bool isPublic = true, bool isWritable = true, bool isAnnouncable = true);

    QStatus setAppId(qcc::String const& appId, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    QStatus setAppName(qcc::String const& appName, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    QStatus setDefaultLang(qcc::String const& defaultLang, bool isPublic = true, bool isWritable = true, bool isAnnouncable = true);

    QStatus setSupportedLangs(std::vector<qcc::String> const& supportedLangs, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);

    QStatus setDescription(qcc::String const& description, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);

    QStatus setManufacturer(qcc::String const& manufacturer, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    QStatus setDateOfManufacture(qcc::String const& dateOfManufacture, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);

    QStatus setSoftwareVersion(qcc::String const& softwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);

    QStatus setAjSoftwareVersion(qcc::String const& ajSoftwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);

    QStatus setHardwareVersion(qcc::String const& hardwareVersion, bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);

    QStatus setModelNumber(qcc::String const& modelNumber, bool isPublic = true, bool isWritable = false, bool isAnnouncable = true);

    QStatus setSupportUrl(qcc::String const& supportUrl, qcc::String const& language = "", bool isPublic = true, bool isWritable = false, bool isAnnouncable = false);

    static qcc::String const& getPropertyStoreName(PropertyStoreKey propertyStoreKey);

  protected:

    PropertyMap m_Properties;

    static qcc::String PropertyStoreName[NUMBER_OF_KEYS + 1];

    QStatus isLanguageSupported(const char* language);

    std::vector<qcc::String> m_SupportedLangs;

    bool removeExisting(PropertyStoreKey propertyKey);

    bool removeExisting(PropertyStoreKey propertyKey, qcc::String const& language);

    QStatus validateValue(PropertyStoreKey propertyKey, ajn::MsgArg const& value, qcc::String const& languageTag = "");

    QStatus setProperty(PropertyStoreKey propertyKey, const qcc::String& value, bool isPublic, bool isWritable, bool isAnnouncable);

    QStatus setProperty(PropertyStoreKey propertyKey, const qcc::String& value, const qcc::String& language, bool isPublic,
                        bool isWritable, bool isAnnouncable);
};

} /* namespace services */
} /* namespace ajn */
#endif /* ABOUTPROPERTYSTOREIMPL_H_ */

