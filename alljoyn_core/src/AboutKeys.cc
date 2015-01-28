/******************************************************************************
 * Copyright (c) 2014-2015, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/AboutKeys.h>

#define QCC_MODULE "ALLJOYN_ABOUT"

namespace ajn {
const char* AboutKeys::APP_ID = "AppId";
const char* AboutKeys::DEFAULT_LANGUAGE = "DefaultLanguage";
const char* AboutKeys::DEVICE_NAME = "DeviceName";
const char* AboutKeys::DEVICE_ID = "DeviceId";
const char* AboutKeys::APP_NAME = "AppName";
const char* AboutKeys::MANUFACTURER = "Manufacturer";
const char* AboutKeys::MODEL_NUMBER = "ModelNumber";
const char* AboutKeys::SUPPORTED_LANGUAGES = "SupportedLanguages";
const char* AboutKeys::DESCRIPTION = "Description";
const char* AboutKeys::DATE_OF_MANUFACTURE = "DateOfManufacture";
const char* AboutKeys::SOFTWARE_VERSION = "SoftwareVersion";
const char* AboutKeys::AJ_SOFTWARE_VERSION = "AJSoftwareVersion";
const char* AboutKeys::HARDWARE_VERSION = "HardwareVersion";
const char* AboutKeys::SUPPORT_URL = "SupportUrl";

}
