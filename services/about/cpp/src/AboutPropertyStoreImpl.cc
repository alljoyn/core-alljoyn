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

#include <alljoyn/about/AboutPropertyStoreImpl.h>
#include <qcc/Debug.h>
#include <algorithm>

#define QCC_MODULE "ALLJOYN_ABOUT_PROPERTYSTORE"

namespace ajn {
namespace services {

qcc::String AboutPropertyStoreImpl::PropertyStoreName[NUMBER_OF_KEYS + 1] = { "DeviceId", "DeviceName", "AppId",
                                                                              "AppName", "DefaultLanguage", "SupportedLanguages", "Description", "Manufacturer",
                                                                              "DateOfManufacture", "ModelNumber", "SoftwareVersion", "AJSoftwareVersion", "HardwareVersion", "SupportUrl", "" };

AboutPropertyStoreImpl::AboutPropertyStoreImpl()
{
}

AboutPropertyStoreImpl::~AboutPropertyStoreImpl()
{
}

static char HexToChar(char c)
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return c + 10 - 'a';
    } else if ('A' <= c && c <= 'F') {
        return c + 10 - 'A';
    }

    return -1;
}

static void HexStringToBytes(const qcc::String& hex, uint8_t* outBytes, size_t len)
{
    unsigned char achar, bchar;
    for (size_t i = 0; i < len; i++) {

        achar = HexToChar(hex[i * 2]);
        bchar = HexToChar(hex[i * 2 + 1]);
        outBytes[i] = ((achar << 4) | bchar);
    }
}

QStatus AboutPropertyStoreImpl::isLanguageSupported(const char* language)
{
    if (!language) {
        return ER_LANGUAGE_NOT_SUPPORTED;
    }

    PropertyMap::iterator it = m_Properties.find(SUPPORTED_LANGS);
    if (it == m_Properties.end()) {
        return ER_LANGUAGE_NOT_SUPPORTED;
    }

    if (std::find(m_SupportedLangs.begin(), m_SupportedLangs.end(), qcc::String(language)) == m_SupportedLangs.end()) {
        return ER_LANGUAGE_NOT_SUPPORTED;
    }

    return ER_OK;
}

QStatus AboutPropertyStoreImpl::ReadAll(const char* languageTag, Filter filter, ajn::MsgArg& all)
{
    QStatus status;
    if (filter == ANNOUNCE) {
        /*
         * TODO for now if ReadAll is called and the property store is empty we
         * create an empty array to return. This should probably return an error
         * status if the property store is empty.
         */
        if (m_Properties.size() == 0) {
            // Trying to get properties from an empty property store.
            status = all.Set("a{sv}", 0, NULL);
            if (status != ER_OK) {
                return status;
            }
            all.SetOwnershipFlags(MsgArg::OwnsArgs, true);
            return status;
        }

        PropertyMap::iterator defaultLang = m_Properties.find(DEFAULT_LANG);

        qcc::String defaultLanguage = "";
        if (defaultLang != m_Properties.end()) {
            char* tempdefLang;
            defaultLang->second.getPropertyValue().Get("s", &tempdefLang);
            defaultLanguage.assign(tempdefLang);
        }

        MsgArg* argsAnnounceData = new MsgArg[m_Properties.size()];
        uint32_t announceArgCount = 0;

        do {
            for (PropertyMap::const_iterator it = m_Properties.begin(); it != m_Properties.end(); ++it) {
                const PropertyStoreProperty& property = it->second;

                if (!property.getIsAnnouncable()) {
                    continue;
                }

                // check that it is from the defaultLanguage or empty.
                if (!(property.getLanguage().empty() || property.getLanguage().compare(defaultLanguage) == 0)) {
                    continue;
                }

                status = argsAnnounceData[announceArgCount].Set("{sv}", property.getPropertyName().c_str(),
                                                                new MsgArg(property.getPropertyValue()));
                if (status != ER_OK) {
                    break;
                }
                argsAnnounceData[announceArgCount].SetOwnershipFlags(MsgArg::OwnsArgs, true);
                announceArgCount++;
            }

            status = all.Set("a{sv}", announceArgCount, argsAnnounceData);
            if (status != ER_OK) {
                break;
            }
            all.SetOwnershipFlags(MsgArg::OwnsArgs, true);
        } while (0);
        if (status != ER_OK) {
            delete[] argsAnnounceData;
            return status;
        }
    } else if (filter == READ) {
        /*
         * TODO for now if ReadAll is called and the property store is empty we
         * create an empty array to return. This should probably return an error
         * status if the property store is empty.
         */
        if (m_Properties.size() == 0) {
            // Trying to get properties from an empty property store.
            status = all.Set("a{sv}", 0, NULL);
            if (status != ER_OK) {
                return status;
            }
            all.SetOwnershipFlags(MsgArg::OwnsArgs, true);
            return status;
        }
        if (languageTag != NULL && languageTag[0] != 0) { // check that the language is in the supported languages;
            status = isLanguageSupported(languageTag);
            if (status != ER_OK) {
                return status;
            }
        } else {
            PropertyMap::iterator it = m_Properties.find(DEFAULT_LANG);
            if (it == m_Properties.end()) {
                return ER_LANGUAGE_NOT_SUPPORTED;
            }
            status = it->second.getPropertyValue().Get("s", &languageTag);
            if (status != ER_OK) {
                return status;
            }
        }

        MsgArg* argsReadData = new MsgArg[m_Properties.size()];
        uint32_t readArgCount = 0;
        do {
            for (PropertyMap::const_iterator it = m_Properties.begin(); it != m_Properties.end(); ++it) {
                const PropertyStoreProperty& property = it->second;

                if (!property.getIsPublic()) {
                    continue;
                }

                // check that it is from the defaultLanguage or empty.
                if (!(property.getLanguage().empty() || property.getLanguage().compare(languageTag) == 0)) {
                    continue;
                }

                status = argsReadData[readArgCount].Set("{sv}", property.getPropertyName().c_str(),
                                                        new MsgArg(property.getPropertyValue()));
                if (status != ER_OK) {
                    break;
                }
                argsReadData[readArgCount].SetOwnershipFlags(MsgArg::OwnsArgs, true);
                readArgCount++;
            }

            status = all.Set("a{sv}", readArgCount, argsReadData);
            if (status != ER_OK) {
                break;
            }
            all.SetOwnershipFlags(MsgArg::OwnsArgs, true);
        } while (0);
        if (status != ER_OK) {
            delete[] argsReadData;
            return status;
        }
    } else {
        return ER_NOT_IMPLEMENTED;
    }
    return ER_OK;
}

QStatus AboutPropertyStoreImpl::Update(const char* name, const char* languageTag, const ajn::MsgArg* value)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus AboutPropertyStoreImpl::Delete(const char* name, const char* languageTag)
{
    return ER_NOT_IMPLEMENTED;
}

PropertyStoreProperty* AboutPropertyStoreImpl::getProperty(PropertyStoreKey propertyKey)
{
    PropertyMap::iterator iter = m_Properties.find(propertyKey);
    if (iter != m_Properties.end()) {
        return &iter->second;
    }
    return 0;
}

PropertyStoreProperty* AboutPropertyStoreImpl::getProperty(PropertyStoreKey propertyKey, qcc::String const& language)
{
    PropertyMap::iterator defaultLang = m_Properties.find(DEFAULT_LANG);
    qcc::String defaultLanguage = "";
    if (defaultLang != m_Properties.end()) {
        char* tempdefLang;
        defaultLang->second.getPropertyValue().Get("s", &tempdefLang);
        defaultLanguage.assign(tempdefLang);
    }

    std::pair<PropertyMap::iterator, PropertyMap::iterator> iter = m_Properties.equal_range(propertyKey);
    for (PropertyMap::iterator it = iter.first; it != iter.second; it++) {
        /*
         * The first check is solely for backwards compatibility with the
         * deprecated setDeviceName.  An empty language string implies the
         * default language for that case.
         */
        if ((it->second.getLanguage().empty() && (language == defaultLanguage)) ||
            (it->second.getLanguage().compare(language) == 0)) {
            return &it->second;
        }
    }

    return 0;
}

QStatus AboutPropertyStoreImpl::setProperty(PropertyStoreKey propertyKey, const qcc::String& value,
                                            bool isPublic, bool isWritable, bool isAnnouncable)
{
    QStatus status = ER_OK;
    MsgArg msgArg("s", value.c_str());
    status = validateValue(propertyKey, msgArg);
    if (status != ER_OK) {
        return status;
    }

    removeExisting(propertyKey);

    PropertyStoreProperty property(PropertyStoreName[propertyKey], msgArg, isPublic, isWritable, isAnnouncable);
    m_Properties.insert(PropertyPair(propertyKey, property));
    return status;
}

QStatus AboutPropertyStoreImpl::setProperty(PropertyStoreKey propertyKey, const qcc::String& value, const qcc::String& language,
                                            bool isPublic, bool isWritable, bool isAnnouncable)
{
    QStatus status = ER_OK;
    MsgArg msgArg("s", value.c_str());
    status = validateValue(propertyKey, msgArg, language);
    if (status != ER_OK) {
        return status;
    }

    removeExisting(propertyKey, language);

    PropertyStoreProperty property(PropertyStoreName[propertyKey], msgArg, language, isPublic, isWritable, isAnnouncable);
    m_Properties.insert(PropertyPair(propertyKey, property));
    return status;
}

qcc::String const& AboutPropertyStoreImpl::getPropertyStoreName(PropertyStoreKey propertyStoreKey)
{
    if (propertyStoreKey < 0 || propertyStoreKey >= NUMBER_OF_KEYS) {
        return PropertyStoreName[NUMBER_OF_KEYS];
    }

    return PropertyStoreName[propertyStoreKey];
}

bool AboutPropertyStoreImpl::removeExisting(PropertyStoreKey propertyKey)
{
    PropertyMap::iterator iter = m_Properties.find(propertyKey);
    if (iter != m_Properties.end()) {
        m_Properties.erase(iter);
        return true;
    }
    return false;
}

bool AboutPropertyStoreImpl::removeExisting(PropertyStoreKey propertyKey, qcc::String const& language)
{
    std::pair<PropertyMap::iterator, PropertyMap::iterator> iter = m_Properties.equal_range(propertyKey);
    for (PropertyMap::iterator it = iter.first; it != iter.second; it++) {
        if (it->second.getLanguage().compare(language) == 0) {
            m_Properties.erase(it);
            return true;
        }
    }
    return false;
}

QStatus AboutPropertyStoreImpl::setDeviceId(const qcc::String& deviceId, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(DEVICE_ID, deviceId, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setDeviceName(const qcc::String& deviceName, qcc::String const& language, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(DEVICE_NAME, deviceName, language, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setAppName(const qcc::String& appName, qcc::String const& language, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(APP_NAME, appName, language, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setDefaultLang(const qcc::String& defaultLang, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(DEFAULT_LANG, defaultLang, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setDateOfManufacture(const qcc::String& dateOfManufacture, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(DATE_OF_MANUFACTURE, dateOfManufacture, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setSoftwareVersion(const qcc::String& softwareVersion, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(SOFTWARE_VERSION, softwareVersion, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setAjSoftwareVersion(const qcc::String& ajSoftwareVersion, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(AJ_SOFTWARE_VERSION, ajSoftwareVersion, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setHardwareVersion(const qcc::String& hardwareVersion, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(HARDWARE_VERSION, hardwareVersion, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setModelNumber(const qcc::String& modelNumber, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(MODEL_NUMBER, modelNumber, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setAppId(const qcc::String& appId, bool isPublic, bool isWritable, bool isAnnouncable)
{
    QStatus status = ER_OK;
    PropertyStoreKey propertyKey = APP_ID;
    uint8_t AppId[16];
    HexStringToBytes(appId, AppId, 16);
    MsgArg msgArg("ay", sizeof(AppId) / sizeof(*AppId), AppId);

    status = validateValue(propertyKey, msgArg);
    if (status != ER_OK) {
        return status;
    }
    removeExisting(propertyKey);

    PropertyStoreProperty property(PropertyStoreName[propertyKey], msgArg, isPublic, isWritable, isAnnouncable);
    m_Properties.insert(PropertyPair(propertyKey, property));
    return status;
}

QStatus AboutPropertyStoreImpl::setSupportedLangs(const std::vector<qcc::String>& supportedLangs, bool isPublic, bool isWritable, bool isAnnouncable)
{
    QStatus status = ER_OK;
    PropertyStoreKey propertyKey = SUPPORTED_LANGS;
    std::vector<const char*> supportedLangsVec(supportedLangs.size());
    for (uint32_t indx = 0; indx < supportedLangs.size(); indx++) {
        supportedLangsVec[indx] = supportedLangs[indx].c_str();
    }


    MsgArg msgArg("as", supportedLangsVec.size(), (supportedLangsVec.empty()) ? NULL : &supportedLangsVec.front());

    status = validateValue(propertyKey, msgArg);
    if (status != ER_OK) {
        return status;
    }
    removeExisting(propertyKey);
    m_SupportedLangs = supportedLangs;

    PropertyStoreProperty property(PropertyStoreName[propertyKey], msgArg, isPublic, isWritable, isAnnouncable);
    m_Properties.insert(PropertyPair(propertyKey, property));
    return status;
}

QStatus AboutPropertyStoreImpl::setDescription(const qcc::String& description, qcc::String const& language, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(DESCRIPTION, description, language, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setManufacturer(const qcc::String& manufacturer, qcc::String const& language, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(MANUFACTURER, manufacturer, language, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::setSupportUrl(const qcc::String& supportUrl, qcc::String const& language, bool isPublic, bool isWritable, bool isAnnouncable)
{
    return setProperty(SUPPORT_URL, supportUrl, language, isPublic, isWritable, isAnnouncable);
}

QStatus AboutPropertyStoreImpl::validateValue(PropertyStoreKey propertyKey, ajn::MsgArg const& value, qcc::String const& languageTag)
{
    QStatus status = ER_OK;

    switch (propertyKey) {

    case APP_ID:
        if (value.typeId != ALLJOYN_BYTE_ARRAY) {
            status = ER_INVALID_VALUE;
            break;
        }
        break;

    case DEVICE_ID:
    case DEVICE_NAME:
    case APP_NAME:
        if (value.typeId != ALLJOYN_STRING) {
            status = ER_INVALID_VALUE;
            break;
        }
        if (value.v_string.len == 0) {
            status = ER_INVALID_VALUE;
            break;
        }
        break;

    case DESCRIPTION:
    case MANUFACTURER:
    case DATE_OF_MANUFACTURE:
    case MODEL_NUMBER:
    case SOFTWARE_VERSION:
    case AJ_SOFTWARE_VERSION:
    case HARDWARE_VERSION:
    case SUPPORT_URL:
        if (value.typeId != ALLJOYN_STRING) {
            status = ER_INVALID_VALUE;
            break;
        }
        break;

    case DEFAULT_LANG:
        if (value.typeId != ALLJOYN_STRING) {
            status = ER_INVALID_VALUE;
            break;
        }
        if (value.v_string.len == 0) {
            status = ER_INVALID_VALUE;
            break;
        }

        status = isLanguageSupported(value.v_string.str);
        break;

    case SUPPORTED_LANGS:
        if (value.typeId != ALLJOYN_ARRAY) {
            status = ER_INVALID_VALUE;
            break;
        }

        if (value.v_array.GetNumElements() == 0) {
            status = ER_INVALID_VALUE;
            break;
        }

        if (strcmp(value.v_array.GetElemSig(), "s") != 0) {
            status = ER_INVALID_VALUE;
            break;
        }

        break;

    case NUMBER_OF_KEYS:
    default:
        status = ER_INVALID_VALUE;
        break;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("Validation of PropertyStore value failed"));
    }
    return status;
}

} /* namespace services */
} /* namespace ajn */
