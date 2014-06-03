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

import org.alljoyn.about.AboutService;
import org.alljoyn.about.AboutServiceImpl;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.Status;

public class AboutClientSample
{
    static
    {
        System.loadLibrary("alljoyn_java");
    }

    static BusAttachment mBus;
    private static AboutService aboutService;
    private static DeviceAnnouncementHandler announcementHandler;

    public static void main(String[] args)
    {
        Status status;
        System.out.println("starting.....");
        Runtime.getRuntime().addShutdownHook(new Thread()
        {
            public void run()
            {
                mBus.release();
            }
        });

        mBus = new BusAttachment("AppName", BusAttachment.RemoteMessage.Receive);
        System.out.println("created bus attachment");

        status = mBus.connect();
        if (status != Status.OK)
        {
            System.out.println(status.name());
            System.exit(0);
            return;
        }
        // uncomment these lines to see additional debug output
        //mBus.setDaemonDebug("ALL", 7);
        //mBus.setLogLevels("ALL=7");
        //mBus.useOSLogging(true);

        try
        {
            aboutService = AboutServiceImpl.getInstance();
            System.out.println("starting about service in client mode");
            aboutService.setLogger(new SampleLogger());
            aboutService.startAboutClient(mBus);
            System.out.println("started about service in client mode");
            announcementHandler = new DeviceAnnouncementHandler(mBus);
            String[] interfaces = new String[] { "org.alljoyn.Icon", "org.alljoyn.About" };
            aboutService.addAnnouncementHandler(announcementHandler, interfaces);
            System.out.println("added announcement handler");

            System.out.println("waiting for announcement");
            Thread.sleep(120000);

            shutdownAboutService();
            System.out.println("Disconnecting busAttachment");
            mBus.disconnect();
            mBus.release();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    private static void shutdownAboutService()
    {
        if (aboutService != null)
        {
            try
            {
                System.out.format("aboutClient running: %s\n", aboutService.isClientRunning());
                System.out.println("Stopping aboutClient");
                String[] interfaces = new String[] { "org.alljoyn.Icon", "org.alljoyn.About" };
                aboutService.removeAnnouncementHandler(announcementHandler, interfaces);
                aboutService.stopAboutClient();
            }
            catch (Exception e)
            {
                System.out.format("Exception calling stopAboutClient() %s\n", e.getMessage());
            }
        }
    }
}
