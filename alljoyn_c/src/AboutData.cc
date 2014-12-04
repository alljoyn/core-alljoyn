/**
 * @file
 * This code is experimental, and as such has not been fully tested.
 * Please help make it more robust by contributing fixes if you find problems.
 */
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

#include <alljoyn_c/AboutData.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/version.h>
#include <qcc/Debug.h>

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
                                                const qcc::String& aboutDataXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->CreateFromXml((qcc::String const &)aboutDataXml);
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

QStatus AJ_CALL alljoyn_aboutdata_getajsoftwareversion(alljoyn_aboutdata data,
                                                       char** ajSoftwareVersion)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetAJSoftwareVersion(ajSoftwareVersion);
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
                                           alljoyn_msgarg value,
                                           const char* language)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetField(name, (ajn::MsgArg * &) * value, language);
}

size_t AJ_CALL alljoyn_aboutdata_getfields(alljoyn_aboutdata data,
                                           const char** fields,
                                           size_t num_fields)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutData*)data)->GetFields(fields, num_fields);
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