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
#include <gtest/gtest.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include <qcc/GUID.h>

using namespace ajn;
using namespace ajn::services;

/*
 * The PropertyStoreImpl is an implementation of the PropertyStore Interface.
 *
 * The PropertyStoreImpl must provide an implementation of the ReadAll member
 * function.
 *
 * If needed the implmentation of the PropertyStore should also provide the
 * following member functions.
 *  - Reset
 *  - Update
 *  - Delete
 *
 * If unimplemented the `Reset`, `Update` and `Delete` member functions will
 * return the #ER_NOT_IMPLEMENTED status
 */

TEST(PropertyStoreImplTest, member_functions_not_implemented_status)
{
    QStatus status;
    AboutPropertyStoreImpl ps;
    status = ps.Reset();
    EXPECT_EQ(ER_NOT_IMPLEMENTED, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg arg("s", "TestMsg");
    status = ps.Update("DeviceId", "en", &arg);
    EXPECT_EQ(ER_NOT_IMPLEMENTED, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.Delete("DeviceId", "en");
    EXPECT_EQ(ER_NOT_IMPLEMENTED, status) << "  Actual Status: " << QCC_StatusText(status);

    //Call ReadAll default language
    MsgArg writeArg;
    status = ps.ReadAll("en", PropertyStore::WRITE, writeArg);
    EXPECT_EQ(ER_NOT_IMPLEMENTED, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST(PropertyStoreImplTest, getPropertyStoreName)
{
    EXPECT_STREQ("DeviceId", AboutPropertyStoreImpl::getPropertyStoreName(DEVICE_ID).c_str());
    EXPECT_STREQ("DeviceName", AboutPropertyStoreImpl::getPropertyStoreName(DEVICE_NAME).c_str());
    EXPECT_STREQ("AppId", AboutPropertyStoreImpl::getPropertyStoreName(APP_ID).c_str());
    EXPECT_STREQ("AppName", AboutPropertyStoreImpl::getPropertyStoreName(APP_NAME).c_str());
    EXPECT_STREQ("DefaultLanguage", AboutPropertyStoreImpl::getPropertyStoreName(DEFAULT_LANG).c_str());
    EXPECT_STREQ("SupportedLanguages", AboutPropertyStoreImpl::getPropertyStoreName(SUPPORTED_LANGS).c_str());
    EXPECT_STREQ("Description", AboutPropertyStoreImpl::getPropertyStoreName(DESCRIPTION).c_str());
    EXPECT_STREQ("Manufacturer", AboutPropertyStoreImpl::getPropertyStoreName(MANUFACTURER).c_str());
    EXPECT_STREQ("DateOfManufacture", AboutPropertyStoreImpl::getPropertyStoreName(DATE_OF_MANUFACTURE).c_str());
    EXPECT_STREQ("ModelNumber", AboutPropertyStoreImpl::getPropertyStoreName(MODEL_NUMBER).c_str());
    EXPECT_STREQ("SoftwareVersion", AboutPropertyStoreImpl::getPropertyStoreName(SOFTWARE_VERSION).c_str());
    EXPECT_STREQ("AJSoftwareVersion", AboutPropertyStoreImpl::getPropertyStoreName(AJ_SOFTWARE_VERSION).c_str());
    EXPECT_STREQ("HardwareVersion", AboutPropertyStoreImpl::getPropertyStoreName(HARDWARE_VERSION).c_str());
    EXPECT_STREQ("SupportUrl", AboutPropertyStoreImpl::getPropertyStoreName(SUPPORT_URL).c_str());
}

TEST(PropertyStoreImplTest, setDeviceId)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setDeviceId("MyDeviceId");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(DEVICE_ID);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("MyDeviceId", out);
}

// ASACORE-1119
TEST(ProperteyStoreImplTest, setDeviceIdUsingGUIDWithZeroByteInString) {
    const char* GUID = "00112233445566778899AABBCCDDEEFF";
    AboutPropertyStoreImpl propertyStore;
    qcc::GUID128* deviceId = new qcc::GUID128(qcc::String(GUID, 32));
    QStatus status = propertyStore.setDeviceId(deviceId->ToString());
    if (status == ER_INVALID_VALUE) {
        ajn::MsgArg ma = propertyStore.getProperty(ajn::services::DEVICE_ID)->getPropertyValue();
        printf("setDeviceId() failed because the length of the GUID is: %d \n", ma.v_string.len);
    }
    PropertyStoreProperty* psp = propertyStore.getProperty(DEVICE_ID);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STRCASEEQ(GUID, out);
    delete deviceId;
}

// TODO HLD says device name should be localizable
TEST(PropertyStoreImplTest, setDeviceName)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setDeviceName("MyDeviceName", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(DEVICE_NAME);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_TRUE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("MyDeviceName", out);
}

TEST(PropertyStoreImplTest, setAppId)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setAppId("000102030405060708090A0B0C0D0E0C");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(APP_ID);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    uint8_t* appIdBuffer;
    size_t num;
    status = arg.Get("ay", &num, &appIdBuffer);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_EQ(16U, num);
    for (size_t i = 0; i < num - 1; ++i) {
        EXPECT_EQ(i, appIdBuffer[i]);
    }
    EXPECT_EQ(12, appIdBuffer[15]);
}


TEST(PropertyStoreImplTest, setAppName)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setAppName("MyAppName");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(APP_NAME);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("MyAppName", out);

    status = ps.setAppName("Another MyAppName", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    psp = ps.getProperty(APP_NAME, "en");
    ASSERT_TRUE(psp != NULL);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());
    EXPECT_STREQ("en", psp->getLanguage().c_str());

    arg = psp->getPropertyValue();
    status = arg.Get("s", &out);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Another MyAppName", out);
}

TEST(PropertyStoreImplTest, setDefaultLang)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setDefaultLang("en");
    EXPECT_EQ(ER_LANGUAGE_NOT_SUPPORTED, status) << "  Actual Status: " << QCC_StatusText(status);

    // The default language must be in the list of supported languages
    std::vector<qcc::String> languages(1);
    languages[0] = "en";
    status = ps.setSupportedLangs(languages);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setDefaultLang("en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(DEFAULT_LANG);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_TRUE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("en", out);
}

TEST(PropertyStoreImplTest, setSupportedLangs)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    // The default language must be in the list of supported languages
    std::vector<qcc::String> languages;
    languages.push_back("en");
    languages.push_back("es");
    languages.push_back("fr");
    status = ps.setSupportedLangs(languages);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(SUPPORTED_LANGS);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_FALSE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    MsgArg* out;
    size_t num;
    status = arg.Get("as", &num, &out);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(3U, num);
    const char* lang;
    status = out[0].Get("s", &lang);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", lang);

    status = out[1].Get("s", &lang);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("es", lang);

    status = out[2].Get("s", &lang);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("fr", lang);
}

TEST(PropertyStoreImplTest, setDescription)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setDescription("Test Description");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(DESCRIPTION);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_FALSE(psp->getIsAnnouncable());
    EXPECT_STREQ("", psp->getLanguage().c_str());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    status = arg.Get("s", &out);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Test Description", out);

    status = ps.setDescription("Another Test Description", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    psp = ps.getProperty(DESCRIPTION, "en");
    ASSERT_TRUE(psp != NULL);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_FALSE(psp->getIsAnnouncable());
    EXPECT_STREQ("en", psp->getLanguage().c_str());

    arg = psp->getPropertyValue();
    status = arg.Get("s", &out);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Another Test Description", out);
}

TEST(PropertyStoreImplTest, setManufacturer)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setManufacturer("The Manufacturer");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(MANUFACTURER);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());
    EXPECT_STREQ("", psp->getLanguage().c_str());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    status = arg.Get("s", &out);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("The Manufacturer", out);

    status = ps.setManufacturer("Another The Manufacturer", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    psp = ps.getProperty(MANUFACTURER, "en");
    ASSERT_TRUE(psp != NULL);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());
    EXPECT_STREQ("en", psp->getLanguage().c_str());

    arg = psp->getPropertyValue();
    status = arg.Get("s", &out);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Another The Manufacturer", out);
}

TEST(PropertyStoreImplTest, setDateOfManufacture)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setDateOfManufacture("2014-04-24");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(DATE_OF_MANUFACTURE);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_FALSE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("2014-04-24", out);
}

TEST(PropertyStoreImplTest, setSoftwareVersion)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setSoftwareVersion("1.2.3");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(SOFTWARE_VERSION);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_FALSE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("1.2.3", out);
}

TEST(PropertyStoreImplTest, setAjSoftwareVersion)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setAjSoftwareVersion(ajn::GetVersion());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(AJ_SOFTWARE_VERSION);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_FALSE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ(ajn::GetVersion(), out);
}

TEST(PropertyStoreImplTest, setHardwareVersion)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setHardwareVersion("3.2.1");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(HARDWARE_VERSION);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_FALSE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("3.2.1", out);
}

TEST(PropertyStoreImplTest, setModelNumber)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setModelNumber("ABC123");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(MODEL_NUMBER);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_TRUE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("ABC123", out);
}

TEST(PropertyStoreImplTest, setSupportURL)
{
    QStatus status;
    AboutPropertyStoreImpl ps;

    status = ps.setSupportUrl("www.allseenalliance.org");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    PropertyStoreProperty* psp = ps.getProperty(SUPPORT_URL);

    EXPECT_TRUE(psp->getIsPublic());
    EXPECT_FALSE(psp->getIsWritable());
    EXPECT_FALSE(psp->getIsAnnouncable());

    MsgArg arg = psp->getPropertyValue();
    const char* out;
    arg.Get("s", &out);
    EXPECT_STREQ("www.allseenalliance.org", out);
}

// TODO add tests for ReadAll member function

TEST(PropertyStoreImplTest, ReadAll_announce)
{
    QStatus status;
    // create a AboutPropertyStoreImpl and fill it with the correct values
    AboutPropertyStoreImpl ps;

    status = ps.setDeviceId("1231232145667745675477");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = ps.setDeviceName("MyDeviceName", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = ps.setAppId("000102030405060708090A0B0C0D0E0C");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    std::vector<qcc::String> languages(3);
    languages[0] = "en";
    languages[1] = "es";
    languages[2] = "fr";
    status = ps.setSupportedLangs(languages);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = ps.setDefaultLang("en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setAppName("My App Name", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setAppName("Mi Nombre App", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setAppName("Mon Nom App", "fr");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setModelNumber("Wxfy388i");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = ps.setDateOfManufacture("2014-04-24");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setSoftwareVersion("12.20.44 build 44454");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setAjSoftwareVersion(ajn::GetVersion());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setHardwareVersion("355.499. b");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setDescription("This is an Alljoyn Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setDescription("Esta es una Alljoyn aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setDescription("C'est une Alljoyn application", "fr");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setManufacturer("Company", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setManufacturer("Empresa", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setManufacturer("Entreprise", "fr");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setSupportUrl("http://www.alljoyn.org");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg announceArg;
    ps.ReadAll("en", PropertyStore::ANNOUNCE, announceArg);
    //printf("\n%s\n", announce_arg.ToString().c_str());

    EXPECT_STREQ("a{sv}", announceArg.Signature().c_str());

    MsgArg* args;

    /*
     * Announce should have the following keys
     *  - AppId
     *  - DefaultLanguage
     *  - DeviceName
     *  - DeviceId
     *  - AppName
     *  - Manufacturer
     *  - ModelNumber
     */
    status = announceArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(APP_ID).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int8_t* appIdOut;
    size_t appIdNum;
    status = args->Get("ay", &appIdNum, &appIdOut);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, appIdNum);
    for (size_t i = 0; i < appIdNum - 1; ++i) {
        EXPECT_EQ((int8_t)i, appIdOut[i]);
    }
    EXPECT_EQ(12, appIdOut[appIdNum - 1]);

    status = announceArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DEFAULT_LANG).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* defaultLanguage;
    status = args->Get("s", &defaultLanguage);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", defaultLanguage);

    status = announceArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DEVICE_NAME).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* deviceName;
    status = args->Get("s", &deviceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("MyDeviceName", deviceName);

    status = announceArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DEVICE_ID).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* deviceId;
    status = args->Get("s", &deviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("1231232145667745675477", deviceId);

    status = announceArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(APP_NAME).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* appName;
    status = args->Get("s", &appName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("My App Name", appName);

    status = announceArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(MANUFACTURER).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* manufacturer;
    status = args->Get("s", &manufacturer);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Company", manufacturer);

    status = announceArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(MODEL_NUMBER).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* modelNumber;
    status = args->Get("s", &modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Wxfy388i", modelNumber);
}

TEST(PropertyStoreImplTest, ReadAll_read)
{
    QStatus status;
    // create a AboutPropertyStoreImpl and fill it with the correct values
    AboutPropertyStoreImpl ps;

    status = ps.setDeviceId("1231232145667745675477");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = ps.setDeviceName("MyDeviceName", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = ps.setAppId("000102030405060708090A0B0C0D0E0C");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    std::vector<qcc::String> languages(3);
    languages[0] = "en";
    languages[1] = "es";
    languages[2] = "fr";
    status = ps.setSupportedLangs(languages);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = ps.setDefaultLang("en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setAppName("My App Name", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setAppName("Mi Nombre App", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setAppName("Mon Nom App", "fr");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setModelNumber("Wxfy388i");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = ps.setDateOfManufacture("2014-04-24");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setSoftwareVersion("12.20.44 build 44454");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setAjSoftwareVersion(ajn::GetVersion());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setHardwareVersion("355.499. b");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setDescription("This is an Alljoyn Application", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setDescription("Esta es una Alljoyn aplicacion", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setDescription("C'est une Alljoyn application", "fr");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setManufacturer("Company", "en");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setManufacturer("Empresa", "es");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setManufacturer("Entreprise", "fr");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = ps.setSupportUrl("http://www.alljoyn.org");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //Call ReadAll default language
    MsgArg readArg;
    ps.ReadAll("en", PropertyStore::READ, readArg);
    EXPECT_STREQ("a{sv}", readArg.Signature().c_str());

    MsgArg readArg2;
    ps.ReadAll("", PropertyStore::READ, readArg2);
    EXPECT_STREQ("a{sv}", readArg2.Signature().c_str());

    // an empty string should return the same as the default language
    EXPECT_TRUE(readArg2 == readArg) << "Read Arg for \"en\" \n----------------------\n" <<
    readArg.ToString().c_str() << "\nRead Arg for \"\"\n---------------------\n" << readArg2.ToString().c_str();

    MsgArg* args;

    /*
     * Announce should have the following keys
     *  - AppId
     *  - DefaultLanguage
     *  - DeviceName
     *  - DeviceId
     *  - AppName
     *  - Manufacturer
     *  - ModelNumber
     *  - SupportedLanguages
     *  - Description
     *  - DateOfManufacture
     *  - SoftwareVersion
     *  - AJSoftwareVersion
     *  - HardwareVersion
     *  - SupporteUrl
     */
    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(APP_ID).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int8_t* appIdOut;
    size_t appIdNum;
    status = args->Get("ay", &appIdNum, &appIdOut);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(16u, appIdNum);
    for (size_t i = 0; i < appIdNum - 1; ++i) {
        EXPECT_EQ((int8_t)i, appIdOut[i]);
    }
    EXPECT_EQ(12, appIdOut[appIdNum - 1]);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DEFAULT_LANG).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* defaultLanguage;
    status = args->Get("s", &defaultLanguage);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", defaultLanguage);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DEVICE_NAME).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* deviceName;
    status = args->Get("s", &deviceName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("MyDeviceName", deviceName);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DEVICE_ID).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* deviceId;
    status = args->Get("s", &deviceId);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("1231232145667745675477", deviceId);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(APP_NAME).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* appName;
    status = args->Get("s", &appName);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("My App Name", appName);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(MANUFACTURER).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* manufacturer;
    status = args->Get("s", &manufacturer);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Company", manufacturer);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(MODEL_NUMBER).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* modelNumber;
    status = args->Get("s", &modelNumber);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Wxfy388i", modelNumber);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(SUPPORTED_LANGS).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    MsgArg* languageArg;
    size_t num;
    status = args->Get("as", &num, &languageArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(3U, num);
    const char* lang;
    status = languageArg[0].Get("s", &lang);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("en", lang);

    status = languageArg[1].Get("s", &lang);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("es", lang);

    status = languageArg[2].Get("s", &lang);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("fr", lang);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DESCRIPTION).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* description;
    status = args->Get("s", &description);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("This is an Alljoyn Application", description);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DATE_OF_MANUFACTURE).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* dateOfManufacture;
    status = args->Get("s", &dateOfManufacture);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("2014-04-24", dateOfManufacture);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(SOFTWARE_VERSION).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* softwareVersion;
    status = args->Get("s", &softwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("12.20.44 build 44454", softwareVersion);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(AJ_SOFTWARE_VERSION).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* ajsoftwareVersion;
    status = args->Get("s", &ajsoftwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ(ajn::GetVersion(), ajsoftwareVersion);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(HARDWARE_VERSION).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* hardwareVersion;
    status = args->Get("s", &hardwareVersion);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("355.499. b", hardwareVersion);

    status = readArg.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(SUPPORT_URL).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* supportUrl;
    status = args->Get("s", &supportUrl);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("http://www.alljoyn.org", supportUrl);

    //Call ReadAll spanish es language
    MsgArg readArg_es;
    ps.ReadAll("es", PropertyStore::READ, readArg_es);
    EXPECT_STREQ("a{sv}", readArg_es.Signature().c_str());

    status = readArg_es.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DESCRIPTION).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* description_es;
    status = args->Get("s", &description_es);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Esta es una Alljoyn aplicacion", description_es);

    status = readArg_es.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(MANUFACTURER).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* manufacturer_es;
    status = args->Get("s", &manufacturer_es);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Empresa", manufacturer_es);

    //Call ReadAll spanish es language
    MsgArg readArg_fr;
    ps.ReadAll("fr", PropertyStore::READ, readArg_fr);
    EXPECT_STREQ("a{sv}", readArg.Signature().c_str());

    status = readArg_fr.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(DESCRIPTION).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* description_fr;
    status = args->Get("s", &description_fr);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("C'est une Alljoyn application", description_fr);

    status = readArg_fr.GetElement("{sv}", AboutPropertyStoreImpl::getPropertyStoreName(MANUFACTURER).c_str(), &args);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* manufacturer_fr;
    status = args->Get("s", &manufacturer_fr);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("Entreprise", manufacturer_fr);
}
