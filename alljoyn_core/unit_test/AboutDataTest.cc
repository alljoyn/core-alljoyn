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
#include <gtest/gtest.h>
#include <qcc/String.h>

#include <map>


using namespace ajn;

TEST(AboutData, constants) {
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

TEST(AboutData, Constructor) {
    AboutData aboutData("en");
    char* language;
    aboutData.GetDefaultLanguage(&language);
    EXPECT_STREQ("en", language);
    char* ajSoftwareVersion;
    aboutData.GetAJSoftwareVersion(&ajSoftwareVersion);
    EXPECT_STREQ(ajn::GetVersion(), ajSoftwareVersion);
}

TEST(AboutData, AddAppId) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    status = aboutData.AddAppId(originalAppId, 16u);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint8_t* appId;
    size_t num;
    status = aboutData.GetAppId(&appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(originalAppId[i], appId[i]);
    }
}
TEST(AboutData, AddDeviceName) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");
    char* language;
    status = aboutData.GetDefaultLanguage(&language);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", language);
    char* ajSoftwareVersion;
    status = aboutData.GetAJSoftwareVersion(&ajSoftwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(ajn::GetVersion(), ajSoftwareVersion);

    status = aboutData.AddDeviceName(qcc::String("Device").c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* deviceName;
    status = aboutData.GetDeviceName(&deviceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Device", deviceName);

    status = aboutData.AddDeviceName("dispositivo", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.GetDeviceName(&deviceName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("dispositivo", deviceName);
}

TEST(AboutData, AddDeviceId) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddDeviceId("avec-awe1213-1234559xvc123");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* deviceId;
    status = aboutData.GetDeviceId(&deviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("avec-awe1213-1234559xvc123", deviceId);
}

TEST(AboutData, AddAppName) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddAppName("Application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* appName;
    status = aboutData.GetAppName(&appName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Application", appName);

    status = aboutData.AddAppName("aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.GetAppName(&appName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("aplicacion", appName);
}

TEST(AboutData, AddManufacture) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddManufacture("Manufacture");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* manufacture;
    status = aboutData.GetManufacture(&manufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Manufacture", manufacture);

    status = aboutData.AddManufacture("manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.GetManufacture(&manufacture, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("manufactura", manufacture);
}

TEST(AboutData, AddModelNumber) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddModelNumber("xBnc345");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* modelNumber;
    status = aboutData.GetModelNumber(&modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("xBnc345", modelNumber);
}

TEST(AboutData, AddSupportedLanguage)
{
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    qcc::String* languages;
    size_t numLanguages;

    status = aboutData.GetSupportedLanguages(&languages, &numLanguages);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(1u, numLanguages);
    EXPECT_STREQ("en", languages[0].c_str());

    status = aboutData.AddSupportedLanguage("es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.GetSupportedLanguages(&languages, &numLanguages);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(2u, numLanguages);
    EXPECT_STREQ("en", languages[0].c_str());
    EXPECT_STREQ("es", languages[1].c_str());
}

TEST(AboutData, AddDescription) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddDescription("A poetic description of this application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* description;
    status = aboutData.GetDescription(&description);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("A poetic description of this application", description);

    status = aboutData.AddDescription("Una descripcion poetica de esta aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.GetDescription(&description, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Una descripcion poetica de esta aplicacion", description);
}

TEST(AboutData, AddDateOfManufacture) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddDateOfManufacture("2014-01-20");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* dateOfManufacture;
    status = aboutData.GetDateOfManufacture(&dateOfManufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("2014-01-20", dateOfManufacture);
}

TEST(AboutData, AddSoftwareVersion) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddSoftwareVersion("0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* softwareVersion;
    status = aboutData.GetSoftwareVersion(&softwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("0.1.2", softwareVersion);
}

TEST(AboutData, AddHardwareVersion) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddHardwareVersion("3.2.1");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* hardwareVersion;
    status = aboutData.GetHardwareVersion(&hardwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("3.2.1", hardwareVersion);
}

TEST(AboutData, AddSupportUrl) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddSupportUrl("www.example.com");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* supportUrl;
    status = aboutData.GetSupportUrl(&supportUrl);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("www.example.com", supportUrl);
}

TEST(AboutData, IsValid)
{
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    EXPECT_FALSE(aboutData.IsValid());
    uint8_t appId[] = { 0, 1, 2, 3, 4, 5 };
    status = aboutData.AddAppId(appId, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDeviceId("fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("Application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("Manufacture");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddModelNumber("123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("A poetic description of this application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddSoftwareVersion("0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid());

    EXPECT_FALSE(aboutData.IsValid("es"));

    status = aboutData.AddSupportedLanguage("es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("Una descripcion poetica de esta aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid("es"));
}

TEST(AboutData, GetMsgArg)
{
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5 };
    status = aboutData.AddAppId(appId, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDeviceId("fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("Application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("Manufacture");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddModelNumber("123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("A poetic description of this application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddSoftwareVersion("0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid());

    status = aboutData.AddSupportedLanguage("es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("Una descripcion poetica de esta aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid("es"));

    MsgArg aboutArg;
    aboutData.GetMsgArg(&aboutArg);

    //printf("*****\n%s\n*****\n", aboutArg.ToString().c_str());

    MsgArg* args;

    aboutArg.GetElement("{sv}", AboutData::APP_ID, &args);
    int8_t* appIdOut;
    size_t appIdNum;
    args->Get("ay", &appIdNum, &appIdOut);
    ASSERT_EQ(6u, appIdNum);
    for (size_t i = 0; i < appIdNum; ++i) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    aboutArg.GetElement("{sv}", AboutData::DEFAULT_LANGUAGE, &args);
    char* defaultLanguage;
    args->Get("s", &defaultLanguage);
    EXPECT_STREQ("en", defaultLanguage);

    aboutArg.GetElement("{sv}", AboutData::DEVICE_ID, &args);
    char* deviceId;
    args->Get("s", &deviceId);
    EXPECT_STREQ("fakeID", deviceId);

    aboutArg.GetElement("{sv}", AboutData::APP_NAME, &args);
    char* appName;
    args->Get("s", &appName);
    EXPECT_STREQ("Application", appName);

    aboutArg.GetElement("{sv}", AboutData::MANUFACTURER, &args);
    char* manufacturer;
    args->Get("s", &manufacturer);
    EXPECT_STREQ("Manufacture", manufacturer);

    aboutArg.GetElement("{sv}", AboutData::MODEL_NUMBER, &args);
    char* modelNumber;
    args->Get("s", &modelNumber);
    EXPECT_STREQ("123456", modelNumber);
}

TEST(AboutData, GetMsgArgAnnounce)
{
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5 };
    status = aboutData.AddAppId(appId, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDeviceId("fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("Application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("Manufacture");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddModelNumber("123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("A poetic description of this application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddSoftwareVersion("0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid());

    MsgArg announceArg;
    status = aboutData.GetMsgArgAnnounce(&announceArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg* args;

    announceArg.GetElement("{sv}", AboutData::APP_ID, &args);
    int8_t* appIdOut;
    size_t appIdNum;
    args->Get("ay", &appIdNum, &appIdOut);
    ASSERT_EQ(6u, appIdNum);
    for (size_t i = 0; i < appIdNum; ++i) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    announceArg.GetElement("{sv}", AboutData::DEFAULT_LANGUAGE, &args);
    char* defaultLanguage;
    args->Get("s", &defaultLanguage);
    EXPECT_STREQ("en", defaultLanguage);

    announceArg.GetElement("{sv}", AboutData::DEVICE_ID, &args);
    char* deviceId;
    args->Get("s", &deviceId);
    EXPECT_STREQ("fakeID", deviceId);

    announceArg.GetElement("{sv}", AboutData::APP_NAME, &args);
    char* appName;
    args->Get("s", &appName);
    EXPECT_STREQ("Application", appName);

    announceArg.GetElement("{sv}", AboutData::MANUFACTURER, &args);
    char* manufacturer;
    args->Get("s", &manufacturer);
    EXPECT_STREQ("Manufacture", manufacturer);

    announceArg.GetElement("{sv}", AboutData::MODEL_NUMBER, &args);
    char* modelNumber;
    args->Get("s", &modelNumber);
    EXPECT_STREQ("123456", modelNumber);
}

TEST(AboutData, AddOEMSpecificField)
{
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    MsgArg arg;
    status = arg.Set("s", "888-555-1234");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.AddField("SupportNumber", arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = arg.Set("s", "800-555-4321");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.AddField("SupportNumber", arg, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg* value;
    status = aboutData.GetField("SupportNumber", value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("s", value->Signature().c_str());
    const char* supportNumber;
    status = value->Get("s", &supportNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("888-555-1234", supportNumber);

    status = aboutData.GetField("SupportNumber", value, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("s", value->Signature().c_str());
    status = value->Get("s", &supportNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("800-555-4321", supportNumber);
}
TEST(AboutData, GetMsgArgWithOEMSpecificField)
{
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5 };
    status = aboutData.AddAppId(appId, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDeviceId("fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("Application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("Manufacture");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddModelNumber("123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("A poetic description of this application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddSoftwareVersion("0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid());

    status = aboutData.AddSupportedLanguage("es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("Una descripcion poetica de esta aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid("es"));

    MsgArg arg;
    status = arg.Set("s", "888-555-1234");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.AddField("SupportNumber", arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = arg.Set("s", "800-555-4321");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.AddField("SupportNumber", arg, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg aboutArg;
    aboutData.GetMsgArg(&aboutArg);

    //printf("*****\n%s\n*****\n", aboutArg.ToString().c_str());

    MsgArg* args;

    aboutArg.GetElement("{sv}", AboutData::APP_ID, &args);
    int8_t* appIdOut;
    size_t appIdNum;
    args->Get("ay", &appIdNum, &appIdOut);
    ASSERT_EQ(6u, appIdNum);
    for (size_t i = 0; i < appIdNum; ++i) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    aboutArg.GetElement("{sv}", AboutData::DEFAULT_LANGUAGE, &args);
    char* defaultLanguage;
    args->Get("s", &defaultLanguage);
    EXPECT_STREQ("en", defaultLanguage);

    aboutArg.GetElement("{sv}", AboutData::DEVICE_ID, &args);
    char* deviceId;
    args->Get("s", &deviceId);
    EXPECT_STREQ("fakeID", deviceId);

    aboutArg.GetElement("{sv}", AboutData::APP_NAME, &args);
    char* appName;
    args->Get("s", &appName);
    EXPECT_STREQ("Application", appName);

    aboutArg.GetElement("{sv}", AboutData::MANUFACTURER, &args);
    char* manufacturer;
    args->Get("s", &manufacturer);
    EXPECT_STREQ("Manufacture", manufacturer);

    aboutArg.GetElement("{sv}", AboutData::MODEL_NUMBER, &args);
    char* modelNumber;
    args->Get("s", &modelNumber);
    EXPECT_STREQ("123456", modelNumber);

    aboutArg.GetElement("{sv}", "SupportNumber", &args);
    char* supportNumber;
    args->Get("s", &supportNumber);
    EXPECT_STREQ("888-555-1234", supportNumber);
}

TEST(AboutData, InitUsingMsgArgBadSignature) {
    QStatus status = ER_FAIL;
    AboutData aboutData("en");
    MsgArg notADictionary("s", "incorrect type.");

    status = aboutData.Initialize(notADictionary);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST(AboutData, InitUsingMsgArg)
{
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5 };
    status = aboutData.AddAppId(appId, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDeviceId("fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("Application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("Manufacture");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddModelNumber("123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("A poetic description of this application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddSoftwareVersion("0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid());

    status = aboutData.AddSupportedLanguage("es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("manufactura", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("Una descripcion poetica de esta aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid("es"));

    MsgArg arg;
    status = arg.Set("s", "888-555-1234");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.AddField("SupportNumber", arg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = arg.Set("s", "800-555-4321");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = aboutData.AddField("SupportNumber", arg, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg aboutArg;
    aboutData.GetMsgArg(&aboutArg);

    //TODO make a constructor for about data that does not require the default
    //     language
    AboutData aboutDataInit("en");
    status = aboutDataInit.Initialize(aboutArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint8_t* appIdOut;
    size_t numOut;
    status = aboutDataInit.GetAppId(&appIdOut, &numOut);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(6u, numOut);
    for (size_t i = 0; i < numOut; i++) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    char* deviceId;
    status = aboutDataInit.GetDeviceId(&deviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("fakeID", deviceId);

    char* appName;
    status = aboutDataInit.GetAppName(&appName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Application", appName);

    char* manufacture;
    status = aboutDataInit.GetManufacture(&manufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Manufacture", manufacture);

    char* modelNumber;
    status = aboutDataInit.GetModelNumber(&modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("123456", modelNumber);

    char* description;
    status = aboutDataInit.GetDescription(&description);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("A poetic description of this application", description);

    MsgArg* value;
    status = aboutDataInit.GetField("SupportNumber", value);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("s", value->Signature().c_str());
    const char* supportNumber;
    status = value->Get("s", &supportNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("888-555-1234", supportNumber);

    //TODO complete the test for language and other required
}

TEST(AboutData, UTF8_test)
{
    char str[] = "привет";
    QStatus status = ER_FAIL;
    AboutData aboutData("en");

    status = aboutData.AddAppName(str, "ru");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    char* ruOut;
    status = aboutData.GetAppName(&ruOut, "ru");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(str, ruOut);
}

TEST(AboutData, CreateFromXml) {
    QStatus status = ER_FAIL;
    AboutData aboutData;
    qcc::String xml =
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
        "  <SupportedLanguages>"
        "    <language>en</language>"
        "    <language>es</language>"
        "  </SupportedLanguages>"
        "  <Description>A detailed description provided by the application.</Description>"
        "  <Description lang = 'es'>Una descripción detallada proporcionada por la aplicación.</Description>" /*TODO look into utf8*/
        "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
        "  <SoftwareVersion>1.0.0</SoftwareVersion>"
        "  <AJSoftwareVersion>" + static_cast<qcc::String>(ajn::GetVersion()) + "</AJSoftwareVersion>"
        "  <HardwareVersion>1.0.0</HardwareVersion>"
        "  <SupportUrl>www.example.com</SupportUrl>"
        "  <UserDefinedTag>Can only accept strings anything other than strings must be done using the AboutData Class AddField method</UserDefinedTag>"
        "  <UserDefinedTag lang='es'>Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método AboutData Clase AddField</UserDefinedTag>"
        "</AboutData>";
    status = aboutData.CreateFromXml(xml);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    // originalAppId is the string 000102030405060708090A0B0C0D0E0C converted to an array of bytes
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 12 };
    uint8_t* appId;
    size_t num;
    status = aboutData.GetAppId(&appId, &num);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, num);
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(originalAppId[i], appId[i]) << i << ": " << originalAppId[i] << " = " << appId[i];
    }

    char* defaultLanguage;
    status = aboutData.GetDefaultLanguage(&defaultLanguage);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", defaultLanguage);

    char* deviceName;
    status = aboutData.GetDeviceName(&deviceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("My Device Name", deviceName);

    status = aboutData.GetDeviceName(&deviceName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Nombre de mi dispositivo", deviceName);

    char* deviceId;
    status = aboutData.GetDeviceId(&deviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("baddeviceid", deviceId);

    char* appName;
    status = aboutData.GetAppName(&appName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("My Application Name", appName);

    status = aboutData.GetAppName(&appName, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Mi Nombre de la aplicación", appName);

    char* manufacture;
    status = aboutData.GetManufacture(&manufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Company", manufacture);

    status = aboutData.GetManufacture(&manufacture, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Empresa", manufacture);

    qcc::String* languages;
    size_t numLanguages;

    status = aboutData.GetSupportedLanguages(&languages, &numLanguages);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(2u, numLanguages);
    EXPECT_STREQ("en", languages[0].c_str());
    EXPECT_STREQ("es", languages[1].c_str());

    char* description;
    status = aboutData.GetDescription(&description);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("A detailed description provided by the application.", description);

    status = aboutData.GetDescription(&description, "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Una descripción detallada proporcionada por la aplicación.", description);

    char* modelNumber;
    status = aboutData.GetModelNumber(&modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Wxfy388i", modelNumber);

    char* dateOfManufacture;
    status = aboutData.GetDateOfManufacture(&dateOfManufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("2014-01-08", dateOfManufacture);

    char* softwareVersion;
    status = aboutData.GetSoftwareVersion(&softwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("1.0.0", softwareVersion);

    char* ajSoftwareVersion;
    status = aboutData.GetAJSoftwareVersion(&ajSoftwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(ajn::GetVersion(), ajSoftwareVersion);

    char* hardwareVersion;
    status = aboutData.GetHardwareVersion(&hardwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("1.0.0", hardwareVersion);

    char* supportUrl;
    status = aboutData.GetSupportUrl(&supportUrl);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("www.example.com", supportUrl);

//    MsgArg* value;
//    status = aboutData.GetField("UserDefinedTag", value);
//    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
//
//    EXPECT_STREQ("s", value->Signature().c_str());
//    const char* userDefined;
//    status = value->Get("s", &userDefined);
//    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
//    EXPECT_STREQ("Can only accept strings anything other than strings must be done using the AboutData Class AddField method", userDefined);
//
//    status = aboutData.GetField("UserDefinedTag", value, "es");
//    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
//
//    EXPECT_STREQ("s", value->Signature().c_str());
//    status = value->Get("s", &userDefined);
//    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
//    EXPECT_STREQ("Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método AboutData Clase AddField", userDefined);
}

class AboutDataTestAboutData : public AboutData {
  public:
    static const char* TEST_FIELDABC;

    AboutDataTestAboutData() : AboutData() {
        // test field abc is required, is announced, not localized
        AddNewFieldDetails(TEST_FIELDABC, true, true, false, "s");
    }
    AboutDataTestAboutData(const char* defaultLanguage) : AboutData(defaultLanguage) {
        AddNewFieldDetails(TEST_FIELDABC, true, true, false, "s");
    }

    QStatus AddTestFieldABC(char* testFieldABC)
    {
        QStatus status = ER_OK;
        MsgArg arg;
        status = arg.Set(m_aboutFields[TEST_FIELDABC].signature.c_str(), testFieldABC);
        if (status != ER_OK) {
            return status;
        }
        status = AddField(TEST_FIELDABC, arg);
        return status;
    }

    QStatus GetTestFieldABC(char** testFieldABC)
    {
        QStatus status;
        MsgArg* arg;
        status = GetField(TEST_FIELDABC, arg);
        if (status != ER_OK) {
            return status;
        }
        status = arg->Get(m_aboutFields[TEST_FIELDABC].signature.c_str(), testFieldABC);
        return status;
    }
};

const char* AboutDataTestAboutData::TEST_FIELDABC = "TestFieldABC";

TEST(AboutData, AddNewField) {
    QStatus status = ER_FAIL;
    AboutDataTestAboutData aboutData("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5 };
    status = aboutData.AddAppId(appId, 6);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDeviceId("fakeID");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddAppName("Application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddManufacture("Manufacture");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddModelNumber("123456");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddDescription("A poetic description of this application");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = aboutData.AddSoftwareVersion("0.1.2");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_FALSE(aboutData.IsValid());
    status = aboutData.AddTestFieldABC("Mary had a little lamb.");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_TRUE(aboutData.IsValid());

    MsgArg announceArg;
    status = aboutData.GetMsgArgAnnounce(&announceArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg* args;

    announceArg.GetElement("{sv}", AboutDataTestAboutData::APP_ID, &args);
    int8_t* appIdOut;
    size_t appIdNum;
    args->Get("ay", &appIdNum, &appIdOut);
    ASSERT_EQ(6u, appIdNum);
    for (size_t i = 0; i < appIdNum; ++i) {
        EXPECT_EQ(appId[i], appIdOut[i]);
    }

    announceArg.GetElement("{sv}", AboutDataTestAboutData::DEFAULT_LANGUAGE, &args);
    char* defaultLanguage;
    args->Get("s", &defaultLanguage);
    EXPECT_STREQ("en", defaultLanguage);

    announceArg.GetElement("{sv}", AboutDataTestAboutData::DEVICE_ID, &args);
    char* deviceId;
    args->Get("s", &deviceId);
    EXPECT_STREQ("fakeID", deviceId);

    announceArg.GetElement("{sv}", AboutDataTestAboutData::APP_NAME, &args);
    char* appName;
    args->Get("s", &appName);
    EXPECT_STREQ("Application", appName);

    announceArg.GetElement("{sv}", AboutDataTestAboutData::MANUFACTURER, &args);
    char* manufacturer;
    args->Get("s", &manufacturer);
    EXPECT_STREQ("Manufacture", manufacturer);

    announceArg.GetElement("{sv}", AboutDataTestAboutData::MODEL_NUMBER, &args);
    char* modelNumber;
    args->Get("s", &modelNumber);
    EXPECT_STREQ("123456", modelNumber);

    announceArg.GetElement("{sv}", AboutDataTestAboutData::TEST_FIELDABC, &args);
    char* testFieldabc;
    args->Get("s", &testFieldabc);
    EXPECT_STREQ("Mary had a little lamb.", testFieldabc);
}

//TEST(AboutData, AddSupportUrlEmpty) {
//    QStatus status = ER_FAIL;
//    AboutData aboutData("en");
//
//    char* supportUrl;
//    status = aboutData.GetSupportUrl(&supportUrl);
//    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
//    EXPECT_STREQ("www.example.com", supportUrl);
//}
