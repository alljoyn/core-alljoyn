/*
 * Copyright (c) 2014 AllSeen Alliance. All rights reserved.
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

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import junit.framework.TestCase;

public class AboutDataTest extends TestCase{
    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment serviceBus;
    static short PORT_NUMBER = 542;

    public String ABOUT_REQUIRED_FIELDS[] = {
            "AppId",
            "DefaultLanguage",
            "DeviceId",
            "ModelNumber",
            "SupportedLanguages",
            "SoftwareVersion",
            "AJSoftwareVersion",
            "AppName",
            "Manufacturer",
            "Description"
    };

    public String ABOUT_ANNOUNCED_FIELDS[] = {
            "AppId",
            "DefaultLanguage",
            "DeviceId",
            "ModelNumber",
            "DeviceName",
            "AppName",
            "Manufacturer"
    };

    public synchronized void stopWait() {
        this.notifyAll();
    }

    public void setUp() throws Exception {
        serviceBus = new BusAttachment("AboutListenerTestService");
        assertEquals(Status.OK, serviceBus.connect());

        AboutDataTestSessionPortListener listener = new AboutDataTestSessionPortListener();
        short contactPort = PORT_NUMBER;
        Mutable.ShortValue sessionPort = new Mutable.ShortValue(contactPort);
        assertEquals(Status.OK, serviceBus.bindSessionPort(sessionPort, new SessionOpts(), listener));
    }

    public void tearDown() throws Exception {
        serviceBus.disconnect();
        serviceBus.release();
    }

    public class AboutDataTestSessionPortListener extends SessionPortListener {
        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
            System.out.println("SessionPortListener.acceptSessionJoiner called");
            if (sessionPort == PORT_NUMBER) {
                return true;
            } else {
                return false;
            }
        }
        public void sessionJoined(short sessionPort, int id, String joiner) {
            System.out.println(String.format("SessionPortListener.sessionJoined(%d, %d, %s)", sessionPort, id, joiner));
            sessionId = id;
            sessionEstablished = true;
        }

        int sessionId;
        boolean sessionEstablished;
    }
    // Helper class to check if about data and announced data valid
    public class AboutDataCheck
    {
        private boolean isValid = false;
        // Are required fields all set?
        private boolean areRequiredFieldsSet(Map<String, Variant> aboutData)
        {
            boolean allSet = true;

            for (String fieldName: ABOUT_REQUIRED_FIELDS)
            {
                if (!aboutData.containsKey(fieldName))
                {
                    System.out.println("AboutData miss required field " + fieldName);
                    allSet = false;
                    break;
                }
            }

            return allSet;
        }

        // Are announced fields all set?
        private boolean areAnnouncedFieldsSet(Map<String, Variant> announceData)
        {
            boolean allSet = true;

            for (String fieldName: ABOUT_ANNOUNCED_FIELDS)
            {
                if (!announceData.containsKey(fieldName))
                {
                    System.out.println("AboutData miss announced field " + fieldName);
                    allSet = false;
                    break;
                }
            }

            return allSet;
        }

        // Is announced data and getAboutData consistent
        private boolean isDataConsistent(Map<String, Variant> aboutData,Map<String, Variant> announceData)
        {
            boolean isConsistent = true;
            Set<String> announcedFields = announceData.keySet();

            for (String announcedField: announcedFields)
            {
                // AppId equals does not work
                if (!announcedField.equals("AppId") && aboutData.containsKey(announcedField))
                {
                    // Check value match
                    Variant announcedValue = announceData.get(announcedField);
                    Variant aboutValue = aboutData.get(announcedField);

                    if (!announcedValue.equals(aboutValue))
                    {
                        isConsistent = false;
                        System.out.println(announcedField + " mismatch: " + announcedValue + " vs " + aboutValue);
                        break;
                    }
                }
            }

            return isConsistent;
        }

        // Is data valid
        public boolean isDataValid(Map<String, Variant> aboutData,
                Map<String, Variant> announceData)
        {
            isValid = false;

            // Check if any required field is missing
            if (areRequiredFieldsSet(aboutData))
            {
                // Check if any announced field is missing
                if (areAnnouncedFieldsSet(announceData))
                {
                    // Check if any mismatch between about value and announced value
                    if (isDataConsistent(aboutData, announceData))
                    {
                        isValid = true;
                    }
                }
            }

            return isValid;
        }
    }

    /*
     * Correct About data
     */
    public class AboutData implements AboutDataListener {
        private Map<String, Variant> aboutData = new HashMap<String, Variant>();
        private Map<String, Variant> announceData = new HashMap<String, Variant>();

        private void setAboutData(String language)
        {
            //nonlocalized values
            aboutData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            aboutData.put("DefaultLanguage",  new Variant(new String("en")));
            aboutData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
            aboutData.put("SupportedLanguages", new Variant(new String[] {"en", "es"}));
            aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
            aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
            aboutData.put("AJSoftwareVersion", new Variant(new String("0.0.1")));
            aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
            //localized values
            // If the language String is null or an empty string we return the
            // default language
            if ((language == null) || (language.length() == 0) || language.equalsIgnoreCase("en")) {
                aboutData.put("DeviceName", new Variant(new String("A device name")));
                aboutData.put("AppName", new Variant(new String("An application name")));
                aboutData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
                aboutData.put("Description", new Variant(new String("Sample showing the about feature in a service application")));
            } else if (language.equalsIgnoreCase("es")) { //Spanish
                aboutData.put("DeviceName", new Variant(new String("Un nombre de dispositivo")));
                aboutData.put("AppName", new Variant(new String("Un nombre de aplicación")));
                aboutData.put("Manufacturer", new Variant(new String("Una empresa de fabricación de poderosos")));
                aboutData.put("Description", new Variant(new String("Muestra que muestra la característica de sobre en una aplicación de servicio")));
            }
        }

        private void setAnnouncedData()
        {
            announceData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            announceData.put("DefaultLanguage",  new Variant(new String("en")));
            announceData.put("DeviceName", new Variant(new String("A device name")));
            announceData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            announceData.put("AppName", new Variant(new String("An application name")));
            announceData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
            announceData.put("ModelNumber", new Variant(new String("A1B2C3")));
        }

        public AboutData(String language)
        {
            setAboutData(language);
            setAnnouncedData();
        }

        @Override
        public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException
        {
            return aboutData;
        }

        @Override
        public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException
        {
            return announceData;
        }

    }

    /*
     * Invalid about data since required field DeviceId is missing
     */
    public class AboutDataMissingRequiredField implements AboutDataListener {

        private Map<String, Variant> aboutData = new HashMap<String, Variant>();
        private Map<String, Variant> announceData = new HashMap<String, Variant>();

        private void setAboutData(String language)
        {
            //nonlocalized values
            aboutData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            aboutData.put("DefaultLanguage",  new Variant(new String("en")));
            // Device Id is missing
            aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
            aboutData.put("SupportedLanguages", new Variant(new String[] {"en", "es"}));
            aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
            aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
            aboutData.put("AJSoftwareVersion", new Variant(new String("0.0.1")));
            aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
            //localized values
            // If the language String is null or an empty string we return the
            // default language
            if ((language == null) || (language.length() == 0) || language.equalsIgnoreCase("en")) {
                aboutData.put("DeviceName", new Variant(new String("A device name")));
                aboutData.put("AppName", new Variant(new String("An application name")));
                aboutData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
                aboutData.put("Description", new Variant(new String("Sample showing the about feature in a service application")));
            } else if (language.equalsIgnoreCase("es")) { //Spanish
                aboutData.put("DeviceName", new Variant(new String("Un nombre de dispositivo")));
                aboutData.put("AppName", new Variant(new String("Un nombre de aplicación")));
                aboutData.put("Manufacturer", new Variant(new String("Una empresa de fabricación de poderosos")));
                aboutData.put("Description", new Variant(new String("Muestra que muestra la característica de sobre en una aplicación de servicio")));
            }
        }

        private void setAnnouncedData()
        {
            announceData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            announceData.put("DefaultLanguage",  new Variant(new String("en")));
            announceData.put("DeviceName", new Variant(new String("A device name")));
            announceData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            announceData.put("AppName", new Variant(new String("An application name")));
            announceData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
            announceData.put("ModelNumber", new Variant(new String("A1B2C3")));
        }

        public AboutDataMissingRequiredField(String language)
        {
            setAboutData(language);
            setAnnouncedData();
        }

        @Override
        public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException
        {
            return aboutData;
        }

        @Override
        public Map<String, Variant> getAnnouncedAboutData()
        {
            return announceData;
        }

    }

    /*
     * Invalid announce data since announced field AppName is missing
     */
    public class AnnounceDataMissingRequiredField implements AboutDataListener {

        private Map<String, Variant> aboutData = new HashMap<String, Variant>();
        private Map<String, Variant> announceData = new HashMap<String, Variant>();

        private void setAboutData(String language)
        {
            //nonlocalized values
            aboutData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            aboutData.put("DefaultLanguage",  new Variant(new String("en")));
            aboutData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
            aboutData.put("SupportedLanguages", new Variant(new String[] {"en", "es"}));
            aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
            aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
            aboutData.put("AJSoftwareVersion", new Variant(new String("0.0.1")));
            aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
            //localized values
            // If the language String is null or an empty string we return the
            // default language
            if ((language == null) || (language.length() == 0) || language.equalsIgnoreCase("en")) {
                aboutData.put("DeviceName", new Variant(new String("A device name")));
                aboutData.put("AppName", new Variant(new String("An application name")));
                aboutData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
                aboutData.put("Description", new Variant(new String("Sample showing the about feature in a service application")));
            } else if (language.equalsIgnoreCase("es")) { //Spanish
                aboutData.put("DeviceName", new Variant(new String("Un nombre de dispositivo")));
                aboutData.put("AppName", new Variant(new String("Un nombre de aplicación")));
                aboutData.put("Manufacturer", new Variant(new String("Una empresa de fabricación de poderosos")));
                aboutData.put("Description", new Variant(new String("Muestra que muestra la característica de sobre en una aplicación de servicio")));
            }
        }

        private void setAnnouncedData()
        {
            announceData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            announceData.put("DefaultLanguage",  new Variant(new String("en")));
            announceData.put("DeviceName", new Variant(new String("A device name")));
            announceData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            //AppName is missing
            announceData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
            announceData.put("ModelNumber", new Variant(new String("A1B2C3")));
        }

        public AnnounceDataMissingRequiredField(String language)
        {
            setAboutData(language);
            setAnnouncedData();
        }

        @Override
        public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException
        {
            return aboutData;
        }

        @Override
        public Map<String, Variant> getAnnouncedAboutData()
        {
            return announceData;
        }

    }
    /*
     * Inconsistent ModelNumber value
     */
    public class InconsistentData implements AboutDataListener {

        private Map<String, Variant> aboutData = new HashMap<String, Variant>();
        private Map<String, Variant> announceData = new HashMap<String, Variant>();

        private void setAboutData(String language)
        {
            //nonlocalized values
            aboutData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            aboutData.put("DefaultLanguage",  new Variant(new String("en")));
            aboutData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            // Inconsistent with announcement
            aboutData.put("ModelNumber", new Variant(new String("D4B2C3")));
            aboutData.put("SupportedLanguages", new Variant(new String[] {"en", "es"}));
            aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
            aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
            aboutData.put("AJSoftwareVersion", new Variant(new String("0.0.1")));
            aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
            //localized values
            // If the language String is null or an empty string we return the
            // default language
            if ((language == null) || (language.length() == 0) || language.equalsIgnoreCase("en")) {
                aboutData.put("DeviceName", new Variant(new String("A device name")));
                aboutData.put("AppName", new Variant(new String("An application name")));
                aboutData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
                aboutData.put("Description", new Variant(new String("Sample showing the about feature in a service application")));
            } else if (language.equalsIgnoreCase("es")) { //Spanish
                aboutData.put("DeviceName", new Variant(new String("Un nombre de dispositivo")));
                aboutData.put("AppName", new Variant(new String("Un nombre de aplicación")));
                aboutData.put("Manufacturer", new Variant(new String("Una empresa de fabricación de poderosos")));
                aboutData.put("Description", new Variant(new String("Muestra que muestra la característica de sobre en una aplicación de servicio")));
            }
        }

        private void setAnnouncedData()
        {
            announceData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
            announceData.put("DefaultLanguage",  new Variant(new String("en")));
            announceData.put("DeviceName", new Variant(new String("A device name")));
            announceData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            announceData.put("AppName", new Variant(new String("An application name")));
            announceData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
            // Inconsistent with About data value
            announceData.put("ModelNumber", new Variant(new String("A1B2C3")));
        }

        public InconsistentData(String language)
        {
            setAboutData(language);
            setAnnouncedData();
        }

        @Override
        public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException
        {
            return aboutData;
        }

        @Override
        public Map<String, Variant> getAnnouncedAboutData()
        {
            return announceData;
        }

    }
    class Intfa implements AboutListenerTestInterfaceA, BusObject {

        @Override
        public String echo(String str) throws BusException {
            return str;
        }
    }

    class TestAboutListener implements AboutListener {
        TestAboutListener() {
            remoteBusName = null;
            version = 0;
            port = 0;
            announcedFlag = false;
            aod = null;
        }
        public void announced(String busName, int version, short port, AboutObjectDescription[] objectDescriptions, Map<String, Variant> aboutData) {
            announcedFlag = true;
            remoteBusName = busName;
            this.version = version;
            this.port = port;
            aod = objectDescriptions;
            aod = new AboutObjectDescription[objectDescriptions.length];
            for (int i = 0; i < objectDescriptions.length; ++i) {
                aod[i] = objectDescriptions[i];
            }
            announcedAboutData = aboutData;
            stopWait();
        }
        public String remoteBusName;
        public int version;
        public short port;
        public boolean announcedFlag;
        public AboutObjectDescription[] aod;
        public Map<String,Variant> announcedAboutData;
    }

    /*
     * Positive test
     *  About data and announced data are correct in this test
     */
    public synchronized void testGoodData()
    {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        AboutObj aboutObj = new AboutObj(serviceBus);
        AboutData aboutData = new AboutData("en");
        AboutDataCheck dataChecker = new AboutDataCheck();

        try {
            assertTrue(dataChecker.isDataValid(
                    aboutData.getAboutData("en"),
                    aboutData.getAnnouncedAboutData()));
        } catch (ErrorReplyBusException e) {
            fail("About data check failed unexpectly!");
        }

        assertEquals(Status.OK, aboutObj.announce(PORT_NUMBER, aboutData));
    }

    /*
     * Negative test
     *  if any required field is missing from about data, announce should fail
     */
    public synchronized void testAboutDataMissingRequiredField() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        AboutObj aboutObj = new AboutObj(serviceBus);

        AboutDataMissingRequiredField aboutBadData = new AboutDataMissingRequiredField("en");
        AboutDataCheck dataChecker = new AboutDataCheck();

        try {
            assertFalse(dataChecker.isDataValid(
                    aboutBadData.getAboutData("en"),
                    aboutBadData.getAnnouncedAboutData()));
        } catch (ErrorReplyBusException e) {
            fail("About data check succeed unexpectly!");
        }

        assertEquals(Status.ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, aboutObj.announce(PORT_NUMBER, aboutBadData));
    }

    /*
     * Negative test
     *  if any announced field is missing from announce data, announce should fail
     */
    public synchronized void testAnnounceDataMissingRequiredField() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        AboutObj aboutObj = new AboutObj(serviceBus);

        AnnounceDataMissingRequiredField aboutBadData = new AnnounceDataMissingRequiredField("en");
        AboutDataCheck dataChecker = new AboutDataCheck();

        try {
            assertFalse(dataChecker.isDataValid(
                    aboutBadData.getAboutData("en"),
                    aboutBadData.getAnnouncedAboutData()));
        } catch (ErrorReplyBusException e) {
            fail("About data check succeed unexpectly!");
        }

        assertEquals(Status.ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, aboutObj.announce(PORT_NUMBER, aboutBadData));
    }

    /*
     * Negative test
     *  Announced value(ModelNumber etc.) is inconsistent from about value, announce should fail
     */
    public synchronized void testInconsistentValue() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        AboutObj aboutObj = new AboutObj(serviceBus);

        InconsistentData aboutBadData = new InconsistentData("en");
        AboutDataCheck dataChecker = new AboutDataCheck();

        try {
            assertFalse(dataChecker.isDataValid(
                    aboutBadData.getAboutData("en"),
                    aboutBadData.getAnnouncedAboutData()));
        } catch (ErrorReplyBusException e) {
            fail("About data check succeed unexpectly!");
        }

        assertEquals(Status.ABOUT_INVALID_ABOUTDATA_LISTENER, aboutObj.announce(PORT_NUMBER, aboutBadData));
    }
    
    /*
     * Everything is correct except the AppId is not 128-bits
     */
    public class AboutDataBadAppId implements AboutDataListener {
        private Map<String, Variant> aboutData = new HashMap<String, Variant>();
        private Map<String, Variant> announceData = new HashMap<String, Variant>();

        private void setAboutData(String language)
        {
            //nonlocalized values
            aboutData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}));
            aboutData.put("DefaultLanguage",  new Variant(new String("en")));
            aboutData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
            aboutData.put("SupportedLanguages", new Variant(new String[] {"en", "es"}));
            aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
            aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
            aboutData.put("AJSoftwareVersion", new Variant(new String("0.0.1")));
            aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
            //localized values
            // If the language String is null or an empty string we return the
            // default language
            if ((language == null) || (language.length() == 0) || language.equalsIgnoreCase("en")) {
                aboutData.put("DeviceName", new Variant(new String("A device name")));
                aboutData.put("AppName", new Variant(new String("An application name")));
                aboutData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
                aboutData.put("Description", new Variant(new String("Sample showing the about feature in a service application")));
            } else if (language.equalsIgnoreCase("es")) { //Spanish
                aboutData.put("DeviceName", new Variant(new String("Un nombre de dispositivo")));
                aboutData.put("AppName", new Variant(new String("Un nombre de aplicación")));
                aboutData.put("Manufacturer", new Variant(new String("Una empresa de fabricación de poderosos")));
                aboutData.put("Description", new Variant(new String("Muestra que muestra la característica de sobre en una aplicación de servicio")));
            }
        }

        private void setAnnouncedData()
        {
            announceData.put("AppId",  new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}));
            announceData.put("DefaultLanguage",  new Variant(new String("en")));
            announceData.put("DeviceName", new Variant(new String("A device name")));
            announceData.put("DeviceId",  new Variant(new String("sampleDeviceId")));
            announceData.put("AppName", new Variant(new String("An application name")));
            announceData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
            announceData.put("ModelNumber", new Variant(new String("A1B2C3")));
        }

        public AboutDataBadAppId(String language)
        {
            setAboutData(language);
            setAnnouncedData();
        }

        @Override
        public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException
        {
            return aboutData;
        }

        @Override
        public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException
        {
            return announceData;
        }

    }

    /*
     * Negative test ASACORE-1130
     *  AppId does not contain 128-bits
     */
    public synchronized void testValidateAboutDataFields_BadAppId() {
        Intfa intfa = new Intfa();
        assertEquals(Status.OK, serviceBus.registerBusObject(intfa, "/about/test"));

        AboutObj aboutObj = new AboutObj(serviceBus);

        AboutDataBadAppId aboutBadData = new AboutDataBadAppId("en");
        AboutDataCheck dataChecker = new AboutDataCheck();

        try {
            assertTrue(dataChecker.isDataValid(
                    aboutBadData.getAboutData("en"),
                    aboutBadData.getAnnouncedAboutData()));
        } catch (ErrorReplyBusException e) {
            fail("About data check succeed unexpectly!");
        }

        assertEquals(Status.ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE, aboutObj.announce(PORT_NUMBER, aboutBadData));
    }
}
