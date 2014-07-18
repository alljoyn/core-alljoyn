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
import org.alljoyn.about.transport.AboutTransport;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.services.common.AnnouncementHandler;
import org.alljoyn.services.common.BusObjectDescription;
import org.alljoyn.services.common.utils.TransportUtil;

public class DeviceAnnouncementHandler implements AnnouncementHandler
{
    private BusAttachment mBusAttachment;

    /*
     * When pinging a remote bus wait a max of 5 seconds
     */
    private final int PING_WAIT_TIME = 5000;
    DeviceAnnouncementHandler(BusAttachment busAttachment) {
        mBusAttachment = busAttachment;
    }

    public void onAnnouncement(String serviceName, short port, BusObjectDescription[] objectDescriptions, Map<String, Variant> announceData)
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
        System.out.println("AnnounceData:");
        printAnnounceData(announceData);
        System.out.println("");

        /*
         * Check that the unique bus name contained in the Announce handler
         * is reachable.
         *
         * It is possible for the Announce signal to contain stale information.
         * By pinging the bus name we are able to determine if it is still
         * present and responsive before joining a session to it
         */
        mBusAttachment.enableConcurrentCallbacks();
        Status status = mBusAttachment.ping(serviceName, PING_WAIT_TIME);
        if (Status.OK == status) {
            System.out.println("BusAttachment with name " + serviceName + " is still present joining session.");
            attemptToJoinSession(serviceName, port, announceData);
        } else {
            /*
             * If the BusAttachment.ping fails its for one of two reasons. The remote
             * BusAttachment can no longer be found or the remote device is running
             * a version of AllJoyn that is older than 14.06.
             *
             * Its still possible to Join the session with the older BusAttachment.
             * However, if the remote BusAttachment is no longer present the JoinSession
             * call could run to timeout (about 90 seconds by default). Blocking
             * inside the onAnnounce handler for 90 seconds could cause concurrency
             * problems so we call JoinSession on a different thread.
             *
             * If no services are running a version of AllJoyn older than 14.06
             * then there is no need to attempt to JoinSessions.  Just exit without
             * doing anything since the remote bus is no longer present.
             */
            class JoinSessionThread extends Thread {
                JoinSessionThread(String serviceName, short port, Map<String, Variant> announceData) {
                    mServiceName = serviceName;
                    mPort = port;
                    mAnnounceData = announceData;
                }
                public void run() {
                    attemptToJoinSession(mServiceName, mPort, mAnnounceData);
                }
                String mServiceName;
                short mPort;
                Map<String, Variant> mAnnounceData;
            }

            System.out.println("BusAttachment with name " + serviceName + " is no longer present or is older than v14.06.");
            System.out.println("Attempting to JoinSession just incase " + serviceName + " is running an older version of AllJoyn.");
            new JoinSessionThread(serviceName, port, announceData).start();
        }
    }

    public void attemptToJoinSession(String serviceName, short port, Map<String, Variant> announceData)
    {
        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

        Status status = mBusAttachment.joinSession(serviceName, port, sessionId, sessionOpts, new SessionListener());
        if (Status.OK == status) {
            System.out.println("Succesfully joined session with: \"" + serviceName + "\" SessionId: \"" + sessionId.value +"\"");
            ProxyBusObject aboutObj = mBusAttachment.getProxyBusObject(serviceName, AboutTransport.OBJ_PATH,
                    sessionId.value, new Class<?>[] { AboutTransport.class});
            AboutTransport aboutIntf = aboutObj.getInterface(AboutTransport.class);
            try {
                /*
                 * Get property org.alljoyn.About.Version
                 */
                System.out.println("--------------Getting org.alljoyn.About.Version property-------------");
                int version = aboutIntf.getVersion();
                System.out.println("version: " + version);
            } catch(BusException e1){
                System.out.println(e1.getMessage());
            }

            try{
                /*
                 * Call org.alljoyn.About.GetObjectDescription method for default language
                 */
                System.out.println("--------Calling org.alljoyn.About.GetObjectDescription method--------");
                BusObjectDescription[] objDescription = aboutIntf.GetObjectDescription();
                System.out.println("ObjectDescriptions:");
                for(BusObjectDescription busObjectDescription : objDescription) {
                    System.out.println("\tObjectPath = " + busObjectDescription.path);
                    for(String allJoynInterface : busObjectDescription.interfaces) {
                        System.out.println("\t\tInterface = " + allJoynInterface);
                    }
                }
            } catch(BusException e2) {
                System.out.println(e2.getMessage());
            }

            try {
                /*
                 * Call org.alljoyn.About.GetAboutData method for default language
                 */
                String defaultLanguage = announceData.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE).getObject(String.class);
                System.out.println("---------Calling org.alljoyn.About.GetAboutData method for " + defaultLanguage + "--------");
                Map<String, Variant> defaultAboutData = aboutIntf.GetAboutData(defaultLanguage);
                System.out.println("AboutData: for " + defaultLanguage + " language");
                printAboutData(defaultAboutData);

                Variant variant = defaultAboutData.get(AboutKeys.ABOUT_SUPPORTED_LANGUAGES);
                String[] supportedLangs = variant.getObject(String[].class);
                /*
                 * Call org.alljoyn.About.GetAboutData method for supported languages
                 */
                for (String lang: supportedLangs) {
                    if (!lang.equals(defaultLanguage)) {
                        System.out.println("---------Calling org.alljoyn.About.GetAboutData method for " + lang + "--------");
                        Map<String, Variant> aboutData = aboutIntf.GetAboutData(lang);
                        System.out.println("AboutData: for " + lang + " language");
                        printAboutData(aboutData);
                    }
                }

            } catch(BusException e3) {
                System.out.println(e3.getMessage());
            }
            status = mBusAttachment.leaveSession(sessionId.value);
            if (Status.OK == status) {
                System.out.println("left session with: \"" + serviceName + "\" SessionId: \"" + sessionId.value +"\"");
            }
        }
    }
    public void onDeviceLost(String deviceName)
    {
        System.out.format("onDeviceLost: %s\n", deviceName);
    }

    private void printAnnounceData(Map<String, Variant> announceData) {
        try{
            if(announceData.containsKey(AboutKeys.ABOUT_APP_ID)) {
                Variant variant = announceData.get(AboutKeys.ABOUT_APP_ID);
                UUID appId = TransportUtil.byteArrayToUUID(variant.getObject(byte[].class));
                System.out.println("\tKey = " + AboutKeys.ABOUT_APP_ID + " Value = " + appId);
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_APP_ID + "****");
            }
            if(announceData.containsKey(AboutKeys.ABOUT_DEFAULT_LANGUAGE)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_DEFAULT_LANGUAGE + " Value = " +
                        announceData.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_DEFAULT_LANGUAGE +"****");
            }
            if(announceData.containsKey(AboutKeys.ABOUT_DEVICE_NAME)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_DEVICE_NAME + " Value = " +
                        announceData.get(AboutKeys.ABOUT_DEVICE_NAME).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_DEVICE_NAME +"****");
            }
            if(announceData.containsKey(AboutKeys.ABOUT_DEVICE_ID)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_DEVICE_ID + " Value = " +
                        announceData.get(AboutKeys.ABOUT_DEVICE_ID).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_DEVICE_ID +"****");
            }
            if(announceData.containsKey(AboutKeys.ABOUT_APP_NAME)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_APP_NAME + " Value = " +
                        announceData.get(AboutKeys.ABOUT_APP_NAME).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_APP_NAME +"****");
            }
            if(announceData.containsKey(AboutKeys.ABOUT_MANUFACTURER)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_MANUFACTURER + " Value = " +
                        announceData.get(AboutKeys.ABOUT_MANUFACTURER).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_MANUFACTURER +"****");
            }
            if(announceData.containsKey(AboutKeys.ABOUT_MODEL_NUMBER)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_MODEL_NUMBER + " Value = " +
                        announceData.get(AboutKeys.ABOUT_MODEL_NUMBER).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_MODEL_NUMBER +"****");
            }
        }
        catch (BusException e)
        {
            System.out.println(e.getMessage());
        }
    }

    private void printAboutData(Map<String, Variant> aboutData) {
        printAnnounceData(aboutData);
        try{
            if(aboutData.containsKey(AboutKeys.ABOUT_SUPPORTED_LANGUAGES)) {
                Variant variant = aboutData.get(AboutKeys.ABOUT_SUPPORTED_LANGUAGES);
                String[] supportedLangs = variant.getObject(String[].class);
                StringBuffer languagesStr = new StringBuffer();
                for (String l : supportedLangs){
                    languagesStr.append(" ");
                    languagesStr.append(l);
                }
                System.out.println("\tKey = " + AboutKeys.ABOUT_SUPPORTED_LANGUAGES + " Value =" + languagesStr.toString());
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_SUPPORTED_LANGUAGES + "****");
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_DESCRIPTION)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_DESCRIPTION + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_DESCRIPTION).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_DESCRIPTION +"****");
            }
            // About DateOfManufature is an optional value so its fine if it is not found
            if(aboutData.containsKey(AboutKeys.ABOUT_DATE_OF_MANUFACTURE)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_DATE_OF_MANUFACTURE + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_DATE_OF_MANUFACTURE).getObject(String.class));
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_SOFTWARE_VERSION)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_SOFTWARE_VERSION + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_SOFTWARE_VERSION).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_SOFTWARE_VERSION +"****");
            }
            if(aboutData.containsKey(AboutKeys.ABOUT_AJ_SOFTWARE_VERSION)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_AJ_SOFTWARE_VERSION + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_AJ_SOFTWARE_VERSION).getObject(String.class));
            } else {
                System.err.println("\t**** Missing Required field "+ AboutKeys.ABOUT_AJ_SOFTWARE_VERSION +"****");
            }
            // About HardwareVersion is an optional value so its fine if it is not found
            if(aboutData.containsKey(AboutKeys.ABOUT_HARDWARE_VERSION)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_HARDWARE_VERSION + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_HARDWARE_VERSION).getObject(String.class));
            }
            // About SupportUrl is an optional value so its fine if it is not found
            if(aboutData.containsKey(AboutKeys.ABOUT_SUPPORT_URL)) {
                System.out.println("\tKey = " + AboutKeys.ABOUT_SUPPORT_URL + " Value = " +
                        aboutData.get(AboutKeys.ABOUT_SUPPORT_URL).getObject(String.class));
            }
        }
        catch (BusException e)
        {
            System.out.println(e.getMessage());
        }

    }
}
