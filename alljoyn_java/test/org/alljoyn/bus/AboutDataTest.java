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
package org.alljoyn.bus;

import java.io.IOException;
import java.util.UUID;

import javax.xml.parsers.ParserConfigurationException;

import org.xml.sax.SAXException;

import junit.framework.TestCase;

public class AboutDataTest extends TestCase{
    static {
        System.loadLibrary("alljoyn_java");
    }

    public void setUp() throws Exception {
    }

    public void tearDown() throws Exception {
    }

    public void testConstructors() {
        try {
            AboutData aboutData = new AboutData();
            Variant v_ajSoftwareVersion = aboutData.getField(AboutKeys.ABOUT_AJ_SOFTWARE_VERSION);
            assertEquals("s", v_ajSoftwareVersion.getSignature());
            assertEquals(Version.get(), v_ajSoftwareVersion.getObject(String.class));
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            AboutData aboutData = new AboutData("en");
            Variant v_ajSoftwareVersion = aboutData.getField(AboutKeys.ABOUT_AJ_SOFTWARE_VERSION);
            assertEquals("s", v_ajSoftwareVersion.getSignature());
            assertEquals(Version.get(), v_ajSoftwareVersion.getObject(String.class));

            Variant v_defaultLanguage = aboutData.getField(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
            assertEquals("s", v_defaultLanguage.getSignature());
            assertEquals("en", v_defaultLanguage.getObject(String.class));
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testVerifyFieldValues() {
        AboutData aboutData = new AboutData();

        //AppId
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_APP_ID));
        assertTrue(aboutData.isFieldAnnounced(AboutData.ABOUT_APP_ID));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_APP_ID));
        assertEquals("ay", aboutData.getFieldSignature(AboutData.ABOUT_APP_ID));

        //DefaultLanguage
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_DEFAULT_LANGUAGE));
        assertTrue(aboutData.isFieldAnnounced(AboutData.ABOUT_DEFAULT_LANGUAGE));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_DEFAULT_LANGUAGE));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_DEFAULT_LANGUAGE));

        //DeviceName
        assertFalse(aboutData.isFieldRequired(AboutData.ABOUT_DEVICE_NAME));
        assertTrue(aboutData.isFieldAnnounced(AboutData.ABOUT_DEVICE_NAME));
        assertTrue(aboutData.isFieldLocalized(AboutData.ABOUT_DEVICE_NAME));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_DEVICE_NAME));

        //DeviceId
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_DEVICE_ID));
        assertTrue(aboutData.isFieldAnnounced(AboutData.ABOUT_DEVICE_ID));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_DEVICE_ID));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_DEVICE_ID));

        //AppName
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_APP_NAME));
        assertTrue(aboutData.isFieldAnnounced(AboutData.ABOUT_APP_NAME));
        assertTrue(aboutData.isFieldLocalized(AboutData.ABOUT_APP_NAME));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_APP_NAME));

        //Manufacturer
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_MANUFACTURER));
        assertTrue(aboutData.isFieldAnnounced(AboutData.ABOUT_MANUFACTURER));
        assertTrue(aboutData.isFieldLocalized(AboutData.ABOUT_MANUFACTURER));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_MANUFACTURER));

        //ModelNumber
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_MODEL_NUMBER));
        assertTrue(aboutData.isFieldAnnounced(AboutData.ABOUT_MODEL_NUMBER));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_MODEL_NUMBER));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_MODEL_NUMBER));

        //SupportedLanguages
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_SUPPORTED_LANGUAGES));
        assertFalse(aboutData.isFieldAnnounced(AboutData.ABOUT_SUPPORTED_LANGUAGES));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_SUPPORTED_LANGUAGES));
        assertEquals("as", aboutData.getFieldSignature(AboutData.ABOUT_SUPPORTED_LANGUAGES));

        //Description
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_DESCRIPTION));
        assertFalse(aboutData.isFieldAnnounced(AboutData.ABOUT_DESCRIPTION));
        assertTrue(aboutData.isFieldLocalized(AboutData.ABOUT_DESCRIPTION));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_DESCRIPTION));

        //DateOfManufacture
        assertFalse(aboutData.isFieldRequired(AboutData.ABOUT_DATE_OF_MANUFACTURE));
        assertFalse(aboutData.isFieldAnnounced(AboutData.ABOUT_DATE_OF_MANUFACTURE));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_DATE_OF_MANUFACTURE));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_DATE_OF_MANUFACTURE));

        //SoftwareVersion
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_SOFTWARE_VERSION));
        assertFalse(aboutData.isFieldAnnounced(AboutData.ABOUT_SOFTWARE_VERSION));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_SOFTWARE_VERSION));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_SOFTWARE_VERSION));

        //AJSoftwareVersion
        assertTrue(aboutData.isFieldRequired(AboutData.ABOUT_AJ_SOFTWARE_VERSION));
        assertFalse(aboutData.isFieldAnnounced(AboutData.ABOUT_AJ_SOFTWARE_VERSION));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_AJ_SOFTWARE_VERSION));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_AJ_SOFTWARE_VERSION));

        //HardwareVersion
        assertFalse(aboutData.isFieldRequired(AboutData.ABOUT_HARDWARE_VERSION));
        assertFalse(aboutData.isFieldAnnounced(AboutData.ABOUT_HARDWARE_VERSION));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_HARDWARE_VERSION));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_HARDWARE_VERSION));

        //SupportUrl
        assertFalse(aboutData.isFieldRequired(AboutData.ABOUT_SUPPORT_URL));
        assertFalse(aboutData.isFieldAnnounced(AboutData.ABOUT_SUPPORT_URL));
        assertFalse(aboutData.isFieldLocalized(AboutData.ABOUT_SUPPORT_URL));
        assertEquals("s", aboutData.getFieldSignature(AboutData.ABOUT_SUPPORT_URL));

        //Unknown field
        assertFalse(aboutData.isFieldRequired("Unknown"));
        assertFalse(aboutData.isFieldAnnounced("Unknown"));
        assertFalse(aboutData.isFieldLocalized("Unknown"));
        assertTrue(null == aboutData.getFieldSignature("Unknown"));
    }

    public void testDefaultLanguageNotSpecified() {
        AboutData aboutData = new AboutData();

        try {
            aboutData.setDeviceName("Device Name");
        } catch (BusException e) {
            assertEquals("Specified language tag not found.", e.getMessage());
        }

        try {
            aboutData.setAppName("Application Name");
        } catch (BusException e) {
            assertEquals("Specified language tag not found.", e.getMessage());
        }

        try {
            aboutData.setManufacturer("Manufacturer Name");
        } catch (BusException e) {
            assertEquals("Specified language tag not found.", e.getMessage());
        }

        try {
            aboutData.setDescription("A description of the application.");
        } catch (BusException e) {
            assertEquals("Specified language tag not found.", e.getMessage());
        }
    }

    public void testGetSetAppId() {
        try {
            AboutData aboutData = new AboutData("en");
            byte[] appIdIn = new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
            aboutData.setAppId(appIdIn);
            byte[] appIdOut = aboutData.getAppId();

            assertEquals(appIdIn, appIdOut);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            AboutData aboutData = new AboutData("en");
            byte[] appIdIn = new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
            try {
                aboutData = new AboutData("en");
                aboutData.setAppId(appIdIn);
                fail("Expected error thrown");
            } catch (BusException e) {
                assertEquals("AppId is not 128-bits. AppId passed in is still used.", e.getMessage());
            }
            assertEquals(appIdIn, aboutData.getAppId());
        } catch (BusException e1) {
            e1.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetAppIdUsingUUID() {
        try {
            AboutData aboutData = new AboutData("en");
            UUID appIdIn = UUID.fromString("b8b12eb2-5b66-4f28-b277-cbb05ad9a5f6");
            aboutData.setAppId(appIdIn);
            UUID appIdOut = aboutData.getAppIdAsUUID();

            assertEquals(appIdIn, appIdOut);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            AboutData aboutData = new AboutData("en");
            UUID appIdIn = UUID.randomUUID();
            aboutData.setAppId(appIdIn);
            UUID appIdOut = aboutData.getAppIdAsUUID();

            assertEquals(appIdIn, appIdOut);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetAppIdUsingHexString() {
        try {
            AboutData aboutData = new AboutData("en");
            aboutData.setAppId("000102030405060708090A0B0C0D0E0C");
            byte[] appIdOut = aboutData.getAppId();

            byte[] appIdIn = new byte[] {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 12};
            assertEquals(appIdIn.length, appIdOut.length);
            for (int i = 0; i < appIdOut.length; ++i) {
                assertEquals(appIdIn[i], appIdOut[i]);
            }

            assertEquals("000102030405060708090A0B0C0D0E0C", aboutData.getAppIdAsHexString());
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetDefaultLanguage() {
        try {
            AboutData aboutData = new AboutData("en");
            String defaultLanguage = aboutData.getDefaultLanguage();
            assertEquals("en", defaultLanguage);

            aboutData.setDefaultLanguage("es");
            defaultLanguage = aboutData.getDefaultLanguage();
            assertEquals("es", defaultLanguage);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetDeviceName() {
        AboutData aboutData = new AboutData("en");
        try {
            aboutData.setDeviceName("My Device Name");
            aboutData.setDeviceName("Mon appareil Nom", "fr");
            String deviceName = aboutData.getDeviceName();
            assertEquals("My Device Name", deviceName);
            deviceName = aboutData.getDeviceName("fr");
            assertEquals("Mon appareil Nom", deviceName);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
        try {
            //no Chinese language should throw a BusException
            String deviceName = aboutData.getDeviceName("zh");
            fail("Expected error thrown and it was not thrown.");
        } catch (BusException e) {
            assertEquals("Specified language tag not found.", e.getMessage());
        }
    }

    public void testGetSetDeviceId() {
        try {
            AboutData aboutData = new AboutData("en");
            aboutData.setDeviceId("MyDeviceId");
            String deviceId = aboutData.getDeviceId();
            assertEquals("MyDeviceId", deviceId);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetAppName() {
        AboutData aboutData = new AboutData("en");
        try {
            aboutData.setAppName("My Application Name");
            aboutData.setAppName("Mon Nom de l'application", "fr");
            String appName = aboutData.getAppName();
            assertEquals("My Application Name", appName);
            appName = aboutData.getAppName("fr");
            assertEquals("Mon Nom de l'application", appName);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
        try {
            //no Chinese language should throw a BusException
            String appName = aboutData.getAppName("zh");
            fail("Expected error thrown and it was not thrown.");
        } catch (BusException e) {
            assertEquals("Specified language tag not found.", e.getMessage());
        }
    }

    public void testGetSetManufacturer() {
        AboutData aboutData = new AboutData("en");
        try {
            aboutData.setManufacturer("Company XYZ");
            aboutData.setManufacturer("Société XYZ", "fr");
            String manufacturer = aboutData.getManufacturer();
            assertEquals("Company XYZ", manufacturer);
            manufacturer = aboutData.getManufacturer("fr");
            assertEquals("Société XYZ", manufacturer);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
        try {
            //no Chinese language should throw a BusException
            String manufacturer = aboutData.getManufacturer("zh");
            fail("Expected error thrown and it was not thrown.");
        } catch (BusException e) {
            assertEquals("Specified language tag not found.", e.getMessage());
        }
    }

    public void testGetSetModelNumber() {
        try {
            AboutData aboutData = new AboutData("en");
            aboutData.setModelNumber("123xyz.2");
            String modelNumber = aboutData.getModelNumber();
            assertEquals("123xyz.2", modelNumber);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetSupportedLanguages() {
        try {
            AboutData aboutData = new AboutData();
            String languagesIn[] = new String[3];
            languagesIn[0] = "en-US";
            languagesIn[1] = "fr-CA";
            languagesIn[2] = "cmn"; //Madarin chinese
            aboutData.setSupportedLanguages(languagesIn);

            String languagesOut[] = aboutData.getSupportedLanguages();
            assertEquals(3, languagesOut.length);


            aboutData.setSupportedLanguage("fr-ca");
            languagesOut = aboutData.getSupportedLanguages();
            assertEquals(3, languagesOut.length);

            aboutData.setSupportedLanguage("EN-us");
            languagesOut = aboutData.getSupportedLanguages();
            assertEquals(3, languagesOut.length);

            aboutData.setSupportedLanguage("de"); //German

            languagesOut = aboutData.getSupportedLanguages();
            assertEquals(4, languagesOut.length);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            AboutData aboutData = new AboutData();
            String languagesOut[] = aboutData.getSupportedLanguages();
            fail("Expected to throw a BusExcpetion");
            assertEquals(0, languagesOut.length);
        } catch (BusException e) {
            assertEquals("About Field Not Found.", e.getMessage());
        }
    }

    public void testGetSetDescription() {
        try {
            AboutData aboutData = new AboutData("en");

            aboutData.setDescription("Poetic application description.");
            aboutData.setDescription("Description poétique d'application.", "fr");
            String description = aboutData.getDescription();
            assertEquals("Poetic application description.", description);
            description = aboutData.getDescription("fr");
            assertEquals("Description poétique d'application.", description);

            //no Chinese language should throw a BusException
            description = aboutData.getDescription("zh");
            fail("Expected error thrown and it was not thrown.");
        } catch (BusException e) {
            assertEquals("Specified language tag not found.", e.getMessage());
        }
    }

    public void testGetSetDateOfManufacture() {
        try {
            AboutData aboutData = new AboutData("en");
            aboutData.setDateOfManufacture("2014-12-19");
            String dateOfManufacture = aboutData.getDateOfManufacture();
            assertEquals("2014-12-19", dateOfManufacture);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetSoftwareVersion() {
        try {
            AboutData aboutData = new AboutData("en");
            aboutData.setSoftwareVersion("1.0");
            String softwareVersion = aboutData.getSoftwareVersion();
            assertEquals("1.0", softwareVersion);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetAJSoftwareVersion() {
        try {
            AboutData aboutData = new AboutData("en");
            String ajSoftwareVersion = aboutData.getAJSoftwareVersion();
            assertEquals(Version.get(), ajSoftwareVersion);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetHardwareVersion() {
        try {
            AboutData aboutData = new AboutData("en");
            aboutData.setHardwareVersion("yzy123v");
            String hardwareVersion = aboutData.getHardwareVersion();
            assertEquals("yzy123v", hardwareVersion);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    public void testGetSetSupportURL() {
        try {
            AboutData aboutData = new AboutData("en");
            aboutData.setSupportUrl("www.example.com");
            String supportURL = aboutData.getSupportUrl();
            assertEquals("www.example.com", supportURL);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    class AboutDataTestAboutData extends AboutData {
        static final String TEST_FIELDABC = "TestFieldABC";
        AboutDataTestAboutData() {
            super();
            setNewFieldDetails(TEST_FIELDABC, FieldDetails.REQUIRED | FieldDetails.ANNOUNCED, "s");
        }

        AboutDataTestAboutData(String defaultLanguage) {
            super(defaultLanguage);
            setNewFieldDetails(TEST_FIELDABC, FieldDetails.REQUIRED | FieldDetails.ANNOUNCED, "s");
        }

        public void setTestFieldABC(String testFieldABC) throws BusException {
            setField(TEST_FIELDABC, new Variant(testFieldABC));
        }

        public String getTestFieldABC() throws BusException {
            return getField(TEST_FIELDABC).getObject(String.class);
        }
    }

    public void testGetSetUserDefinedField() {
        AboutDataTestAboutData aboutData = new AboutDataTestAboutData("en");
        try {
            aboutData.setTestFieldABC("Mary had a little lamb.");
            String testFieldABC = aboutData.getTestFieldABC();
            assertEquals("Mary had a little lamb.", testFieldABC);
            assertTrue(aboutData.isFieldAnnounced(AboutDataTestAboutData.TEST_FIELDABC));
            assertTrue(aboutData.isFieldRequired(AboutDataTestAboutData.TEST_FIELDABC));
            assertFalse(aboutData.isFieldLocalized(AboutDataTestAboutData.TEST_FIELDABC));
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

    }

    // This xml uses a UUID as per RFC-4122 as recommended in the design documents
    public void testCreateFromXml() {
        AboutData aboutData = new AboutData();
        String xml;
        xml = "<AboutData>"
            + "  <AppId>b8b12eb2-5b66-4f28-b277-cbb05ad9a5f6</AppId>"
            + "  <DefaultLanguage>en</DefaultLanguage>"
            + "  <DeviceName>My Device Name</DeviceName>"
            + "  <DeviceName lang = 'es'>Nombre de mi dispositivo</DeviceName>"
            + "  <DeviceId>baddeviceid</DeviceId>"
            + "  <AppName>My Application Name</AppName>"
            + "  <AppName lang = 'es'>Mi Nombre de la aplicación</AppName>"
            + "  <Manufacturer>Company</Manufacturer>"
            + "  <Manufacturer lang = 'es'>Empresa</Manufacturer>"
            + "  <ModelNumber>Wxfy388i</ModelNumber>"
            + "  <Description>A detailed description provided by the application.</Description>"
            + "  <Description lang = 'es'>Una descripción detallada proporcionada por la aplicación.</Description>" /*TODO look into utf8*/
            + "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
            + "  <SoftwareVersion>1.0.0</SoftwareVersion>"
            + "  <HardwareVersion>1.0.0</HardwareVersion>"
            + "  <SupportUrl>www.example.com</SupportUrl>"
            + "  <UserDefinedTag>Can only accept strings anything other than strings must be done using the AboutData Class SetField method</UserDefinedTag>"
            + "  <UserDefinedTag lang='es'>Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método AboutData Clase SetField</UserDefinedTag>"
            + "</AboutData>";
        try {
            aboutData.createFromXml(xml);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (SAXException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (IOException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            UUID uuid;
            uuid = aboutData.getAppIdAsUUID();
          assertEquals(UUID.fromString("b8b12eb2-5b66-4f28-b277-cbb05ad9a5f6"), uuid);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String defaultLanguage = aboutData.getDefaultLanguage();
            assertEquals("en", defaultLanguage);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String languagesOut[] = aboutData.getSupportedLanguages();
            assertEquals(2, languagesOut.length);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String deviceName = aboutData.getDeviceName();
            assertEquals("My Device Name", deviceName);
            deviceName = aboutData.getDeviceName("en");
            assertEquals("My Device Name", deviceName);
            deviceName = aboutData.getDeviceName("es");
            assertEquals("Nombre de mi dispositivo", deviceName);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String deviceId = aboutData.getDeviceId();
            assertEquals("baddeviceid", deviceId);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String appName = aboutData.getAppName();
            assertEquals("My Application Name", appName);
            appName = aboutData.getAppName("en");
            assertEquals("My Application Name", appName);
            appName = aboutData.getAppName("es");
            assertEquals("Mi Nombre de la aplicación", appName);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String manufacturer = aboutData.getManufacturer();
            assertEquals("Company", manufacturer);
            manufacturer = aboutData.getManufacturer("en");
            assertEquals("Company", manufacturer);
            manufacturer = aboutData.getManufacturer("es");
            assertEquals("Empresa", manufacturer);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String modelNumber = aboutData.getModelNumber();
            assertEquals("Wxfy388i", modelNumber);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String description = aboutData.getDescription();
            assertEquals("A detailed description provided by the application.", description);
            description = aboutData.getDescription("en");
            assertEquals("A detailed description provided by the application.", description);
            description = aboutData.getDescription("es");
            assertEquals("Una descripción detallada proporcionada por la aplicación.", description);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String dateOfManufacture = aboutData.getDateOfManufacture();
            assertEquals("2014-01-08", dateOfManufacture);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String softwareVersion = aboutData.getSoftwareVersion();
            assertEquals("1.0.0", softwareVersion);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String hardwareVersion = aboutData.getHardwareVersion();
            assertEquals("1.0.0", hardwareVersion);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String ajSoftwareVersion = aboutData.getAJSoftwareVersion();
            assertEquals(Version.get(), ajSoftwareVersion);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String supportUrl = aboutData.getSupportUrl();
            assertEquals("www.example.com", supportUrl);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            Variant userDefiend = aboutData.getField("UserDefinedTag");
            assertEquals("Can only accept strings anything other than strings must be done using the AboutData Class SetField method", userDefiend.getObject(String.class));
            userDefiend = aboutData.getField("UserDefinedTag", "en");
            assertEquals("Can only accept strings anything other than strings must be done using the AboutData Class SetField method", userDefiend.getObject(String.class));
            userDefiend = aboutData.getField("UserDefinedTag", "es");
            assertEquals("Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método AboutData Clase SetField", userDefiend.getObject(String.class));
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

    // This xml uses an hex encoded byte string to pass in the AppId.
    public void testCreateFromXml2() {
        AboutData aboutData = new AboutData();
        String xml;
        xml = "<AboutData>"
            + "  <AppId>000102030405060708090A0B0C0D0E0C</AppId>"
            + "  <DefaultLanguage>en</DefaultLanguage>"
            + "  <DeviceName>My Device Name</DeviceName>"
            + "  <DeviceName lang = 'es'>Nombre de mi dispositivo</DeviceName>"
            + "  <DeviceId>baddeviceid</DeviceId>"
            + "  <AppName>My Application Name</AppName>"
            + "  <AppName lang = 'es'>Mi Nombre de la aplicación</AppName>"
            + "  <Manufacturer>Company</Manufacturer>"
            + "  <Manufacturer lang = 'es'>Empresa</Manufacturer>"
            + "  <ModelNumber>Wxfy388i</ModelNumber>"
            + "  <Description>A detailed description provided by the application.</Description>"
            + "  <Description lang = 'es'>Una descripción detallada proporcionada por la aplicación.</Description>" /*TODO look into utf8*/
            + "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
            + "  <SoftwareVersion>1.0.0</SoftwareVersion>"
            + "  <HardwareVersion>1.0.0</HardwareVersion>"
            + "  <SupportUrl>www.example.com</SupportUrl>"
            + "  <UserDefinedTag>Can only accept strings anything other than strings must be done using the AboutData Class SetField method</UserDefinedTag>"
            + "  <UserDefinedTag lang='es'>Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método AboutData Clase SetField</UserDefinedTag>"
            + "</AboutData>";
        try {
            aboutData.createFromXml(xml);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (SAXException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (IOException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String uuid = aboutData.getAppIdAsHexString();
          assertEquals("000102030405060708090A0B0C0D0E0C", uuid);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }
    }

 // This xml uses an hex encoded byte string to pass in the AppId.
    public void testCreateFromXml_missing_appid() {
        AboutData aboutData = new AboutData();
        String xml;
        xml = "<AboutData>"
            + "  <DefaultLanguage>en</DefaultLanguage>"
            + "  <DeviceName>My Device Name</DeviceName>"
            + "  <DeviceName lang = 'es'>Nombre de mi dispositivo</DeviceName>"
            + "  <DeviceId>baddeviceid</DeviceId>"
            + "  <AppName>My Application Name</AppName>"
            + "  <AppName lang = 'es'>Mi Nombre de la aplicación</AppName>"
            + "  <Manufacturer>Company</Manufacturer>"
            + "  <Manufacturer lang = 'es'>Empresa</Manufacturer>"
            + "  <ModelNumber>Wxfy388i</ModelNumber>"
            + "  <Description>A detailed description provided by the application.</Description>"
            + "  <Description lang = 'es'>Una descripción detallada proporcionada por la aplicación.</Description>" /*TODO look into utf8*/
            + "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
            + "  <SoftwareVersion>1.0.0</SoftwareVersion>"
            + "  <HardwareVersion>1.0.0</HardwareVersion>"
            + "  <SupportUrl>www.example.com</SupportUrl>"
            + "  <UserDefinedTag>Can only accept strings anything other than strings must be done using the AboutData Class SetField method</UserDefinedTag>"
            + "  <UserDefinedTag lang='es'>Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método AboutData Clase SetField</UserDefinedTag>"
            + "</AboutData>";
        try {
            aboutData.createFromXml(xml);
        } catch (BusException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (SAXException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (IOException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        }

        try {
            String uuid = aboutData.getAppIdAsHexString();
            assertEquals("000102030405060708090A0B0C0D0E0C", uuid);
            fail("expected error was not thrown.");
        } catch (BusException e) {
            assertEquals("About Field Not Found.", e.getMessage());
        }
    }

    // This xml uses an hex encoded byte string to pass in the AppId.
    public void testCreateFromXml_default_language_not_specified() {
        AboutData aboutData = new AboutData();
        String xml;
        xml = "<AboutData>"
            + "  <AppId>000102030405060708090A0B0C0D0E0C</AppId>"
            + "  <DeviceName>My Device Name</DeviceName>"
            + "  <DeviceName lang = 'en'>My Device Name</DeviceName>"
            + "  <DeviceName lang = 'es'>Nombre de mi dispositivo</DeviceName>"
            + "  <DeviceId>baddeviceid</DeviceId>"
            + "  <AppName>My Application Name</AppName>"
            + "  <AppName lang = 'es'>Mi Nombre de la aplicación</AppName>"
            + "  <Manufacturer>Company</Manufacturer>"
            + "  <Manufacturer lang = 'es'>Empresa</Manufacturer>"
            + "  <ModelNumber>Wxfy388i</ModelNumber>"
            + "  <Description>A detailed description provided by the application.</Description>"
            + "  <Description lang = 'es'>Una descripción detallada proporcionada por la aplicación.</Description>" /*TODO look into utf8*/
            + "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
            + "  <SoftwareVersion>1.0.0</SoftwareVersion>"
            + "  <HardwareVersion>1.0.0</HardwareVersion>"
            + "  <SupportUrl>www.example.com</SupportUrl>"
            + "  <UserDefinedTag>Can only accept strings anything other than strings must be done using the AboutData Class SetField method</UserDefinedTag>"
            + "  <UserDefinedTag lang='es'>Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método AboutData Clase SetField</UserDefinedTag>"
            + "</AboutData>";
        try {
            aboutData.createFromXml(xml);
            fail("expected error was not thrown.");
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (SAXException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (IOException e) {
            e.printStackTrace();
            fail("Unexpected error thrown");
        } catch (BusException e) {
            assertEquals("DefaultLanguage language tag not found.", e.getMessage());
        }

        try {
            String defaultLanguage = aboutData.getDefaultLanguage();
            fail("expected error was not thrown.");
            assertEquals("en", defaultLanguage);
        } catch (BusException e) {
            assertEquals("About Field Not Found.", e.getMessage());
        }

        try {
            String deviceName = aboutData.getDeviceName();
            assertEquals("My Device Name", deviceName);
        } catch (BusException e) {
            assertEquals("DefaultLanguage language tag not found.", e.getMessage());
        }
        try {
            String deviceName = aboutData.getDeviceName("en");
            assertEquals("My Device Name", deviceName);
        } catch (BusException e) {
            fail("expected error thrown.");
        }
        try {
            String deviceName = aboutData.getDeviceName("es");
            assertEquals("Nombre de mi dispositivo", deviceName);
        } catch (BusException e) {
            fail("expected error thrown.");
        }
    }
}
