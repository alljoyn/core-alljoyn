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
import java.io.StringReader;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.UUID;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

public class AboutData implements AboutDataListener, AboutKeys {

    /**
     * Create an AboutData class. The default language will will not be set.
     * Use the constructor that takes a default language tag; or set the
     * language using the SetDefaultLanguage member function, CreateFromMsgArg
     * member function or the CreateFromXml member function.
     *
     * The default language should be specified before any tag that requires
     * localization. These tags are.
     *  - DeviceName
     *  - AppName
     *  - Manufacturer
     *  - Description
     * @throws BusException
     */
    AboutData() {
        initializeFieldDetails();
        propertyStore = new HashMap<String, Variant>();
        localizedPropertyStore = new HashMap<String, Map<String, Variant>>();
        supportedLanguages = new TreeSet<String>(String.CASE_INSENSITIVE_ORDER);
        try {
            setField(ABOUT_AJ_SOFTWARE_VERSION, new Variant(Version.get()));
        } catch (BusException e) {
            assert false: "Failed to set the AllJoyn software version.";
        }
    }

    /**
     * constructor
     *
     * @param defaultLanguage a Locale containing a IETF language tag specified
     *                        by RFC 5646 specifying the default language for the
     *                        AboutData fields
     * @throws BusException
     */
    AboutData(String defaultLanguage) {
        initializeFieldDetails();
        propertyStore = new HashMap<String, Variant>();
        localizedPropertyStore = new HashMap<String, Map<String, Variant>>();
        supportedLanguages = new TreeSet<String>(String.CASE_INSENSITIVE_ORDER);
        try {
            setField(ABOUT_AJ_SOFTWARE_VERSION, new Variant(Version.get()));
        } catch (BusException e) {
            assert false: "Failed to set the AllJoyn software version.";
        }
        try {
            setField(ABOUT_DEFAULT_LANGUAGE, new Variant(defaultLanguage));
        } catch (BusException e) {
            assert false: "Failed to set the default language.";
        }
    }

    /**
     * constructor
     *
     * All Localized tags will be set to the default language.
     *
     * @param aboutData Map containing the AboutData dictionary
     *
     * @throws BusException
     */
    AboutData(Map<String, Variant> aboutData) throws BusException {
        createFromAnnoncedAboutData(aboutData, null);
    }

    /**
     * constructor
     *
     * @param aboutData Map containing the AboutData dictionary
     * @param language a Locale containing a IETF language tag specified by RFC 5646
     *
     * @throws BusException
     */

    AboutData(Map<String, Variant> aboutData, String language) throws BusException {
        createFromAnnoncedAboutData(aboutData, language);
    }
    /**
     * use xml definition of AboutData to set the about data.
       @code
       "<AboutData>"
       "  <AppId>000102030405060708090A0B0C0D0E0C</AppId>"
       "  <DefaultLanguage>en</DefaultLanguage>"
       "  <DeviceName>My Device Name</DeviceName>"
       "  <DeviceName lang = 'es'>Nombre de mi dispositivo</DeviceName>"
       "  <DeviceId>93c06771-c725-48c2-b1ff-6a2a59d445b8</DeviceId>"
       "  <AppName>My Application Name</AppName>"
       "  <AppName lang = 'es'>Mi Nombre de la aplicacion</AppName>"
       "  <Manufacturer>Company</Manufacturer>"
       "  <Manufacturer lang = 'sp'>Empresa</Manufacturer>"
       "  <ModelNumber>Wxfy388i</ModelNumber>"
       "  <Description>A detailed description provided by the application.</Description>"
       "  <Description lang = 'es'>Una descripcion detallada proporcionada por la aplicacion.</Description>"
       "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
       "  <SoftwareVersion>1.0.0</SoftwareVersion>"
       "  <HardwareVersion>1.0.0</HardwareVersion>"
       "  <SupportUrl>www.example.com</SupportUrl>"
       "</AboutData>"
       @endcode
     *
     * Note: AJSoftwareVersion is automatically set to the version of Alljoyn that
     * is being used. The SupportedLanguages tag is automatically implied from
     * the DefaultLanguage tag and the lang annotation from tags that are
     * localizable.
     *
     * @param aboutDataXml a string that contains an XML representation of
     *                     the AboutData fields.
     * @return ER_OK on success
     * @throws BusException Indicating failure to parse the xml or failure to
     *                      add one or more fields.
     */
    void createFromXml(String aboutDataXml) throws BusException {

        //DOMParser builder = new DOMParser();
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        DocumentBuilder builder = null;
        try {
            builder = factory.newDocumentBuilder();
        } catch (ParserConfigurationException e1) {
            // TODO Auto-generated catch block
            e1.printStackTrace();
        }
        InputSource inputSource = new InputSource();
        inputSource.setCharacterStream(new StringReader(aboutDataXml));
        try {
            Document doc = builder.parse(inputSource);
            //Document doc = parser.getDocument();
            // First process all fields that are not localized so we can get the
            // default language
            for(String field : aboutFields.keySet()) {
                // Supported languages are implicitly added no need to look for a
                // SupportedLanguages languages tag.
                if (field == AboutData.ABOUT_SUPPORTED_LANGUAGES) {
                    continue;
                }
                // The Alljoyn software version is implicitly added so we don't need to
                // look for this tag
                if (field == AboutData.ABOUT_AJ_SOFTWARE_VERSION) {
                    continue;
                }
                // We only expect to see one tag for non-localized values.
                // So we automatically take the first field seen by the parser.
                if(!isFieldLocalized(field)) {
                    try {
                        NodeList nl = doc.getElementsByTagName(field);
                        if(field == AboutData.ABOUT_APP_ID) {
                            try {
                                setAppId(UUID.fromString(nl.item(0).getTextContent()));
                            } catch(IllegalArgumentException e) {
                                setAppId(nl.item(0).getTextContent());
                            } catch (BusException e) {
                                setAppId(nl.item(0).getTextContent());
                            }
                        } else {
                            setField(field, new Variant(nl.item(0).getTextContent()));
                        }
                    // if a element is not found it will throw a NullPointerException
                    // we will continue to process any tags that are found.
                    } catch(NullPointerException e) {
                        continue;
                    } catch (DOMException e) {
                        throw new BusException("Failed to parse XML");
                    }
                }
            }
            // At this point we should have the default language if its not set
            // we will throw a BusException if the 'lang' attribute is not set.
            org.w3c.dom.NodeList nl = doc.getElementsByTagName("*");
            for (int i = 0; i < nl.getLength(); ++i) {
                // If the Tag is unknown is will be added using the default rules
                // if the Tag is already known we check to see if its a localized
                // field.  If the field is localized we process the tag. If its
                // not localized we ignore the tag since it was processed above
                if (!aboutFields.containsKey(nl.item(i).getNodeName()) || isFieldLocalized(nl.item(i).getNodeName())) {
                    // if the 'lang' attribute is found us the language
                    // specified.  Otherwise use the default language
                    if (nl.item(i).getAttributes().getNamedItem("lang") != null) {
                        setField(nl.item(i).getNodeName(), new Variant(nl.item(i).getTextContent()), nl.item(i).getAttributes().getNamedItem("lang").getNodeValue());
                    } else {
                        setField(nl.item(i).getNodeName(), new Variant(nl.item(i).getTextContent()));
                    }
                }
            }
        } catch (SAXException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    /**
     * The AboutData has all of the required fields
     *
     * If no language is given default language will be checked
     *
     * @param language a Locale containing a IETF language tag specified by RFC 5646
     *
     * @return true if all required field are listed for the given language
     */
    public boolean isValid(String language) {
        if (language == null || language.length() == 0) {
            try {
                language = propertyStore.get(ABOUT_DEFAULT_LANGUAGE).getObject(String.class);
            } catch (BusException e) {
                return false;
            }
        }

        if (!supportedLanguages.contains(language)) {
            return false;
        }

        for (String s: aboutFields.keySet()) {
            if(isFieldRequired(s)) {
                if (isFieldLocalized(s)) {
                    if (!localizedPropertyStore.containsKey(s) ||!localizedPropertyStore.get(s).containsKey(language)) {
                        return false;
                    }
                } else {
                    if (!propertyStore.containsKey(s)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    /**
     * The AboutData has all of the required fields.
     *
     * Only fields for the default language will be checked.
     *
     * @return true if all required field are listed for the given language
     */
    public boolean isValid() {
        return isValid(null);
    }

    /**
     * Fill in the AboutData fields using a String/Variant Map.
     *
     * The MsgArg must contain a dictionary of type a{sv} The expected use of this
     * class is to fill in the AboutData using a Map obtain from the Announce
     * signal or the GetAboutData method from org.alljoyn.about interface.
     *
     * @param aboutData Map containing the AboutData dictionary
     * @param language a Locale containing a IETF language tag specified by RFC 5646
     *
     * @throws BusException indicating that the aboutData Map is missing a
     *                      required field. Typically this means the default
     *                      language was not specified.
     */
    public void createFromAnnoncedAboutData(Map<String, Variant> aboutData, String language) throws BusException {
        if (language == null || language.length() == 0) {
            try {
                language = aboutData.get(ABOUT_DEFAULT_LANGUAGE).getObject(String.class);
            } catch (NullPointerException e) {
                throw new ErrorReplyBusException(Status.ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD);
            }
        }

        if (language == null || language.length() == 0) {
            new ErrorReplyBusException(Status.ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD);
        }

        for (String s : aboutData.keySet()) {
            setField(s, aboutData.get(s), language);
        }
    }

    /**
     * Fill in the AboutData fields using a String/Variant Map.
     *
     * All Localized tags will be set to the default language.
     *
     * The MsgArg must contain a dictionary of type a{sv} The expected use of this
     * class is to fill in the AboutData using a Map obtain from the Announce
     * signal or the GetAboutData method from org.alljoyn.about interface.
     *
     * @param aboutData Map containing the AboutData dictionary
     *
     * @throws BusException indicating that the aboutData Map is missing a
     *                      required field. Typically this means the default
     *                      language was not specified.
     */
    public void createFromAnnouncedAboutData(Map<String, Variant> aboutData) throws BusException {
        createFromAnnoncedAboutData(aboutData, null);
    }

    /**
     * Set the AppId for the AboutData
     *
     * AppId Should be a 128-bit UUID as specified in by RFC 4122.
     *
     * Passing in non-128-bit byte arrays will still Set the AppId but the
     * setAppId member function will always throw a BusException indicating
     * the AppId is not 128-bits (16-bytes).
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @param appId the a globally unique array of bytes used as an ID for the
     *              application
     *
     * @throws BusException if the AppId is not a 128-bit field
     */
    public void setAppId(byte[] appId) throws BusException {
        setField(ABOUT_APP_ID, new Variant(appId));
        if (appId.length != 16) {
            throw new BusException("AppId is not 128-bits. AppId passed in is still used.");
        }
    }

    /**
     * Get the AppId from the AboutData
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @return bytes representing a unique AppId
     *
     * @throws BusException indicating failure to find the AppId
     */
    public byte[] getAppId() throws BusException {
        return getField(ABOUT_APP_ID).getObject(byte[].class);
    }

    /**
     * Set the AppId for the AboutData using a hex encoded String.
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @param appId Hex encoded String representing a globally unique array of bytes used
     *              as an ID for the application.
     *
     * @throws BusException indicating failure to set the AppId
     */
    public void setAppId(String appId) throws BusException {
        setAppId(hexStringToByteArray(appId));
    }

    private static byte[] hexStringToByteArray(String hexString) throws BusException {
        if ( (hexString.length() % 2) != 0) {
            throw new BusException("Failed to parse AppId.");
        }
        byte[] byteArray = new byte[hexString.length()/2];
        for (int i = 0; i < hexString.length(); i +=2) {
            byteArray[i/2] = (byte)((Character.digit(hexString.charAt(i), 16) << 4) +
                                Character.digit(hexString.charAt(i + 1), 16));
        }
        return byteArray;
    }

    public String getAppIdAsHexString() throws BusException {
        byte[] appId = getField(ABOUT_APP_ID).getObject(byte[].class);
        return byteArrayToHexString(appId);
    }

    final protected static char[] hexCharArray = "0123456789ABCDEF".toCharArray();

    private static String byteArrayToHexString(byte[] byteArray) {
        char[] hexChars = new char[byteArray.length * 2];
        for (int i = 0; i < byteArray.length; ++i) {
            int x = byteArray[i] & 0xff;
            hexChars[i*2] = hexCharArray[x >> 4];
            hexChars[i*2 + 1] = hexCharArray[x & 0x0f];
        }
        return new String(hexChars);
    }

    /**
     * Set the AppId for the AboutData using a UUID.
     *
     * Unlike setAppId(byte[]) this member function will only set the AppId if
     * a UUID is specified.
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @param appId UUID representing a globally unique array of bytes used
     *              as an ID for the application.
     *
     * @throws BusException indicating failure to set the AppId
     */
    public void setAppId(UUID appId) throws BusException {
        byte[] appIdBytes = new byte[16];
        ByteBuffer byteBuffer = ByteBuffer.wrap(appIdBytes);
        byteBuffer.putLong(appId.getMostSignificantBits());
        byteBuffer.putLong(appId.getLeastSignificantBits());
        setAppId(appIdBytes);
    }

    /**
     * Get the AppId UUID from the AboutData
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @return the UUID for the AppId
     *
     * @throws BusException indicating failure to find the AppId
     */
    public UUID getAppIdAsUUID() throws BusException {
        ByteBuffer byteBuffer = ByteBuffer.wrap(getField(ABOUT_APP_ID).getObject(byte[].class));
        return new UUID(byteBuffer.getLong(), byteBuffer.getLong());
    }

    /**
     * Set the default language.
     *
     * This Locale language tag is automatically added to the SupportedLanguage
     * list. The Locale language tag should be an IETF language tag specified by
     * RFC 5646.
     *
     * DefaultLanguage is Required
     * DefaultLanguage is part of the Announce signal
     *
     * @param defaultLanguage Locale specifying the IETF language tag for the
     *                        default language
     *
     * @throws BusException indicating error when setting the default language
     */
    public void setDefaultLanguage(String defaultLanguage) throws BusException {
        setField(ABOUT_DEFAULT_LANGUAGE, new Variant(defaultLanguage));
    }

    /**
     * Get the DefaultLanguage from the AboutData
     *
     * @return Locale indicating the default language.
     * @throws BusException indicating failure to get the default language
     */
     public String getDefaultLanguage() throws BusException {
         return getField(ABOUT_DEFAULT_LANGUAGE).getObject(String.class);
     }

    /**
     * Set the DeviceName to the AboutData
     *
     * DeviceName is not required
     * DeviceName is part of the Announce signal
     * DeviceName can be localized for other languages
     *
     * @param deviceName the deviceName (UTF8 encoded string)
     * @param language Locale language tag specified by RFC 5646 if language is
     *                 null the DeviceName will be set for the default language.
     *
     * @throws BusException indicating failure to set the device name
     */
    public void setDeviceName(String deviceName, String language) throws BusException {
        setField(ABOUT_DEVICE_NAME, new Variant(deviceName), language);
    }

    /**
     * Set the DeviceName to the AboutData for the default language
     *
     * Default language must be set.
     *
     * DeviceName is not required
     * DeviceName is part of the Announce signal
     * DeviceName can be localized for other languages
     *
     * @param deviceName deviceName the deviceName (UTF8 encoded string)
     *
     * @throws BusException indicating failure to set the device name
     */
    public void setDeviceName(String deviceName) throws BusException {
        setDeviceName(deviceName, null);
    }

    /**
     * Get the DeviceName from the About data
     *
     * DeviceName is not required
     * DeviceName is part of the Announce signal
     * DeviceName can be localized for other languages
     *
     * @param language Locale for the IETF language tag specified by RFC 5646
     *                 if language is null the DeviceName for the default
     *                 language will be returned.
     *
     * @return the deviceName found in the AboutData
     * @throws BusException error indicating failure to obtain the device name
     */
    public String getDeviceName(String language) throws BusException {
        return getField(ABOUT_DEVICE_NAME, language).getObject(String.class);
    }

    public String getDeviceName() throws BusException {
        return getDeviceName(null);
    }

    /**
     * Set the DeviceId from the AboutData
     *
     * DeviceId IS required
     * DeviceId IS part of the announce signal
     * DeviceId CAN NOT be localized for other languages
     *
     * @param deviceId is a string with a value generated using platform specific
     *                 means
     *
     * @throws BusException indicating failure to obtain the device id.
     */
    public void setDeviceId(String deviceId) throws BusException {
        setField(ABOUT_DEVICE_ID, new Variant(deviceId));
    }
    /**
     * Get the DeviceId from the AboutData
     *
     * DeviceId IS required
     * DeviceId IS part of the announce signal
     * DeviceId CAN NOT be localized for other languages
     *
     * @return the DeviceId string
     * @throws BusException indicating failure to find device id.
     */
    public String getDeviceId() throws BusException {
        return getField(ABOUT_DEVICE_ID).getObject(String.class);
    }

    /**
     * Set the AppName to the AboutData
     *
     * AppName is required
     * AppName is part of the announce signal
     * AppName can be localized for other languages
     *
     * @param appName the AppName (UTF8 encoded string)
     * @param language Locale for the IETF language tag specified by RFC 5646
     *                 if language is null the AppName will be set for the
     *                 default language.
     *
     * @throws BusException indicating failure to set the AppName.
     */
    public void setAppName(String appName, String language) throws BusException {
        setField(ABOUT_APP_NAME, new Variant(appName), language);
    }

    /**
     * Set the AppName to the AboutData for the default language.
     *
     * AppName is required
     * AppName is part of the announce signal
     * AppName can be localized for other languages
     *
     * @param appName the AppName (UTF8 encoded string)
     *
     * @throws BusException indicating failure to set the AppName.
     */
    public void setAppName(String appName) throws BusException {
        setAppName(appName, null);
    }

    /**
     * Get the AppName from the About data
     *
     * AppName is required
     * AppName is part of the announce signal
     * AppName can be localized for other languages
     *
     * @param language Locale for the IETF language tag specified by RFC 5646
     *                 if language is null the AppName for the default language
     *                 will be returned.
     *
     * @return The application name
     * @throws BusException indicates failure to obtain the application name.
     */
    public String getAppName(String language) throws BusException {
        return getField(ABOUT_APP_NAME, language).getObject(String.class);
    }

    /**
     * Get the AppName from the About data for the default language
     *
     * AppName is required
     * AppName is part of the announce signal
     * AppName can be localized for other languages
     *
     * @return The application name
     * @throws BusException indicates failure to obtain the application name.
     */
    public String getAppName() throws BusException {
        return getAppName(null);
    }

    /**
     * Set the Manufacture for the AboutData
     *
     * Manufacture is required
     * Manufacture is part of the announce signal
     * Manufacture can be localized for other languages
     *
     * @param manufacturer the Manufacturer (UTF8 encoded string)
     * @param language Locale containing the IETF language tag specified by
     *                 RFC 5646 if language is null the Manufacture will be set
     *                 for the default language.
     *
     * @throws BusException unable to set the Manufacture
     */
    public void setManufacturer(String manufacturer, String language) throws BusException {
        setField(ABOUT_MANUFACTURER, new Variant(manufacturer), language);
    }

    /**
     * Set the Manufacture for the AboutData for the default language.
     *
     * Manufacture is required
     * Manufacture is part of the announce signal
     * Manufacture can be localized for other languages
     *
     * @param manufacturer the Manufacturer (UTF8 encoded string)
     *
     * @throws BusException unable to set the Manufacture
     */
    public void setManufacturer(String manufacturer) throws BusException {
        setManufacturer(manufacturer, null);
    }

    /**
     * Get the Manufacturer from the About data.
     *
     * Manufacture is required
     * Manufacture is part of the announce signal
     * Manufacture can be localized for other languages
     *
     * @param language Locale containing the IETF language tag specified by
     *                 RFC 5646 if language is null the Manufacturer for the
     *                 default language will be returned.
     *
     * @return The Manufacture for the specified language
     * @throws BusException indicating failure to obtain the manufacturer name
     */
    public String getManufacturer(String language) throws BusException {
        return getField(ABOUT_MANUFACTURER, language).getObject(String.class);
    }

    /**
     * Get the Manufacturer from the About data for the default language.
     *
     * Manufacture is required
     * Manufacture is part of the announce signal
     * Manufacture can be localized for other languages
     *
     * @return The Manufacture for the default language.
     * @throws BusException indicating failure to obtain the manufacturer name
     */
    public String getManufacturer() throws BusException {
        return getManufacturer(null);
    }

    /**
     * Set the ModelNumber to the AboutData
     *
     * ModelNumber is required
     * ModelNumber is part of the announce signal
     * ModelNumber can not be localized for other languages
     *
     * @param modelNumber the application model number
     *
     * @throws BusException indicating failure to set the model number
     */
    public void setModelNumber(String modelNumber) throws BusException {
        setField(ABOUT_MODEL_NUMBER, new Variant(modelNumber));
    }

    /**
     * Get the ModelNumber from the AboutData
     *
     * ModelNumber IS required
     * ModelNumber IS part of the announce signal
     * ModelNumber CAN NOT be localized for other languages
     *
     * @return the model number
     * @throws BusException indicating failure to obtain the model number
     */
    public String getModelNumber() throws BusException {
        return getField(ABOUT_MODEL_NUMBER).getObject(String.class);
    }

    /**
     * Set a supported language.
     *
     * This is a Locale representing the a single language. The language is
     * specified in a Locale using IETF language tags specified by the RFC 5646.
     *
     * If the language tag has already been added no error will be thrown. The
     * method will returned with no changes being made.
     *
     * @param language Locale containing the IETF language tag
     *
     * @throws BusException indicating failure to set the language tag.
     */
    public void setSupportedLanguage(String language) throws BusException {
        supportedLanguages.add(language);
        setField(ABOUT_SUPPORTED_LANGUAGES, new Variant(supportedLanguages.toArray(new String[supportedLanguages.size()])));
    }

    /**
     * Set supported languages.
     *
     * This is an array of Locals representing the supported languages. The
     * languages are specified in a Locals using IETF language tags specified
     * by the RFC 5646.
     *
     * If a language tag has already been added no error will be thrown. The
     * method will continue to add other languages in the array.
     *
     * @param languages An array of Locals containing the IETF language tag
     *
     * @throws BusException indicating failure to set the language tags.
     */
    public void setSupportedLanguages(String[] languages) throws BusException {
        supportedLanguages.addAll(Arrays.asList(languages));
        setField(ABOUT_SUPPORTED_LANGUAGES, new Variant(supportedLanguages.toArray(new String[supportedLanguages.size()])));
    }
    /**
     * Get and array of supported languages
     *
     * @return An array of locals containing languageTags.
     * @throws BusException indicating failure obtaining language tags
     */
    public String[] getSupportedLanguages() throws BusException {
        return getField(ABOUT_SUPPORTED_LANGUAGES).getObject(String[].class);
    }


    /**
     * Set the Description to the AboutData
     *
     * Description IS required
     * Description IS NOT part of the announce signal
     * Description CAN BE localized for other languages
     *
     * @param description the Description (UTF8 encoded string)
     * @param language Locale containing the IETF language tag specified by
     *                 RFC 5646 if language is null the Description will be set
     *                 for the default language.
     *
     * @throws BusException indicating failure to set the description.
     */
    public void setDescription(String description, String language) throws BusException {
        setField(ABOUT_DESCRIPTION, new Variant(description), language);
    }

    /**
     * Set the Description to the AboutData for the default language.
     *
     * Description IS required
     * Description IS NOT part of the announce signal
     * Description CAN BE localized for other languages
     *
     * @param description the Description (UTF8 encoded string)
     *
     * @throws BusException indicating failure to set the description.
     */
    public void setDescription(String description) throws BusException {
        setDescription(description, null);
    }

    /**
     * Get the Description from the About data
     *
     * Description IS required
     * Description IS NOT part of the announce signal
     * Description CAN BE localized for other languages
     *
     * @param language the IETF language tag specified by RFC 5646
     *        if language is NULL the Description for the default language will be returned.
     *
     * @return The description.
     * @throws BusException indicating failure to get the description
     */
    public String getDescription(String language) throws BusException {
        return getField(ABOUT_DESCRIPTION, language).getObject(String.class);
    }

    /**
     * Get the Description from the About data for the default language.
     *
     * Description IS required
     * Description IS NOT part of the announce signal
     * Description CAN BE localized for other languages
     *
     * @return The description for the default language.
     * @throws BusException indicating failure to get the description
     */
    public String getDescription() throws BusException {
        return getDescription(null);
    }

    /**
     * Set the DateOfManufacture to the AboutData
     *
     * The date of manufacture using the format YYYY-MM-DD.  Known as XML
     * DateTime format.
     *
     * ModelNumber IS NOT required
     * ModelNumber IS NOT part of the announce signal
     * ModelNumber CAN NOT be localized for other languages
     *
     * @param dateOfManufacture the date of manufacture using YYYY-MM-DD format
     *
     * @throws BusException indicating failure to set the date of manufacture
     */
    public void setDateOfManufacture(String dateOfManufacture) throws BusException {
        setField(ABOUT_DATE_OF_MANUFACTURE, new Variant(dateOfManufacture));
    }

    /**
     * Get the DatOfManufacture from the AboutData
     *
     * The date of manufacture using the format YYYY-MM-DD.  Known as XML
     * DateTime format.
     *
     * ModelNumber IS NOT required
     * ModelNumber IS NOT part of the announce signal
     * ModelNumber CAN NOT be localized for other languages
     *
     * @return the date of manufacture
     * @throws BusException indicating failure to get the date of manufacture.
     */
    public String getDateOfManufacture() throws BusException {
        return getField(ABOUT_DATE_OF_MANUFACTURE).getObject(String.class);
    }

    /**
     * Set the SoftwareVersion to the AboutData
     *
     * SoftwareVersion IS required
     * SoftwareVersion IS NOT part of the announce signal
     * SoftwareVersion CAN NOT be localized for other languages
     *
     * @param softwareVersion the software version for the OEM software
     *
     * @throws BusException indicating failure to set the software version.
     */
    public void setSoftwareVersion(String softwareVersion) throws BusException {
        setField(ABOUT_SOFTWARE_VERSION, new Variant(softwareVersion));
    }

    /**
     * Get the SoftwareVersion from the AboutData
     *
     * SoftwareVersion IS required
     * SoftwareVersion IS NOT part of the announce signal
     * SoftwareVersion CAN NOT be localized for other languages
     *
     * @return The software version
     * @throws BusException indicating failure to get the software version.
     */
    public String getSoftwareVersion() throws BusException {
        return getField(ABOUT_SOFTWARE_VERSION).getObject(String.class);
    }

    /**
     * Get the AJSoftwareVersion from the AboutData
     *
     * The AJSoftwareVersion is automatically set when the AboutData is created
     * or when it is read from remote device.
     *
     * ModelNumber IS required
     * ModelNumber IS NOT part of the announce signal
     * ModelNumber CAN NOT be localized for other languages
     *
     * @return the AllJoyn software version
     * @throws BusException indicating failure to get the AllJoyn software version
     */
    public String getAJSoftwareVersion() throws BusException {
        return getField(ABOUT_AJ_SOFTWARE_VERSION).getObject(String.class);
    }

    /**
     * Set the HardwareVersion to the AboutData
     *
     * HardwareVersion IS NOT required
     * HardwareVersion IS NOT part of the announce signal
     * HardwareVersion CAN NOT be localized for other languages
     *
     * @param hardwareVersion the device hardware version
     *
     * @throws BusException indicating failure to set the hardware version
     */
    public void setHardwareVersion(String hardwareVersion) throws BusException {
        setField(ABOUT_HARDWARE_VERSION, new Variant(hardwareVersion));
    }

    /**
     * Get the HardwareVersion from the AboutData
     *
     * HardwareVersion IS NOT required
     * HardwareVersion IS NOT part of the announce signal
     * HardwareVersion CAN NOT be localized for other languages
     *
     * @return The hardware version
     * @throws BusException indicating failure to read the hardware version.
     */
    public String getHardwareVersion() throws BusException {
        return getField(ABOUT_HARDWARE_VERSION).getObject(String.class);
    }

    /**
     * Set the SupportUrl to the AboutData
     *
     * SupportUrl IS NOT required
     * SupportUrl IS NOT part of the announce signal
     * SupportUrl CAN NOT be localized for other languages
     *
     * @param supportUrl the support URL to be populated by OEM
     *
     * @throws BusException indicating failure to set the support URL
     */
    public void setSupportUrl(String supportUrl) throws BusException {
        setField(ABOUT_SUPPORT_URL, new Variant(supportUrl));
    }

    /**
     * Get the SupportUrl from the AboutData
     *
     * SupportUrl IS NOT required
     * SupportUrl IS NOT part of the announce signal
     * SupportUrl CAN NOT be localized for other languages
     *
     * @return The support URL
     * @throws BusException indicating failure to get the support URL
     */
    public String getSupportUrl() throws BusException {
        return getField(ABOUT_SUPPORT_URL).getObject(String.class);
    }

    /**
     * generic way to Set new field.  Every field could be set this way.
     *
     * Unless the generic field is one of the pre-defined fields when they are
     * set they will have the following specifications
     *   NOT required
     *   NOT part of the announce signal
     *   CAN be localized if it is a string NOT localized otherwise
     *
     * Since every field can be localized even if the field is not localized it
     * must be set for every language.
     *
     * @param name     the name of the field to set
     * @param value    a MsgArg that contains the value that is set for the field
     * @param language The IETF language tag specified by RFC 5646 if language
     *                 is null the default language will be used.  Only used for
     *                 fields that are marked as localizable.
     *
     * @throws BusException indicating failure to set the field
     */
    public void setField(String name, Variant value, String language) throws BusException {
        // The user is adding an OEM specific field.
        // At this time OEM specific fields are added as
        //    not required
        //    not announced
        //    can be localized
        if (!aboutFields.containsKey(name)) {
            if (value.getSignature().equals("s")) {
            aboutFields.put(name, new FieldDetails(FieldDetails.LOCALIZED, value.getSignature()));
            } else {
                aboutFields.put(name, new FieldDetails(FieldDetails.EMPTY_MASK, value.getSignature()));
            }
        }
        if (name == ABOUT_DEFAULT_LANGUAGE) {
            setSupportedLanguage(value.getObject(String.class));
        }
        if (isFieldLocalized(name)) {
            if (language == null || language.length() == 0) {
                if (propertyStore.containsKey(ABOUT_DEFAULT_LANGUAGE)){
                    language = propertyStore.get(ABOUT_DEFAULT_LANGUAGE).getObject(String.class);
                } else {
                    throw new BusException("Specified language tag not found.");
                }
                if (!localizedPropertyStore.containsKey(name)) {
                    localizedPropertyStore.put(name, new TreeMap<String, Variant>(String.CASE_INSENSITIVE_ORDER));
                }
                localizedPropertyStore.get(name).put(language, value);
            } else {
                if (!localizedPropertyStore.containsKey(name)) {
                    localizedPropertyStore.put(name, new TreeMap<String, Variant>(String.CASE_INSENSITIVE_ORDER));
                }
                localizedPropertyStore.get(name).put(language, value);
                //implicitly add all language tags to the supported languages
                if(!supportedLanguages.contains(language)) {
                  setSupportedLanguage(language);
                }
            }
        } else {
            propertyStore.put(name, value);
        }
    }

    public void setField(String name, Variant value) throws BusException {
        setField(name, value, null);
    }

    /**
     * Generic way to get field.
     *
     * @param name     the name of the field to get
     * @param language Locale containing the IETF language tag specified by
     *                 RFC 5646 if language is NULL the field for the default
     *                 language will be returned.
     *
     * @return return a Variant that holds the requested field value.
     * @throws BusException indicating failure to get the requested field.
     */
    Variant getField(String name, String language) throws BusException {
        if (!propertyStore.containsKey(name) && !localizedPropertyStore.containsKey(name)) {
            throw new BusException("About Field Not Found.");
        }
        if (!isFieldLocalized(name)) {
            return propertyStore.get(name);
        } else {
            if (language == null || language.length() == 0) {
                language = propertyStore.get(ABOUT_DEFAULT_LANGUAGE).getObject(String.class);
            }
            if(!supportedLanguages.contains(language)) {
                throw new BusException("Specified language tag not found.");
            }
            return localizedPropertyStore.get(name).get(language);
        }
    }

    Variant getField(String name) throws BusException{
        return getField(name, null);
    }
    /**
     * Get a Set listing the fields contained in this AboutData class.  This may be
     * required if a the AboutData comes from a remote source. User defined
     * fields are permitted. Use the getFields method to get a list of all fields
     * currently known by the AboutData.
     *
     * @return
     *  Set containing a list of all known fields in the AboutData class.
     */
    Set<String> getFields() {
        return aboutFields.keySet();
    }


    /**
     * Is the given field name required to make an About announcement
     *
     * @param fieldName the name of the field
     *
     * @return
     * <ul>
     *   <li> <code>true</code> if the field is required to make an About announcement</li>
     *   <li> <code>false</code> otherwise.  If the fieldName is unknown false will be returned</li>
     * </ul>
     */
    public boolean isFieldRequired(String fieldName) {
        try {
            return ((aboutFields.get(fieldName).fieldMask & FieldDetails.REQUIRED) == FieldDetails.REQUIRED);
        } catch (NullPointerException e) {
            return false;
        }
    }

    /**
     * Is the given field part of the announce signal
     *
     * @param fieldName the name of the field
     *
     * @return
     * <ul>
     *   <li><code>true</code> if the field is part of the announce signal</li>
     *   <li><code>false</code> otherwise.  If the fieldName is unknown false will be returned</li>
     * </ul>
     */
    public boolean isFieldAnnounced(String fieldName) {
        try {
            return ((aboutFields.get(fieldName).fieldMask  & FieldDetails.ANNOUNCED) == FieldDetails.ANNOUNCED);
        } catch(NullPointerException e) {
            return false;
        }
    }

    /**
     * Is the given field a localized field.
     *
     * Localized fields should be provided for every supported language.
     *
     * @param fieldName the name of the field
     *
     * @return
     * <ul>
     *   <li><code>true</code> if the field is a localizable value</li>
     *   <li><code>false</code> otherwise.  If the fieldName is unknown false will be returned.</li>
     * </ul>
     */
    public boolean isFieldLocalized(String fieldName) {
        try {
            return ((aboutFields.get(fieldName).fieldMask  & FieldDetails.LOCALIZED) == FieldDetails.LOCALIZED);
        } catch(NullPointerException e) {
            return false;
        }
    }

    /**
     * Get the signature for the given field.
     *
     * @param fieldName the name of the field
     *
     * @return
     * <ul>
     *   <li>the signature of the field</li>
     *   <li><code>null</code> means field is unknown</li>
     * </ul>
     */
    String getFieldSignature(String fieldName) {
        try {
            return aboutFields.get(fieldName).signature;
        } catch(NullPointerException e) {
            return null;
        }
    }

    /**
     * @param language IETF language tags specified by RFC 5646 if the string
     *                     is NULL or an empty string the MsgArg for the default
     *                     language will be returned
     *
     * @return Map containing AboutData key/value pairs.
     * @throws ErrorReplyBusException
     * <ul>
     *   <li>ErrorReplyBusException LANGUAGE_NOT_SUPPORTED if language is not supported
     *   <li>ErrorReplyBusException ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD if a required field is missing
     * </ul>
     */
    @Override
    public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException {
        if (language == null || language.length() == 0) {
            try {
                language = propertyStore.get(ABOUT_DEFAULT_LANGUAGE).getObject(String.class);
            } catch (BusException e) {
                throw new ErrorReplyBusException(Status.LANGUAGE_NOT_SUPPORTED);
            }
        }

        if (!supportedLanguages.contains(language)) {
            throw new ErrorReplyBusException(Status.LANGUAGE_NOT_SUPPORTED);
        }

        for (String s: aboutFields.keySet()) {
            if(isFieldRequired(s)) {
                if (isFieldLocalized(s)) {
                    if (!localizedPropertyStore.containsKey(s) ||!localizedPropertyStore.get(s).containsKey(language)) {
                        throw new ErrorReplyBusException(Status.ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD);
                    }
                } else {
                    if (!propertyStore.containsKey(s)) {
                        throw new ErrorReplyBusException(Status.ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD);
                    }
                }
            }
        }

        Map<String, Variant> aboutData = new HashMap<String, Variant>(propertyStore);
        for (String s : localizedPropertyStore.keySet()) {
            if (localizedPropertyStore.get(s).containsKey(language)){
                aboutData.put(s, localizedPropertyStore.get(s).get(language));
            }
        }
        return aboutData;
    }

    /**
     * Return a MsgArg pointer containing dictionary containing the AboutData that
     * is announced with the org.alljoyn.About.announce signal.
     * This will always be the default language and will only contain the fields
     * that are announced.
     *
     * The fields that will be part of the announced MsgArg are:
     *  - AppId
     *  - DefaultLanguage
     *  - DeviceName
     *  - DeviceId
     *  - AppName
     *  - Manufacture
     *  - ModelNumber
     *
     * If you require other fields or need the localized AboutData
     *   The org.alljoyn.About.GetAboutData method can be used.
     * @return Map containing the announced AboutData key/value pairs.
     * @throws ErrorReplyBusException
     * <ul>
     *   <li>ErrorReplyBusException LANGUAGE_NOT_SUPPORTED if language is not supported
     *   <li>ErrorReplyBusException ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD if a required field is missing
     * </ul>
     */
    @Override
    public Map<String, Variant> getAnnouncedAboutData()
            throws ErrorReplyBusException {
        String language;
        try {
            language = propertyStore.get(ABOUT_DEFAULT_LANGUAGE).getObject(String.class);
        } catch (BusException e) {
            throw new ErrorReplyBusException(Status.LANGUAGE_NOT_SUPPORTED);
        }

        if (!supportedLanguages.contains(language)) {
            new ErrorReplyBusException(Status.LANGUAGE_NOT_SUPPORTED);
        }

        for (String s: aboutFields.keySet()) {
            if(isFieldRequired(s)) {
                if (isFieldLocalized(s)) {
                    if (!localizedPropertyStore.containsKey(s) ||!localizedPropertyStore.get(s).containsKey(language)) {
                        new ErrorReplyBusException(Status.ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD);
                    }
                } else {
                    if (!propertyStore.containsKey(s)) {
                        new ErrorReplyBusException(Status.ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD);
                    }
                }
            }
        }

        Map<String, Variant> aboutData = new HashMap<String, Variant>();
        for (String s: aboutFields.keySet()) {
            if(isFieldAnnounced(s)) {
                if (isFieldLocalized(s)) {
                    if (!localizedPropertyStore.containsKey(s) ||!localizedPropertyStore.get(s).containsKey(language)) {
                        aboutData.put(s, localizedPropertyStore.get(s).get(language));
                    }
                } else {
                    if (!propertyStore.containsKey(s)) {
                        aboutData.put(s, propertyStore.get(s));
                    }
                }
            }
        }
        return aboutData;
    }

    protected class FieldDetails {
        public static final int EMPTY_MASK = 0x00;
        public static final int REQUIRED = 0x01;
        public static final int ANNOUNCED = 0x02;
        public static final int LOCALIZED = 0x04;

        FieldDetails(int aboutFieldMask, String signature){
            fieldMask = aboutFieldMask;
            this.signature = signature;
        }
        int fieldMask;
        String signature;
    }

    protected void setNewFieldDetails(String fieldName, int aboutFieldMask, String signature) {
        aboutFields.put(fieldName, new FieldDetails(aboutFieldMask, signature));
    }

    private void initializeFieldDetails() {
        aboutFields = new HashMap<String, FieldDetails>();
        aboutFields.put(ABOUT_APP_ID, new FieldDetails(FieldDetails.REQUIRED | FieldDetails.ANNOUNCED, "ay"));
        aboutFields.put(ABOUT_DEFAULT_LANGUAGE, new FieldDetails(FieldDetails.REQUIRED | FieldDetails.ANNOUNCED, "s"));
        aboutFields.put(ABOUT_DEVICE_NAME, new FieldDetails(FieldDetails.ANNOUNCED | FieldDetails.LOCALIZED, "s"));
        aboutFields.put(ABOUT_DEVICE_ID, new FieldDetails(FieldDetails.REQUIRED | FieldDetails.ANNOUNCED, "s"));
        aboutFields.put(ABOUT_APP_NAME, new FieldDetails(FieldDetails.REQUIRED | FieldDetails.ANNOUNCED | FieldDetails.LOCALIZED, "s"));
        aboutFields.put(ABOUT_MANUFACTURER, new FieldDetails(FieldDetails.REQUIRED | FieldDetails.ANNOUNCED | FieldDetails.LOCALIZED, "s"));
        aboutFields.put(ABOUT_MODEL_NUMBER, new FieldDetails(FieldDetails.REQUIRED | FieldDetails.ANNOUNCED, "s"));
        aboutFields.put(ABOUT_SUPPORTED_LANGUAGES, new FieldDetails(FieldDetails.REQUIRED, "as"));
        aboutFields.put(ABOUT_DESCRIPTION, new FieldDetails(FieldDetails.REQUIRED | FieldDetails.LOCALIZED, "s"));
        aboutFields.put(ABOUT_DATE_OF_MANUFACTURE, new FieldDetails(FieldDetails.EMPTY_MASK, "s"));
        aboutFields.put(ABOUT_SOFTWARE_VERSION, new FieldDetails(FieldDetails.REQUIRED, "s"));
        aboutFields.put(ABOUT_AJ_SOFTWARE_VERSION, new FieldDetails(FieldDetails.REQUIRED, "s"));
        aboutFields.put(ABOUT_HARDWARE_VERSION, new FieldDetails(FieldDetails.EMPTY_MASK, "s"));
        aboutFields.put(ABOUT_SUPPORT_URL, new FieldDetails(FieldDetails.EMPTY_MASK, "s"));
    }
    private Set<String> supportedLanguages;
    private Map<String, Variant> propertyStore;
    private Map<String, Map<String, Variant>> localizedPropertyStore;
    private Map<String, FieldDetails> aboutFields;
}
