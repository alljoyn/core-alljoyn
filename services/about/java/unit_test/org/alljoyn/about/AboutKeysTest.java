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

package org.alljoyn.about;

import static org.junit.Assert.*;

import org.alljoyn.about.AboutKeys;
import org.junit.Test;

public class AboutKeysTest {
    @Test
    public void testKeys() {
        assertEquals("AppId", AboutKeys.ABOUT_APP_ID);
        assertEquals("DefaultLanguage", AboutKeys.ABOUT_DEFAULT_LANGUAGE);
        assertEquals("DeviceName", AboutKeys.ABOUT_DEVICE_NAME);
        assertEquals("DeviceId", AboutKeys.ABOUT_DEVICE_ID);
        assertEquals("AppName", AboutKeys.ABOUT_APP_NAME);
        assertEquals("Manufacturer", AboutKeys.ABOUT_MANUFACTURER);
        assertEquals("ModelNumber", AboutKeys.ABOUT_MODEL_NUMBER);
        assertEquals("SupportedLanguages", AboutKeys.ABOUT_SUPPORTED_LANGUAGES);
        assertEquals("Description", AboutKeys.ABOUT_DESCRIPTION);
        assertEquals("DateOfManufacture", AboutKeys.ABOUT_DATE_OF_MANUFACTURE);
        assertEquals("SoftwareVersion", AboutKeys.ABOUT_SOFTWARE_VERSION);
        assertEquals("AJSoftwareVersion", AboutKeys.ABOUT_AJ_SOFTWARE_VERSION);
        assertEquals("HardwareVersion", AboutKeys.ABOUT_HARDWARE_VERSION);
        assertEquals("SupportUrl", AboutKeys.ABOUT_SUPPORT_URL);
    }

    @Test
    public void testImplementAboutKeys(){
        class AboutKeysImpl implements AboutKeys {
            public final static String ABOUT_CUSTOM_KEY = "CustomKey";
        }

        assertEquals("AppId", AboutKeysImpl.ABOUT_APP_ID);
        assertEquals("DefaultLanguage", AboutKeysImpl.ABOUT_DEFAULT_LANGUAGE);
        assertEquals("DeviceName", AboutKeysImpl.ABOUT_DEVICE_NAME);
        assertEquals("DeviceId", AboutKeysImpl.ABOUT_DEVICE_ID);
        assertEquals("AppName", AboutKeysImpl.ABOUT_APP_NAME);
        assertEquals("Manufacturer", AboutKeysImpl.ABOUT_MANUFACTURER);
        assertEquals("ModelNumber", AboutKeysImpl.ABOUT_MODEL_NUMBER);
        assertEquals("SupportedLanguages", AboutKeysImpl.ABOUT_SUPPORTED_LANGUAGES);
        assertEquals("Description", AboutKeysImpl.ABOUT_DESCRIPTION);
        assertEquals("DateOfManufacture", AboutKeysImpl.ABOUT_DATE_OF_MANUFACTURE);
        assertEquals("SoftwareVersion", AboutKeysImpl.ABOUT_SOFTWARE_VERSION);
        assertEquals("AJSoftwareVersion", AboutKeysImpl.ABOUT_AJ_SOFTWARE_VERSION);
        assertEquals("HardwareVersion", AboutKeysImpl.ABOUT_HARDWARE_VERSION);
        assertEquals("SupportUrl", AboutKeysImpl.ABOUT_SUPPORT_URL);
        assertEquals("CustomKey", AboutKeysImpl.ABOUT_CUSTOM_KEY);
    }

}
