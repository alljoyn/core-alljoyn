/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/AboutData.h>
#include <alljoyn/version.h>

#include <qcc/XmlElement.h>
#include <qcc/String.h>
#include <qcc/StringSource.h>

#include <stdio.h>
#include <assert.h>

#define QCC_MODULE "ALLJOYN_ABOUT"

namespace ajn {

const char* AboutData::APP_ID = "AppId";
const char* AboutData::DEFAULT_LANGUAGE = "DefaultLanguage";
const char* AboutData::DEVICE_NAME = "DeviceName";
const char* AboutData::DEVICE_ID = "DeviceId";
const char* AboutData::APP_NAME = "AppName";
const char* AboutData::MANUFACTURER = "Manufacturer";
const char* AboutData::MODEL_NUMBER = "ModelNumber";
const char* AboutData::SUPPORTED_LANGUAGES = "SupportedLanguages";
const char* AboutData::DESCRIPTION = "Description";
const char* AboutData::DATE_OF_MANUFACTURE = "DateOfManufacture";
const char* AboutData::SOFTWARE_VERSION = "SoftwareVersion";
const char* AboutData::AJ_SOFTWARE_VERSION = "AJSoftwareVersion";
const char* AboutData::HARDWARE_VERSION = "HardwareVersion";
const char* AboutData::SUPPORT_URL = "SupportUrl";

AboutData::AboutData() {
    InitializeFieldDetails();

    MsgArg arg;
    // The AllJoyn software version should always be set by default.
    arg.Set(m_aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), GetVersion());
    SetField(AJ_SOFTWARE_VERSION, arg);

    //TODO should the constructor also set the DeviceID as well?
}

AboutData::AboutData(const char* defaultLanguage) {
    InitializeFieldDetails();

    // The user must specify a default language when creating the AboutData class
    MsgArg arg;
    arg.Set(m_aboutFields[DEFAULT_LANGUAGE].signature.c_str(), defaultLanguage);
    SetField(DEFAULT_LANGUAGE, arg);
    // The default language should automatically be added to the list of supported languages
    SetSupportedLanguage(defaultLanguage);
    // The AllJoyn software version should always be set by default.
    arg.Set(m_aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), GetVersion());
    SetField(AJ_SOFTWARE_VERSION, arg);

    //TODO should the constructor also set the DeviceID as well?
}

AboutData::AboutData(const MsgArg arg, const char* language) {
    InitializeFieldDetails();

    QStatus status = CreatefromMsgArg(arg, language);
    if (ER_OK != status) {
        QCC_LogError(status, ("AboutData::AboutData(MsgArg): failed to parse MsgArg."));
    }
}

void AboutData::InitializeFieldDetails() {
    // FieldDetails: Required, Announced, Localized, signature
    m_aboutFields[APP_ID] = FieldDetails(REQUIRED | ANNOUNCED, "ay");
    m_aboutFields[DEFAULT_LANGUAGE] = FieldDetails(REQUIRED | ANNOUNCED, "s");
    m_aboutFields[DEVICE_NAME] = FieldDetails(ANNOUNCED | LOCALIZED, "s");
    m_aboutFields[DEVICE_ID] = FieldDetails(REQUIRED | ANNOUNCED, "s");
    m_aboutFields[APP_NAME] = FieldDetails(REQUIRED | ANNOUNCED | LOCALIZED, "s");
    m_aboutFields[MANUFACTURER] = FieldDetails(REQUIRED | ANNOUNCED | LOCALIZED, "s");
    m_aboutFields[MODEL_NUMBER] = FieldDetails(REQUIRED | ANNOUNCED, "s");
    m_aboutFields[SUPPORTED_LANGUAGES] = FieldDetails(REQUIRED, "as");
    m_aboutFields[DESCRIPTION] = FieldDetails(REQUIRED | LOCALIZED, "s");
    m_aboutFields[DATE_OF_MANUFACTURE] = FieldDetails(EMPTY_MASK, "s");
    m_aboutFields[SOFTWARE_VERSION] = FieldDetails(REQUIRED, "s");
    m_aboutFields[AJ_SOFTWARE_VERSION] = FieldDetails(REQUIRED, "s");
    m_aboutFields[HARDWARE_VERSION] = FieldDetails(EMPTY_MASK, "s");
    m_aboutFields[SUPPORT_URL] = FieldDetails(EMPTY_MASK, "s");
}

AboutData::~AboutData()
{

}

uint8_t CharToNibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    return -1; // Error!
}
QStatus AboutData::CreateFromXml(const qcc::String& aboutDataXml)
{
    QStatus status;
    qcc::StringSource source(aboutDataXml);
    qcc::XmlParseContext pc(source);
    status = qcc::XmlElement::Parse(pc);
    if (status != ER_OK) {
        return status;
    }
    const qcc::XmlElement* root = pc.GetRoot();

    /*
     * This will iterate through the list of known fields in the about data.  If
     * the field is not localized set that field. We grab the non-localized values
     * first because we need the DefaultLanguage to to set a localized value where
     * the language tag is not given.
     */
    typedef std::map<qcc::String, FieldDetails>::iterator it_aboutFields;
    MsgArg arg;
    for (it_aboutFields it = m_aboutFields.begin(); it != m_aboutFields.end(); ++it) {
        // Supported languages are implicitly added no need to look for a
        // SupportedLanguages languages tag.
        if (it->first == SUPPORTED_LANGUAGES) {
            continue;
        }
        // The Alljoyn software version is implicitly added so we don't need to
        // look for this tag
        if (it->first == AJ_SOFTWARE_VERSION) {
            continue;
        }
        if (!IsFieldLocalized(it->first.c_str())) {
            if (root->GetChild(it->first)->GetContent() != "") {
                // All non-localized fields in the about data are strings and are
                // treated like a string except for the AppId and SupportedLanguages
                // Since Languages are implicitly added we don't look for the
                // SupportedLanguages tag.
                if (it->first == APP_ID) {
                    size_t strSize = root->GetChild(it->first)->GetContent().size();
                    if (strSize % 2 == 0) {
                        uint8_t* appId_bites = new uint8_t[strSize / 2];
                        for (size_t i = 0; i < strSize / 2; ++i) {
                            appId_bites[i] = (CharToNibble(root->GetChild(it->first)->GetContent()[2 * i]) << 4) |
                                             CharToNibble(root->GetChild(it->first)->GetContent()[2 * i + 1]);
                            status = SetAppId(appId_bites, strSize / 2);
                            if (status != ER_OK) {
                                return status;
                            }
                        }
                    } else {
                        // we must have an even number of characters
                        // TODO put more meaningful status
                        return ER_FAIL;
                    }
                } else {
                    assert(m_aboutFields[it->first].signature == "s");
                    arg.Set("s", root->GetChild(it->first)->GetContent().c_str());
                    status = SetField(it->first.c_str(), arg);
                    if (status != ER_OK) {
                        return status;
                    }
                    // Make sure the DefaultLanguage is added to the list of
                    // SupportedLanguages.
                    if (it->first == DEFAULT_LANGUAGE) {
                        status = SetSupportedLanguage(root->GetChild(it->first)->GetContent().c_str());
                    }
                }
            }
        }
    }

    //TODO check for default language here return error if not set
    /*
     * Now that we have iterated through all of the tags that are not localized
     * we are going to iterate through them all again. Except this time we will
     * only be looking at tags that are localized or are unknown.
     *
     * Unknown tags are assumed to be OEM defined tags and are added the limitation
     * is that we can only accept OEM defined tags that contain a string.
     */
    std::vector<qcc::XmlElement*> elements = root->GetChildren();
    std::vector<qcc::XmlElement*>::iterator it;
    for (std::vector<qcc::XmlElement*>::iterator it = elements.begin(); it != elements.end(); ++it) {
        if (IsFieldLocalized((*it)->GetName().c_str()) || (m_aboutFields.find((*it)->GetName()) == m_aboutFields.end())) {
            //printf("Setting field: name=%s, value=%s, lang=%s\n", (*it)->GetName().c_str(), (*it)->GetContent().c_str(), (*it)->GetAttribute("lang").c_str());
            //assert(m_aboutFields[it->first].dataType == "s");
            status = arg.Set("s", (*it)->GetContent().c_str());
            if (status != ER_OK) {
                return status;
            }
            status = SetField((*it)->GetName().c_str(), arg, (*it)->GetAttribute("lang").c_str());
            if (status != ER_OK) {
                return status;
            }
        }
    }

    return status;
}

bool AboutData::IsValid(const char* language)
{
    /*
     * required fields
     * AppId
     * DefaultLanguage
     * DeviceId
     * AppName
     * Manufacture
     * ModelNumber
     * SupportedLanguages
     * Description
     * SoftwareVersion
     * AJSoftwareVersion
     */
    /*
     * This will iterate through the list of known fields in the about data.  If
     * the field is required check to see if the field has been set. When
     * checking localizing will be taken into account. If the language is not
     * specified the default language is assumed.
     */
    typedef std::map<qcc::String, FieldDetails>::const_iterator it_aboutFields;
    for (it_aboutFields it = m_aboutFields.begin(); it != m_aboutFields.end(); ++it) {
        if (IsFieldRequired(it->first.c_str())) {
            if (IsFieldLocalized(it->first.c_str())) {
                if (m_localizedPropertyStore.find(it->first) == m_localizedPropertyStore.end()) {
                    return false;
                } else {
                    if (language == NULL) {
                        char* defaultLanguage;
                        QStatus status = m_propertyStore[DEFAULT_LANGUAGE].Get(m_aboutFields[DEFAULT_LANGUAGE].signature.c_str(), &defaultLanguage);
                        if (status != ER_OK) {
                            return false;
                        }
                        if (m_localizedPropertyStore[it->first].find(defaultLanguage) == m_localizedPropertyStore[it->first].end()) {
                            return false;
                        }
                    } else {
                        if (m_localizedPropertyStore[it->first].find(language) == m_localizedPropertyStore[it->first].end()) {
                            return false;
                        }
                    }
                }
            } else {
                if (m_propertyStore.find(it->first) == m_propertyStore.end()) {
                    return false;
                }
            }
        }
    }
    return true;
}

QStatus AboutData::CreatefromMsgArg(const MsgArg& arg, const char* language)
{
    QStatus status = ER_OK;
    char* defaultLanguage;
    MsgArg* fields;
    size_t numFields;
    status = arg.Get("a{sv}", &numFields, &fields);
    if (status != ER_OK) {
        return status;
    }
    if (language == NULL) {
        MsgArg* argDefualtLang;
        status = arg.GetElement("{sv}", DEFAULT_LANGUAGE, &argDefualtLang);
        if (status != ER_OK) {
            //printf("GetElement Failed");
            return status;
        }
        status = argDefualtLang->Get("s", &defaultLanguage);
        if (status != ER_OK) {
            //printf("Get default language failed: \n%s\n %s\n", argDefualtLang->ToString().c_str());
            return status;
        }
    }
    for (size_t i = 0; i < numFields; ++i) {
        char* fieldName;
        MsgArg* fieldValue;
        status = fields[i].Get("{sv}", &fieldName, &fieldValue);
        if (status != ER_OK) {
            return status;
        }
        // OEM specific field found add that field to the m_aboutFields
        if (m_aboutFields.find(fieldName) == m_aboutFields.end()) {
            m_aboutFields[fieldName] = FieldDetails(LOCALIZED, fieldValue->Signature().c_str());
        }
        if (fieldValue->Signature() != m_aboutFields[fieldName].signature) {
            return ER_BUS_SIGNATURE_MISMATCH;
        }
        if (IsFieldLocalized(fieldName)) {
            if (language != NULL) {
                m_localizedPropertyStore[fieldName][language] = *fieldValue;
            } else {
                m_localizedPropertyStore[fieldName][defaultLanguage] = *fieldValue;
            }

        } else {
            m_propertyStore[fieldName] = *fieldValue;
            //Since the GetSupportedLanguages function looks at the member
            // variable m_supportedLanguages we must set up the make sure
            // the member function is filled in.
            if (strcmp(SUPPORTED_LANGUAGES, fieldName) == 0) {
                size_t language_count;
                MsgArg* languagesArg;
                fieldValue->Get(this->GetFieldSignature(SUPPORTED_LANGUAGES), &language_count, &languagesArg);
                for (size_t i = 0; i < language_count; ++i) {
                    char* lang;
                    languagesArg[i].Get("s", &lang);
                    m_supportedLanguages.insert(lang);
                }
            }
        }
    }
    return status;
}

QStatus AboutData::SetAppId(const uint8_t* appId, const size_t num)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[APP_ID].signature.c_str(), num, appId);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(APP_ID, arg);
    return status;
}
QStatus AboutData::GetAppId(uint8_t** appId, size_t* num)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(APP_ID, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[APP_ID].signature.c_str(), num, appId);
    return status;
}

QStatus AboutData::SetDefaultLanguage(const char* defaultLanguage)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[DEFAULT_LANGUAGE].signature.c_str(), defaultLanguage);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(DEFAULT_LANGUAGE, arg);
    //The default language should automatically be added to the supported languages
    SetSupportedLanguage(defaultLanguage);
    return status;
}

QStatus AboutData::GetDefaultLanguage(char** defaultLanguage)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(DEFAULT_LANGUAGE, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[DEFAULT_LANGUAGE].signature.c_str(), defaultLanguage);
    return status;
}

QStatus AboutData::SetDeviceName(const char* deviceName, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[DEVICE_NAME].signature.c_str(), deviceName);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(DEVICE_NAME, arg, language);
    return status;
}

QStatus AboutData::GetDeviceName(char** deviceName, const char* language)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(DEVICE_NAME, arg, language);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[DEVICE_NAME].signature.c_str(), deviceName);
    return status;
}

QStatus AboutData::SetDeviceId(const char* deviceId)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[DEVICE_ID].signature.c_str(), deviceId);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(DEVICE_ID, arg);
    return status;
}
QStatus AboutData::GetDeviceId(char** deviceId)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(DEVICE_ID, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[DEVICE_ID].signature.c_str(), deviceId);
    return status;
}

QStatus AboutData::SetAppName(const char* appName, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[APP_NAME].signature.c_str(), appName);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(APP_NAME, arg, language);
    return status;
}

QStatus AboutData::GetAppName(char** appName, const char* language)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(APP_NAME, arg, language);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[APP_NAME].signature.c_str(), appName);
    return status;
}

QStatus AboutData::SetManufacturer(const char* manufacturer, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[MANUFACTURER].signature.c_str(), manufacturer);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(MANUFACTURER, arg, language);
    return status;
}
QStatus AboutData::GetManufacturer(char** manufacturer, const char* language)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(MANUFACTURER, arg, language);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[MANUFACTURER].signature.c_str(), manufacturer);
    return status;
}

QStatus AboutData::SetModelNumber(const char* modelNumber)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[MODEL_NUMBER].signature.c_str(), modelNumber);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(MODEL_NUMBER, arg);
    return status;
}
QStatus AboutData::GetModelNumber(char** modelNumber)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(MODEL_NUMBER, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[MODEL_NUMBER].signature.c_str(), modelNumber);
    return status;
}

QStatus AboutData::SetSupportedLanguage(const char* language)
{
    //TODO add in logic to check information about the tag all of the tags must
    //     conform to the RFC5646 there is currently nothing to make sure the
    //     tags conform to this RFC
    QStatus status = ER_OK;

    std::pair<std::set<qcc::String>::iterator, bool> ret =  m_supportedLanguages.insert(language);
    // A new language has been added rebuild the MsgArg and update the field.
    if (true == ret.second) {
        size_t supportedLangsNum = m_supportedLanguages.size();
        //char* supportedLangs[supportedLangsNum];
        const char** supportedLangs = new const char*[supportedLangsNum];
        size_t count = 0;
        for (std::set<qcc::String>::iterator it = m_supportedLanguages.begin(); it != m_supportedLanguages.end(); ++it) {
            supportedLangs[count] = it->c_str();
            ++count;
        }
        MsgArg arg;
        status = arg.Set(m_aboutFields[SUPPORTED_LANGUAGES].signature.c_str(), supportedLangsNum, supportedLangs);
        if (status != ER_OK) {
            return status;
        }
        status = SetField(SUPPORTED_LANGUAGES, arg);
        delete [] supportedLangs;
    }
    return status;
}

size_t AboutData::GetSupportedLanguages(qcc::String* supportedLanguages, size_t num)
{
    if (supportedLanguages == NULL) {
        return m_supportedLanguages.size();
    }
    size_t count = 0;
    for (std::set<qcc::String>::iterator it = m_supportedLanguages.begin(); it != m_supportedLanguages.end() && count < num; ++it) {
        supportedLanguages[count] = *it;
        ++count;
    }
    return count;
}

QStatus AboutData::SetDescription(const char* descritption, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[DESCRIPTION].signature.c_str(), descritption);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(DESCRIPTION, arg, language);
    return status;
}
QStatus AboutData::GetDescription(char** description, const char* language)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(DESCRIPTION, arg, language);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[DESCRIPTION].signature.c_str(), description);
    return status;
}

QStatus AboutData::SetDateOfManufacture(const char* dateOfManufacture)
{
    //TODO check that the dateOfManufacture string is of the correct format YYYY-MM-DD
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[DATE_OF_MANUFACTURE].signature.c_str(), dateOfManufacture);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(DATE_OF_MANUFACTURE, arg);
    return status;
}
QStatus AboutData::GetDateOfManufacture(char** dateOfManufacture)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(DATE_OF_MANUFACTURE, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[DATE_OF_MANUFACTURE].signature.c_str(), dateOfManufacture);
    return status;
}

QStatus AboutData::SetSoftwareVersion(const char* softwareVersion)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[SOFTWARE_VERSION].signature.c_str(), softwareVersion);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(SOFTWARE_VERSION, arg);
    return status;
}
QStatus AboutData::GetSoftwareVersion(char** softwareVersion)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(SOFTWARE_VERSION, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[SOFTWARE_VERSION].signature.c_str(), softwareVersion);
    return status;
}

QStatus AboutData::GetAJSoftwareVersion(char** ajSoftwareVersion)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(AJ_SOFTWARE_VERSION, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), ajSoftwareVersion);
    return status;
}

QStatus AboutData::SetHardwareVersion(const char* hardwareVersion)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[HARDWARE_VERSION].signature.c_str(), hardwareVersion);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(HARDWARE_VERSION, arg);
    return status;
}

QStatus AboutData::GetHardwareVersion(char** hardwareVersion)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(HARDWARE_VERSION, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[HARDWARE_VERSION].signature.c_str(), hardwareVersion);
    return status;
}

QStatus AboutData::SetSupportUrl(const char* supportUrl)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(m_aboutFields[SUPPORT_URL].signature.c_str(), supportUrl);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(SUPPORT_URL, arg);
    return status;
}
QStatus AboutData::GetSupportUrl(char** supportUrl)
{
    QStatus status;
    MsgArg* arg;
    status = GetField(SUPPORT_URL, arg);
    if (status != ER_OK) {
        return status;
    }
    status = arg->Get(m_aboutFields[SUPPORT_URL].signature.c_str(), supportUrl);
    return status;
}

QStatus AboutData::SetField(const char* name, ajn::MsgArg value, const char* language) {
    QStatus status = ER_OK;
    // The user is adding an OEM specific field.
    // At this time OEM specific fields are added as
    //    not required
    //    not announced
    //    can be localized
    if (m_aboutFields.find(name) == m_aboutFields.end()) {
        m_aboutFields[name] = FieldDetails(LOCALIZED, value.Signature().c_str());
    }
    if (IsFieldLocalized(name)) {
        if (language == NULL || strcmp(language, "") == 0) {
            std::map<qcc::String, MsgArg>::iterator it = m_propertyStore.find(DEFAULT_LANGUAGE);
            if (it == m_propertyStore.end()) {
                return ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED;
            }
            char* defaultLanguage;
            status = it->second.Get(m_aboutFields[DEFAULT_LANGUAGE].signature.c_str(), &defaultLanguage);
            if (status != ER_OK) {
                return status;
            }
            m_localizedPropertyStore[name][static_cast<qcc::String>(defaultLanguage)] = value;
        } else {
            m_localizedPropertyStore[name][language] = value;
            //implicitly add all language tags to the supported languages
            status = SetSupportedLanguage(language);
            if (status != ER_OK) {
                return status;
            }
        }
    } else {
        m_propertyStore[name] = value;
    }
    return status;
}

QStatus AboutData::GetField(const char* name, ajn::MsgArg*& value, const char* language) {
    QStatus status = ER_OK;
    if (!IsFieldLocalized(name)) {
        value = &(m_propertyStore[name]);
        size_t num;
        uint8_t* appId;
        value->Get(m_aboutFields[APP_ID].signature.c_str(), &num, &appId);
    } else {
        if (language == NULL) {
            char* defaultLanguage;
            status = m_propertyStore[DEFAULT_LANGUAGE].Get(m_aboutFields[DEFAULT_LANGUAGE].signature.c_str(), &defaultLanguage);
            if (status != ER_OK) {
                return status;
            }
            value = &(m_localizedPropertyStore[name][static_cast<qcc::String>(defaultLanguage)]);
        } else {
            value = &(m_localizedPropertyStore[name][language]);
        }
    }
    return status;
}

size_t AboutData::GetFields(const char** fields, size_t num_fields) const
{
    if (fields == NULL) {
        return (m_propertyStore.size() + m_localizedPropertyStore.size());
    }
    size_t field_count = 0;
    std::map<qcc::String, MsgArg>::const_iterator pstore_it;
    for (pstore_it = m_propertyStore.begin(); (pstore_it != m_propertyStore.end()) && (field_count < num_fields); ++pstore_it) {
        fields[field_count] = pstore_it->first.c_str();
        ++field_count;
    }
    std::map<qcc::String, std::map<qcc::String, MsgArg> >::const_iterator lpstore_it;
    for (lpstore_it = m_localizedPropertyStore.begin(); (lpstore_it != m_localizedPropertyStore.end()) && (field_count < num_fields); ++lpstore_it) {
        fields[field_count] = lpstore_it->first.c_str();
        ++field_count;
    }
    return field_count;
}

QStatus AboutData::GetAboutData(MsgArg* msgArg, const char* language)
{
    QStatus status;
    if (!IsValid()) {
        return ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD;
    }

    char* defaultLanguage;
    status = GetDefaultLanguage(&defaultLanguage);
    if (status != ER_OK) {
        return status;
    }

    // Check to see if the language is a supported language
    if (!(language == NULL || 0 == strcmp(language, ""))) {
        bool languageTagFound = false;
        for (std::set<qcc::String>::iterator it = m_supportedLanguages.begin(); it != m_supportedLanguages.end(); ++it) {
            if (it->compare(language) == 0) {
                languageTagFound = true;
                break;
            }
        }
        if (!languageTagFound) {
            return ER_LANGUAGE_NOT_SUPPORTED;
        }
    }


    size_t dictonarySize = 0;
    typedef std::map<qcc::String, FieldDetails>::iterator it_aboutFields;
    for (it_aboutFields it = m_aboutFields.begin(); it != m_aboutFields.end(); ++it) {
        if (IsFieldRequired(it->first.c_str())) {
            dictonarySize++;
        } else {
            if (IsFieldLocalized(it->first.c_str())) {
                if (language == NULL || 0 == strcmp(language, "")) {
                    if (m_localizedPropertyStore[it->first].find(defaultLanguage) != m_localizedPropertyStore[it->first].end()) {
                        dictonarySize++;
                    }
                } else {
                    if (m_localizedPropertyStore[it->first].find(language) != m_localizedPropertyStore[it->first].end()) {
                        dictonarySize++;
                    }
                }
            } else {
                if (m_propertyStore.find(it->first) != m_propertyStore.end()) {
                    dictonarySize++;
                }
            }
        }
    }

    MsgArg* aboutDictonary = new MsgArg[dictonarySize];
    size_t count = 0;

    for (it_aboutFields it = m_aboutFields.begin(); it != m_aboutFields.end(); ++it) {
        if (IsFieldRequired(it->first.c_str())) {
            if (IsFieldLocalized(it->first.c_str())) {
                if (language == NULL || 0 == strcmp(language, "")) {
                    status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &m_localizedPropertyStore[it->first][defaultLanguage]);
                } else {
                    status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &m_localizedPropertyStore[it->first][language]);
                }

            } else {
                status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &m_propertyStore[it->first]);
            }
        } else {
            if (IsFieldLocalized(it->first.c_str())) {
                if (language == NULL || 0 == strcmp(language, "")) {
                    if (m_localizedPropertyStore[it->first].find(defaultLanguage) != m_localizedPropertyStore[it->first].end()) {
                        status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &m_localizedPropertyStore[it->first][defaultLanguage]);
                    }
                } else {
                    if (m_localizedPropertyStore[it->first].find(defaultLanguage) != m_localizedPropertyStore[it->first].end()) {
                        status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &m_localizedPropertyStore[it->first][language]);
                    }
                }
            } else {
                if (m_propertyStore.find(it->first) != m_propertyStore.end()) {
                    status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &m_propertyStore[it->first]);
                }
            }
        }
        if (status != ER_OK) {
            return status;
        }
    }
    assert(dictonarySize == count);

    msgArg->Set("a{sv}", dictonarySize, aboutDictonary);
    msgArg->Stabilize();
    return status;
}

QStatus AboutData::GetAnnouncedAboutData(MsgArg* msgArg)
{
    QStatus status;
    if (!IsValid()) {
        // TODO put in an ER code that is more meaningful
        return ER_FAIL;
    }
    char* defaultLanguage;
    status = GetDefaultLanguage(&defaultLanguage);
    //QStatus status = m_defaultLanguage.Get(m_aboutFields[DEFAULT_LANGUAGE].dataType.c_str(), &defaultLanguage);
    if (status != ER_OK) {
        return status;
    }

    size_t dictonarySize = 0;
    typedef std::map<qcc::String, FieldDetails>::iterator it_aboutFields;
    for (it_aboutFields it = m_aboutFields.begin(); it != m_aboutFields.end(); ++it) {
        if (IsFieldAnnounced(it->first.c_str())) {
            if (IsFieldRequired(it->first.c_str())) {
                dictonarySize++;
            } else {
                if (IsFieldLocalized(it->first.c_str())) {
                    if (m_localizedPropertyStore[it->first].find(defaultLanguage) != m_localizedPropertyStore[it->first].end()) {
                        dictonarySize++;
                    }
                } else {
                    if (m_propertyStore.find(it->first) != m_propertyStore.end()) {
                        dictonarySize++;
                    }
                }
            }
        }
    }

    MsgArg* announceDictonary = new MsgArg[dictonarySize];
    size_t count = 0;
    for (it_aboutFields it = m_aboutFields.begin(); it != m_aboutFields.end(); ++it) {
        if (IsFieldAnnounced(it->first.c_str())) {
            if (IsFieldRequired(it->first.c_str())) {
                if (IsFieldLocalized(it->first.c_str())) {
                    status = announceDictonary[count++].Set("{sv}", it->first.c_str(), &m_localizedPropertyStore[it->first][defaultLanguage]);
                } else {
                    status = announceDictonary[count++].Set("{sv}", it->first.c_str(), &m_propertyStore[it->first]);
                }
            } else {
                if (IsFieldLocalized(it->first.c_str())) {
                    if (m_localizedPropertyStore[it->first].find(defaultLanguage) != m_localizedPropertyStore[it->first].end()) {
                        status = announceDictonary[count++].Set("{sv}", it->first.c_str(), &m_localizedPropertyStore[it->first][defaultLanguage]);
                    }
                } else {
                    if (m_propertyStore.find(it->first) != m_propertyStore.end()) {
                        status = announceDictonary[count++].Set("{sv}", it->first.c_str(), &m_propertyStore[it->first]);
                    }
                }
            }
            if (status != ER_OK) {
                return status;
            }
        }
    }

    assert(dictonarySize == count);

    msgArg->Set("a{sv}", dictonarySize, announceDictonary);
    msgArg->Stabilize();
    return status;
}

bool AboutData::IsFieldRequired(const char* fieldName) {
    if (m_aboutFields.find(fieldName) != m_aboutFields.end()) {
        return ((m_aboutFields[fieldName].fieldMask & REQUIRED) == REQUIRED);
    }
    return false;
}
bool AboutData::IsFieldAnnounced(const char* fieldName) {
    if (m_aboutFields.find(fieldName) != m_aboutFields.end()) {
        return ((m_aboutFields[fieldName].fieldMask & ANNOUNCED) == ANNOUNCED);
    }
    return false;
}

bool AboutData::IsFieldLocalized(const char* fieldName) {
    if (m_aboutFields.find(fieldName) != m_aboutFields.end()) {
        return ((m_aboutFields[fieldName].fieldMask & LOCALIZED) == LOCALIZED);
    }
    return false;
}

const char* AboutData::GetFieldSignature(const char* fieldName) {
    if (m_aboutFields.find(fieldName) != m_aboutFields.end()) {
        return m_aboutFields[fieldName].signature.c_str();
    }
    return NULL;
}

QStatus AboutData::SetNewFieldDetails(const char* fieldName, AboutFieldMask fieldMask, const char* signature)
{
    if (m_aboutFields.find(fieldName) == m_aboutFields.end()) {
        m_aboutFields[fieldName] = FieldDetails(fieldMask, signature);
    } else {
        return ER_ABOUT_FIELD_ALREADY_SPECIFIED;
    }
    return ER_OK;
}
} //end namespace ajn
