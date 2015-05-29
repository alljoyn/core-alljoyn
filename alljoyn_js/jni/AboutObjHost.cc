/*
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
 */
#include "AboutObjHost.h"
#include "CallbackNative.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>
#include <assert.h>

#define QCC_MODULE "ALLJOYN_JS"

_AboutObjHost::_AboutObjHost(Plugin& plugin, BusAttachment& busAttachment) :
    ScriptableObject(plugin),
    busAttachment(busAttachment),
    aboutObj(*busAttachment),
    aboutData(0) {
    QCC_DbgTrace(("%s %p", __FUNCTION__, this));

    OPERATION("announce", &_AboutObjHost::announce);
}

_AboutObjHost::~_AboutObjHost() {
    QCC_DbgTrace(("%s %p", __FUNCTION__, this));
    delete aboutData;
}

bool _AboutObjHost::announce(const NPVariant* args, uint32_t argCount, NPVariant* result) {
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    CallbackNative* callbackNative = NULL;
    NPVariant nplength = NPVARIANT_VOID;
    size_t lengthOfId = 0;
    size_t lengthOfLanguage = 0;
    size_t i;

    // mandatory
    ajn::SessionPort sessionPort = ajn::SESSION_PORT_ANY;
    uint8_t* AppId = NULL;
    qcc::String DefaultLanguage;
    qcc::String DeviceId;
    qcc::String AppName;
    qcc::String Manufacturer;
    qcc::String ModelNumber;
    qcc::String Description;
    qcc::String SoftwareVersion;
    qcc::String AJSoftwareVersion;

    // optional
    qcc::String DeviceName;
    qcc::String DateOfManufacture;
    qcc::String HardwareVersion;
    char** SupportedLanguages = NULL;
    qcc::String SupportUrl;

    /*
     * Pull out the parameters from the native object.
     */
    NPVariant variant;
    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    sessionPort = ToUnsignedShort(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a number");
        goto exit;
    }

    if (!NPVARIANT_IS_OBJECT(args[1])) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    /*
     * Mandatory parameters
     */
    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier(ajn::AboutData::APP_ID),
                    &variant);

    if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetStringIdentifier("length"), &nplength)
        || !(NPVARIANT_IS_INT32(nplength) || NPVARIANT_IS_DOUBLE(nplength))) {
        plugin->RaiseTypeError("arg[1]'s AppId is not an array");
        typeError = true;
        goto exit;
    }
    lengthOfId = ToLong(plugin, nplength, typeError);
    AppId = new uint8_t[lengthOfId];

    for (i = 0; i < lengthOfId; ++i) {
        NPVariant npelem = NPVARIANT_VOID;
        if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &npelem)) {
            plugin->RaiseTypeError("get array element failed");
            typeError = true;
            goto exit;
        }
        AppId[i] = ToOctet(plugin, npelem, typeError);
        NPN_ReleaseVariantValue(&npelem);
        if (typeError) {
            plugin->RaiseTypeError("array element is not a number");
            goto exit;
        }
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]),
                    NPN_GetStringIdentifier(ajn::AboutData::DEFAULT_LANGUAGE), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        DefaultLanguage = ToDOMString(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || DefaultLanguage.empty()) {
        typeError = true;
        plugin->RaiseTypeError("property 'DefaultLanguage' of argument 1 is undefined");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier(ajn::AboutData::DEVICE_ID),
                    &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        DeviceId = ToDOMString(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || DeviceId.empty()) {
        typeError = true;
        plugin->RaiseTypeError("property 'DeviceId' of argument 1 is undefined");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier(ajn::AboutData::APP_NAME),
                    &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        AppName = ToDOMString(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || AppName.empty()) {
        typeError = true;
        plugin->RaiseTypeError("property 'AppName' of argument 1 is undefined");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier(ajn::AboutData::MANUFACTURER),
                    &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        Manufacturer = ToDOMString(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || Manufacturer.empty()) {
        typeError = true;
        plugin->RaiseTypeError("property 'Manufacturer' of argument 1 is undefined");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier(ajn::AboutData::MODEL_NUMBER),
                    &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        ModelNumber = ToDOMString(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || ModelNumber.empty()) {
        typeError = true;
        plugin->RaiseTypeError("property 'ModelNumber' of argument 1 is undefined");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier(ajn::AboutData::DESCRIPTION),
                    &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        Description = ToDOMString(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || Description.empty()) {
        typeError = true;
        plugin->RaiseTypeError("property 'Description' of argument 1 is undefined");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]),
                    NPN_GetStringIdentifier(ajn::AboutData::SOFTWARE_VERSION), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        SoftwareVersion = ToDOMString(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || SoftwareVersion.empty()) {
        typeError = true;
        plugin->RaiseTypeError("property 'SoftwareVersion' of argument 1 is undefined");
        goto exit;
    }

    /*
     * Optional parameters
     */
    VOID_TO_NPVARIANT(variant);
    if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]),
                        NPN_GetStringIdentifier(ajn::AboutData::SUPPORTED_LANGUAGES), &variant)) {
        if (!NPVARIANT_IS_VOID(variant)) {
            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetStringIdentifier("length"),
                                 &nplength)) {
                plugin->RaiseTypeError("property 'SupportedLanguages' is not an array");
                typeError = true;
                goto exit;
            }
            lengthOfLanguage = ToLong(plugin, nplength, typeError);
            SupportedLanguages = new char*[lengthOfLanguage];

            for (i = 0; i < lengthOfLanguage; ++i) {
                SupportedLanguages[i] = NULL;
                NPVariant curValue;
                NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &curValue);
                if (!NPVARIANT_IS_VOID(curValue)) {
                    int32_t utf8length = NPVARIANT_TO_STRING(curValue).UTF8Length;
                    SupportedLanguages[i] = new char[utf8length + 1];
                    strncpy(SupportedLanguages[i], NPVARIANT_TO_STRING(curValue).UTF8Characters, utf8length);
                    SupportedLanguages[i][utf8length] = 0;
                }
                NPN_ReleaseVariantValue(&curValue);
            }
        }
        NPN_ReleaseVariantValue(&variant);
    }

    VOID_TO_NPVARIANT(variant);
    if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier(ajn::AboutData::DEVICE_NAME),
                        &variant)) {
        if (!NPVARIANT_IS_VOID(variant)) {
            DeviceName = ToDOMString(plugin, variant, typeError);
        }

        NPN_ReleaseVariantValue(&variant);
        if (typeError) {
            plugin->RaiseTypeError("'DeviceName' is not a string");
            goto exit;
        }
    }

    VOID_TO_NPVARIANT(variant);
    if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]),
                        NPN_GetStringIdentifier(ajn::AboutData::DATE_OF_MANUFACTURE), &variant)) {
        if (!NPVARIANT_IS_VOID(variant)) {
            DateOfManufacture = ToDOMString(plugin, variant, typeError);
        }

        NPN_ReleaseVariantValue(&variant);
        if (typeError) {
            plugin->RaiseTypeError("'DateOfManufacture' is not a string");
            goto exit;
        }
    }

    VOID_TO_NPVARIANT(variant);
    if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]),
                        NPN_GetStringIdentifier(ajn::AboutData::HARDWARE_VERSION), &variant)) {
        if (!NPVARIANT_IS_VOID(variant)) {
            HardwareVersion = ToDOMString(plugin, variant, typeError);
        }

        NPN_ReleaseVariantValue(&variant);
        if (typeError) {
            plugin->RaiseTypeError("'HardwareVersion' is not a string");
            goto exit;
        }
    }

    VOID_TO_NPVARIANT(variant);
    if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier(ajn::AboutData::SUPPORT_URL),
                        &variant)) {
        if (!NPVARIANT_IS_VOID(variant)) {
            SupportUrl = ToDOMString(plugin, variant, typeError);
        }

        NPN_ReleaseVariantValue(&variant);
        if (typeError) {
            plugin->RaiseTypeError("'SupportUrl' is not a string");
            goto exit;
        }
    }

    /*
     * TODO
     * localized is not supported.
     */
    aboutData = new ajn::AboutData();
    status = aboutData->SetDefaultLanguage(DefaultLanguage.c_str());
    if (ER_OK != status) {
        plugin->RaiseTypeError("set default language fail");
        goto exit;
    }
    status = aboutData->SetAppId(AppId, lengthOfId);
    if (ER_OK != status) {
        plugin->RaiseTypeError("set AppId fail");
        goto exit;
    }
    status = aboutData->SetDeviceId(DeviceId.c_str());
    if (ER_OK != status) {
        plugin->RaiseTypeError("set device id fail");
        goto exit;
    }
    status = aboutData->SetAppName(AppName.c_str());
    if (ER_OK != status) {
        plugin->RaiseTypeError("set app name fail");
        goto exit;
    }
    status = aboutData->SetManufacturer(Manufacturer.c_str());
    if (ER_OK != status) {
        plugin->RaiseTypeError("set manufacturer fail");
        goto exit;
    }
    status = aboutData->SetModelNumber(ModelNumber.c_str());
    if (ER_OK != status) {
        plugin->RaiseTypeError("set model number fail");
        goto exit;
    }
    //aboutData->SetSupportedLanguage(SupportedLanguages.c_str());
    status = aboutData->SetDescription(Description.c_str());
    if (ER_OK != status) {
        plugin->RaiseTypeError("set description fail");
        goto exit;
    }
    status = aboutData->SetSoftwareVersion(SoftwareVersion.c_str());
    if (ER_OK != status) {
        plugin->RaiseTypeError("set software version fail");
        goto exit;
    }

    /*
     * optional
     */
    if (!DeviceName.empty()) {
        status = aboutData->SetDeviceName(DeviceName.c_str());
        if (ER_OK != status) {
            plugin->RaiseTypeError("set device name fail");
            goto exit;
        }
    }
    if (!DateOfManufacture.empty()) {
        status = aboutData->SetDateOfManufacture(DateOfManufacture.c_str());
        if (ER_OK != status) {
            plugin->RaiseTypeError("set date of manufacture fail");
            goto exit;
        }
    }
    if (!HardwareVersion.empty()) {
        status = aboutData->SetHardwareVersion(HardwareVersion.c_str());
        if (ER_OK != status) {
            plugin->RaiseTypeError("set hardware version fail");
            goto exit;
        }
    }
    if (!SupportUrl.empty()) {
        status = aboutData->SetSupportUrl(SupportUrl.c_str());
        if (ER_OK != status) {
            plugin->RaiseTypeError("set support url fail");
            goto exit;
        }
    }

    if (!aboutData->IsValid()) {
        delete aboutData;
        aboutData = 0;
        plugin->RaiseTypeError("about data is invalid");
        goto exit;
    }

    status = aboutObj->Announce(sessionPort, *aboutData);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    delete[] AppId;
    for (i = 0; i < lengthOfLanguage; ++i)
        delete[] SupportedLanguages[i];
    delete[] SupportedLanguages;
    NPN_ReleaseVariantValue(&nplength);
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

