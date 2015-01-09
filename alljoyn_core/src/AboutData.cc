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
#include <qcc/StringUtil.h>
#include <qcc/StringSource.h>

#include <stdio.h>
#include <assert.h>

#include "AboutDataInternal.h"

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
    arg.Set(aboutDataInternal->aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), GetVersion());
    SetField(AJ_SOFTWARE_VERSION, arg);

    //TODO should the constructor also set the DeviceID as well?
}

AboutData::AboutData(const char* defaultLanguage) {
    InitializeFieldDetails();

    // The user must specify a default language when creating the AboutData class
    MsgArg arg;
    arg.Set(aboutDataInternal->aboutFields[DEFAULT_LANGUAGE].signature.c_str(), defaultLanguage);
    SetField(DEFAULT_LANGUAGE, arg);
    // The default language should automatically be added to the list of supported languages
    SetSupportedLanguage(defaultLanguage);
    // The AllJoyn software version should always be set by default.
    arg.Set(aboutDataInternal->aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), GetVersion());
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

AboutData::AboutData(const AboutData& src) {
    InitializeFieldDetails();
    *aboutDataInternal = *(src.aboutDataInternal);
}

AboutData& AboutData::operator=(const AboutData& src) {
    if (&src == this) {
        return *this;
    }
    delete aboutDataInternal;
    aboutDataInternal = NULL;
    InitializeFieldDetails();
    *aboutDataInternal = *(src.aboutDataInternal);
    return *this;
}

void AboutData::InitializeFieldDetails() {
    aboutDataInternal = new AboutData::Internal();
    // FieldDetails: Required, Announced, Localized, signature
    aboutDataInternal->aboutFields[APP_ID] = FieldDetails(REQUIRED | ANNOUNCED, "ay");
    aboutDataInternal->aboutFields[DEFAULT_LANGUAGE] = FieldDetails(REQUIRED | ANNOUNCED, "s");
    aboutDataInternal->aboutFields[DEVICE_NAME] = FieldDetails(ANNOUNCED | LOCALIZED, "s");
    aboutDataInternal->aboutFields[DEVICE_ID] = FieldDetails(REQUIRED | ANNOUNCED, "s");
    aboutDataInternal->aboutFields[APP_NAME] = FieldDetails(REQUIRED | ANNOUNCED | LOCALIZED, "s");
    aboutDataInternal->aboutFields[MANUFACTURER] = FieldDetails(REQUIRED | ANNOUNCED | LOCALIZED, "s");
    aboutDataInternal->aboutFields[MODEL_NUMBER] = FieldDetails(REQUIRED | ANNOUNCED, "s");
    aboutDataInternal->aboutFields[SUPPORTED_LANGUAGES] = FieldDetails(REQUIRED, "as");
    aboutDataInternal->aboutFields[DESCRIPTION] = FieldDetails(REQUIRED | LOCALIZED, "s");
    aboutDataInternal->aboutFields[DATE_OF_MANUFACTURE] = FieldDetails(EMPTY_MASK, "s");
    aboutDataInternal->aboutFields[SOFTWARE_VERSION] = FieldDetails(REQUIRED, "s");
    aboutDataInternal->aboutFields[AJ_SOFTWARE_VERSION] = FieldDetails(REQUIRED, "s");
    aboutDataInternal->aboutFields[HARDWARE_VERSION] = FieldDetails(EMPTY_MASK, "s");
    aboutDataInternal->aboutFields[SUPPORT_URL] = FieldDetails(EMPTY_MASK, "s");
}

AboutData::~AboutData()
{
    delete aboutDataInternal;
    aboutDataInternal = NULL;
}

bool isHexChar(char c) {
    return ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f'));
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
    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
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
                    status = SetAppId(root->GetChild(it->first)->GetContent().c_str());
                    if (ER_OK != status) {
                        return status;
                    }
                } else {
                    assert(aboutDataInternal->aboutFields[it->first].signature == "s");
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
        if (IsFieldLocalized((*it)->GetName().c_str()) || (aboutDataInternal->aboutFields.find((*it)->GetName()) == aboutDataInternal->aboutFields.end())) {
            //printf("Setting field: name=%s, value=%s, lang=%s\n", (*it)->GetName().c_str(), (*it)->GetContent().c_str(), (*it)->GetAttribute("lang").c_str());
            //assert(aboutDataInternal->m_aboutFields[it->first].dataType == "s");
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
    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        if (IsFieldRequired(it->first.c_str())) {
            if (IsFieldLocalized(it->first.c_str())) {
                if (aboutDataInternal->localizedPropertyStore.find(it->first) == aboutDataInternal->localizedPropertyStore.end()) {
                    return false;
                } else {
                    if (language == NULL) {
                        char* defaultLanguage;
                        QStatus status = aboutDataInternal->propertyStore[DEFAULT_LANGUAGE].Get(aboutDataInternal->aboutFields[DEFAULT_LANGUAGE].signature.c_str(), &defaultLanguage);
                        if (status != ER_OK) {
                            return false;
                        }
                        if (aboutDataInternal->localizedPropertyStore[it->first].find(defaultLanguage) == aboutDataInternal->localizedPropertyStore[it->first].end()) {
                            return false;
                        }
                    } else {
                        if (aboutDataInternal->localizedPropertyStore[it->first].find(language) == aboutDataInternal->localizedPropertyStore[it->first].end()) {
                            return false;
                        }
                    }
                }
            } else {
                if (aboutDataInternal->propertyStore.find(it->first) == aboutDataInternal->propertyStore.end()) {
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
        // OEM specific field found add that field to the aboutDataInternal->m_aboutFields
        if (aboutDataInternal->aboutFields.find(fieldName) == aboutDataInternal->aboutFields.end()) {
            aboutDataInternal->aboutFields[fieldName] = FieldDetails(LOCALIZED, fieldValue->Signature().c_str());
        }
        if (fieldValue->Signature() != aboutDataInternal->aboutFields[fieldName].signature) {
            return ER_BUS_SIGNATURE_MISMATCH;
        }
        if (IsFieldLocalized(fieldName)) {
            if (language != NULL) {
                aboutDataInternal->localizedPropertyStore[fieldName][language] = *fieldValue;
            } else {
                aboutDataInternal->localizedPropertyStore[fieldName][defaultLanguage] = *fieldValue;
            }

        } else {
            aboutDataInternal->propertyStore[fieldName] = *fieldValue;
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
                    aboutDataInternal->supportedLanguages.insert(lang);
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
    status = arg.Set(aboutDataInternal->aboutFields[APP_ID].signature.c_str(), num, appId);
    if (status != ER_OK) {
        return status;
    }
    status = SetField(APP_ID, arg);
    if (status != ER_OK) {
        return status;
    }
    if (num != 16) {
        status = ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE;
    }
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
    status = arg->Get(aboutDataInternal->aboutFields[APP_ID].signature.c_str(), num, appId);
    return status;
}

QStatus AboutData::SetAppId(const char* appId) {
    QStatus status = ER_OK;
    //The number of bytes needed to make a 128-bit AppId
    const size_t APPID_BYTE_SIZE = 16;
    //APPID_BYTE_SIZE * 2 + 4 the number of hex characers to make a 128-bit AppId
    // plus four for each possible '-' character from a RFC 4122 UUID. (i.e. 4a354637-5649-4518-8a48-323c158bc02d)
    size_t strSize = strnlen(appId, APPID_BYTE_SIZE * 2 + 4);
    if (strSize % 2 == 0) {
        if (strSize / 2 == APPID_BYTE_SIZE) {
            //Check that every character is a hex char
            for (size_t i = 0; i < strSize; ++i) {
                if (!isHexChar(appId[i])) {
                    return ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE;
                }
            }
            uint8_t appId_bytes[APPID_BYTE_SIZE];
            qcc::HexStringToBytes(static_cast<const qcc::String>(appId), appId_bytes, APPID_BYTE_SIZE);
            status = SetAppId(appId_bytes, APPID_BYTE_SIZE);
            if (status != ER_OK) {
                return status;
            }
            aboutDataInternal->propertyStore[APP_ID].Stabilize();
        } else if (strSize / 2 == 18) {
            // since the sting is 36 characters long we assume its a UUID as per
            // section 3 of RFC 4122  (i.e. 4a354637-5649-4518-8a48-323c158bc02d)
            // The UUID according to RFC 4122 is basically a 16-byte array encoded
            // in hexOctets with '-' characters separating parts of the string.
            // After checking that the '-' characters are in the correct location
            // we remove the '-' characters and pass the string without the '-'
            // back to the SetAppId function.

            // The four locations of the '-' character according to RFC 4122
            size_t TIME_LOW_END = 8;               //location of fist '-' char
            size_t TIME_MID_END = 13;              //location of second '-' char
            size_t TIME_HIGH_AND_VERSION_END = 18; //location of third '-' char
            size_t CLOCK_SEQ_END = 23;              // location of fourth '-' char
            if (appId[TIME_LOW_END] != '-' ||
                appId[TIME_MID_END] != '-' ||
                appId[TIME_HIGH_AND_VERSION_END] != '-' ||
                appId[CLOCK_SEQ_END] != '-') {
                return ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE;
            }
            //APPID_BYTE_SIZE * 2 + 1 number of hex characters +1 for nul
            char hexAppId[APPID_BYTE_SIZE * 2 + 1];
            size_t location = 0;
            for (size_t i = 0; i < strSize; ++i) {
                if (appId[i] != '-') {
                    hexAppId[location] = appId[i];
                    ++location;
                }
            }
            hexAppId[location] = '\0';
            return SetAppId(hexAppId);
        } else {
            return ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE;
        }
    } else {
        return ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE;
    }
    return status;
}
QStatus AboutData::SetDefaultLanguage(const char* defaultLanguage)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[DEFAULT_LANGUAGE].signature.c_str(), defaultLanguage);
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
    status = arg->Get(aboutDataInternal->aboutFields[DEFAULT_LANGUAGE].signature.c_str(), defaultLanguage);
    return status;
}

QStatus AboutData::SetDeviceName(const char* deviceName, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[DEVICE_NAME].signature.c_str(), deviceName);
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
    status = arg->Get(aboutDataInternal->aboutFields[DEVICE_NAME].signature.c_str(), deviceName);
    return status;
}

QStatus AboutData::SetDeviceId(const char* deviceId)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[DEVICE_ID].signature.c_str(), deviceId);
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
    status = arg->Get(aboutDataInternal->aboutFields[DEVICE_ID].signature.c_str(), deviceId);
    return status;
}

QStatus AboutData::SetAppName(const char* appName, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[APP_NAME].signature.c_str(), appName);
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
    status = arg->Get(aboutDataInternal->aboutFields[APP_NAME].signature.c_str(), appName);
    return status;
}

QStatus AboutData::SetManufacturer(const char* manufacturer, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[MANUFACTURER].signature.c_str(), manufacturer);
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
    status = arg->Get(aboutDataInternal->aboutFields[MANUFACTURER].signature.c_str(), manufacturer);
    return status;
}

QStatus AboutData::SetModelNumber(const char* modelNumber)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[MODEL_NUMBER].signature.c_str(), modelNumber);
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
    status = arg->Get(aboutDataInternal->aboutFields[MODEL_NUMBER].signature.c_str(), modelNumber);
    return status;
}

QStatus AboutData::SetSupportedLanguage(const char* language)
{
    //TODO add in logic to check information about the tag all of the tags must
    //     conform to the RFC5646 there is currently nothing to make sure the
    //     tags conform to this RFC
    QStatus status = ER_OK;

    std::pair<AboutData::Internal::supportedLanguagesIterator, bool> ret =  aboutDataInternal->supportedLanguages.insert(language);
    // A new language has been added rebuild the MsgArg and update the field.
    if (true == ret.second) {
        size_t supportedLangsNum = aboutDataInternal->supportedLanguages.size();
        //char* supportedLangs[supportedLangsNum];
        const char** supportedLangs = new const char*[supportedLangsNum];
        size_t count = 0;
        for (AboutData::Internal::supportedLanguagesIterator it = aboutDataInternal->supportedLanguages.begin(); it != aboutDataInternal->supportedLanguages.end(); ++it) {
            supportedLangs[count] = it->c_str();
            ++count;
        }
        MsgArg arg;
        status = arg.Set(aboutDataInternal->aboutFields[SUPPORTED_LANGUAGES].signature.c_str(), supportedLangsNum, supportedLangs);
        if (status != ER_OK) {
            return status;
        }
        status = SetField(SUPPORTED_LANGUAGES, arg);
        delete [] supportedLangs;
    }
    return status;
}

size_t AboutData::GetSupportedLanguages(const char** languageTags, size_t num)
{
    if (languageTags == NULL) {
        return aboutDataInternal->supportedLanguages.size();
    }
    size_t count = 0;
    for (AboutData::Internal::supportedLanguagesIterator it = aboutDataInternal->supportedLanguages.begin(); it != aboutDataInternal->supportedLanguages.end() && count < num; ++it) {
        languageTags[count] = it->c_str();
        ++count;
    }
    return count;
}

QStatus AboutData::SetDescription(const char* description, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[DESCRIPTION].signature.c_str(), description);
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
    status = arg->Get(aboutDataInternal->aboutFields[DESCRIPTION].signature.c_str(), description);
    return status;
}

QStatus AboutData::SetDateOfManufacture(const char* dateOfManufacture)
{
    //TODO check that the dateOfManufacture string is of the correct format YYYY-MM-DD
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[DATE_OF_MANUFACTURE].signature.c_str(), dateOfManufacture);
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
    status = arg->Get(aboutDataInternal->aboutFields[DATE_OF_MANUFACTURE].signature.c_str(), dateOfManufacture);
    return status;
}

QStatus AboutData::SetSoftwareVersion(const char* softwareVersion)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[SOFTWARE_VERSION].signature.c_str(), softwareVersion);
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
    status = arg->Get(aboutDataInternal->aboutFields[SOFTWARE_VERSION].signature.c_str(), softwareVersion);
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
    status = arg->Get(aboutDataInternal->aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), ajSoftwareVersion);
    return status;
}

QStatus AboutData::SetHardwareVersion(const char* hardwareVersion)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[HARDWARE_VERSION].signature.c_str(), hardwareVersion);
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
    status = arg->Get(aboutDataInternal->aboutFields[HARDWARE_VERSION].signature.c_str(), hardwareVersion);
    return status;
}

QStatus AboutData::SetSupportUrl(const char* supportUrl)
{
    QStatus status = ER_OK;
    MsgArg arg;
    status = arg.Set(aboutDataInternal->aboutFields[SUPPORT_URL].signature.c_str(), supportUrl);
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
    status = arg->Get(aboutDataInternal->aboutFields[SUPPORT_URL].signature.c_str(), supportUrl);
    return status;
}

QStatus AboutData::SetField(const char* name, ajn::MsgArg value, const char* language) {
    QStatus status = ER_OK;
    // The user is adding an OEM specific field.
    // At this time OEM specific fields are added as
    //    not required
    //    not announced
    //    can be localized
    if (aboutDataInternal->aboutFields.find(name) == aboutDataInternal->aboutFields.end()) {
        aboutDataInternal->aboutFields[name] = FieldDetails(LOCALIZED, value.Signature().c_str());
    }
    if (IsFieldLocalized(name)) {
        if (language == NULL || strcmp(language, "") == 0) {
            std::map<qcc::String, MsgArg>::iterator it = aboutDataInternal->propertyStore.find(DEFAULT_LANGUAGE);
            if (it == aboutDataInternal->propertyStore.end()) {
                return ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED;
            }
            char* defaultLanguage;
            status = it->second.Get(aboutDataInternal->aboutFields[DEFAULT_LANGUAGE].signature.c_str(), &defaultLanguage);
            if (status != ER_OK) {
                return status;
            }
            aboutDataInternal->localizedPropertyStore[name][static_cast<qcc::String>(defaultLanguage)] = value;
        } else {
            aboutDataInternal->localizedPropertyStore[name][language] = value;
            //implicitly add all language tags to the supported languages
            status = SetSupportedLanguage(language);
            if (status != ER_OK) {
                return status;
            }
        }
    } else {
        aboutDataInternal->propertyStore[name] = value;
    }
    return status;
}

QStatus AboutData::GetField(const char* name, ajn::MsgArg*& value, const char* language) {
    QStatus status = ER_OK;
    if (!IsFieldLocalized(name)) {
        value = &(aboutDataInternal->propertyStore[name]);
    } else {
        if (language == NULL) {
            char* defaultLanguage;
            status = aboutDataInternal->propertyStore[DEFAULT_LANGUAGE].Get(aboutDataInternal->aboutFields[DEFAULT_LANGUAGE].signature.c_str(), &defaultLanguage);
            if (status != ER_OK) {
                return status;
            }
            value = &(aboutDataInternal->localizedPropertyStore[name][static_cast<qcc::String>(defaultLanguage)]);
        } else {
            value = &(aboutDataInternal->localizedPropertyStore[name][language]);
        }
    }
    return status;
}

size_t AboutData::GetFields(const char** fields, size_t num_fields) const
{
    if (fields == NULL) {
        return (aboutDataInternal->propertyStore.size() + aboutDataInternal->localizedPropertyStore.size());
    }
    size_t field_count = 0;
    std::map<qcc::String, MsgArg>::const_iterator pstore_it;
    for (pstore_it = aboutDataInternal->propertyStore.begin();
         (pstore_it != aboutDataInternal->propertyStore.end()) && (field_count < num_fields);
         ++pstore_it) {
        fields[field_count] = pstore_it->first.c_str();
        ++field_count;
    }

    AboutData::Internal::localizedPropertyStoreConstIterator lpstore_it;
    for (lpstore_it = aboutDataInternal->localizedPropertyStore.begin();
         (lpstore_it != aboutDataInternal->localizedPropertyStore.end()) && (field_count < num_fields);
         ++lpstore_it) {
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
        for (AboutData::Internal::supportedLanguagesIterator it = aboutDataInternal->supportedLanguages.begin(); it != aboutDataInternal->supportedLanguages.end(); ++it) {
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
    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        if (IsFieldRequired(it->first.c_str())) {
            dictonarySize++;
        } else {
            if (IsFieldLocalized(it->first.c_str())) {
                if (language == NULL || 0 == strcmp(language, "")) {
                    if (aboutDataInternal->localizedPropertyStore[it->first].find(defaultLanguage) != aboutDataInternal->localizedPropertyStore[it->first].end()) {
                        dictonarySize++;
                    }
                } else {
                    if (aboutDataInternal->localizedPropertyStore[it->first].find(language) != aboutDataInternal->localizedPropertyStore[it->first].end()) {
                        dictonarySize++;
                    }
                }
            } else {
                if (aboutDataInternal->propertyStore.find(it->first) != aboutDataInternal->propertyStore.end()) {
                    dictonarySize++;
                }
            }
        }
    }

    MsgArg* aboutDictonary = new MsgArg[dictonarySize];
    size_t count = 0;

    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        if (IsFieldRequired(it->first.c_str())) {
            if (IsFieldLocalized(it->first.c_str())) {
                if (language == NULL || 0 == strcmp(language, "")) {
                    status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->localizedPropertyStore[it->first][defaultLanguage]);
                } else {
                    status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->localizedPropertyStore[it->first][language]);
                }

            } else {
                status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->propertyStore[it->first]);
            }
        } else {
            if (IsFieldLocalized(it->first.c_str())) {
                if (language == NULL || 0 == strcmp(language, "")) {
                    if (aboutDataInternal->localizedPropertyStore[it->first].find(defaultLanguage) != aboutDataInternal->localizedPropertyStore[it->first].end()) {
                        status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->localizedPropertyStore[it->first][defaultLanguage]);
                    }
                } else {
                    if (aboutDataInternal->localizedPropertyStore[it->first].find(defaultLanguage) != aboutDataInternal->localizedPropertyStore[it->first].end()) {
                        status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->localizedPropertyStore[it->first][language]);
                    }
                }
            } else {
                if (aboutDataInternal->propertyStore.find(it->first) != aboutDataInternal->propertyStore.end()) {
                    status = aboutDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->propertyStore[it->first]);
                }
            }
        }
        if (status != ER_OK) {
            delete [] aboutDictonary;
            return status;
        }
    }
    assert(dictonarySize == count);

    msgArg->Set("a{sv}", dictonarySize, aboutDictonary);
    msgArg->Stabilize();
    delete [] aboutDictonary;
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
    //QStatus status = m_defaultLanguage.Get(aboutDataInternal->m_aboutFields[DEFAULT_LANGUAGE].dataType.c_str(), &defaultLanguage);
    if (status != ER_OK) {
        return status;
    }

    size_t dictonarySize = 0;
    typedef std::map<qcc::String, FieldDetails>::iterator it_aboutFields;
    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        if (IsFieldAnnounced(it->first.c_str())) {
            if (IsFieldRequired(it->first.c_str())) {
                dictonarySize++;
            } else {
                if (IsFieldLocalized(it->first.c_str())) {
                    if (aboutDataInternal->localizedPropertyStore[it->first].find(defaultLanguage) != aboutDataInternal->localizedPropertyStore[it->first].end()) {
                        dictonarySize++;
                    }
                } else {
                    if (aboutDataInternal->propertyStore.find(it->first) != aboutDataInternal->propertyStore.end()) {
                        dictonarySize++;
                    }
                }
            }
        }
    }

    MsgArg* announceDictonary = new MsgArg[dictonarySize];
    size_t count = 0;
    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        if (IsFieldAnnounced(it->first.c_str())) {
            if (IsFieldRequired(it->first.c_str())) {
                if (IsFieldLocalized(it->first.c_str())) {
                    status = announceDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->localizedPropertyStore[it->first][defaultLanguage]);
                } else {
                    status = announceDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->propertyStore[it->first]);
                }
            } else {
                if (IsFieldLocalized(it->first.c_str())) {
                    if (aboutDataInternal->localizedPropertyStore[it->first].find(defaultLanguage) != aboutDataInternal->localizedPropertyStore[it->first].end()) {
                        status = announceDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->localizedPropertyStore[it->first][defaultLanguage]);
                    }
                } else {
                    if (aboutDataInternal->propertyStore.find(it->first) != aboutDataInternal->propertyStore.end()) {
                        status = announceDictonary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->propertyStore[it->first]);
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
    delete [] announceDictonary;
    return status;
}

bool AboutData::IsFieldRequired(const char* fieldName) {
    if (aboutDataInternal->aboutFields.find(fieldName) != aboutDataInternal->aboutFields.end()) {
        return ((aboutDataInternal->aboutFields[fieldName].fieldMask & REQUIRED) == REQUIRED);
    }
    return false;
}
bool AboutData::IsFieldAnnounced(const char* fieldName) {
    if (aboutDataInternal->aboutFields.find(fieldName) != aboutDataInternal->aboutFields.end()) {
        return ((aboutDataInternal->aboutFields[fieldName].fieldMask & ANNOUNCED) == ANNOUNCED);
    }
    return false;
}

bool AboutData::IsFieldLocalized(const char* fieldName) {
    if (aboutDataInternal->aboutFields.find(fieldName) != aboutDataInternal->aboutFields.end()) {
        return ((aboutDataInternal->aboutFields[fieldName].fieldMask & LOCALIZED) == LOCALIZED);
    }
    return false;
}

const char* AboutData::GetFieldSignature(const char* fieldName) {
    if (aboutDataInternal->aboutFields.find(fieldName) != aboutDataInternal->aboutFields.end()) {
        return aboutDataInternal->aboutFields[fieldName].signature.c_str();
    }
    return NULL;
}

QStatus AboutData::SetNewFieldDetails(const char* fieldName, AboutFieldMask fieldMask, const char* signature)
{
    if (aboutDataInternal->aboutFields.find(fieldName) == aboutDataInternal->aboutFields.end()) {
        aboutDataInternal->aboutFields[fieldName] = FieldDetails(fieldMask, signature);
    } else {
        return ER_ABOUT_FIELD_ALREADY_SPECIFIED;
    }
    return ER_OK;
}
} //end namespace ajn
