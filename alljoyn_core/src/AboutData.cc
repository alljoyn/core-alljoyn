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

#include <alljoyn/AboutData.h>
#include <alljoyn/version.h>

#include <qcc/XmlElement.h>
#include <qcc/String.h>
#include <qcc/StringSource.h>

#include <stdio.h>
#include <utility>

#include "AboutDataInternal.h"

#define QCC_MODULE "ALLJOYN_ABOUT"
using ajn::AboutKeys;

namespace ajn {

AboutData::AboutData()
{
    InitializeFieldDetails();

    MsgArg arg;
    // The AllJoyn software version should always be set by default.
    arg.Set(aboutDataInternal->aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), GetVersion());
    aboutDataInternal->SetField(AJ_SOFTWARE_VERSION, arg);

    //TODO should the constructor also set the DeviceID as well?
}

AboutData::AboutData(const char* defaultLanguage)
{
    InitializeFieldDetails();

    // The user must specify a default language when creating the AboutData class
    MsgArg arg;
    arg.Set(aboutDataInternal->aboutFields[DEFAULT_LANGUAGE].signature.c_str(), defaultLanguage);
    aboutDataInternal->SetField(DEFAULT_LANGUAGE, arg);
    // The default language should automatically be added to the list of supported languages
    aboutDataInternal->SetSupportedLanguage(defaultLanguage);
    // The AllJoyn software version should always be set by default.
    arg.Set(aboutDataInternal->aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), GetVersion());
    aboutDataInternal->SetField(AJ_SOFTWARE_VERSION, arg);

    //TODO should the constructor also set the DeviceID as well?
}

AboutData::AboutData(const qcc::String& defaultLanguage)
{
    InitializeFieldDetails();

    MsgArg arg;
    arg.Set(aboutDataInternal->aboutFields[DEFAULT_LANGUAGE].signature.c_str(), defaultLanguage.c_str());
    aboutDataInternal->SetField(DEFAULT_LANGUAGE, arg);
    // The default language should automatically be added to the list of supported languages
    aboutDataInternal->SetSupportedLanguage(defaultLanguage.c_str());
    // The AllJoyn software version should always be set by default.
    arg.Set(aboutDataInternal->aboutFields[AJ_SOFTWARE_VERSION].signature.c_str(), GetVersion());
    aboutDataInternal->SetField(AJ_SOFTWARE_VERSION, arg);
}

AboutData::AboutData(const MsgArg arg, const char* language)
{
    InitializeFieldDetails();

    QStatus status = aboutDataInternal->CreateFromMsgArg(arg, language);
    if (ER_OK != status) {
        QCC_LogError(status, ("AboutData::AboutData(MsgArg): failed to parse MsgArg."));
    }
}

AboutData::AboutData(const MsgArg arg, const qcc::String& language)
{
    InitializeFieldDetails();

    QStatus status = aboutDataInternal->CreateFromMsgArg(arg, language.c_str());
    if (ER_OK != status) {
        QCC_LogError(status, ("AboutData::AboutData(MsgArg): failed to parse MsgArg."));
    }
}

AboutData::AboutData(const AboutData& src)
{
    InitializeFieldDetails();
    src.aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    *aboutDataInternal = *(src.aboutDataInternal);
    if (src.aboutDataInternal->translator == &src.aboutDataInternal->defaultTranslator) {
        aboutDataInternal->translator = &aboutDataInternal->defaultTranslator;
    }
    src.aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
}

AboutData& AboutData::operator=(const AboutData& src)
{
    if (&src == this) {
        return *this;
    }
    AboutData temp(src);
    std::swap(aboutDataInternal, temp.aboutDataInternal);
    return *this;
}

void AboutData::InitializeFieldDetails()
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
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
    aboutDataInternal->translator = &aboutDataInternal->defaultTranslator;
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
}

AboutData::~AboutData()
{
    delete aboutDataInternal;
    aboutDataInternal = nullptr;
}

QStatus AboutData::CreateFromXml(const char* aboutDataXml)
{
    return CreateFromXml(qcc::String(aboutDataXml));
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
    QStatus returnStatus = ER_OK;
    /*
     * This will iterate through the list of known fields in the about data.  If
     * the field is not localized set that field. We grab the non-localized values
     * first because we need the DefaultLanguage to to set a localized value where
     * the language tag is not given.
     */
    typedef std::map<qcc::String, FieldDetails>::iterator it_aboutFields;
    MsgArg arg;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
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
        if (!aboutDataInternal->IsFieldLocalized(it->first.c_str())) {
            // if we are unable to find one of the required fields continue
            // trying to find the rest of the fields.
            if (root->GetChild(it->first) == nullptr) {
                if (aboutDataInternal->IsFieldRequired(it->first.c_str())) {
                    returnStatus = ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD;
                }
                continue;
            }
            if (root->GetChild(it->first)->GetContent() != "") {
                // All non-localized fields in the about data are strings and are
                // treated like a string except for the AppId and SupportedLanguages
                // Since Languages are implicitly added we don't look for the
                // SupportedLanguages tag.
                if (it->first == APP_ID) {
                    status = aboutDataInternal->SetAppId(root->GetChild(it->first)->GetContent().c_str());
                    if (ER_OK != status) {
                        returnStatus = status;
                        continue;
                    }
                } else {
                    QCC_ASSERT(aboutDataInternal->aboutFields[it->first].signature == "s");
                    arg.Set("s", root->GetChild(it->first)->GetContent().c_str());
                    status = aboutDataInternal->SetField(it->first.c_str(), arg);
                    if (status != ER_OK) {
                        returnStatus = status;
                        continue;
                    }
                    // Make sure the DefaultLanguage is added to the list of
                    // SupportedLanguages.
                    if (it->first == DEFAULT_LANGUAGE) {
                        status = aboutDataInternal->SetSupportedLanguage(root->GetChild(it->first)->GetContent().c_str());
                        if (status != ER_OK) {
                            returnStatus = status;
                            continue;
                        }
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
    for (std::vector<qcc::XmlElement*>::iterator it = elements.begin(); it != elements.end(); ++it) {
        if (aboutDataInternal->IsFieldLocalized((*it)->GetName().c_str()) ||
            (aboutDataInternal->aboutFields.find((*it)->GetName()) == aboutDataInternal->aboutFields.end())) {
            status = arg.Set("s", (*it)->GetContent().c_str());
            if (status != ER_OK) {
                returnStatus = status;
                continue;
            }
            status = aboutDataInternal->SetField((*it)->GetName().c_str(), arg, (*it)->GetAttribute("lang").c_str());
            if (status != ER_OK) {
                returnStatus = status;
                continue;
            }
        }
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return returnStatus;
}

bool AboutData::IsValid(const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    bool isValid = aboutDataInternal->IsValid(language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return isValid;
}

bool AboutData::IsValid(const qcc::String& language) const
{
    return IsValid(language.c_str());
}

QStatus AboutData::CreatefromMsgArg(const MsgArg& arg, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->CreateFromMsgArg(arg, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::CreateFromMsgArg(const MsgArg& arg, const qcc::String& language)
{
    return CreatefromMsgArg(arg, language.c_str());
}

QStatus AboutData::SetAppId(const uint8_t* appId, const size_t num)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetAppId(appId, num);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetAppId(uint8_t** appId, size_t* num)
{
    QStatus status;
    MsgArg* arg;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    status = aboutDataInternal->GetField(APP_ID, arg);
    if (status != ER_OK) {
        aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
        return status;
    }
    status = arg->Get(aboutDataInternal->aboutFields[APP_ID].signature.c_str(), num, appId);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return status;
}

QStatus AboutData::GetAppId(qcc::String& appId) const
{
    QStatus status;
    MsgArg* arg;
    size_t num;
    char* appIdChar;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    status = aboutDataInternal->GetField(APP_ID, arg);
    if (status != ER_OK) {
        aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
        return status;
    }
    status = arg->Get(aboutDataInternal->aboutFields[APP_ID].signature.c_str(), &num, &appIdChar);
    if (status == ER_OK) {
        appId.assign_std(appIdChar, num);
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return status;
}

QStatus AboutData::SetAppId(const char* appId)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetAppId(appId);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetAppId(const qcc::String& appId)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetAppId(appId.c_str());
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetDefaultLanguage(const char* defaultLanguage)
{
    QStatus status = ER_OK;
    MsgArg arg;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    status = aboutDataInternal->SetField(DEFAULT_LANGUAGE, defaultLanguage);
    //The default language should automatically be added to the supported languages
    aboutDataInternal->SetSupportedLanguage(defaultLanguage);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return status;
}

QStatus AboutData::SetDefaultLanguage(const qcc::String& defaultLanguage)
{
    QStatus status = ER_OK;
    MsgArg arg;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    status = aboutDataInternal->SetField(DEFAULT_LANGUAGE, defaultLanguage);
    //The default language should automatically be added to the supported languages
    aboutDataInternal->SetSupportedLanguage(defaultLanguage.c_str());
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return status;
}

QStatus AboutData::GetDefaultLanguage(char** defaultLanguage) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetDefaultLanguage(defaultLanguage);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDefaultLanguage(qcc::String& defaultLanguage) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DEFAULT_LANGUAGE, defaultLanguage);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetDeviceName(const char* deviceName, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(DEVICE_NAME, deviceName, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetDeviceName(const qcc::String& deviceName, const qcc::String& language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(DEVICE_NAME, deviceName, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDeviceName(char** deviceName, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DEVICE_NAME, deviceName, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDeviceName(qcc::String& deviceName, const qcc::String& language) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DEVICE_NAME, deviceName, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetDeviceId(const char* deviceId)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(DEVICE_ID, deviceId);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetDeviceId(const qcc::String& deviceId)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(DEVICE_ID, deviceId);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDeviceId(char** deviceId)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DEVICE_ID, deviceId);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDeviceId(qcc::String& deviceId) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DEVICE_ID, deviceId);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetAppName(const char* appName, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(APP_NAME, appName, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetAppName(const qcc::String& appName, const qcc::String& language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(APP_NAME, appName, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetAppName(char** appName, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(APP_NAME, appName, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetAppName(qcc::String& appName, const qcc::String& language) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(APP_NAME, appName, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetManufacturer(const char* manufacturer, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(MANUFACTURER, manufacturer, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetManufacturer(const qcc::String& manufacturer, const qcc::String& language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(MANUFACTURER, manufacturer, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetManufacturer(char** manufacturer, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(MANUFACTURER, manufacturer, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetManufacturer(qcc::String& manufacturer, const qcc::String& language) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(MANUFACTURER, manufacturer, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetModelNumber(const char* modelNumber)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(MODEL_NUMBER, modelNumber);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetModelNumber(const qcc::String& modelNumber)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(MODEL_NUMBER, modelNumber);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetModelNumber(char** modelNumber)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(MODEL_NUMBER, modelNumber);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetModelNumber(qcc::String& modelNumber) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(MODEL_NUMBER, modelNumber);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetSupportedLanguage(const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetSupportedLanguage(language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetSupportedLanguage(const qcc::String& language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetSupportedLanguage(language.c_str());
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

size_t AboutData::GetSupportedLanguages(const char** languageTags, size_t num)
{
    size_t size = 0;
    MsgArg* arg = nullptr;
    MsgArg* args = nullptr;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(SUPPORTED_LANGUAGES, arg);
    if (ER_OK == status) {
        status = arg->Get(aboutDataInternal->aboutFields[SUPPORTED_LANGUAGES].signature.c_str(), &size, &args);
    }

    if (languageTags == nullptr) {
        aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
        return size;
    }

    size_t count = 0;
    if ((ER_OK == status) && (args != nullptr)) {
        for (count = 0; (count < size) && (count < num); count++) {
            args[count].Get("s", &(languageTags[count]));
        }
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return count;
}

std::set<qcc::String> AboutData::GetSupportedLanguagesSet() const
{
    size_t size = 0;
    MsgArg* arg = nullptr;
    MsgArg* args = nullptr;
    std::set<qcc::String> languageTags;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(SUPPORTED_LANGUAGES, arg);
    if (status == ER_OK) {
        status = arg->Get(aboutDataInternal->aboutFields[SUPPORTED_LANGUAGES].signature.c_str(), &size, &args);
    }

    if ((status == ER_OK) && (args != nullptr)) {
        for (size_t count = 0; count < size; count++) {
            char* languageTag;
            args[count].Get("s", &languageTag);
            languageTags.insert(qcc::String(languageTag));
        }
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return languageTags;
}

QStatus AboutData::SetDescription(const char* description, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(DESCRIPTION, description, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetDescription(const qcc::String& description, const qcc::String& language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(DESCRIPTION, description, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDescription(char** description, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DESCRIPTION, description, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDescription(qcc::String& description, const qcc::String& language) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DESCRIPTION, description, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetDateOfManufacture(const char* dateOfManufacture)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    //TODO check that the dateOfManufacture string is of the correct format YYYY-MM-DD
    QStatus status = aboutDataInternal->SetField(DATE_OF_MANUFACTURE, dateOfManufacture);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetDateOfManufacture(const qcc::String& dateOfManufacture)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(DATE_OF_MANUFACTURE, dateOfManufacture);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDateOfManufacture(char** dateOfManufacture)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DATE_OF_MANUFACTURE, dateOfManufacture);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetDateOfManufacture(qcc::String& dateOfManufacture) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(DATE_OF_MANUFACTURE, dateOfManufacture);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetSoftwareVersion(const char* softwareVersion)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(SOFTWARE_VERSION, softwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetSoftwareVersion(const qcc::String& softwareVersion)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(SOFTWARE_VERSION, softwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetSoftwareVersion(char** softwareVersion)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(SOFTWARE_VERSION, softwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetSoftwareVersion(qcc::String& softwareVersion) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(SOFTWARE_VERSION, softwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetAJSoftwareVersion(char** ajSoftwareVersion)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(AJ_SOFTWARE_VERSION, ajSoftwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetAJSoftwareVersion(qcc::String& ajSoftwareVersion) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(AJ_SOFTWARE_VERSION, ajSoftwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetHardwareVersion(const char* hardwareVersion)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(HARDWARE_VERSION, hardwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetHardwareVersion(const qcc::String& hardwareVersion)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(HARDWARE_VERSION, hardwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetHardwareVersion(char** hardwareVersion)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(HARDWARE_VERSION, hardwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetHardwareVersion(qcc::String& hardwareVersion) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(HARDWARE_VERSION, hardwareVersion);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetSupportUrl(const char* supportUrl)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(SUPPORT_URL, supportUrl);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetSupportUrl(const qcc::String& supportUrl)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(SUPPORT_URL, supportUrl);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetSupportUrl(char** supportUrl)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(SUPPORT_URL, supportUrl);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetSupportUrl(qcc::String& supportUrl) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(SUPPORT_URL, supportUrl);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetField(const char* name, ajn::MsgArg value, const char* language)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->SetField(name, value, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::SetField(const qcc::String& name, ajn::MsgArg value, const qcc::String& language)
{
    return SetField(name.c_str(), value, language.c_str());
}

QStatus AboutData::GetField(const char* name, ajn::MsgArg*& value, const char* language) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    QStatus status = aboutDataInternal->GetField(name, value, language);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus AboutData::GetField(const qcc::String& name, ajn::MsgArg*& value, const qcc::String& language) const
{
    return GetField(name.c_str(), value, language.c_str());
}

size_t AboutData::GetFields(const char** fields, size_t num_fields) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    size_t numLocalizedFields = aboutDataInternal->defaultTranslator.NumFields();
    if (fields == nullptr) {
        aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
        return (aboutDataInternal->propertyStore.size() + numLocalizedFields);
    }
    size_t field_count = 0;
    std::map<qcc::String, MsgArg>::const_iterator pstore_it;
    for (pstore_it = aboutDataInternal->propertyStore.begin();
         (pstore_it != aboutDataInternal->propertyStore.end()) && (field_count < num_fields);
         ++pstore_it) {
        fields[field_count] = pstore_it->first.c_str();
        ++field_count;
    }

    for (size_t index = 0; index < numLocalizedFields; index++) {
        fields[field_count] = aboutDataInternal->defaultTranslator.GetFieldId(index);
        ++field_count;
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return field_count;
}

std::set<qcc::String> AboutData::GetFieldsSet() const
{
    std::set<qcc::String> fields;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    for (auto const& itP : aboutDataInternal->propertyStore) {
        fields.insert(itP.first);
    }

    size_t numLocalizedFields = aboutDataInternal->defaultTranslator.NumFields();
    for (size_t index = 0; index < numLocalizedFields; index++) {
        fields.insert(aboutDataInternal->defaultTranslator.GetFieldId(index));
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return fields;
}

QStatus AboutData::GetAboutData(MsgArg* msgArg, const char* language)
{
    QStatus status = ER_OK;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    if (!IsValid(nullptr)) {
        aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
        return ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD;
    }

    char* defaultLanguage = nullptr;
    aboutDataInternal->GetDefaultLanguage(&defaultLanguage);
    // A default language must exist or IsValid would have been false above.
    QCC_ASSERT(defaultLanguage != nullptr);

    qcc::String defaultLang(defaultLanguage);
    qcc::String bestLanguage;
    aboutDataInternal->translator->GetBestLanguage(language, defaultLang, bestLanguage);

    // At least a default language must exist or IsValid would have been
    // false above.
    QCC_ASSERT(bestLanguage.c_str() != nullptr);

    size_t dictionarySize = 0;
    typedef std::map<qcc::String, FieldDetails>::iterator it_aboutFields;
    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        const char* fieldname = it->first.c_str();
        if (aboutDataInternal->IsFieldRequired(fieldname)) {
            dictionarySize++;
        } else {
            if (aboutDataInternal->IsFieldLocalized(fieldname)) {
                MsgArg* arg;
                if ((aboutDataInternal->translator->TranslateToMsgArg(aboutDataInternal->keyLanguage.c_str(), bestLanguage.c_str(), fieldname, arg) == ER_OK) && (arg->typeId == ALLJOYN_STRING)) {
                    dictionarySize++;
                }
            } else {
                if (aboutDataInternal->propertyStore.find(it->first) != aboutDataInternal->propertyStore.end()) {
                    dictionarySize++;
                }
            }
        }
    }

    MsgArg* aboutDictionary = new MsgArg[dictionarySize];
    size_t count = 0;

    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        const char* fieldname = it->first.c_str();
        if (aboutDataInternal->IsFieldRequired(fieldname)) {
            if (aboutDataInternal->IsFieldLocalized(fieldname)) {
                MsgArg* arg;
                status = aboutDataInternal->translator->TranslateToMsgArg(aboutDataInternal->keyLanguage.c_str(), bestLanguage.c_str(), fieldname, arg);
                if (status == ER_OK) {
                    status = aboutDictionary[count++].Set("{sv}", fieldname, arg);
                }
            } else {
                status = aboutDictionary[count++].Set("{sv}", fieldname, &aboutDataInternal->propertyStore[it->first]);
            }
        } else {
            if (aboutDataInternal->IsFieldLocalized(fieldname)) {
                MsgArg* arg;
                if ((aboutDataInternal->translator->TranslateToMsgArg(aboutDataInternal->keyLanguage.c_str(), bestLanguage.c_str(), fieldname, arg) == ER_OK) &&
                    (arg->typeId == ALLJOYN_STRING)) {
                    status = aboutDictionary[count++].Set("{sv}", fieldname, arg);
                }
            } else {
                if (aboutDataInternal->propertyStore.find(it->first) != aboutDataInternal->propertyStore.end()) {
                    status = aboutDictionary[count++].Set("{sv}", fieldname, &aboutDataInternal->propertyStore[it->first]);
                }
            }
        }
        if (status != ER_OK) {
            aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
            delete [] aboutDictionary;
            return status;
        }
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    QCC_ASSERT(dictionarySize == count);

    msgArg->Set("a{sv}", dictionarySize, aboutDictionary);
    msgArg->Stabilize();
    delete [] aboutDictionary;
    return status;
}

QStatus AboutData::GetAboutData(MsgArg* msgArg, const qcc::String& language) const
{
    return GetAboutData(msgArg, language.c_str());
}

QStatus AboutData::GetAnnouncedAboutData(MsgArg* msgArg)
{
    QStatus status;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    if (!aboutDataInternal->IsValid()) {
        // TODO put in an ER code that is more meaningful
        aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
        return ER_FAIL;
    }
    char* defaultLanguage;
    status = aboutDataInternal->GetDefaultLanguage(&defaultLanguage);
    //QStatus status = m_defaultLanguage.Get(aboutDataInternal->m_aboutFields[DEFAULT_LANGUAGE].dataType.c_str(), &defaultLanguage);
    if (status != ER_OK) {
        aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
        return status;
    }

    size_t dictionarySize = 0;
    typedef std::map<qcc::String, FieldDetails>::iterator it_aboutFields;
    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        if (aboutDataInternal->IsFieldAnnounced(it->first.c_str())) {
            if (aboutDataInternal->IsFieldRequired(it->first.c_str())) {
                dictionarySize++;
            } else {
                if (aboutDataInternal->IsFieldLocalized(it->first.c_str())) {
                    MsgArg* arg;
                    if ((aboutDataInternal->translator->TranslateToMsgArg(aboutDataInternal->keyLanguage.c_str(), defaultLanguage, it->first.c_str(), arg) == ER_OK) && (arg->typeId == ALLJOYN_STRING)) {
                        dictionarySize++;
                    }
                } else {
                    if (aboutDataInternal->propertyStore.find(it->first) != aboutDataInternal->propertyStore.end()) {
                        dictionarySize++;
                    }
                }
            }
        }
    }

    MsgArg* announceDictionary = new MsgArg[dictionarySize];
    size_t count = 0;
    for (it_aboutFields it = aboutDataInternal->aboutFields.begin(); it != aboutDataInternal->aboutFields.end(); ++it) {
        if (aboutDataInternal->IsFieldAnnounced(it->first.c_str())) {
            if (aboutDataInternal->IsFieldRequired(it->first.c_str())) {
                if (aboutDataInternal->IsFieldLocalized(it->first.c_str())) {
                    MsgArg* arg;
                    status = aboutDataInternal->translator->TranslateToMsgArg(aboutDataInternal->keyLanguage.c_str(), defaultLanguage, it->first.c_str(), arg);
                    if (status == ER_OK) {
                        status = announceDictionary[count++].Set("{sv}", it->first.c_str(), arg);
                    }
                } else {
                    status = announceDictionary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->propertyStore[it->first]);
                }
            } else {
                if (aboutDataInternal->IsFieldLocalized(it->first.c_str())) {
                    MsgArg* arg;
                    if ((aboutDataInternal->translator->TranslateToMsgArg(aboutDataInternal->keyLanguage.c_str(), defaultLanguage, it->first.c_str(), arg) == ER_OK) &&
                        (arg->typeId == ALLJOYN_STRING)) {
                        status = announceDictionary[count++].Set("{sv}", it->first.c_str(), arg);
                    }
                } else {
                    if (aboutDataInternal->propertyStore.find(it->first) != aboutDataInternal->propertyStore.end()) {
                        status = announceDictionary[count++].Set("{sv}", it->first.c_str(), &aboutDataInternal->propertyStore[it->first]);
                    }
                }
            }
            if (status != ER_OK) {
                aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
                delete [] announceDictionary;
                return status;
            }
        }
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    QCC_ASSERT(dictionarySize == count);

    msgArg->Set("a{sv}", dictionarySize, announceDictionary);
    msgArg->Stabilize();
    delete [] announceDictionary;
    return status;
}

bool AboutData::IsFieldRequired(const char* fieldName)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    bool isRequired = aboutDataInternal->IsFieldRequired(fieldName);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return isRequired;
}

bool AboutData::IsFieldRequired(const qcc::String& fieldName) const
{
    return IsFieldRequired(fieldName.c_str());
}

bool AboutData::IsFieldAnnounced(const char* fieldName)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    bool isAnnounced = aboutDataInternal->IsFieldAnnounced(fieldName);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return isAnnounced;
}

bool AboutData::IsFieldAnnounced(const qcc::String& fieldName) const
{
    return IsFieldAnnounced(fieldName.c_str());
}

bool AboutData::IsFieldLocalized(const char* fieldName) const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    bool isLocalized = aboutDataInternal->IsFieldLocalized(fieldName);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return isLocalized;
}

bool AboutData::IsFieldLocalized(const qcc::String& fieldName) const
{
    return IsFieldLocalized(fieldName.c_str());
}

const char* AboutData::GetFieldSignature(const char* fieldName)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    const char* signature = aboutDataInternal->GetFieldSignature(fieldName);
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return signature;
}

qcc::String AboutData::GetFieldSignature(const qcc::String& fieldName) const
{
    return GetFieldSignature(fieldName.c_str());
}

QStatus AboutData::SetNewFieldDetails(const char* fieldName, AboutFieldMask fieldMask, const char* signature)
{
    QStatus status = ER_OK;

    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    if (aboutDataInternal->aboutFields.find(fieldName) == aboutDataInternal->aboutFields.end()) {
        aboutDataInternal->aboutFields[fieldName] = FieldDetails(fieldMask, signature);
    } else {
        status = ER_ABOUT_FIELD_ALREADY_SPECIFIED;
    }
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);

    return status;
}

void AboutData::SetTranslator(Translator* translator)
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    aboutDataInternal->translator = translator;
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
}

Translator* AboutData::GetTranslator() const
{
    aboutDataInternalLock.Lock(MUTEX_CONTEXT);
    Translator* translator = aboutDataInternal->translator;
    aboutDataInternalLock.Unlock(MUTEX_CONTEXT);
    return translator;
}

} //end namespace ajn
