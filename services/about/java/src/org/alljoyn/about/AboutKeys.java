/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.about;

/**
 * About keys as defined by HLD
 */

public interface AboutKeys {
    /**
     * Required. If Config service exists on the device, About instance
     * populates the DeviceId as per Config service; If there is no Config on
     * the device, it is set using the platform specific means
     */
    public final static String ABOUT_DEVICE_ID = "DeviceId";

    /**
     * Required. If Config service exists on the device, About instance
     * populates the value as DeviceName in Config; If there is no Config, it is
     * set by platform specific means and reflected in About
     */
    public final static String ABOUT_DEVICE_NAME = "DeviceName";

    /**
     * Required. The globally unique id for the application
     */
    public final static String ABOUT_APP_ID = "AppId";

    /**
     * Required. This field specifies the Application Name. It is assigned by
     * the App Manufacturer
     */
    public final static String ABOUT_APP_NAME = "AppName";

    /**
     * Required. The manufacturer's name of the application.
     */
    public final static String ABOUT_MANUFACTURER = "Manufacturer";

    /**
     * Required. Detailed description
     */
    public final static String ABOUT_DESCRIPTION = "Description";

    /**
     * Required. The default language supported by the device. IETF language
     * tags specified by RFC 5646. It is populated by Config service when
     * present else it is specified by device specific means
     */
    public final static String ABOUT_DEFAULT_LANGUAGE = "DefaultLanguage";

    /**
     * Required. The software version for the OEM software.
     */
    public final static String ABOUT_SOFTWARE_VERSION = "SoftwareVersion";

    /**
     * Required. The current version of the AllJoyn SDK utilized by the
     * application.
     */
    public final static String ABOUT_AJ_SOFTWARE_VERSION = "AJSoftwareVersion";

    /**
     * Required. The application model number.
     */
    public final static String ABOUT_MODEL_NUMBER = "ModelNumber";

    /**
     * Optional. The date of manufacture using format YYYY-MM-DD (known as XML
     * DateTime Format)
     */
    public final static String ABOUT_DATE_OF_MANUFACTURE = "DateOfManufacture";

    /**
     * Optional. The device hardware version
     */
    public final static String ABOUT_HARDWARE_VERSION = "HardwareVersion";

    /**
     * Optional. The support URL to be populated by device OEM
     */
    public final static String ABOUT_SUPPORT_URL = "SupportUrl"; 

    /**
     * Required. This field returns the list of supported languages by the
     * device.
     */
    public final static String ABOUT_SUPPORTED_LANGUAGES = "SupportedLanguages";

}
