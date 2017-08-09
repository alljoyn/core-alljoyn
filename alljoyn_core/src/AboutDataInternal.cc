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

#include "AboutDataInternal.h"

#include <qcc/StringUtil.h>

namespace ajn {

bool AboutData::Internal::isHexChar(char c) const
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f'));
}

QStatus AboutData::Internal::SetField(const char* name, MsgArg value, const char* language)
{
    QStatus status = ER_OK;
    // The user is adding an OEM-specific field.
    // At this time OEM-specific fields are added as
    //    not required
    //    not announced
    //    if the field is a string it can be localized not localized otherwise
    if (aboutFields.find(name) == aboutFields.end()) {
        if (value.Signature() == "s") {
            aboutFields[name] = FieldDetails(LOCALIZED, value.Signature().c_str());
        } else {
            aboutFields[name] = FieldDetails(EMPTY_MASK, value.Signature().c_str());
        }
    }
    if (IsFieldLocalized(name)) {
        if (language == nullptr || strcmp(language, "") == 0) {
            std::map<qcc::String, MsgArg>::iterator it = propertyStore.find(DEFAULT_LANGUAGE);
            if (it == propertyStore.end()) {
                return ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED;
            }
            char* defaultLanguage;
            status = it->second.Get(aboutFields[DEFAULT_LANGUAGE].signature.c_str(), &defaultLanguage);
            if (status != ER_OK) {
                return status;
            }
            translator->AddMsgArgTranslation(name, &value, defaultLanguage);
        } else {
            translator->AddMsgArgTranslation(name, &value, language);

            // Implicitly add all language tags to the supported languages.
            status = SetSupportedLanguage(language);
            if (status != ER_OK) {
                return status;
            }
        }
    } else {
        propertyStore[name] = value;
    }
    return status;
}

QStatus AboutData::Internal::SetField(const char* fieldName, const char* value, const char* language)
{
    QStatus status = ER_OK;
    MsgArg arg;

    QCC_ASSERT(fieldName != nullptr);

    status = arg.Set(aboutFields[fieldName].signature.c_str(), value);
    if (status != ER_OK) {
        // ERROR
    } else {
        status = SetField(fieldName, arg, language);
    }

    return status;
}

QStatus AboutData::Internal::SetField(const qcc::String& fieldName, const qcc::String& value, const qcc::String& language)
{
    QStatus status = ER_OK;
    MsgArg arg;

    QCC_ASSERT(!fieldName.empty());

    status = arg.Set(aboutFields[fieldName].signature.c_str(), value.c_str());
    if (status != ER_OK) {
        // ERROR
    } else {
        status = SetField(fieldName.c_str(), arg, language.c_str());
    }

    return status;
}

QStatus AboutData::Internal::GetField(const char* name, MsgArg*& value, const char* language)
{
    QStatus status = ER_OK;
    if (!IsFieldLocalized(name)) {
        value = &(propertyStore[name]);
    } else {
        if (language == nullptr || strcmp(language, "") == 0) {
            char* defaultLanguage;
            status = propertyStore[DEFAULT_LANGUAGE].Get(aboutFields[DEFAULT_LANGUAGE].signature.c_str(), &defaultLanguage);
            if (status != ER_OK) {
                return status;
            }
            status = translator->TranslateToMsgArg(keyLanguage.c_str(), defaultLanguage, name, value);
        } else {
            status = translator->TranslateToMsgArg(keyLanguage.c_str(), language, name, value);
        }
    }
    return status;
}

QStatus AboutData::Internal::GetField(const char* fieldName, char** value, const char* language)
{
    QStatus status;
    MsgArg* arg;

    QCC_ASSERT(fieldName != nullptr);

    status = GetField(fieldName, arg, language);
    if (status != ER_OK) {
        // ERROR
    } else {
        status = arg->Get(aboutFields[fieldName].signature.c_str(), value);
    }

    return status;
}

QStatus AboutData::Internal::GetField(const qcc::String& fieldName, qcc::String& value, const qcc::String& language)
{
    QStatus status;
    char* charValue;

    status = GetField(fieldName.c_str(), &charValue, language.c_str());
    if (status == ER_OK) {
        value.assign(charValue);
    }

    return status;
}

QStatus AboutData::Internal::SetSupportedLanguage(const char* language)
{
    //TODO add in logic to check information about the tag all of the tags must
    //     conform to the RFC5646 there is currently nothing to make sure the
    //     tags conform to this RFC
    bool added;

    QStatus status = translator->AddTargetLanguage(language, &added);

    if (status == ER_OK) {
        size_t supportedLangsNum = translator->NumTargetLanguages();
        if (!added) {
            // Verify the language numbers in translator and about field are the same
            size_t size = 0;
            MsgArg* arg;
            QStatus status = GetField(SUPPORTED_LANGUAGES, arg);
            if (ER_OK == status) {
                MsgArg* langs;
                status = arg->Get(aboutFields[SUPPORTED_LANGUAGES].signature.c_str(), &size, &langs);
                added = (size != supportedLangsNum);    // Numbers don't match. Need to update the field
            }
        }

        if (added) {
            // A new language has been added. Rebuild the MsgArg and update the field.
            char** supportedLangs = new char*[supportedLangsNum];

            for (size_t count = 0; count < supportedLangsNum; count++) {
                qcc::String lang;
                translator->GetTargetLanguage(count, lang);
                supportedLangs[count] = new char[lang.length() + 1];
                strcpy(supportedLangs[count], lang.c_str());
            }

            MsgArg arg;
            status = arg.Set(aboutFields[SUPPORTED_LANGUAGES].signature.c_str(), supportedLangsNum, supportedLangs);
            if (status == ER_OK) {
                status = SetField(SUPPORTED_LANGUAGES, arg);
            }

            for (size_t count = 0; count < supportedLangsNum; count++) {
                delete[](supportedLangs[count]);
            }
            delete[] supportedLangs;
        }
    }

    return status;
}

QStatus AboutData::Internal::CreateFromMsgArg(const MsgArg& arg, const char* language)
{
    QStatus status = ER_OK;
    char* defaultLanguage = nullptr;
    MsgArg* fields;
    size_t numFields;
    status = arg.Get("a{sv}", &numFields, &fields);
    if (status != ER_OK) {
        return status;
    }
    if (language == nullptr) {
        MsgArg* argDefaultLang;
        status = arg.GetElement("{sv}", DEFAULT_LANGUAGE, &argDefaultLang);
        if (status != ER_OK) {
            //printf("GetElement Failed");
            return status;
        }
        status = argDefaultLang->Get("s", &defaultLanguage);
        if (status != ER_OK) {
            //printf("Get default language failed: \n%s\n %s\n", argDefaultLang->ToString().c_str());
            return status;
        }
    }

    for (size_t i = 0; (i < numFields) && (status == ER_OK); ++i) {
        char* fieldName;
        MsgArg* fieldValue;
        status = fields[i].Get("{sv}", &fieldName, &fieldValue);
        if (status != ER_OK) {
            return status;
        }
        // OEM-specific field found. Add that field to the m_aboutFields.
        if (aboutFields.find(fieldName) == aboutFields.end()) {
            aboutFields[fieldName] = FieldDetails(LOCALIZED, fieldValue->Signature().c_str());
        }
        if (fieldValue->Signature() != aboutFields[fieldName].signature) {
            return ER_BUS_SIGNATURE_MISMATCH;
        }
        if (IsFieldLocalized(fieldName)) {
            if (language != nullptr) {
                status = translator->AddMsgArgTranslation(fieldName, fieldValue, language);
            } else {
                status = translator->AddMsgArgTranslation(fieldName, fieldValue, defaultLanguage);
            }
        } else {
            propertyStore[fieldName] = *fieldValue;

            // Since the GetSupportedLanguages function looks at the
            // translator's target languages, we must set up the make sure
            // the member is filled in.
            if (strcmp(SUPPORTED_LANGUAGES, fieldName) == 0) {
                size_t language_count;
                MsgArg* languagesArg;
                fieldValue->Get(this->GetFieldSignature(SUPPORTED_LANGUAGES), &language_count, &languagesArg);
                for (size_t j = 0; j < language_count; ++j) {
                    char* lang;
                    languagesArg[j].Get("s", &lang);
                    status = translator->AddTargetLanguage(lang);
                }
            }
        }
    }
    return status;
}

bool AboutData::Internal::IsFieldRequired(const char* fieldName) const
{
    bool isRequired = false;
    if (aboutFields.find(fieldName) != aboutFields.end()) {
        isRequired = ((aboutFields.at(fieldName).fieldMask & REQUIRED) == REQUIRED);
    }
    return isRequired;
}

bool AboutData::Internal::IsFieldAnnounced(const char* fieldName) const
{
    bool isAnnounced = false;
    if (aboutFields.find(fieldName) != aboutFields.end()) {
        isAnnounced = ((aboutFields.at(fieldName).fieldMask & ANNOUNCED) == ANNOUNCED);
    }
    return isAnnounced;
}

bool AboutData::Internal::IsFieldLocalized(const char* fieldName) const
{
    bool isLocalized = false;
    if (aboutFields.find(fieldName) != aboutFields.end()) {
        isLocalized = ((aboutFields.at(fieldName).fieldMask & LOCALIZED) == LOCALIZED);
    }
    return isLocalized;
}

const char* AboutData::Internal::GetFieldSignature(const char* fieldName) const
{
    const char* signature = nullptr;
    if (aboutFields.find(fieldName) != aboutFields.end()) {
        signature = aboutFields.at(fieldName).signature.c_str();
    }
    return signature;
}

QStatus AboutData::Internal::SetAppId(const uint8_t* appId, const size_t num)
{
    QStatus status = ER_OK;
    MsgArg arg;

    status = arg.Set(aboutFields[APP_ID].signature.c_str(), num, appId);
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

QStatus AboutData::Internal::SetAppId(const char* appId)
{
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
            propertyStore[APP_ID].Stabilize();
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

bool AboutData::Internal::IsValid(const char* language)
{
    bool isValid = true;

    if (language == nullptr) {
        char* defaultLanguage;
        if (GetDefaultLanguage(&defaultLanguage) != ER_OK) {
            // No default language exists.
            return false;
        }
        language = const_cast<char*>(defaultLanguage);
    }

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
    for (it_aboutFields it = aboutFields.begin(); it != aboutFields.end(); ++it) {
        const char* fieldname = it->first.c_str();
        if (IsFieldRequired(fieldname)) {
            if (IsFieldLocalized(fieldname)) {
                MsgArg* arg;
                if ((translator->TranslateToMsgArg(keyLanguage.c_str(), language, fieldname, arg) != ER_OK) || (arg->typeId == ALLJOYN_INVALID)) {
                    isValid = false;
                    break;
                }
            } else {
                if (propertyStore.find(it->first) == propertyStore.end()) {
                    isValid = false;
                    break;
                }
            }
        }
    }

    return isValid;
}

QStatus AboutData::Internal::GetDefaultLanguage(char** defaultLanguage)
{
    return GetField(DEFAULT_LANGUAGE, defaultLanguage);
}

} //end namespace ajn
