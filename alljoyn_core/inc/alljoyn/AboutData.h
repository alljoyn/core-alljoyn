/**
 * @file
 * This contains the AboutData class responsible for holding the org.alljoyn.About
 * interface Data fields.
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
#ifndef _ALLJOYN_ABOUTDATA_H
#define _ALLJOYN_ABOUTDATA_H

#include <alljoyn/MsgArg.h>
#include <alljoyn/Status.h>

#include <map>
#include <vector>

#include <qcc/String.h>
#include <qcc/Mutex.h>


namespace ajn {

/**
 * AboutData is responsible for holding the org.alljoyn.about interface Data fields
 */
class AboutData {
  public:

    /**
     * constructor
     *
     * Any tag that requires a language requires that the default language is specified
     * for that reason you are strongly encouraged to use the constructor that allows
     * you to set the default language in the constructor.
     */
    AboutData();

    /**
     * constructor
     *
     * @param defaultLanguage the default language for the AboutData fields
     */
    AboutData(const char* defaultLanguage);

    /*
     * constructor
     *
     * Fill in the fields of the AboutData class using a MsgArg.  The provided MsgArg
     * must contain a dictionary with signature a{sv} with AboutData fields.
     * @param arg
     */
    //AboutData(const MsgArg arg);

    /**
     * Destructor
     */
    ~AboutData();

//    /*
//     * copy constructor
//     */
//    /*
//     * TODO: unknown at this point in time if this is need however since this will have
//     * dictonary of data its likely it will be needed.
//     */
//    AboutData(const AboutData& other);
//
//    /*
//     * assignment operator
//     *
//     * unknown at this point in time if this is need however since this will have
//     * dictonary of data its likely it will be needed.
//     */
//    AboutData& operator=(const AboutData& rhs);

//    /**
//     * this will provide an ID string used to identify the device.  This string
//     * is required to be unique to the device and should be constant for the specified
//     * device.
//     *
//     * @return the DeviceId
//     * TODO
//     */
//    static const char* GetDeviceID();

    /**
     * use xml definition of AboutData to set the about data.
       @code
       "<AboutData>"
       "  <AppId>000102030405060708090A0B0C0D0E0C</AppId>"
       "  <DefaultLanguage>en</DefaultLanguage>"
       "  <DeviceName>My Device Name</DeviceName>"
       "  <DeviceName lang = 'sp'>Nombre de mi dispositivo</DeviceName>"
       "  <DeviceId>" + About.GetDeviceId() +"</DeviceId>"
       "  <AppName>My Application Name</AppName>"
       "  <AppName lang = 'sp'>Mi Nombre de la aplicacion</AppName>"
       "  <Manufacturer>Company</Manufacturer>"
       "  <Manufacturer lang = 'sp'>Empresa</Manufacturer>"
       "  <ModelNumber>Wxfy388i</ModelNumber>"
       "  <SupportedLanguages>"
       "    <language>en</language>"
       "    <language>sp</language>"
       "  </SupportedLanguages>"
       "  <Description>A detailed description provided by the application.</Description>"
       "  <Description lang = 'sp'>Una descripcion detallada proporcionada por la aplicacion.</Description>"
       "  <DateOfManufacture>2014-01-08</DateOfManufacture>"
       "  <SoftwareVersion>1.0.0</SoftwareVersion>"
       "  <AJSoftwareVersion>"+ GetVersion() + "</AJSoftwareVersion>"
       "  <HardwareVersion>1.0.0</HardwareVersion>"
       "  <SupportUrl>www.example.com</SupportUrl>"
       "</AboutData>"
       @endcode
     * @param[in] aboutDataXml a string that contains an XML representation of
     *                         the AboutData fields.
     * @return ER_OK on success
     * TODO
     */
    QStatus CreateFromXml(qcc::String aboutDataXml);

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
    QStatus Initialize(const MsgArg& arg, const char* language = NULL);

    /**
     * Get the AppId from the AboutData
     *
     * AppId IS required
     * AppId IS part of the announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @param[in] appId the a globally unique array of bites used as an ID for the application
     * @param[in] num   the number of bites in the appId array
     *
     * @return ER_OK on success
     */
    QStatus SetAppId(const uint8_t* appId, const size_t num);

    /**
     * Get the AppId from the AboutData
     *
     * AppId IS required
     * AppId IS part of the announce signal
     * AppId CAN NOT be localized for other languages
     *
     * @param[out] appId a pointer to an array of bites used as a globally unique ID for an application
     * @param[out] num   the size of the appId array
     *
     * @return ER_OK on success
     */
    QStatus GetAppId(uint8_t** appId, size_t* num);

    /**
     * This language is automatically added to the SupportedLanguage list
     * Required, announced
     */
    QStatus SetDefaultLanguage(char* defaultLanguage);

    /**
     * This language is automatically added to the SupportedLanguage list
     * Required, announced
     */
    QStatus GetDefaultLanguage(char** defaultLanguage);
    /**
     * Set the DeviceName to the AboutData
     *
     * DeviceName is not required
     * DeviceName is part of the announced signal
     * DeviceName can be localized for other languages
     *
     * @param[in] deviceName the deviceName (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the DeviceName will be set for the default language.
     *
     * @return ER_OK on success
     */
    QStatus SetDeviceName(const char* deviceName, const char* language = NULL);

    /**
     * Get the DeviceName from the About data
     *
     * DeviceName is not required
     * DeviceName is part of the announce signal
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
     * @return ER_OK on success
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
     * @param[in] manufacture the Manufacture (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the Manufacture will be set for the default language.
     *
     * @return ER_OK on success
     */
    QStatus SetManufacture(const char* manufacture, const char* language = NULL);

    /**
     * Get the Manufacture from the About data
     *
     * Manufacture is required
     * Manufacture is part of the announce signal
     * Manufacture can be localized for other languages
     *
     * @param[out] manufacture the Manufacture found in the AboutData (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the Manufacture for the default language will be returned.
     *
     * @return ER_OK on success
     */
    QStatus GetManufacture(char** manufacture, const char* language = NULL);

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
     * This is a string representing the a single language. The language is
     * specified using IETF language tags specified by the RFC 5646.
     *
     * @param[in] language the IETF language tag
     *
     * @return ER_OK on success
     */
    QStatus SetSupportedLanguage(const char* language);

    /**
     * Get and array of supported languages
     *
     * @param languageTags a pointer to an array of IETF language tags
     * @param num the number of language tags in the array
     *
     * @return ER_OK on success
     * TODO
     */
    QStatus GetSupportedLanguages(qcc::String** languageTags, size_t* num);
    /**
     * Set the Description to the AboutData
     *
     * Description IS required
     * Description IS NOT part of the announce signal
     * Description CAN BE localized for other languages
     *
     * @param[in] descritption the Description (UTF8 encoded string)
     * @param[in] language the IETF language tag specified by RFC 5646
     *            if language is NULL the Description will be set for the default language.
     *
     * @return ER_OK on success
     */
    QStatus SetDescription(const char* descritption, const char* language = NULL);

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
     *            if language is NULL the field will be set for the default language
     *            will be used.
     *
     * @return ER_OK on success
     */
    QStatus SetField(const char* name, ajn::MsgArg value, const char* language = NULL);
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
    QStatus GetField(const char* name, ajn::MsgArg*& value, const char* language = NULL);

    /**
     * @param[out] msgArg a the dictionary containing all of the AboutData fields for
     *                    the specified language.  If language is not specified the default
     *                    language will be returned
     * @param[in] language IETF language tags specified by RFC 5646 if the string
     *                     is NULL or an empty string the MsgArg for the default
     *                     language will be returned
     *
     * @return ER_OK on successful
     */
    QStatus GetMsgArg(MsgArg* msgArg, const char* language = NULL);

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
    QStatus GetMsgArgAnnounce(MsgArg* msgArg);
    /*
     * we should add a way to generate a UUID based on RFC-4122. This should probably
     * be a in common since generation of the RFC-4122 id is not really core to alljoyn
     * its self.
     */
  public:
    /**
     * @anchor AboutFields
     * @name Known AboutFields
     *
     * The known fields in the About interface
     * TODO put in a table listing the properties for all of the about fields
     */
    // @{
    static const char* APP_ID;           ///< The globally unique id for the application
    static const char* DEFAULT_LANGUAGE; ///< The default language supported by the device. IETF language tags specified by RFC 5646.
    static const char* DEVICE_NAME; ///< The name of the device
    static const char* DEVICE_ID; ///< A unique strign with a value generated using platform specific means
    static const char* APP_NAME; ///< The application name assigned by the manufacture
    static const char* MANUFACTURER; ///< The manufacture's name
    static const char* MODEL_NUMBER; ///< The application model number
    static const char* SUPPORTED_LANGUAGES; ///< List of supported languages
    static const char* DESCRIPTION; ///< Detailed descritption provided by the application
    static const char* DATE_OF_MANUFACTURE; ///< The date of manufacture usign format YYYY-MM-DD
    static const char* SOFTWARE_VERSION; ///< The software version for the OEM software
    static const char* AJ_SOFTWARE_VERSION; ///< The current version of the AllJoyn SDK utilized by the application
    static const char* HARDWARE_VERSION; ///< The device hardware version
    static const char* SUPPORT_URL; ///< The support URL provided by the OEM or software developer
    // @}
  protected:
    /**
     * Derived classes have the ability to fully specify there own AboutData including
     * requirements that can not be changed using the base class.  The derived class
     * can specify if a value is required or optional, the value is part of the announce
     * signal or must be read using the GetAboutData method. Specify if the value is
     * a localizable value or not.
     *
     * @param[in] fieldName the Field Name being set
     * @param[in] isRequried is this field required befor the AboutData can be announced
     * @param[in] isAnnounced is this field part of the Announce signal
     * @param[in] isLocalized can this field be localized
     * @param[in] signature the data type held by the field this is a MsgArg signature
     * @return
     *     - #ER_OK on success
     *     - #ER_ABOUT_FIELD_ALREADY_SPECIFIED if that field has already been specified
     */
    QStatus SetNewFieldDetails(qcc::String fieldName, bool isRequired, bool isAnnounced, bool isLocalized, qcc::String signature);

    typedef struct FieldDetails {
        FieldDetails() { }
        FieldDetails(bool r, bool a, bool l, qcc::String s) : required(r), announced(a), localized(l), signature(s) { }
        bool required;
        bool announced;
        bool localized;
        qcc::String signature;
    } FieldDetails;

    std::map<qcc::String, FieldDetails> m_aboutFields;
  private:
    /**
     * property store used to hold property store values that are not localized
     * key: Field Name
     * value: Data
     */
    std::map<qcc::String, MsgArg> m_propertyStore;
    /**
     * key: Field Name
     * value: map of language / Data
     */
    std::map<qcc::String, std::map<qcc::String, MsgArg> > m_localizedPropertyStore;

    /**
     * local member variable for supported languages
     */
    // TODO should this be a set since we don't want the same language listed twice
    std::vector<qcc::String> m_supportedLanguages;

    /**
     * mutex lock to protect the property store.
     */
    qcc::Mutex m_propertyStoreLock;
};
}

#endif
