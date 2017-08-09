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

#include <gtest/gtest.h>
#include <alljoyn_c/AboutData.h>
#include <alljoyn_c/InterfaceDescription.h>
#include <alljoyn_c/AjAPI.h>

#include <alljoyn/AboutData.h>
#include <alljoyn/version.h>

#include <qcc/Thread.h>
#include <qcc/Util.h>
#include "ajTestCommon.h"

using namespace ajn;

TEST(AboutDataTest, constants) {
    EXPECT_STREQ("AppId", AboutData::APP_ID);
    EXPECT_STREQ("DefaultLanguage", AboutData::DEFAULT_LANGUAGE);
    EXPECT_STREQ("DeviceName", AboutData::DEVICE_NAME);
    EXPECT_STREQ("DeviceId", AboutData::DEVICE_ID);
    EXPECT_STREQ("AppName", AboutData::APP_NAME);
    EXPECT_STREQ("Manufacturer", AboutData::MANUFACTURER);
    EXPECT_STREQ("ModelNumber", AboutData::MODEL_NUMBER);
    EXPECT_STREQ("SupportedLanguages", AboutData::SUPPORTED_LANGUAGES);
    EXPECT_STREQ("Description", AboutData::DESCRIPTION);
    EXPECT_STREQ("DateOfManufacture", AboutData::DATE_OF_MANUFACTURE);
    EXPECT_STREQ("SoftwareVersion", AboutData::SOFTWARE_VERSION);
    EXPECT_STREQ("AJSoftwareVersion", AboutData::AJ_SOFTWARE_VERSION);
    EXPECT_STREQ("HardwareVersion", AboutData::HARDWARE_VERSION);
    EXPECT_STREQ("SupportUrl", AboutData::SUPPORT_URL);
}

TEST(AboutDataTest, VerifyFieldValues) {
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::APP_ID));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::APP_ID));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::APP_ID));
    EXPECT_STREQ("ay", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::APP_ID));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::DEFAULT_LANGUAGE));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::DEFAULT_LANGUAGE));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::DEFAULT_LANGUAGE));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::DEFAULT_LANGUAGE));

    EXPECT_FALSE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::DEVICE_NAME));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::DEVICE_NAME));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::DEVICE_NAME));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::DEVICE_NAME));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::DEVICE_ID));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::DEVICE_ID));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::DEVICE_ID));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::DEVICE_ID));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::APP_NAME));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::APP_NAME));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::APP_NAME));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::APP_NAME));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::MANUFACTURER));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::MANUFACTURER));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::MANUFACTURER));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::MANUFACTURER));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::MODEL_NUMBER));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::MODEL_NUMBER));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::MODEL_NUMBER));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::MODEL_NUMBER));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::SUPPORTED_LANGUAGES));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::SUPPORTED_LANGUAGES));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::SUPPORTED_LANGUAGES));
    EXPECT_STREQ("as", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::SUPPORTED_LANGUAGES));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::DESCRIPTION));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::DESCRIPTION));
    EXPECT_TRUE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::DESCRIPTION));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::DESCRIPTION));

    EXPECT_FALSE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::DATE_OF_MANUFACTURE));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::DATE_OF_MANUFACTURE));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::DATE_OF_MANUFACTURE));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::DATE_OF_MANUFACTURE));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::SOFTWARE_VERSION));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::SOFTWARE_VERSION));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::SOFTWARE_VERSION));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::SOFTWARE_VERSION));

    EXPECT_TRUE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::AJ_SOFTWARE_VERSION));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::AJ_SOFTWARE_VERSION));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::AJ_SOFTWARE_VERSION));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::AJ_SOFTWARE_VERSION));

    EXPECT_FALSE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::HARDWARE_VERSION));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::HARDWARE_VERSION));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::HARDWARE_VERSION));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::HARDWARE_VERSION));

    EXPECT_FALSE(alljoyn_aboutdata_isfieldrequired(aboutData, AboutData::SUPPORT_URL));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldannounced(aboutData, AboutData::SUPPORT_URL));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, AboutData::SUPPORT_URL));
    EXPECT_STREQ("s", alljoyn_aboutdata_getfieldsignature(aboutData, AboutData::SUPPORT_URL));

    EXPECT_FALSE(alljoyn_aboutdata_isfieldrequired(aboutData, "Unknown"));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldannounced(aboutData, "Unknown"));
    EXPECT_FALSE(alljoyn_aboutdata_isfieldlocalized(aboutData, "Unknown"));
    EXPECT_TRUE(NULL == alljoyn_aboutdata_getfieldsignature(aboutData, "Unknown"));

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, DefaultLanguageNotSpecified) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create_empty();
    status = alljoyn_aboutdata_setdevicename(aboutData, "Device Name", NULL);
    EXPECT_EQ(ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED, status) << "  Actual Status: "
                                                               << QCC_StatusText(status);

    status = alljoyn_aboutdata_setappname(aboutData, "Application Name", NULL);
    EXPECT_EQ(ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED, status) << "  Actual Status: "
                                                               << QCC_StatusText(status);

    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer Name", NULL);
    EXPECT_EQ(ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED, status) << "  Actual Status: "
                                                               << QCC_StatusText(status);

    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A description of the application.",
                                              NULL);
    EXPECT_EQ(ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED, status) << "  Actual Status: "
                                                               << QCC_StatusText(status);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, Constructor) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    char* language;
    status = alljoyn_aboutdata_getdefaultlanguage(aboutData, &language);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", language);
    char* ajSoftwareVersion;
    status = alljoyn_aboutdata_getajsoftwareversion(aboutData, &ajSoftwareVersion);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(ajn::GetVersion(), ajSoftwareVersion);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetAppId) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, originalAppId, 16u);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint8_t* appId;
    size_t num;
    status = alljoyn_aboutdata_getappid(aboutData, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(originalAppId[i], appId[i]);
    }

    size_t length = 17;
    char appIdCopy[length];
    EXPECT_EQ(length - 1, alljoyn_aboutdata_getappidlength(aboutData));
    status = alljoyn_aboutdata_getappidcopy(aboutData, appIdCopy, length);
    for (size_t i = 0; i < length - 1; i++) {
        EXPECT_EQ(i, appIdCopy[i]);
    }

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetAppId_using_uuid_string) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    /* Not a hex digit */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData,
                                                   "g00102030405060708090a0b0c0d0e0f");
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE, status) << "  Actual Status: "
                                                              << QCC_StatusText(status);

    /* Odd number of characters parsing error */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData,
                                                   "00102030405060708090a0b0c0d0e0f");
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE, status) << "  Actual Status: "
                                                              << QCC_StatusText(status);

    /* Too few characters */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData,
                                                   "0102030405060708090a0b0c0d0e0f");
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status) << "  Actual Status: "
                                                                   << QCC_StatusText(status);

    /* Too many characters */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData,
                                                   "000102030405060708090a0b0c0d0e0f10");
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, status) << "  Actual Status: "
                                                                   << QCC_StatusText(status);

    /* Not valid uuid parsing error */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData,
                                                   "000102030405-060708090A0B-0C0D0E0F10");
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE, status) << "  Actual Status: "
                                                              << QCC_StatusText(status);

    /* Not valid uuid parsing error */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData,
                                                   "00010203-04050607-0809-0A0B-0C0D0E0F");
    EXPECT_EQ(ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE, status) << "  Actual Status: "
                                                              << QCC_StatusText(status);

    status = alljoyn_aboutdata_setappid_fromstring(aboutData,
                                                   "000102030405060708090a0b0c0d0e0f");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint8_t* appId;
    size_t num;
    status = alljoyn_aboutdata_getappid(aboutData, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(i, appId[i]);
    }

    size_t length = 17;
    char appIdCopy[length];
    EXPECT_EQ(length - 1, alljoyn_aboutdata_getappidlength(aboutData));
    status = alljoyn_aboutdata_getappidcopy(aboutData, appIdCopy, length);
    for (size_t i = 0; i < length - 1; i++) {
        EXPECT_EQ(i, appIdCopy[i]);
    }

    alljoyn_aboutdata aboutData2 = alljoyn_aboutdata_create("en");

    /* Use capital hex digits */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData2,
                                                   "000102030405060708090A0B0C0D0E0F");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_getappid(aboutData2, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(i, appId[i]);
    }
    EXPECT_EQ(length - 1, alljoyn_aboutdata_getappidlength(aboutData2));
    status = alljoyn_aboutdata_getappidcopy(aboutData2, appIdCopy, length);
    for (size_t i = 0; i < length - 1; i++) {
        EXPECT_EQ(i, appIdCopy[i]);
    }

    alljoyn_aboutdata aboutData3 = alljoyn_aboutdata_create("en");

    /* Use capital hex digits UUID as per RFC 4122 */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData3,
                                                   "00010203-0405-0607-0809-0A0B0C0D0E0F");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_getappid(aboutData3, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(i, appId[i]);
    }
    EXPECT_EQ(length - 1, alljoyn_aboutdata_getappidlength(aboutData3));
    status = alljoyn_aboutdata_getappidcopy(aboutData3, appIdCopy, length);
    for (size_t i = 0; i < length - 1; i++) {
        EXPECT_EQ(i, appIdCopy[i]);
    }

    alljoyn_aboutdata aboutData4 = alljoyn_aboutdata_create("en");

    /* Use lowercase hex digits UUID as per RFC 4122 */
    status = alljoyn_aboutdata_setappid_fromstring(aboutData4,
                                                   "00010203-0405-0607-0809-0a0b0c0d0e0f");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_getappid(aboutData4, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(i, appId[i]);
    }
    EXPECT_EQ(length - 1, alljoyn_aboutdata_getappidlength(aboutData4));
    status = alljoyn_aboutdata_getappidcopy(aboutData4, appIdCopy, length);
    for (size_t i = 0; i < length - 1; i++) {
        EXPECT_EQ(i, appIdCopy[i]);
    }
    alljoyn_aboutdata_destroy(aboutData);
    alljoyn_aboutdata_destroy(aboutData2);
    alljoyn_aboutdata_destroy(aboutData3);
    alljoyn_aboutdata_destroy(aboutData4);
}

TEST(AboutDataTest, SetDeviceName) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    char* language;
    status = alljoyn_aboutdata_getdefaultlanguage(aboutData, &language);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", language);
    char* ajSoftwareVersion;
    size_t length = 20;
    char ajSoftwareVersionCopy[length];
    status = alljoyn_aboutdata_getajsoftwareversion(aboutData, &ajSoftwareVersion);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(ajn::GetVersion(), ajSoftwareVersion);
    status = alljoyn_aboutdata_getajsoftwareversioncopy(aboutData, ajSoftwareVersionCopy, length);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(ajn::GetVersion(), ajSoftwareVersionCopy);

    const char englishDeviceName[] = "Device";
    status = alljoyn_aboutdata_setdevicename(aboutData, englishDeviceName, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* retrievedDeviceName;
    char retrievedDeviceNameCopy[length];
    status = alljoyn_aboutdata_getdevicename(aboutData, &retrievedDeviceName, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(englishDeviceName, retrievedDeviceName);
    EXPECT_EQ((sizeof(englishDeviceName) - 1), alljoyn_aboutdata_getdevicenamelength(aboutData, "en"));
    status = alljoyn_aboutdata_getdevicenamecopy(aboutData, retrievedDeviceNameCopy, length, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(englishDeviceName, retrievedDeviceNameCopy);

    const char spanishDeviceName[] = "dispositivo";
    status = alljoyn_aboutdata_setdevicename(aboutData, spanishDeviceName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_getdevicename(aboutData, &retrievedDeviceName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spanishDeviceName, retrievedDeviceName);
    EXPECT_EQ((sizeof(spanishDeviceName) - 1), alljoyn_aboutdata_getdevicenamelength(aboutData, "es"));
    status = alljoyn_aboutdata_getdevicenamecopy(aboutData, retrievedDeviceNameCopy, length, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spanishDeviceName, retrievedDeviceNameCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetDeviceId) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    const char deviceId[] = "avec-awe1213-1234559xvc123";
    status = alljoyn_aboutdata_setdeviceid(aboutData, deviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* retrievedDeviceId;
    size_t length = 30;
    char retrievedDeviceIdCopy[length];
    status = alljoyn_aboutdata_getdeviceid(aboutData, &retrievedDeviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(deviceId, retrievedDeviceId);
    EXPECT_EQ((sizeof(deviceId) - 1), alljoyn_aboutdata_getdeviceidlength(aboutData));
    status = alljoyn_aboutdata_getdeviceidcopy(aboutData, retrievedDeviceIdCopy, length);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(deviceId, retrievedDeviceIdCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetAppName) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char englishAppName[] = "Application";
    char* retrievedAppName;
    size_t length = 20;
    char retrievedAppNameCopy[length];
    status = alljoyn_aboutdata_getappname(aboutData, &retrievedAppName, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(englishAppName, retrievedAppName);
    EXPECT_EQ((sizeof(englishAppName) - 1), alljoyn_aboutdata_getappnamelength(aboutData, "en"));
    status = alljoyn_aboutdata_getappnamecopy(aboutData, retrievedAppNameCopy, length, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(englishAppName, retrievedAppNameCopy);

    const char spanishAppName[] = "aplicacion";
    status = alljoyn_aboutdata_setappname(aboutData, spanishAppName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_getappname(aboutData, &retrievedAppName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spanishAppName, retrievedAppName);
    EXPECT_EQ((sizeof(spanishAppName) - 1), alljoyn_aboutdata_getappnamelength(aboutData, "es"));
    status = alljoyn_aboutdata_getappnamecopy(aboutData, retrievedAppNameCopy, length, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spanishAppName, retrievedAppNameCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetManufacturer) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    const char englishManufacturer[] = "Manufacturer";
    char* retrievedManufacturer;
    size_t length = 20;
    char retrievedManufacturerCopy[length];
    status = alljoyn_aboutdata_getmanufacturer(aboutData, &retrievedManufacturer, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(englishManufacturer, retrievedManufacturer);
    EXPECT_EQ((sizeof(englishManufacturer) - 1), alljoyn_aboutdata_getmanufacturerlength(aboutData, "en"));
    status = alljoyn_aboutdata_getmanufacturercopy(aboutData, retrievedManufacturerCopy, length, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(englishManufacturer, retrievedManufacturerCopy);

    const char spanishManufacturer[] = "manufactura";
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_getmanufacturer(aboutData, &retrievedManufacturer, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spanishManufacturer, retrievedManufacturer);
    EXPECT_EQ((sizeof(spanishManufacturer) - 1), alljoyn_aboutdata_getmanufacturerlength(aboutData, "es"));
    status = alljoyn_aboutdata_getmanufacturercopy(aboutData, retrievedManufacturerCopy, length, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spanishManufacturer, retrievedManufacturerCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetModelNumber) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    const char modelNumber[] = "xBnc345";
    status = alljoyn_aboutdata_setmodelnumber(aboutData, modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* retrievedModelNumber;
    size_t length = 10;
    char retrievedModelNumberCopy[length];
    status = alljoyn_aboutdata_getmodelnumber(aboutData, &retrievedModelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(modelNumber, retrievedModelNumber);
    EXPECT_EQ((sizeof(modelNumber) - 1), alljoyn_aboutdata_getmodelnumberlength(aboutData));
    status = alljoyn_aboutdata_getmodelnumbercopy(aboutData, retrievedModelNumberCopy, length);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(modelNumber, retrievedModelNumberCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetSupportedLanguage) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    size_t numLanguages =
        alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    const char** languages = new const char*[numLanguages];

    size_t numRetLang =
        alljoyn_aboutdata_getsupportedlanguages(aboutData,
                                                languages,
                                                numLanguages);
    EXPECT_EQ(numLanguages, numRetLang);
    EXPECT_EQ(1u, numLanguages);
    EXPECT_STREQ("en", languages[0]);

    delete [] languages;
    languages = NULL;

    size_t copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    ASSERT_EQ(3u, copySize);
    char* languagesCopy = new char[copySize];
    copySize = alljoyn_aboutdata_getsupportedlanguagescopy(aboutData, languagesCopy, copySize);
    EXPECT_EQ(3u, copySize);
    EXPECT_STREQ("en", languagesCopy);

    delete [] languagesCopy;
    languagesCopy = NULL;

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    numLanguages = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    languages = new const char*[numLanguages];

    numRetLang =
        alljoyn_aboutdata_getsupportedlanguages(aboutData,
                                                languages,
                                                numLanguages);
    EXPECT_EQ(numLanguages, numRetLang);
    EXPECT_EQ(2u, numLanguages);
    EXPECT_STREQ("en", languages[0]);
    EXPECT_STREQ("es", languages[1]);
    delete [] languages;
    languages = NULL;

    copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    ASSERT_EQ(6u, copySize);
    languagesCopy = new char[copySize];
    copySize = alljoyn_aboutdata_getsupportedlanguagescopy(aboutData, languagesCopy, copySize);
    EXPECT_EQ(6u, copySize);
    EXPECT_STREQ("en,es", languagesCopy);

    delete [] languagesCopy;
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetSupportedLanguage_Duplicate) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Duplicate language already added from constructor */
    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Duplicate language already added, error status should be generated */
    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /*
     * Even though "en" and "es" languages have been added multiple times only
     * two languages should be reported in the list of SupportedLanguages.
     */
    size_t numRetLang = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(2u, numRetLang);

    size_t copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(6u, copySize);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, DISABLED_SetSupportedLanguage_Invalid_Tag) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    /* Invalid language tag not defined in RFC5646 */
    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "abc");
    EXPECT_NE(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "232");
    EXPECT_NE(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Invalid subtag not defined in RFC5646 */
    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "en-t324");
    EXPECT_NE(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "zh-gfjk");
    EXPECT_NE(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    size_t numRetLang =
        alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(1u, numRetLang);
    size_t copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(3u, copySize);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, GetSupportedLanguages) {
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    size_t numLanguages;
    numLanguages =
        alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(1u, numLanguages);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetDescription) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    const char englishDescription[] = "A poetic description of this application";
    status = alljoyn_aboutdata_setdescription(aboutData, englishDescription, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* retrievedDescription;
    size_t length = 50;
    char retrievedDescriptionCopy[length];
    status = alljoyn_aboutdata_getdescription(aboutData, &retrievedDescription, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(englishDescription, retrievedDescription);
    EXPECT_EQ((sizeof(englishDescription) - 1), alljoyn_aboutdata_getdescriptionlength(aboutData, "en"));
    status = alljoyn_aboutdata_getdescriptioncopy(aboutData, retrievedDescriptionCopy, length, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(englishDescription, retrievedDescriptionCopy);

    const char spanishDescription[] = "Una descripcion poetica de esta aplicacion";
    status = alljoyn_aboutdata_setdescription(aboutData, spanishDescription, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_getdescription(aboutData, &retrievedDescription, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spanishDescription, retrievedDescription);
    EXPECT_EQ((sizeof(spanishDescription) - 1), alljoyn_aboutdata_getdescriptionlength(aboutData, "es"));
    status = alljoyn_aboutdata_getdescriptioncopy(aboutData, retrievedDescriptionCopy, length, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(spanishDescription, retrievedDescriptionCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetDateOfManufacture) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    const char dateOfManufacture[] = "2014-01-20";
    status = alljoyn_aboutdata_setdateofmanufacture(aboutData, dateOfManufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* retrievedDateOfManufacture;
    size_t length = 15;
    char retrievedDateOfManufactureCopy[length];
    status = alljoyn_aboutdata_getdateofmanufacture(aboutData, &retrievedDateOfManufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(dateOfManufacture, retrievedDateOfManufacture);
    EXPECT_EQ((sizeof(dateOfManufacture) - 1), alljoyn_aboutdata_getdateofmanufacturelength(aboutData));
    status = alljoyn_aboutdata_getdateofmanufacturecopy(aboutData, retrievedDateOfManufactureCopy, length);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(dateOfManufacture, retrievedDateOfManufactureCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, DISABLED_SetDateOfManufacture_Negative) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    /* Invalid date should fail */
    status = alljoyn_aboutdata_setdateofmanufacture(aboutData, "2014-41-20");
    EXPECT_NE(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setdateofmanufacture(aboutData, "201a-02-20");
    EXPECT_NE(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setdateofmanufacture(aboutData, "2013-02-29");
    EXPECT_NE(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setdateofmanufacture(aboutData, "04/31/2014");
    EXPECT_NE(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetSoftwareVersion) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    const char softwareVersion[] = "0.1.2";
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, softwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* retrievedSoftwareVersion;
    size_t length = 10;
    char retrievedSoftwareVersionCopy[length];
    status = alljoyn_aboutdata_getsoftwareversion(aboutData, &retrievedSoftwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(softwareVersion, softwareVersion);
    EXPECT_EQ((sizeof(softwareVersion) - 1), alljoyn_aboutdata_getsoftwareversionlength(aboutData));
    status = alljoyn_aboutdata_getsoftwareversioncopy(aboutData, retrievedSoftwareVersionCopy, length);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(softwareVersion, retrievedSoftwareVersionCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetHardwareVersion) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    const char hardwareVersion[] = "3.2.1";
    status = alljoyn_aboutdata_sethardwareversion(aboutData, hardwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* retrievedHardwareVersion;
    size_t length = 10;
    char retrievedHardwareVersionCopy[length];
    status = alljoyn_aboutdata_gethardwareversion(aboutData, &retrievedHardwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(retrievedHardwareVersion, hardwareVersion);
    EXPECT_EQ((sizeof(hardwareVersion) - 1), alljoyn_aboutdata_gethardwareversionlength(aboutData));
    status = alljoyn_aboutdata_gethardwareversioncopy(aboutData, retrievedHardwareVersionCopy, length);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(hardwareVersion, retrievedHardwareVersionCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetSupportUrl) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    const char supportUrl[] = "www.example.com";
    status = alljoyn_aboutdata_setsupporturl(aboutData, supportUrl);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* retrievedSupportUrl;
    size_t length = 20;
    char retrievedSupportUrlCopy[length];
    status = alljoyn_aboutdata_getsupporturl(aboutData, &retrievedSupportUrl);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(supportUrl, retrievedSupportUrl);
    EXPECT_EQ((sizeof(supportUrl) - 1), alljoyn_aboutdata_getsupporturllength(aboutData));
    status = alljoyn_aboutdata_getsupporturlcopy(aboutData, retrievedSupportUrlCopy, length);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(supportUrl, retrievedSupportUrlCopy);

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, IsValid) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));
    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, appId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdeviceid(aboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A poetic description of this application",
                                              "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "es"));

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "Una descripcion poetica de esta aplicacion",
                                              "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "es"));
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, IsValid_Negative) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    /* DefaultLanguage and other required fields are missing */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, appId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* DeviceId and other required fields are missing */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setdeviceid(aboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* AppName and other required fields are missing */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Manufacturer and other required fields are missing */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* ModelNumber and other required fields are missing */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /*Description and other required fields are missing */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A poetic description of this application",
                                              "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* SoftwareVersion missing */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Now all required fields are set for English */
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Missing AppName/Manufacture/Description */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "es"));

    status = alljoyn_aboutdata_setappname(aboutData, "aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Missing Manufacture/Description */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "es"));

    status = alljoyn_aboutdata_setmanufacturer(aboutData, "manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Missing Description */
    EXPECT_FALSE(alljoyn_aboutdata_isvalid(aboutData, "es"));

    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "Una descripcion poetica de esta aplicacion",
                                              "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "es"));
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, GetAboutData) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, appId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdeviceid(aboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A poetic description of this application",
                                              "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "Una descripcion poetica de esta aplicacion",
                                              "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "es"));

    alljoyn_msgarg aboutArg = alljoyn_msgarg_create();
    status = alljoyn_aboutdata_getaboutdata(aboutData, aboutArg, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg args;

    status = alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                           AboutData::APP_ID, &args);
    int8_t* appIdOut;
    size_t appIdNum;
    alljoyn_msgarg_get(args, "ay", &appIdNum, &appIdOut);
    ASSERT_EQ(16u, appIdNum);
    for (size_t i = 0; i < appIdNum; ++i) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::DEFAULT_LANGUAGE, &args);
    char* defaultLanguage;
    alljoyn_msgarg_get(args, "s", &defaultLanguage);
    EXPECT_STREQ("en", defaultLanguage);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::DEVICE_ID, &args);
    char* deviceId;
    alljoyn_msgarg_get(args, "s", &deviceId);
    EXPECT_STREQ("fakeID", deviceId);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::APP_NAME, &args);
    char* appName;
    alljoyn_msgarg_get(args, "s", &appName);
    EXPECT_STREQ("Application", appName);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::MANUFACTURER, &args);
    char* manufacturer;
    alljoyn_msgarg_get(args, "s", &manufacturer);
    EXPECT_STREQ("Manufacturer", manufacturer);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::MODEL_NUMBER, &args);
    char* modelNumber;
    alljoyn_msgarg_get(args, "s", &modelNumber);
    EXPECT_STREQ("123456", modelNumber);

    alljoyn_msgarg_destroy(aboutArg);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, GetMsgArg_es_language) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, appId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdeviceid(aboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A poetic description of this application",
                                              "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "Una descripcion poetica de esta aplicacion",
                                              "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "es"));

    alljoyn_msgarg aboutArg = alljoyn_msgarg_create();
    alljoyn_msgarg args;
    status = alljoyn_aboutdata_getaboutdata(aboutData, aboutArg, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::APP_ID, &args);
    int8_t* appIdOut;
    size_t appIdNum;
    alljoyn_msgarg_get(args, "ay", &appIdNum, &appIdOut);
    ASSERT_EQ(16u, appIdNum);
    for (size_t i = 0; i < appIdNum; ++i) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::DEFAULT_LANGUAGE, &args);
    char* defaultLanguage;
    alljoyn_msgarg_get(args, "s", &defaultLanguage);
    EXPECT_STREQ("en", defaultLanguage);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::DEVICE_ID, &args);
    char* deviceId;
    alljoyn_msgarg_get(args, "s", &deviceId);
    EXPECT_STREQ("fakeID", deviceId);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::APP_NAME, &args);
    char* appName;
    alljoyn_msgarg_get(args, "s", &appName);
    EXPECT_STREQ("aplicacion", appName);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::MANUFACTURER, &args);
    char* manufacturer;
    alljoyn_msgarg_get(args, "s", &manufacturer);
    EXPECT_STREQ("manufactura", manufacturer);

    alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                  AboutData::MODEL_NUMBER, &args);
    char* modelNumber;
    alljoyn_msgarg_get(args, "s", &modelNumber);
    EXPECT_STREQ("123456", modelNumber);
    alljoyn_msgarg_destroy(aboutArg);
    alljoyn_aboutdata_destroy(aboutData);
}

void VerifyAppName(alljoyn_aboutdata aboutData, const char* language, const char* expectedAppName)
{
    alljoyn_msgarg aboutArg = alljoyn_msgarg_create();
    QStatus status = alljoyn_aboutdata_getaboutdata(aboutData, aboutArg, language);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg args;
    alljoyn_msgarg_getdictelement(aboutArg, "{sv}", AboutData::APP_NAME, &args);
    char* appName;
    alljoyn_msgarg_get(args, "s", &appName);
    EXPECT_STREQ(expectedAppName, appName);

    alljoyn_msgarg_destroy(aboutArg);
}

TEST(AboutDataTest, GetMsgArg_best_language) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, appId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdeviceid(aboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setappname(aboutData, "en appName", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "en manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData, "en description", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "de-CH");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "de-CH appName", "de-CH");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "de-CH manufacturer", "de-CH");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData, "de-CH description", "de-CH");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "de-CH"));

    // Test requesting languages that resolve to the language that happens
    // to be the default language.
    VerifyAppName(aboutData, "EN", "en appName");
    VerifyAppName(aboutData, "EN-US", "en appName");
    VerifyAppName(aboutData, "en-a-bbb-x-a-ccc", "en appName");

    // Test requesting languages that resolve to a language other than
    // the default language.
    VerifyAppName(aboutData, "DE-CH", "de-CH appName");
    VerifyAppName(aboutData, "de-ch-1901", "de-CH appName");

    // Test requesting languages that resolve to nothing and so use the
    // default language.
    VerifyAppName(aboutData, "de", "en appName");
    VerifyAppName(aboutData, "fr", "en appName");

    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, InitUsingMsgArgBadSignature) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    alljoyn_msgarg notADictionary =
        alljoyn_msgarg_create_and_set("s", "incorrect type.");

    status = alljoyn_aboutdata_createfrommsgarg(aboutData, notADictionary, "en");
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: "
                                                 << QCC_StatusText(status);

    alljoyn_msgarg_destroy(notADictionary);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, UTF8_test) {
    char str[] = "привет";
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    status = alljoyn_aboutdata_setappname(aboutData, str, "ru");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* esOut;
    status = alljoyn_aboutdata_getappname(aboutData, &esOut, "ru");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(str, esOut);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, GetAnnouncedAboutData) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, appId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdeviceid(aboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A poetic description of this application",
                                              "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    alljoyn_msgarg announceArg = alljoyn_msgarg_create();
    status = alljoyn_aboutdata_getannouncedaboutdata(aboutData, announceArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg args;
    status = alljoyn_msgarg_getdictelement(announceArg, "{sv}",
                                           AboutData::APP_ID, &args);
    int8_t* appIdOut;
    size_t appIdNum;
    alljoyn_msgarg_get(args, "ay", &appIdNum, &appIdOut);
    ASSERT_EQ(16u, appIdNum);
    for (size_t i = 0; i < appIdNum; ++i) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    status = alljoyn_msgarg_getdictelement(announceArg, "{sv}",
                                           AboutData::DEFAULT_LANGUAGE, &args);
    char* defaultLanguage;
    alljoyn_msgarg_get(args, "s", &defaultLanguage);
    EXPECT_STREQ("en", defaultLanguage);

    status = alljoyn_msgarg_getdictelement(announceArg, "{sv}",
                                           AboutData::DEVICE_ID, &args);
    char* deviceId;
    alljoyn_msgarg_get(args, "s", &deviceId);
    EXPECT_STREQ("fakeID", deviceId);

    status = alljoyn_msgarg_getdictelement(announceArg, "{sv}",
                                           AboutData::APP_NAME, &args);
    char* appName;
    alljoyn_msgarg_get(args, "s", &appName);
    EXPECT_STREQ("Application", appName);

    status = alljoyn_msgarg_getdictelement(announceArg, "{sv}",
                                           AboutData::MANUFACTURER, &args);
    char* manufacturer;
    alljoyn_msgarg_get(args, "s", &manufacturer);
    EXPECT_STREQ("Manufacturer", manufacturer);

    status = alljoyn_msgarg_getdictelement(announceArg, "{sv}",
                                           AboutData::MODEL_NUMBER, &args);
    char* modelNumber;
    alljoyn_msgarg_get(args, "s", &modelNumber);
    EXPECT_STREQ("123456", modelNumber);

    alljoyn_msgarg_destroy(announceArg);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, SetOEMSpecificField) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    alljoyn_msgarg arg = alljoyn_msgarg_create();

    status = alljoyn_msgarg_set(arg, "s", "888-555-1234");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setfield(aboutData, "SupportNumber", arg, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_set(arg, "s", "800-555-4321");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setfield(aboutData, "SupportNumber", arg, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg value;
    status = alljoyn_aboutdata_getfield(aboutData, "SupportNumber", &value, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char buf[2];
    alljoyn_msgarg_signature(value, buf, 2);
    EXPECT_STREQ("s", buf);
    const char* supportNumber;
    status = alljoyn_msgarg_get(value, "s", &supportNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("888-555-1234", supportNumber);

    status = alljoyn_aboutdata_getfield(aboutData, "SupportNumber", &value, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg_signature(value, buf, 2);
    EXPECT_STREQ("s", buf);
    status = alljoyn_msgarg_get(value, "s", &supportNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("800-555-4321", supportNumber);

    alljoyn_msgarg_destroy(arg);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, GetMsgArgWithOEMSpecificField) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, appId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdeviceid(aboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A poetic description of this application",
                                              "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "Una descripcion poetica de esta aplicacion",
                                              "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "es"));

    alljoyn_msgarg arg = alljoyn_msgarg_create();
    status = alljoyn_msgarg_set(arg, "s", "888-555-1234");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setfield(aboutData, "SupportNumber", arg, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_set(arg, "s", "800-555-4321");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setfield(aboutData, "SupportNumber", arg, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg aboutArg = alljoyn_msgarg_create();
    status = alljoyn_aboutdata_getaboutdata(aboutData, aboutArg, "en");

    alljoyn_msgarg args;
    status = alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                           AboutData::APP_ID, &args);
    int8_t* appIdOut;
    size_t appIdNum;
    alljoyn_msgarg_get(args, "ay", &appIdNum, &appIdOut);
    ASSERT_EQ(16u, appIdNum);
    for (size_t i = 0; i < appIdNum; ++i) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    status = alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                           AboutData::DEFAULT_LANGUAGE, &args);
    char* defaultLanguage;
    alljoyn_msgarg_get(args, "s", &defaultLanguage);
    EXPECT_STREQ("en", defaultLanguage);

    status = alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                           AboutData::DEVICE_ID, &args);
    char* deviceId;
    alljoyn_msgarg_get(args, "s", &deviceId);
    EXPECT_STREQ("fakeID", deviceId);

    status = alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                           AboutData::APP_NAME, &args);
    char* appName;
    alljoyn_msgarg_get(args, "s", &appName);
    EXPECT_STREQ("Application", appName);

    status = alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                           AboutData::MANUFACTURER, &args);
    char* manufacturer;
    alljoyn_msgarg_get(args, "s", &manufacturer);
    EXPECT_STREQ("Manufacturer", manufacturer);

    status = alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                           AboutData::MODEL_NUMBER, &args);
    char* modelNumber;
    alljoyn_msgarg_get(args, "s", &modelNumber);
    EXPECT_STREQ("123456", modelNumber);

    status = alljoyn_msgarg_getdictelement(aboutArg, "{sv}",
                                           "SupportNumber", &args);
    char* supportNumber;
    alljoyn_msgarg_get(args, "s", &supportNumber);
    EXPECT_STREQ("888-555-1234", supportNumber);
    alljoyn_msgarg_destroy(aboutArg);
    alljoyn_aboutdata_destroy(aboutData);
}

TEST(AboutDataTest, InitUsingMsgArg) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = alljoyn_aboutdata_setappid(aboutData, originalAppId, 16);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdeviceid(aboutData, "fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A poetic description of this application",
                                              "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "en"));

    status = alljoyn_aboutdata_setsupportedlanguage(aboutData, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setappname(aboutData, "aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "Una descripcion poetica de esta aplicacion",
                                              "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(alljoyn_aboutdata_isvalid(aboutData, "es"));

    alljoyn_msgarg arg = alljoyn_msgarg_create();

    status = alljoyn_msgarg_set(arg, "s", "888-555-1234");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setfield(aboutData, "SupportNumber", arg, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_set(arg, "s", "800-555-4321");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setfield(aboutData, "SupportNumber", arg, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg aboutArg = alljoyn_msgarg_create();
    status = alljoyn_aboutdata_getaboutdata(aboutData, aboutArg, "en");

    alljoyn_aboutdata aboutDataInit = alljoyn_aboutdata_create("en");
    status = alljoyn_aboutdata_createfrommsgarg(aboutDataInit, aboutArg, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint8_t* appId;
    size_t num;
    status = alljoyn_aboutdata_getappid(aboutDataInit, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(originalAppId[i], appId[i]);
    }

    char* deviceId;
    status = alljoyn_aboutdata_getdeviceid(aboutDataInit, &deviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("fakeID", deviceId);

    char* appName;
    status = alljoyn_aboutdata_getappname(aboutDataInit, &appName, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Application", appName);

    char* manufacturer;
    status = alljoyn_aboutdata_getmanufacturer(aboutDataInit, &manufacturer, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Manufacturer", manufacturer);

    char* modelNumber;
    status = alljoyn_aboutdata_getmodelnumber(aboutDataInit, &modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("123456", modelNumber);

    char* description;
    status = alljoyn_aboutdata_getdescription(aboutDataInit, &description, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("A poetic description of this application", description);

    alljoyn_msgarg value;
    alljoyn_aboutdata_getfield(aboutDataInit, "SupportNumber", &value, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char buf[2];
    alljoyn_msgarg_signature(value, buf, 2);
    EXPECT_STREQ("s", buf);
    const char* supportNumber;
    status = alljoyn_msgarg_get(value, "s", &supportNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("888-555-1234", supportNumber);

    size_t number_languages =
        alljoyn_aboutdata_getsupportedlanguages(aboutDataInit, NULL, 0);
    EXPECT_EQ(2u, number_languages);
    size_t copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutDataInit);
    EXPECT_EQ(6u, copySize);
    /* TODO complete the test for language and other required */
    alljoyn_msgarg_destroy(aboutArg);
    alljoyn_aboutdata_destroy(aboutData);
    alljoyn_aboutdata_destroy(aboutDataInit);
}

TEST(AboutDataTest, caseInsensitiveLanguageTag) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");

    char* language;
    status = alljoyn_aboutdata_getdefaultlanguage(aboutData, &language);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", language);

    status = alljoyn_aboutdata_setdevicename(aboutData, "Device", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_aboutdata_setdevicename(aboutData, "dispositivo", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    size_t num_langs;
    num_langs = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(2u, num_langs);
    size_t copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(6u, copySize);

    status = alljoyn_aboutdata_setdevicename(aboutData, "Device", "EN");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    num_langs = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(2u, num_langs);
    copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(6u, copySize);

    status = alljoyn_aboutdata_setdevicename(aboutData, "Device", "En");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    num_langs = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(2u, num_langs);
    copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(6u, copySize);

    status = alljoyn_aboutdata_setdevicename(aboutData, "Device", "eN");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    num_langs = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(2u, num_langs);
    copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(6u, copySize);

    status = alljoyn_aboutdata_setdevicename(aboutData, "dispositivo", "ES");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    num_langs = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(2u, num_langs);
    copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(6u, copySize);

    status = alljoyn_aboutdata_setdevicename(aboutData, "dispositivo", "Es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    num_langs = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(2u, num_langs);
    copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(6u, copySize);

    status = alljoyn_aboutdata_setdevicename(aboutData, "dispositivo", "eS");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    num_langs = alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    EXPECT_EQ(2u, num_langs);
    copySize = alljoyn_aboutdata_getsupportedlanguagescopylength(aboutData);
    EXPECT_EQ(6u, copySize);

    char* deviceName;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, "EN");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Device", deviceName);

    deviceName = NULL;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, "En");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Device", deviceName);

    deviceName = NULL;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, "eN");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Device", deviceName);

    deviceName = NULL;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, "ES");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("dispositivo", deviceName);

    deviceName = NULL;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, "Es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("dispositivo", deviceName);

    deviceName = NULL;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, "eS");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("dispositivo", deviceName);

    alljoyn_aboutdata_destroy(aboutData);
}

/* Needs fix in alljoyn_c/src/AboutData.cc */
TEST(AboutDataTest, CreateFromXml) {
    QStatus status = ER_FAIL;
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    char xml[] =
        "<AboutData>"
        "  <AppId>000102030405060708090A0B0C0D0E0C</AppId>"
        "  <DefaultLanguage>en</DefaultLanguage>"
        "  <DeviceName>My Device Name</DeviceName>"
        "  <DeviceName lang = 'es'>Nombre de mi dispositivo</DeviceName>"
        "  <DeviceId>baddeviceid</DeviceId>"
        "  <AppName>My Application Name</AppName>"
        "  <AppName lang = 'es'>Mi Nombre de la aplicación</AppName>"
        "  <Manufacturer>Company</Manufacturer>"
        "  <Manufacturer lang = 'es'>Empresa</Manufacturer>"
        "  <ModelNumber>Wxfy388i</ModelNumber>"
        "  <Description>A detailed description provided by the application.</Description>"
        "  <Description lang = 'es'>Una descripción detallada proporcionada por la aplicación.</Description>"
        "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
        "  <SoftwareVersion>1.0.0</SoftwareVersion>"
        "  <HardwareVersion>1.0.0</HardwareVersion>"
        "  <SupportUrl>www.example.com</SupportUrl>"
        "  <UserDefinedTag>Can only accept strings anything other than strings must be done using the AboutData Class SetField method</UserDefinedTag>"
        "  <UserDefinedTag lang='es'>Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método AboutData Clase SetField</UserDefinedTag>"
        "</AboutData>";
    status = alljoyn_aboutdata_createfromxml(aboutData, xml);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* OriginalAppId is the string 000102030405060708090A0B0C0D0E0C converted to an array of bytes */
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 12 };
    uint8_t* appId;
    size_t num;
    status = alljoyn_aboutdata_getappid(aboutData, &appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(originalAppId[i], appId[i]) << i
                                              << ": " << originalAppId[i]
                                              << " = " << appId[i];
    }

    char* defaultLanguage;
    status = alljoyn_aboutdata_getdefaultlanguage(aboutData, &defaultLanguage);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", defaultLanguage);

    char* deviceName;
    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("My Device Name", deviceName);

    status = alljoyn_aboutdata_getdevicename(aboutData, &deviceName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Nombre de mi dispositivo", deviceName);

    char* deviceId;
    status = alljoyn_aboutdata_getdeviceid(aboutData, &deviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("baddeviceid", deviceId);

    char* appName;
    status = alljoyn_aboutdata_getappname(aboutData, &appName, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("My Application Name", appName);

    status = alljoyn_aboutdata_getappname(aboutData, &appName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Mi Nombre de la aplicación", appName);

    char* manufacturer;
    status = alljoyn_aboutdata_getmanufacturer(aboutData, &manufacturer, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Company", manufacturer);

    status = alljoyn_aboutdata_getmanufacturer(aboutData, &manufacturer, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Empresa", manufacturer);

    size_t numLanguages =
        alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    const char** languages = new const char*[numLanguages];

    size_t numRetLang =
        alljoyn_aboutdata_getsupportedlanguages(aboutData,
                                                languages,
                                                numLanguages);
    EXPECT_EQ(numLanguages, numRetLang);
    EXPECT_EQ(2u, numLanguages);
    EXPECT_STREQ("en", languages[0]);
    EXPECT_STREQ("es", languages[1]);
    delete [] languages;
    languages = NULL;

    char* description;
    status = alljoyn_aboutdata_getdescription(aboutData, &description, "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("A detailed description provided by the application.", description);

    status = alljoyn_aboutdata_getdescription(aboutData, &description, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Una descripción detallada proporcionada por la aplicación.", description);

    char* modelNumber;
    status = alljoyn_aboutdata_getmodelnumber(aboutData, &modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Wxfy388i", modelNumber);

    char* dateOfManufacture;
    status = alljoyn_aboutdata_getdateofmanufacture(aboutData, &dateOfManufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("2014-01-08", dateOfManufacture);

    char* softwareVersion;
    status = alljoyn_aboutdata_getsoftwareversion(aboutData, &softwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("1.0.0", softwareVersion);

    char* ajSoftwareVersion;
    alljoyn_aboutdata_getajsoftwareversion(aboutData, &ajSoftwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(ajn::GetVersion(), ajSoftwareVersion);

    char* hardwareVersion;
    status = alljoyn_aboutdata_gethardwareversion(aboutData, &hardwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("1.0.0", hardwareVersion);

    char* supportUrl;
    status = alljoyn_aboutdata_getsupporturl(aboutData, &supportUrl);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("www.example.com", supportUrl);

    alljoyn_aboutdata_destroy(aboutData);
}
