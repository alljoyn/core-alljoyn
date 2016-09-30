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
package org.alljoyn.bus.samples;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.Arrays;
import java.util.Scanner;
import java.io.FileOutputStream;
import java.io.File;
import java.nio.ByteBuffer;

import org.alljoyn.bus.AboutDataListener;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.Version;

public class MyAboutData implements AboutDataListener {
    private UUID uuid;
    private File uuid_storage;

    private static final String FILE_NAME = "SecureDoorServiceAppId";

    public MyAboutData() {
        uuid_storage = new File(FILE_NAME);
        try {
            if (uuid_storage.createNewFile()) {
                uuid = UUID.randomUUID();
                FileOutputStream out_uuid = new FileOutputStream(uuid_storage.getAbsolutePath());
                out_uuid.write(uuid.toString().getBytes());
                out_uuid.close();
            } else {
                Scanner scan = new Scanner(uuid_storage);
                String s = scan.next();
                System.out.println(s);
                uuid = UUID.fromString(s);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException {
        System.out.println("MyAboutData.getAboutData was called for `"
                + language + "` language.");
        Map<String, Variant> aboutData = new HashMap<String, Variant>();
        // nonlocalized values
        ByteBuffer bb = ByteBuffer.wrap(new byte[16]);
        bb.putLong(uuid.getMostSignificantBits());
        bb.putLong(uuid.getLeastSignificantBits());

        System.out.println(Arrays.toString(bb.array()));

        aboutData.put("AppId", new Variant(bb.array()));
        aboutData.put("DefaultLanguage", new Variant("en"));
        aboutData.put("DeviceId", new Variant(new String(
                "93c06771-c725-48c2-b1ff-6a2a59d445b8")));
        aboutData.put("ModelNumber", new Variant("A1B2C3"));
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
            aboutData.put("DeviceName", new Variant("A device name"));
            aboutData.put("AppName", new Variant("An application name"));
            aboutData.put("Manufacturer", new Variant(new String(
                    "A mighty manufacturing company")));
            aboutData.put("Description",
                    new Variant( "Sample showing the about feature in a service application"));
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
        ByteBuffer bb = ByteBuffer.wrap(new byte[16]);
        bb.putLong(uuid.getMostSignificantBits());
        bb.putLong(uuid.getLeastSignificantBits());

        System.out.println(Arrays.toString(bb.array()));
        aboutData.put("AppId", new Variant(bb.array()));
        aboutData.put("DefaultLanguage", new Variant("en"));
        aboutData.put("DeviceName", new Variant("A device name"));
        aboutData.put("DeviceId", new Variant(new String("93c06771-c725-48c2-b1ff-6a2a59d445b8")));
        aboutData.put("AppName", new Variant( "An application name"));
        aboutData.put("Manufacturer", new Variant("A mighty manufacturing company"));
        aboutData.put("ModelNumber", new Variant("A1B2C3"));
        return aboutData;
    }

}
