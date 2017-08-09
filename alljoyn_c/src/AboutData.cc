/**
 * @file
 */
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

#include <alljoyn_c/AboutData.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/version.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <set>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_aboutdata {
    /* Empty by design */
};

alljoyn_aboutdata AJ_CALL alljoyn_aboutdata_create_empty()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutdata) new ajn::AboutData();
}

alljoyn_aboutdata AJ_CALL alljoyn_aboutdata_create(const char* defaultLanguage)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutdata) new ajn::AboutData(defaultLanguage);
}

alljoyn_aboutdata AJ_CALL alljoyn_aboutdata_create_full(const alljoyn_msgarg arg,
                                                        const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutdata) new ajn::AboutData(*(ajn::MsgArg*)arg, language);
}

void AJ_CALL alljoyn_aboutdata_destroy(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutData*)data;
}

QStatus AJ_CALL alljoyn_aboutdata_createfromxml(alljoyn_aboutdata data,
                                                const char* aboutDataXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->CreateFromXml(aboutDataXml);
}

bool AJ_CALL alljoyn_aboutdata_isvalid(alljoyn_aboutdata data,
                                       const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->IsValid(language);
}

QStatus AJ_CALL alljoyn_aboutdata_createfrommsgarg(alljoyn_aboutdata data,
                                                   const alljoyn_msgarg arg,
                                                   const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->CreatefromMsgArg(*(ajn::MsgArg*)arg, language);
}

QStatus AJ_CALL alljoyn_aboutdata_setappid(alljoyn_aboutdata data,
                                           const uint8_t* appId,
                                           const size_t num)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetAppId(appId, num);
}

QStatus AJ_CALL alljoyn_aboutdata_setappid_fromstring(alljoyn_aboutdata data,
                                                      const char* appId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetAppId(appId);
}

QStatus AJ_CALL alljoyn_aboutdata_getappid(alljoyn_aboutdata data,
                                           uint8_t** appId,
                                           size_t* num)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetAppId(appId, num);
}

QStatus AJ_CALL alljoyn_aboutdata_getappidcopy(alljoyn_aboutdata data,
                                               char* appId,
                                               size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetAppId(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            memcpy(appId, retrieved.c_str(), maxLength);
            appId[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *appId = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getappidlength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetAppId(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setdefaultlanguage(alljoyn_aboutdata data,
                                                     const char* defaultLanguage)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetDefaultLanguage(defaultLanguage);
}

QStatus AJ_CALL alljoyn_aboutdata_getdefaultlanguage(alljoyn_aboutdata data,
                                                     char** defaultLanguage)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetDefaultLanguage(defaultLanguage);
}

QStatus AJ_CALL alljoyn_aboutdata_getdefaultlanguagecopy(alljoyn_aboutdata data,
                                                         char* defaultLanguage,
                                                         size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDefaultLanguage(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(defaultLanguage, retrieved.c_str(), maxLength);
            defaultLanguage[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *defaultLanguage = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getdefaultlanguagelength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDefaultLanguage(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setdevicename(alljoyn_aboutdata data,
                                                const char* deviceName,
                                                const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetDeviceName(deviceName, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getdevicename(alljoyn_aboutdata data,
                                                char** deviceName,
                                                const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetDeviceName(deviceName, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getdevicenamecopy(alljoyn_aboutdata data,
                                                    char* deviceName,
                                                    size_t maxLength,
                                                    const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDeviceName(retrieved, language);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(deviceName, retrieved.c_str(), maxLength);
            deviceName[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *deviceName = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getdevicenamelength(alljoyn_aboutdata data,
                                                     const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDeviceName(retrieved, language);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setdeviceid(alljoyn_aboutdata data,
                                              const char* deviceId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetDeviceId(deviceId);
}

QStatus AJ_CALL alljoyn_aboutdata_getdeviceid(alljoyn_aboutdata data,
                                              char** deviceId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetDeviceId(deviceId);
}

QStatus AJ_CALL alljoyn_aboutdata_getdeviceidcopy(alljoyn_aboutdata data,
                                                  char* deviceId,
                                                  size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDeviceId(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(deviceId, retrieved.c_str(), maxLength);
            deviceId[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *deviceId = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getdeviceidlength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDeviceId(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setappname(alljoyn_aboutdata data,
                                             const char* appName,
                                             const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetAppName(appName, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getappname(alljoyn_aboutdata data,
                                             char** appName,
                                             const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetAppName(appName, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getappnamecopy(alljoyn_aboutdata data,
                                                 char* appName,
                                                 size_t maxLength,
                                                 const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetAppName(retrieved, language);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(appName, retrieved.c_str(), maxLength);
            appName[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *appName = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getappnamelength(alljoyn_aboutdata data,
                                                  const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetAppName(retrieved, language);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setmanufacturer(alljoyn_aboutdata data,
                                                  const char* manufacturer,
                                                  const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetManufacturer(manufacturer, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getmanufacturer(alljoyn_aboutdata data,
                                                  char** manufacturer,
                                                  const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetManufacturer(manufacturer, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getmanufacturercopy(alljoyn_aboutdata data,
                                                      char* manufacturer,
                                                      size_t maxLength,
                                                      const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetManufacturer(retrieved, language);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(manufacturer, retrieved.c_str(), maxLength);
            manufacturer[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *manufacturer = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getmanufacturerlength(alljoyn_aboutdata data,
                                                       const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetManufacturer(retrieved, language);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setmodelnumber(alljoyn_aboutdata data,
                                                 const char* modelNumber)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetModelNumber(modelNumber);
}

QStatus AJ_CALL alljoyn_aboutdata_getmodelnumber(alljoyn_aboutdata data,
                                                 char** modelNumber)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetModelNumber(modelNumber);
}

QStatus AJ_CALL alljoyn_aboutdata_getmodelnumbercopy(alljoyn_aboutdata data,
                                                     char* modelNumber,
                                                     size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetModelNumber(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(modelNumber, retrieved.c_str(), maxLength);
            modelNumber[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *modelNumber = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getmodelnumberlength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetModelNumber(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setsupportedlanguage(alljoyn_aboutdata data,
                                                       const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetSupportedLanguage(language);
}

size_t AJ_CALL alljoyn_aboutdata_getsupportedlanguages(alljoyn_aboutdata data,
                                                       const char** languageTags,
                                                       size_t num)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetSupportedLanguages(languageTags, num);
}

size_t AJ_CALL alljoyn_aboutdata_getsupportedlanguagescopy(alljoyn_aboutdata data,
                                                           char* languageTags,
                                                           size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    const qcc::String DELIMITER = ",";
    std::set<qcc::String> foundLanguages = ((ajn::AboutData*)data)->GetSupportedLanguagesSet();
    size_t totalSize = 0;

    for (auto const& itL : foundLanguages) {
        qcc::String candidate;
        if (totalSize != 0) {
            /* Not the first language in the array - prepend with the delimiter. */
            candidate = DELIMITER;
        }
        candidate += itL;
        if (totalSize + candidate.size() + 1 > maxLength) {
            /* No space for the candidate with the terminating NULL character - finish
             * without writing the candidate.
             */
            break;
        }
        strncpy(languageTags + totalSize, candidate.c_str(), candidate.size());
        totalSize += candidate.size();
    }
    if (!foundLanguages.empty()) {
        languageTags[totalSize] = '\0';
        totalSize++;
    }
    return totalSize;
}

extern AJ_API size_t AJ_CALL alljoyn_aboutdata_getsupportedlanguagescopylength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    const qcc::String DELIMITER = ",";
    std::set<qcc::String> foundLanguages = ((ajn::AboutData*)data)->GetSupportedLanguagesSet();
    size_t totalSize = 0;

    for (auto const& itL : foundLanguages) {
        if (totalSize != 0) {
            /* Not the first language in the array - prepend with the delimiter. */
            totalSize += DELIMITER.size();
        }
        totalSize += itL.size();
    }
    if (!foundLanguages.empty()) {
        totalSize++;
    }
    return totalSize;
}

QStatus AJ_CALL alljoyn_aboutdata_setdescription(alljoyn_aboutdata data,
                                                 const char* description,
                                                 const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetDescription(description, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getdescription(alljoyn_aboutdata data,
                                                 char** description,
                                                 const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetDescription(description, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getdescriptioncopy(alljoyn_aboutdata data,
                                                     char* description,
                                                     size_t maxLength,
                                                     const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDescription(retrieved, language);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(description, retrieved.c_str(), maxLength);
            description[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *description = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getdescriptionlength(alljoyn_aboutdata data,
                                                      const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDescription(retrieved, language);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setdateofmanufacture(alljoyn_aboutdata data,
                                                       const char* dateOfManufacture)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetDateOfManufacture(dateOfManufacture);
}

QStatus AJ_CALL alljoyn_aboutdata_getdateofmanufacture(alljoyn_aboutdata data,
                                                       char** dateOfManufacture)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetDateOfManufacture(dateOfManufacture);
}

QStatus AJ_CALL alljoyn_aboutdata_getdateofmanufacturecopy(alljoyn_aboutdata data,
                                                           char* dateOfManufacture,
                                                           size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDateOfManufacture(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(dateOfManufacture, retrieved.c_str(), maxLength);
            dateOfManufacture[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *dateOfManufacture = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getdateofmanufacturelength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetDateOfManufacture(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setsoftwareversion(alljoyn_aboutdata data,
                                                     const char* softwareVersion)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetSoftwareVersion(softwareVersion);
}

QStatus AJ_CALL alljoyn_aboutdata_getsoftwareversion(alljoyn_aboutdata data,
                                                     char** softwareVersion)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetSoftwareVersion(softwareVersion);
}

QStatus AJ_CALL alljoyn_aboutdata_getsoftwareversioncopy(alljoyn_aboutdata data,
                                                         char* softwareVersion,
                                                         size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetSoftwareVersion(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(softwareVersion, retrieved.c_str(), maxLength);
            softwareVersion[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *softwareVersion = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getsoftwareversionlength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetSoftwareVersion(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_getajsoftwareversion(alljoyn_aboutdata data,
                                                       char** ajSoftwareVersion)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetAJSoftwareVersion(ajSoftwareVersion);
}

QStatus AJ_CALL alljoyn_aboutdata_getajsoftwareversioncopy(alljoyn_aboutdata data,
                                                           char* ajSoftwareVersion,
                                                           size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetAJSoftwareVersion(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(ajSoftwareVersion, retrieved.c_str(), maxLength);
            ajSoftwareVersion[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *ajSoftwareVersion = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getajsoftwareversionlength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetAJSoftwareVersion(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_sethardwareversion(alljoyn_aboutdata data,
                                                     const char* hardwareVersion)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetHardwareVersion(hardwareVersion);
}

QStatus AJ_CALL alljoyn_aboutdata_gethardwareversion(alljoyn_aboutdata data,
                                                     char** hardwareVersion)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetHardwareVersion(hardwareVersion);
}

QStatus AJ_CALL alljoyn_aboutdata_gethardwareversioncopy(alljoyn_aboutdata data,
                                                         char* hardwareVersion,
                                                         size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetHardwareVersion(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(hardwareVersion, retrieved.c_str(), maxLength);
            hardwareVersion[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *hardwareVersion = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_gethardwareversionlength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetHardwareVersion(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setsupporturl(alljoyn_aboutdata data,
                                                const char* supportUrl)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetSupportUrl(supportUrl);
}

QStatus AJ_CALL alljoyn_aboutdata_getsupporturl(alljoyn_aboutdata data,
                                                char** supportUrl)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetSupportUrl(supportUrl);
}

QStatus AJ_CALL alljoyn_aboutdata_getsupporturlcopy(alljoyn_aboutdata data,
                                                    char* supportUrl,
                                                    size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetSupportUrl(retrieved);
    if (maxLength >= 1) {
        if (status == ER_OK) {
            strncpy(supportUrl, retrieved.c_str(), maxLength);
            supportUrl[maxLength - 1] = '\0';
        } else {
            // Return empty string.
            *supportUrl = '\0';
        }
    }

    return status;
}

size_t AJ_CALL alljoyn_aboutdata_getsupporturllength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String retrieved;
    QStatus status = ((ajn::AboutData*)data)->GetSupportUrl(retrieved);
    if (status == ER_OK) {
        return retrieved.size();
    } else {
        return 0;
    }
}

QStatus AJ_CALL alljoyn_aboutdata_setfield(alljoyn_aboutdata data,
                                           const char* name,
                                           alljoyn_msgarg value,
                                           const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->SetField(name, *(ajn::MsgArg*)value, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getfield(alljoyn_aboutdata data,
                                           const char* name,
                                           alljoyn_msgarg* value,
                                           const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetField(name, (ajn::MsgArg*&) *value, language);
}

size_t AJ_CALL alljoyn_aboutdata_getfields(alljoyn_aboutdata data,
                                           const char** fields,
                                           size_t num_fields)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetFields(fields, num_fields);
}

size_t AJ_CALL alljoyn_aboutdata_getfieldscopy(alljoyn_aboutdata data,
                                               char* fields,
                                               size_t maxLength)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    const qcc::String DELIMITER = ",";
    std::set<qcc::String> foundFields = ((ajn::AboutData*)data)->GetFieldsSet();
    size_t totalSize = 0;

    for (auto const& itF : foundFields) {
        qcc::String candidate;
        if (totalSize != 0) {
            /* Not the first field in the array - prepend with the delimiter. */
            candidate = DELIMITER;
        }
        candidate += itF;
        if (totalSize + candidate.size() + 1 > maxLength) {
            /* No space for the candidate with the terminating NULL character - finish
             * without writing the candidate.
             */
            break;
        }
        strncpy(fields + totalSize, candidate.c_str(), candidate.size());
        totalSize += candidate.size();
    }
    if (!foundFields.empty()) {
        fields[totalSize] = '\0';
        totalSize++;
    }
    return totalSize;
}

size_t AJ_CALL alljoyn_aboutdata_getfieldscopylength(alljoyn_aboutdata data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    const qcc::String DELIMITER = ",";
    std::set<qcc::String> foundFields = ((ajn::AboutData*)data)->GetFieldsSet();
    size_t totalSize = 0;

    for (auto const& itF : foundFields) {
        if (totalSize != 0) {
            /* Not the first field in the array - prepend with the delimiter. */
            totalSize += DELIMITER.size();
        }
        totalSize += itF.size();
    }
    if (!foundFields.empty()) {
        totalSize++;
    }
    return totalSize;
}

QStatus AJ_CALL alljoyn_aboutdata_getaboutdata(alljoyn_aboutdata data,
                                               alljoyn_msgarg msgArg,
                                               const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetAboutData((ajn::MsgArg*)msgArg, language);
}

QStatus AJ_CALL alljoyn_aboutdata_getannouncedaboutdata(alljoyn_aboutdata data,
                                                        alljoyn_msgarg msgArg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetAnnouncedAboutData((ajn::MsgArg*)msgArg);
}

bool AJ_CALL alljoyn_aboutdata_isfieldrequired(alljoyn_aboutdata data,
                                               const char* fieldName)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->IsFieldRequired(fieldName);
}

bool AJ_CALL alljoyn_aboutdata_isfieldannounced(alljoyn_aboutdata data,
                                                const char* fieldName)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->IsFieldAnnounced(fieldName);
}

bool AJ_CALL alljoyn_aboutdata_isfieldlocalized(alljoyn_aboutdata data,
                                                const char* fieldName)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->IsFieldLocalized(fieldName);
}

const char* AJ_CALL alljoyn_aboutdata_getfieldsignature(alljoyn_aboutdata data,
                                                        const char* fieldName)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetFieldSignature(fieldName);
}
