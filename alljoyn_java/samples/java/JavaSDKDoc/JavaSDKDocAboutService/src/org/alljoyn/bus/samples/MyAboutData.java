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
package org.alljoyn.bus.samples;

import java.util.HashMap;
import java.util.Map;

import org.alljoyn.bus.AboutDataListener;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.Version;

public class MyAboutData implements AboutDataListener {

    @Override
    public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException {
        System.out.println("MyAboutData.getAboutData was called for `"
                + language + "` language.");
        Map<String, Variant> aboutData = new HashMap<String, Variant>();
        // nonlocalized values
        aboutData.put("AppId", new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
        aboutData.put("DefaultLanguage", new Variant(new String("en")));
        aboutData.put("DeviceId", new Variant(new String(
                "93c06771-c725-48c2-b1ff-6a2a59d445b8")));
        aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
        aboutData.put("SupportedLanguages", new Variant(new String[] { "en", "es" }));
        aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
        aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
        aboutData.put("AJSoftwareVersion", new Variant(Version.get()));
        aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
        aboutData.put("SupportUrl", new Variant(new String(
                "http://www.example.com/support")));
        // localized values
        // If the language String is null or an empty string we return the default
        // language
        if ((language == null) || (language.length() == 0) || language.equalsIgnoreCase("en")) {
            aboutData.put("DeviceName", new Variant(new String("A device name")));
            aboutData.put("AppName", new Variant(new String("An application name")));
            aboutData.put("Manufacturer", new Variant(new String(
                    "A mighty manufacturing company")));
            aboutData.put("Description",
                    new Variant( new String("Sample showing the about feature in a service application")));
        } else if (language.equalsIgnoreCase("es")) { // Spanish
            aboutData.put("DeviceName", new Variant(new String(
                    "Un nombre de dispositivo")));
            aboutData.put("AppName", new Variant(
                    new String("Un nombre de aplicación")));
            aboutData.put("Manufacturer", new Variant(new String(
                    "Una empresa de fabricación de poderosos")));
            aboutData.put("Description",
                    new Variant( new String("Muestra que muestra la característica de sobre en una aplicación de servicio")));
        } else {
            throw new ErrorReplyBusException(Status.LANGUAGE_NOT_SUPPORTED);
        }
        return aboutData;
    }

    @Override
    public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException {
        System.out.println("MyAboutData.getAnnouncedAboutData was called.");
        Map<String, Variant> aboutData = new HashMap<String, Variant>();
        aboutData.put("AppId", new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
        aboutData.put("DefaultLanguage", new Variant(new String("en")));
        aboutData.put("DeviceName", new Variant(new String("A device name")));
        aboutData.put("DeviceId", new Variant(new String("93c06771-c725-48c2-b1ff-6a2a59d445b8")));
        aboutData.put("AppName", new Variant( new String("An application name")));
        aboutData.put("Manufacturer", new Variant(new String("A mighty manufacturing company")));
        aboutData.put("ModelNumber", new Variant(new String("A1B2C3")));
        return aboutData;
    }

}
