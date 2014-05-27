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
package org.alljoyn.about.sample;

import java.util.Map;
import java.util.UUID;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Variant;
import org.alljoyn.services.common.AnnouncementHandler;
import org.alljoyn.services.common.BusObjectDescription;
import org.alljoyn.services.common.utils.TransportUtil;

public class DeviceAnnouncementHandler implements AnnouncementHandler
{
    public void onAnnouncement(String serviceName, short version, short port, BusObjectDescription[] objectDescriptions, Map<String, Variant> aboutData)
    {
        System.out.format("Received About announcement signal from serviceName: %s\n", serviceName);
        System.out.println("*******************Contents of Announce Signal*******************");
        System.out.println("port  " + port);
        System.out.println("Unique BusName:  " + serviceName);
        System.out.println("ObjectDescriptions:");
        for(BusObjectDescription busObjectDescription : objectDescriptions) {
            System.out.println("\tObjectPath = " + busObjectDescription.path);
            for(String allJoynInterface : busObjectDescription.interfaces) {
                System.out.println("\t\tInterface = " + allJoynInterface);
            }
        }
        System.out.println("AboutData:");
        try{
            if(aboutData.containsKey(AboutKeys.ABOUT_APP_ID)) {
                Variant variant = aboutData.get(AboutKeys.ABOUT_APP_ID);
                UUID appId = TransportUtil.byteArrayToUUID(variant.getObject(byte[].class));
                System.out.println("\tKey = " + AboutKeys.ABOUT_APP_ID + " Value = " + appId);
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_APP_ID + "****");
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_DEFAULT_LANGUAGE)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_DEFAULT_LANGUAGE + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_DEFAULT_LANGUAGE +"****");
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_DEVICE_NAME)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_DEVICE_NAME + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_DEVICE_NAME).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_DEVICE_NAME +"****");
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_DEVICE_ID)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_DEVICE_ID + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_DEVICE_ID).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_DEVICE_ID +"****");
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_APP_NAME)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_APP_NAME + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_APP_NAME).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_APP_NAME +"****");
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_MANUFACTURER)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_MANUFACTURER + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_MANUFACTURER).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_MANUFACTURER +"****");
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_MODEL_NUMBER)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_MODEL_NUMBER + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_MODEL_NUMBER).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_MODEL_NUMBER +"****");
            }
        }
        catch (BusException e)
        {
            System.out.println(e.getMessage());
        }
    }

    public void onDeviceLost(String deviceName)
    {
        System.out.format("onDeviceLost: %s\n", deviceName);
    }
}
