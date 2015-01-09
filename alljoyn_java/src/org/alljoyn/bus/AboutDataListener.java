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

package org.alljoyn.bus;

import java.util.Map;

/**
 * Interface that supplies the list of fields required for an Announce signal
 * and the org.alljoyn.About.GetData method.
 */
public interface AboutDataListener {
    /**
     * Get the Dictionary that is returned when a user calls
     * org.alljoyn.About.GetAboutData. The returned Dictionary must contain the
     * AboutData dictionary for the Language specified.
     *
     * The return value  will contain a Map&lt;String, Variant&gt;. The values defined
     * in org.alljoyn.About interface specification are as follows.
     *
     * <table summary="org.alljoyn.About AboutData fields">
     * <tr>
     *     <th>Field Name</th>
     *     <th>Required</th>
     *     <th>Announced</th>
     *     <th>Localized</th>
     *     <th>Data type</th>
     *     <th>Description</th>
     * </tr>
     * <tr>
     *     <td><strong>AppId</strong></td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>no</td>
     *     <td>ay</td>
     *     <td>The globally unique id for the application.</td>
     * </tr>
     * <tr>
     *     <td><strong>DefaultLanguage</strong></td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>no</td>
     *     <td>s</td>
     *     <td>The default language supported by the device. IETF langauge tags
     *         specified by RFC 5646.</td>
     * </tr>
     * <tr>
     *     <td><strong>DeviceName</strong></td>
     *     <td>no</td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>s</td>
     *     <td>If Config service exist on the device, About instance populates
     *         the value as DeviceName in Config; If there is not Config, it can
     *         be set by the app.  Device Name is optional for a third party
     *         apps but required for system apps. Versions of AllJoyn older than
     *         14.12 this field was required.  If working with older applications
     *         this field may be required.</td>
     * </tr>
     * <tr>
     *     <td><strong>DeviceId</strong></td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>no</td>
     *     <td>s</td>
     *     <td>A string with value generated using platform specific means.</td>
     * </tr>
     * <tr>
     *     <td><strong>AppName</strong></td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>s</td>
     *     <td>The application name assigned by the app manufacturer</td>
     * </tr>
     * <tr>
     *     <td><strong>Manufacturer</strong></td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>s</td>
     *     <td>The manufacturer's name.</td>
     * </tr>
     * <tr>
     *     <td><strong>ModelNumber</strong></td>
     *     <td>yes</td>
     *     <td>yes</td>
     *     <td>no</td>
     *     <td>s</td>
     *     <td>The app model number</td>
     * </tr>
     * <tr>
     *     <td><strong>SupportedLanguages</strong></td>
     *     <td>yes</td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>as</td>
     *     <td>A list of supported languages by the application</td>
     * </tr>
     * <tr>
     *     <td><strong>Description</strong></td>
     *     <td>yes</td>
     *     <td>no</td>
     *     <td>yes</td>
     *     <td>s</td>
     *     <td>Detailed description provided by the application.</td>
     * </tr>
     * <tr>
     *     <td><strong>DateOfManufacture</strong></td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>s</td>
     *     <td>The date of manufacture. using format YYYY-MM-DD.
     *         (Known as XML DateTime Format)</td>
     * </tr>
     * <tr>
     *     <td><strong>SoftwareVersion</strong></td>
     *     <td>yes</td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>s</td>
     *     <td>The software version of the manufacturer's software</td>
     * </tr>
     * <tr>
     *     <td><strong>SoftwareVersion</strong></td>
     *     <td>yes</td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>s</td>
     *     <td>The current version of the AllJoyn SDK utilized by the
     *         application.</td>
     * </tr>
     * <tr>
     *     <td><strong>HardwareVersion</strong></td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>s</td>
     *     <td>The device hardware version.</td>
     * </tr>
     * <tr>
     *     <td><strong>SupportUrl</strong></td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>no</td>
     *     <td>s</td>
     *     <td>The support URL.</td>
     * </tr>
     * </table>
     *
     * Custom fields are allowed. Since the proxy object only receives the field
     * name and the Variant containing the contents for that field the default
     * assumption is that unknown user defined fields are:
     * - are not required
     * - are not announced
     * - are localized if the Variant contains a String (not localized otherwise)
     *
     * Important: All implementations of getAboutData should handle the language
     * specified as an empty string or null. In the case that the language is
     * an empty string or null the getAboutData is expected to return the default
     * language.
     *
     * If the language tag given is not supported throw an ErrorReplyBusException
     * with the Status LANGUAGE_NOT_SUPPORTED. If ALL required fields have not
     * been provided then throw an  ErrorReplyBusException with the Status
     * ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD
     *
     * @param language IETF language tags specified by RFC 5646 if the string
     *                 is null or an empty string the MsgArg for the default
     *                 language will be returned
     *
     *@throws ErrorReplyBusException LANGUAGE_NOT_SUPPORTED if the requested
     *        language is not supported. ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD
     *        if unable to return the AboutData because one or more required
     *        field values can not be obtained
     *
     * @return ER_OK on successful
     */
    public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException;

    /**
     * Return a Dictionary containing the AboutData that
     * is announced with the org.alljoyn.About.Announce signal.
     * The Dictionary will always be the default language and will only contain
     * the fields that are announced.
     *
     * The fields required to be part of the announced MsgArg are:
     *  - AppId
     *  - DefaultLanguage
     *  - DeviceName (optional)
     *  - DeviceId
     *  - AppName
     *  - Manufacture
     *  - ModelNumber
     *
     * If you require other fields or need the localized AboutData
     *   The org.alljoyn.About.GetAboutData method can be used.
     *
     * Or the GetAboutData member function.
     *
     * @throws  ErrorReplyBusException ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD if unable to return the
     *          announced AboutData because one or more required field values
     *          can not be obtained
     *
     * @return a dictionary that contains the Announce data.
     */
    public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException;
}
