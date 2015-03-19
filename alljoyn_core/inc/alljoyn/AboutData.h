/**
 * @file
 * This contains the AboutData class responsible for holding the org.alljoyn.About
 * interface data fields.
 */
/******************************************************************************
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
 ******************************************************************************/
#ifndef _ALLJOYN_ABOUTDATA_H
#define _ALLJOYN_ABOUTDATA_H

#include <alljoyn/AboutDataListener.h>
#include <alljoyn/AboutKeys.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/Status.h>

#include <qcc/String.h>

namespace ajn {

/**
 * AboutData is responsible for holding the org.alljoyn.about interface Data fields
 */
class AboutData : public AboutDataListener, public AboutKeys {
  public:
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
     */
    AboutData();

    /**
     * constructor
     *
     * @param defaultLanguage the default language for the AboutData fields
     */
    AboutData(const char* defaultLanguage);

    /**
     * constructor
     *
     * Fill in the fields of the AboutData class using a MsgArg.  The provided
     * MsgArg must contain a dictionary with signature a{sv} with AboutData fields.
     *
     * If the passed in MsgArg is an ill formed AboutData MsgArg this constructor
     * will fail silently. If the MsgArg does not come from About Announce signal
     * it is best to create an empty AboutData class and use the CreatFromMsgArg
     * member function to fill in the AboutData class.
     *
     * @param arg MsgArg with signature a{sv}containing AboutData fields.
     * @param language the language of the arg MsgArg. Use NULL for default language
     */
    AboutData(const MsgArg arg, const char* language = NULL);

    /**
     * Copy constructor
     *
     * @param src The AboutData instance to be copied
     */
    AboutData(const AboutData& src);

    /**
     * Assignment operator
     *
     * @param src The AboutDate instance to be assigned
     *
     * @return copy of the AboutData src
     */
    AboutData& operator=(const AboutData& src);

    /**
     * Destructor
     */
    ~AboutData();

    /**
     * use an xml representation of AboutData to set the about data.
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
       "  <Manufacturer lang = 'es'>Empresa</Manufacturer>"
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
     * The CreateFromXml function will attempt to process the entire xml passed
     * in.  If an error is encountered it will continue to try and process the
     * xml. If multiple errors are encountered the last error is returned.
     *
     * Note: AJSoftwareVersion is automatically set to the version of Alljoyn that
     * is being used. The SupportedLanguages tag is automatically implied from
     * the DefaultLanguage tag and the lang annotation from tags that are
     * localizable.
     *
     * @param[in] aboutDataXml a string that contains an XML representation of
     *                         the AboutData fields.
     * @return
     *   - ER_OK on success
     *   - ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD if the XML representation
     *     did not include all required AboutData fields.
     *   - ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if a localizable value was
     *     was found with out the `lang` attribute and the DefaultLanguage
     *     field is missing.
     */
    QStatus CreateFromXml(const qcc::String& aboutDataXml);

    /**
     * use an xml representation of AboutData to set the about data.
       @code
       "<AboutData>"
       "  <AppId>000102030405060708090A0B0C0D0E0C</AppId>"
       "  <DefaultLanguage>en</DefaultLanguage>"
       "  <DeviceName>My Device Name</DeviceName>"
       "  <DeviceName lang = 'sp'>Nombre de mi dispositivo</DeviceName>"
       "  <DeviceId>93c06771-c725-48c2-b1ff-6a2a59d445b8</DeviceId>"
       "  <AppName>My Application Name</AppName>"
       "  <AppName lang = 'sp'>Mi Nombre de la aplicacion</AppName>"
       "  <Manufacturer>Company</Manufacturer>"
       "  <Manufacturer lang = 'sp'>Empresa</Manufacturer>"
       "  <ModelNumber>Wxfy388i</ModelNumber>"
       "  <Description>A detailed description provided by the application.</Description>"
       "  <Description lang = 'sp'>Una descripcion detallada proporcionada por la aplicacion.</Description>"
       "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
       "  <SoftwareVersion>1.0.0</SoftwareVersion>"
       "  <HardwareVersion>1.0.0</HardwareVersion>"
       "  <SupportUrl>www.example.com</SupportUrl>"
       "</AboutData>"
       @endcode
     *
     * The CreateFromXml function will attempt to process the entire xml passed
     * in.  If an error is encountered it will continue to try and process the
     * xml. If multiple errors are encountered the last error is returned.
     *
     * Note: AJSoftwareVersion is automatically set to the version of Alljoyn that
     * is being used. The SupportedLanguages tag is automatically implied from
     * the DefaultLanguage tag and the lang annotation from tags that are
     * localizable.
     *
     * @param[in] aboutDataXml a string that contains an XML representation of
     *                         the AboutData fields.
     * @return
     *   - ER_OK on success
     *   - ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD if the XML representation
     *     did not include all required AboutData fields.
     *   - ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if a localizable value was
     *     was found with out the `lang` attribute and the DefaultLanguage
     *     field is missing.
     */
    QStatus CreateFromXml(const char* aboutDataXml);

    /**
     * The AboutData has all of the required fields
     *
     * If a language field is given this will return if all required fields are
     * listed for the given language.
     *
     * If no language is given default language will be checked
     *
     * @param[in] language IETF language tags specified by RFC 5646
     *
     * @return true if all required field are listed for the given language
     */
    bool IsValid(const char* language = NULL);

    /**
     * Fill in the AboutData fields using a MsgArg
     *
     * The MsgArg must contain a dictionary of type a{sv} The expected use of this
     * class is to fill in the AboutData using a MsgArg obtain from the Announce
     * signal or the GetAboutData method from org.alljoyn.about interface.
     *
     * @param arg MsgArg contain AboutData dictionary
     * @param language the language for the MsgArg AboutData.
     *                 If NULL the default language will be used
     *
     * @return ER_OK on success
     */
    QStatus CreatefromMsgArg(const MsgArg& arg, const char* language = NULL);

    /**
     * Set the AppId for the AboutData
     *
     * AppId Should be a 128-bit UUID as specified in by RFC 4122.
     *
     * Passing in non-128-bit byte arrays will still Set the AppId but the
     * SetAppId member function will always return
     * ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE and the application will fail
     * certification and compliance testing.
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @param[in] appId the a globally unique array of bytes used as an ID for the application
     * @param[in] num   the number of bites in the appId array
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE if the AppId is not a 128-bits (16 bytes)
     */
    QStatus SetAppId(const uint8_t* appId, const size_t num = 16);

    /**
     * Get the AppId from the AboutData
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @param[out] appId a pointer to an array of bites used as a globally unique ID for an application
     * @param[out] num   the size of the appId array
     *
     * @return ER_OK on success
     */
    QStatus GetAppId(uint8_t** appId, size_t* num);

    /**
     * Set the AppId for the AboutData using a string.
     *
     * The string must be a ether a 32-character hex digit string
     * (i.e. 4a354637564945188a48323c158bc02d).
     * or a UUID string as specified in RFC 4122
     * (i.e. 4a354637-5649-4518-8a48-323c158bc02d)
     * AppId should be a 128-bit UUID as specified in by RFC 4122
     *
     * Unlike SetAppId(const uint8_t*, const size_t) this member function will
     * only set the AppId if the string is 32-character hex digit string or a
     * UUID as specified by RFC 4122.
     *
     * AppId IS required
     * AppId IS part of the Announce signal
     * AppId CAN NOT be localized for other languages
     *
     *
     * @see #SetAppId(const uint8_t*, const size_t)
     *
     * @param[in] appId String representing a globally unique array of bytes
     *                  used as an ID for the application.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE if the AppId is not a 128-bits (16 bytes)
     *  - #ER_ABOUT_INVALID_ABOUTDATA_FIELD_VALUE if unable to parse the appId string.
     */
    QStatus SetAppId(const char* appId);

    /**
     * This language is automatically added to the SupportedLanguage list. The
     * language tag should be an IETF language tag specified by RFC 5646
     *
     * DefaultLanguage is Required
     * DefaultLanguage is part of the Announce signal
     *
     * @param[in] defaultLanguage the IETF language tag
     *
     * @return ER_OK on success
     */
    QStatus SetDefaultLanguage(const char* defaultLanguage);

    /**
     * Get the DefaultLanguage from the AboutData
     *
     * @param[out] defaultLanguage a pointer to the default language tag
     *
     * @return ER_OK on success
     */
    QStatus GetDefaultLanguage(char** defaultLanguage);
    /**
     * Set the DeviceName to the AboutData
     *
     * DeviceName is not required
     * DeviceName is part of the Announce signal
     * DeviceName can be localized for other languages
     *
     * @param[in] deviceName the deviceName (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the DeviceName will be set for the default language.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if language tag was not specified
     *                                             and the default language is also
     *                                             not found.
     */
    QStatus SetDeviceName(const char* deviceName, const char* language = NULL);

    /**
     * Get the DeviceName from the About data
     *
     * DeviceName is not required
     * DeviceName is part of the Announce signal
     * DeviceName can be localized for other languages
     *
     * @param[out] deviceName the deviceName found in the AboutData (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the DeviceName for the default language will be returned.
     *
     * @return ER_OK on success
     */
    QStatus GetDeviceName(char** deviceName, const char* language = NULL);

    /**
     * Set the DeviceId from the AboutData
     *
     * DeviceId IS required
     * DeviceId IS part of the announce signal
     * DeviceId CAN NOT be localized for other languages
     *
     * @param[in] deviceId is a string with a value generated using platform specific means
     *
     * @return ER_OK on success
     */
    QStatus SetDeviceId(const char* deviceId);
    /**
     * Get the DeviceId from the AboutData
     *
     * DeviceId IS required
     * DeviceId IS part of the announce signal
     * DeviceId CAN NOT be localized for other languages
     *
     * @param[out] deviceId is a string with a value generated using platform specific means
     *
     * @return ER_OK on success
     */
    QStatus GetDeviceId(char** deviceId);
    /**
     * Set the AppName to the AboutData
     *
     * AppName is required
     * AppName is part of the announce signal
     * AppName can be localized for other languages
     *
     * @param[in] appName the AppName (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the AppName will be set for the default language.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if language tag was not specified
     *                                             and the default language is also
     *                                             not found.
     */
    QStatus SetAppName(const char* appName, const char* language = NULL);

    /**
     * Get the AppName from the About data
     *
     * AppName is required
     * AppName is part of the announce signal
     * AppName can be localized for other languages
     *
     * @param[out] appName the AppName found in the AboutData (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the AppName for the default language will be returned.
     *
     * @return ER_OK on success
     */
    QStatus GetAppName(char** appName, const char* language = NULL);

    /**
     * Set the Manufacture to the AboutData
     *
     * Manufacture is required
     * Manufacture is part of the announce signal
     * Manufacture can be localized for other languages
     *
     * @param[in] manufacturer the Manufacturer (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the Manufacture will be set for the default language.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if language tag was not specified
     *                                             and the default language is also
     *                                             not found.
     */
    QStatus SetManufacturer(const char* manufacturer, const char* language = NULL);

    /**
     * Get the Manufacturer from the About data
     *
     * Manufacture is required
     * Manufacture is part of the announce signal
     * Manufacture can be localized for other languages
     *
     * @param[out] manufacturer the Manufacturer found in the AboutData (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the Manufacturer for the default language will be returned.
     *
     * @return ER_OK on success
     */
    QStatus GetManufacturer(char** manufacturer, const char* language = NULL);

    /**
     * Set the ModelNumber to the AboutData
     *
     * ModelNumber is required
     * ModelNumber is part of the announce signal
     * ModelNumber can not be localized for other languages
     *
     * @param[in] modelNumber the application model number
     *
     * @return ER_OK on success
     */
    QStatus SetModelNumber(const char* modelNumber);

    /**
     * Get the ModelNumber from the AboutData
     *
     * ModelNumber IS required
     * ModelNumber IS part of the announce signal
     * ModelNumber CAN NOT be localized for other languages
     *
     * @param[out] modelNumber the application model number
     *
     * @return ER_OK on success
     */
    QStatus GetModelNumber(char** modelNumber);

    /**
     * Set a supported language.
     *
     * This is a string representing the a single language. The language is
     * specified using IETF language tags specified by the RFC 5646.
     *
     * If the language tag has already been added ER_OK will be returned with no
     * additional changes being made.
     *
     * @param[in] language the IETF language tag
     *
     * @return ER_OK on success
     */
    QStatus SetSupportedLanguage(const char* language);

    /**
     * Get and array of supported languages
     *
     * @param languageTags A pointer to a languageTags array to receive the
     *                     language tags. Can be NULL in which case no
     *                     language tags are returned and the return value gives
     *                     the number of language tags available.
     * @param num          the size of the languageTags array.
     *
     * @return The number of languageTags returned or the total number of
     *         language tags if languageTags is NULL.
     */
    size_t GetSupportedLanguages(const char** languageTags = NULL, size_t num = 0);

    /**
     * Set the Description to the AboutData
     *
     * Description IS required
     * Description IS NOT part of the announce signal
     * Description CAN BE localized for other languages
     *
     * @param[in] description the Description (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the Description will be set for the default language.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if language tag was not specified
     *                                             and the default language is also
     *                                             not found.
     */
    QStatus SetDescription(const char* description, const char* language = NULL);

    /**
     * Get the Description from the About data
     *
     * Description IS required
     * Description IS NOT part of the announce signal
     * Description CAN BE localized for other languages
     *
     * @param[out] description the Description found in the AboutData (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the Description for the default language will be returned.
     *
     * @return ER_OK on success
     */
    QStatus GetDescription(char** description, const char* language = NULL);

    /**
     * Set the DatOfManufacture to the AboutData
     *
     * The date of manufacture using the format YYYY-MM-DD.  Known as XML
     * DateTime format.
     *
     * ModelNumber IS NOT required
     * ModelNumber IS NOT part of the announce signal
     * ModelNumber CAN NOT be localized for other languages
     *
     * @param[in] dateOfManufacture the date of manufacture using YYYY-MM-DD format
     *
     * @return ER_OK on success
     */
    QStatus SetDateOfManufacture(const char* dateOfManufacture);

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
     * @param[out] dateOfManufacture the date of manufacture using YYYY-MM-DD format
     *
     * @return ER_OK on success
     */
    QStatus GetDateOfManufacture(char** dateOfManufacture);

    /**
     * Set the SoftwareVersion to the AboutData
     *
     * SoftwareVersion IS required
     * SoftwareVersion IS NOT part of the announce signal
     * SoftwareVersion CAN NOT be localized for other languages
     *
     * @param[in] softwareVersion the software version for the OEM software
     *
     * @return ER_OK on success
     */
    QStatus SetSoftwareVersion(const char* softwareVersion);

    /**
     * Get the SoftwareVersion from the AboutData
     *
     * SoftwareVersion IS required
     * SoftwareVersion IS NOT part of the announce signal
     * SoftwareVersion CAN NOT be localized for other languages
     *
     * @param[out] softwareVersion the software version for the OEM software
     *
     * @return ER_OK on success
     */
    QStatus GetSoftwareVersion(char** softwareVersion);

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
     * @param[out] ajSoftwareVersion the current version of AllJoyn SDK utilized
     *             by the application
     *
     * @return ER_OK on success
     */
    QStatus GetAJSoftwareVersion(char** ajSoftwareVersion);

    /**
     * Set the HardwareVersion to the AboutData
     *
     * HardwareVersion IS NOT required
     * HardwareVersion IS NOT part of the announce signal
     * HardwareVersion CAN NOT be localized for other languages
     *
     * @param[in] hardwareVersion the device hardware version
     *
     * @return ER_OK on success
     */
    QStatus SetHardwareVersion(const char* hardwareVersion);

    /**
     * Get the HardwareVersion from the AboutData
     *
     * HardwareVersion IS NOT required
     * HardwareVersion IS NOT part of the announce signal
     * HardwareVersion CAN NOT be localized for other languages
     *
     * @param[out] hardwareVersion the device hardware version
     *
     * @return ER_OK on success
     */
    QStatus GetHardwareVersion(char** hardwareVersion);
    /**
     * Set the SupportUrl to the AboutData
     *
     * SupportUrl IS NOT required
     * SupportUrl IS NOT part of the announce signal
     * SupportUrl CAN NOT be localized for other languages
     *
     * @param[in] supportUrl the support URL to be populated by OEM
     *
     * @return ER_OK on success
     */
    QStatus SetSupportUrl(const char* supportUrl);

    /**
     * Get the SupportUrl from the AboutData
     *
     * SupportUrl IS NOT required
     * SupportUrl IS NOT part of the announce signal
     * SupportUrl CAN NOT be localized for other languages
     *
     * @param[out] supportUrl the support URL
     *
     * @return ER_OK on success
     */
    QStatus GetSupportUrl(char** supportUrl);

    /**
     * generic way to Set new field.  Everything could be done this way.
     *
     * Unless the generic field is one of the pre-defined fields when they are
     * set they will have the following specifications
     *   NOT required
     *   NOT part of the announce signal
     *   CAN be localized
     *
     * Since every field can be localized even if the field is not localized it
     * must be set for every language.
     *
     * @param[in] name the name of the field to set
     * @param[in] value a MsgArg that contains the value that is set for the field
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the default language will be used.  Only
     *            used for fields that are marked as localizable.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_ABOUT_DEFAULT_LANGUAGE_NOT_SPECIFIED if language tag was not specified
     *                                             and the default language is also
     *                                             not found.
     */
    QStatus SetField(const char* name, MsgArg value, const char* language = NULL);

    /**
     * generic way to get field.
     *
     * @param[in] name the name of the field to get
     * @param[out] value MsgArg holding a variant value that represents the field
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the field for the default language will be
     *            returned.
     *
     * @return ER_OK on success
     */
    QStatus GetField(const char* name, MsgArg*& value, const char* language = NULL);

    /**
     * Get a list of the fields contained in this AboutData class.  This may be
     * required if a the AboutData comes from a remote source. User defined
     * fields are permitted. Use the GetFields method to get a list of all fields
     * currently found known by the AboutData.
     *
     * @param[out] fields an array of const char* that will contain all the strings
     * @param[in]  num_fields the size of the array
     *
     * @return
     *  The number of fields returned or the total number of fields if the fields parameter is NULL
     */
    size_t GetFields(const char** fields = NULL, size_t num_fields = 0) const;

    /**
     * @param[out] msgArg a the dictionary containing all of the AboutData fields for
     *                    the specified language.  If language is not specified the default
     *                    language will be returned
     * @param[in] language IETF language tags specified by RFC 5646 if the string
     *                     is NULL or an empty string the MsgArg for the default
     *                     language will be returned
     *
     * @return
     *  - ER_OK on successful
     *  - ER_LANGUAGE_NOT_SUPPORTED if language is not supported
     *  - ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD if a required field is missing
     *  - other error indicating failure
     */
    QStatus GetAboutData(MsgArg* msgArg, const char* language = NULL);

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
     *
     * @param[out] msgArg a MsgArg dictionary with the a{sv} that contains the Announce
     *                    data.
     * @return ER_OK if successful
     */
    QStatus GetAnnouncedAboutData(MsgArg* msgArg);

    /**
     * Is the given field name required to make an About announcement
     *
     * @param[in] fieldName the name of the field
     *
     * @return
     *  - true if the field is required to make an About announcement
     *  - false otherwise.  If the fieldName is unknown false will be returned
     */
    bool IsFieldRequired(const char* fieldName);

    /**
     * Is the given field part of the announce signal
     *
     * @param[in] fieldName the name of the field
     *
     * @return
     *  - true if the field is part of the announce signal
     *  - false otherwise.  If the fieldName is unknown false will be returned
     */
    bool IsFieldAnnounced(const char* fieldName);

    /**
     * Is the given field a localized field.
     *
     * Localized fields should be provided for every supported language
     *
     * @param[in] fieldName the name of the field
     *
     * @return
     *  - true if the field is a localizable value
     *  - false otherwise.  If the fieldName is unknown false will be returned.
     */
    bool IsFieldLocalized(const char* fieldName);

    /**
     * Get the signature for the given field.
     *
     * @param[in] fieldName the name of the field
     *
     * @return
     *  - the signature of the field
     *  - NULL is field is unknown
     */
    const char* GetFieldSignature(const char* fieldName);

    /*
     * we should add a way to generate a UUID based on RFC-4122. This should probably
     * be a in common since generation of the RFC-4122 id is not really core to alljoyn
     * its self.
     */
  protected:
    /**
     * typedef for byte mask used to specify properties of an AboutData field
     * entry
     */
    typedef uint8_t AboutFieldMask;
    /**
     * The AboutData field is not required, announced, or localized.
     */
    static const AboutFieldMask EMPTY_MASK = 0;
    /**
     * The AboutData field is required.
     */
    static const AboutFieldMask REQUIRED = 1;
    /**
     * The AboutData field is announced.
     */
    static const AboutFieldMask ANNOUNCED = 2;
    /**
     * The AboutData field is localized.
     */
    static const AboutFieldMask LOCALIZED = 4;
    /**
     * Derived classes have the ability to fully specify their own AboutData
     * including requirements that can not be changed using the base class.  The
     * derived class can specify if a value is required or optional, the value
     * is part of the announce signal or must be read using the GetAboutData
     * method. Specify if the value is a localizable value or not.
     *
     * @param[in] fieldName the Field Name being set
     * @param[in] fieldMask byte mask indicating if this field is required to
     *                      send the announce signal, if this field is part of
     *                      the `Announce` signal, and if this field contains a
     *                      string that should be localized.
     * @param[in] signature the data type held by the field this is a MsgArg
     *                      signature
     * @return
     *     - #ER_OK on success
     *     - #ER_ABOUT_FIELD_ALREADY_SPECIFIED if that field has already been specified
     */
    QStatus SetNewFieldDetails(const char* fieldName, AboutFieldMask fieldMask, const char* signature);

    /**
     * Holds information for each AboutData field.
     *
     * Each AboutData field must specify four pieces of information
     *  - required  This is a boolean field if this is `true` then the field is
     *              required for the AboutData to be a Valid.
     *  - announced If this field is marked as `true` then the field will be part
     *              of the Announce signal
     *  - localized This field contains a value that can be localized into
     *              multiple languages/regions
     *  - signature The signature of the underlying MsgArg dictionary value.
     *
     */
    typedef struct FieldDetails {
        /**
         * Create an uninitialized FieldDetails struct
         */
        FieldDetails() : fieldMask(EMPTY_MASK) { }
        /**
         * Create an initialized FieldDetails struct
         * @param[in] fieldMask a bitmask value indicating the properties of the field
         * @param[in] s the value for the signature field
         */
        FieldDetails(AboutFieldMask fieldMask, const char* s) : fieldMask(fieldMask), signature(s) { }
        /**
         * Mask indicating if the field is required, announced, or localized
         */
        AboutFieldMask fieldMask;

        /**
         * The signature of the underlying MsgArg dictionary value.
         */
        qcc::String signature;
    } FieldDetails;

  private:
    /**
     * Initialize the Field Details with the values specified in the About Feature
     * specification. This is called by each constructor.
     */
    void InitializeFieldDetails();

    /// @cond ALLJOYN_DEV
    /**
     * @internal
     * Class for internal state for AboutData.
     */
    class Internal;

    Internal* aboutDataInternal;     /**< Internal state information */
    /// @endcond ALLJOYN_DEV
};
}

#endif
