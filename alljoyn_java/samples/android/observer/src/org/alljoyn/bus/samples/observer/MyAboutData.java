/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */

package org.alljoyn.bus.samples.observer;

import java.util.HashMap;
import java.util.Map;

import org.alljoyn.bus.AboutDataListener;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.Version;

/**
 * A sample class providing about data.
 */
public class MyAboutData implements AboutDataListener {

    @Override
    public Map<String, Variant> getAboutData(String language)
            throws ErrorReplyBusException {
        Map<String, Variant> aboutData = new HashMap<String, Variant>();
        // nonlocalized values
        aboutData.put("AppId", new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
        aboutData.put("DefaultLanguage", new Variant("en"));
        aboutData.put("DeviceId", new Variant(new String("93c06771-c725-48c2-b1ff-6a2a59d445b8")));
        aboutData.put("ModelNumber", new Variant("A1B2C3"));
        aboutData.put("SupportedLanguages", new Variant(new String[] {"en"}));
        aboutData.put("DateOfManufacture", new Variant(new String("2014-09-23")));
        aboutData.put("SoftwareVersion", new Variant(new String("1.0")));
        aboutData.put("AJSoftwareVersion", new Variant(Version.get()));
        aboutData.put("HardwareVersion", new Variant(new String("0.1alpha")));
        aboutData.put("SupportUrl", new Variant(new String("http://www.example.com/support")));
        aboutData.put("DeviceName", new Variant("A device name"));
        aboutData.put("AppName", new Variant("An application name"));
        aboutData.put("Manufacturer", new Variant("A mighty manufacturing company"));
        aboutData.put("Description",
                      new Variant("Sample showing the about feature in a service application"));
        return aboutData;
    }

    @Override
    public Map<String, Variant> getAnnouncedAboutData()
            throws ErrorReplyBusException {
        Map<String, Variant> aboutData = new HashMap<String, Variant>();
        aboutData.put("AppId", new Variant(new byte[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}));
        aboutData.put("DefaultLanguage", new Variant("en"));
        aboutData.put("DeviceName", new Variant("A device name"));
        aboutData.put("DeviceId", new Variant(new String("93c06771-c725-48c2-b1ff-6a2a59d445b8")));
        aboutData.put("AppName", new Variant("An application name"));
        aboutData.put("Manufacturer", new Variant("A mighty manufacturing company"));
        aboutData.put("ModelNumber", new Variant("A1B2C3"));
        return aboutData;
    }
}