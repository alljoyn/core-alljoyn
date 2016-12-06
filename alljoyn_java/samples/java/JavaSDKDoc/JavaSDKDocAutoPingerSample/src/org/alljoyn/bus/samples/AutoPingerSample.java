/*
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
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
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 */
package org.alljoyn.bus.samples;

import java.util.Map;

import org.alljoyn.bus.AboutListener;
import org.alljoyn.bus.AboutObjectDescription;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.AutoPinger;
import org.alljoyn.bus.AutoPingListener;

public class AutoPingerSample {
    static {
        System.loadLibrary("alljoyn_java");
    }
    private static final String GROUP1_NAME = "group1";
    private static final String GROUP2_NAME = "group2";
    private static final String GROUP3_NAME = "group3";

    static BusAttachment mBus;
    static AutoPinger autoPinger;
    static int i = 0;

    static class MyAboutListener implements AboutListener {
        public void announced(String busName, int version, short port, AboutObjectDescription[] objectDescriptions, Map<String, Variant> aboutData) {
            if (i % 3 == 0) {
                autoPinger.addDestination(GROUP1_NAME, busName);
            } else if (i % 3 == 1) {
                autoPinger.addDestination(GROUP2_NAME, busName);
            } else {
                autoPinger.addDestination(GROUP3_NAME, busName);
            }
            i++;
            System.out.println("Announced BusName:     " + busName);
            System.out.println("Announced Version:     " + version);
            System.out.println("Announced SessionPort: " + port);
        }
    }

    static class MyBusListener extends BusListener {
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            if (i % 3 == 0) {
                autoPinger.addDestination(GROUP1_NAME, name);
            } else if (i % 3 == 1) {
                autoPinger.addDestination(GROUP2_NAME, name);
            } else {
                autoPinger.addDestination(GROUP3_NAME, name);
            }
            i++;
            System.out.println(String.format("BusListener.foundAdvertisedName(%s, %d, %s)", name, transport, namePrefix));
        }
    }

    static class MyAutoPingListener implements AutoPingListener {
        public void destinationLost(String group, String destination) {
            System.out.println("MyAutoPingListener Lost destination: " + destination + "\n in group: " + group);
        }
        public void destinationFound(String group, String destination) {
            System.out.println("MyAutoPingListener Found destination: " + destination + "\n in group: " + group);
        }
    }

    public static void main(String[] args) {
        mBus = new BusAttachment("Basic About Client Sample", BusAttachment.RemoteMessage.Receive);
        autoPinger = new AutoPinger(mBus);

        Status status = mBus.connect();
        if (status != Status.OK) {
            return;
        }
        System.out.println("BusAttachment.connect successful on " + System.getProperty("org.alljoyn.bus.address"));

        MyAutoPingListener myAuto = new MyAutoPingListener();
        AutoPingListener anonAuto = new AutoPingListener() {
            public void destinationLost(String group, String destination) {
                System.out.println("anonAuto Lost destination: " + destination + "\n in group: " + group);
            }
            public void destinationFound(String group, String destination) {
                System.out.println("anonAuto Found destination: " + destination + "\n in group: " + group);
            }
        };
        autoPinger.addPingGroup(GROUP1_NAME, anonAuto, 1);
        autoPinger.addPingGroup(GROUP2_NAME, new AutoPingListener() {
            public void destinationLost(String group, String destination) {
                System.out.println("Anonymous Lost destination: " + destination + "\n in group: " + group);
            }
            public void destinationFound(String group, String destination) {
                System.out.println("Anonymous Found destination: " + destination + "\n in group: " + group);
            }
        }, 1);
        autoPinger.addPingGroup(GROUP3_NAME, myAuto, 1);


        AboutListener listener = new MyAboutListener();
        mBus.registerAboutListener(listener);

        String ifaces[] = {"com.example.*"};
        status = mBus.whoImplements(ifaces);
        if (status != Status.OK) {
            return;
        }
        System.out.println("BusAttachment.whoImplements successful " + "com.example.*");

        MyBusListener buslistener = new MyBusListener();
        mBus.registerBusListener(buslistener);

        status = mBus.findAdvertisedName("com.my");
        if (status != Status.OK) {
            return;
        }
        System.out.println("BusAttachment.findAdvertisedName successful " + "com.my");

        while(true) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                System.out.println("Program interrupted");
            }
        }
    }
}
