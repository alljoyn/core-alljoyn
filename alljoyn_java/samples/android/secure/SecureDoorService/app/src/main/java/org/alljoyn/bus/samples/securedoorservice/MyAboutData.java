/*
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*/
package org.alljoyn.bus.samples.securedoorservice;

import android.content.Context;
import android.util.Log;
import android.os.Environment;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.Scanner;
import java.io.FileOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;

import org.alljoyn.bus.AboutDataListener;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.Version;

public class MyAboutData implements AboutDataListener {
    private static final String TAG = "MyAboutDataListener";
    private static final String FILE_NAME = "SecureDoorServiceAppId";

    private UUID uuid;
    private File uuid_storage;

    public MyAboutData(Context context) {
        uuid_storage = new File(context.getFileStreamPath(FILE_NAME).getAbsolutePath());

        FileOutputStream out_uuid = null;
        try {
            if (uuid_storage.createNewFile()) {
                uuid = UUID.randomUUID();
                out_uuid = new FileOutputStream(uuid_storage.getAbsolutePath());
                out_uuid.write(uuid.toString().getBytes());
                out_uuid.close();
            } else {
                Scanner scan = new Scanner(uuid_storage);
                String s = scan.next();
                Log.i(TAG, s);
                uuid = UUID.fromString(s);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (out_uuid != null) {
                try {
                    out_uuid.close();
                } catch (IOException ex) {
                    ex.printStackTrace();
                }
            }
        }
    }

    @Override
    public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException {
        Log.i(TAG, "MyAboutData.getAboutData was called for '" + language + "' language.");
        return createAboutData(language);
    }

    @Override
    public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException {
        Log.i(TAG, "MyAboutData.getAnnouncedAboutData was called.");
        return createAboutData(null);
    }

    private Map<String, Variant> createAboutData(String language) throws ErrorReplyBusException {
        Map<String, Variant> aboutData = new HashMap<String, Variant>();
        // non-localized values
        ByteBuffer bb = ByteBuffer.wrap(new byte[16]);
        bb.putLong(uuid.getMostSignificantBits());
        bb.putLong(uuid.getLeastSignificantBits());

        aboutData.put("AppId", new Variant(bb.array()));
        aboutData.put("DefaultLanguage", new Variant("en"));
        aboutData.put("DeviceId", new Variant("93c06771-c725-48c2-b1ff-6a2a59d445b8"));
        aboutData.put("ModelNumber", new Variant("A1B2C3"));
        aboutData.put("SupportedLanguages", new Variant(new String[] { "en", "es" }));
        aboutData.put("DateOfManufacture", new Variant("2014-09-23"));
        aboutData.put("SoftwareVersion", new Variant("1.0"));
        aboutData.put("AJSoftwareVersion", new Variant(Version.get()));
        aboutData.put("HardwareVersion", new Variant("0.1alpha"));
        aboutData.put("SupportUrl", new Variant("http://www.example.com/support"));
        // localized values
        // If the language String is null or an empty string we return the default
        // language
        if ((language == null) || (language.length() == 0) || language.equalsIgnoreCase("en")) {
            aboutData.put("DeviceName", new Variant("A device name"));
            aboutData.put("AppName", new Variant("SecureDoorService"));
            aboutData.put("Manufacturer", new Variant("A mighty manufacturing company"));
            aboutData.put("Description", new Variant( "Sample showing the about feature in a service application"));
        } else if (language.equalsIgnoreCase("es")) { // Spanish
            aboutData.put("DeviceName", new Variant("Un nombre de dispositivo"));
            aboutData.put("AppName", new Variant("Un nombre de aplicación"));
            aboutData.put("Manufacturer", new Variant("Una empresa de fabricación de poderosos"));
            aboutData.put("Description",new Variant("Muestra que muestra la característica de sobre en una aplicación de servicio"));
        } else {
            throw new ErrorReplyBusException(Status.LANGUAGE_NOT_SUPPORTED);
        }
        return aboutData;
    }
}
