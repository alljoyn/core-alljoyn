/*
 * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
import java.io.File;
import java.util.Scanner;
import java.util.UUID;
import java.util.Set;
import java.util.HashSet;

import org.alljoyn.bus.AboutObj;
import org.alljoyn.bus.AboutData;
import org.alljoyn.bus.PermissionConfigurationListener;
import org.alljoyn.bus.PermissionConfigurator;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Mutable;

public class SecureDoorClient {
    static {
        System.loadLibrary("alljoyn_java");
    }
    private static BusAttachment mBus;

    private static final String defaultManifestXml = "<manifest>" +
    "<node>" +
    "<interface name=\"sample.securitymgr.door.Door\">" +
    "<any>" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>" +
    "</any>" +
    "</interface>" +
    "</node>" +
    "</manifest>";

    private static DoorAboutListener dal;

    public static void main(String[] args) throws Exception {
        String appName = "SecureDoorConsumerJava";
        if (args.length > 0) {
            appName = args[0];
        }
        System.out.println("Starting door consumer " + appName);

        mBus = new BusAttachment(appName, BusAttachment.RemoteMessage.Receive);
        mBus.connect();

        Status status = mBus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", authListener, null, false, pclistener);
        if (status != Status.OK) {
            System.out.println("registerAuthListener " + status);
            return;
        }

        mBus.getPermissionConfigurator().setClaimCapabilities(PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL);
        mBus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);

        SessionOpts sessionOpts = new SessionOpts();

        final Mutable.ShortValue contactPort = new Mutable.ShortValue((short) 12345);
        status = mBus.bindSessionPort(contactPort, sessionOpts,
                new SessionPortListener() {
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                System.out.println("SessionPortListener.acceptSessionJoiner called");
                return sessionPort == contactPort.value;
            }
            public void sessionJoined(short sessionPort, int id, String joiner) {
            }
        });

        if (status != Status.OK) {
            System.out.println("bindSessionPort " + status);
            return;
        }

        System.out.println("Waiting to be claimed...");

        while (mBus.getPermissionConfigurator().getApplicationState() != PermissionConfigurator.ApplicationState.CLAIMED) {
            Thread.sleep(1000);
        }

        dal = new DoorAboutListener();
        mBus.registerAboutListener(dal);

        String ifaces[] = {Door.DOOR_INTF_NAME};
        status = mBus.whoImplements(ifaces);
        if (status != Status.OK) {
            System.out.println("Status whoImplements: " + status);
            return;
        }

        printHelp();
        Scanner scanner = new Scanner(System.in);

        doorSessionManager = new DoorSessionManager(mBus);

        while (true) {
            System.out.print("> ");
            String command = scanner.next();
            if (command.equals("q")) {
                break;
            }
            executeCommand(command);
        }

        doorSessionManager.stop();
    }

    private static DoorSessionManager doorSessionManager;

    private static AuthListener authListener = new AuthListener() {
        public boolean requested(String mechanism, String authPeer, int count, String userName,
                AuthRequest[] requests) {
            return true;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {}
    };

    private static PermissionConfigurationListener pclistener = new PermissionConfigurationListener() {

        public Status factoryReset() {
            return Status.OK;
        }

        public void policyChanged() {
        }

        public void startManagement() {
        }

        public void endManagement() {
        }
    };

    public static void printHelp() {
        System.out.println("Welcome to the door consumer - enter 'h' for this menu\n" +
           "Menu\n" +
           ">o : Open doors\n" +
           ">c : Close doors\n" +
           ">s : Doors state - using ProxyBusObject->MethodCall\n" +
           ">g : Get doors state - using ProxyBusObject->GetProperty\n" +
           ">q : Quit\n");
    }

    public static void executeCommand(String command) {
        Set<String> doors;
        if (command.equals("h")) {
            printHelp();
        }
        if (command.equals("o")) {
            doors = dal.getDoorNames();
            if (doors.size() == 0) {
                System.out.println("no doors found");
            }
            for (String doorName : doors.toArray(new String[doors.size()])) {
                System.out.println(doorName);
                try {
                    doorSessionManager.getProxyDoorObject(doorName).open();
                } catch (BusException e) { e.printStackTrace();
                }
            }
        }
        if (command.equals("c")) {
            doors = dal.getDoorNames();
            if (doors.size() == 0) {
                System.out.println("no doors found");
            }
            for (String doorName : doors.toArray(new String[doors.size()])) {
                System.out.println(doorName);
                try {
                    doorSessionManager.getProxyDoorObject(doorName).close();
                } catch (BusException e) {
                    e.printStackTrace();
                }
            }
        }
        if (command.equals("g")) {
            doors = dal.getDoorNames();
            if (doors.size() == 0) {
                System.out.println("no doors found");
            }
            for (String doorName : doors.toArray(new String[doors.size()])) {
                System.out.print(doorName);
                try {
                    System.out.println(doorSessionManager.getProxyDoorObject(doorName).getState() ? " is open" : " is closed");
                } catch (BusException e) {
                    e.printStackTrace();
                }
            }
        }
        if (command.equals("s")) {
            doors = dal.getDoorNames();
            if (doors.size() == 0) {
                System.out.println("no doors found");
            }
            for (String doorName : doors.toArray(new String[doors.size()])) {
                System.out.print(doorName);
                try {
                    System.out.println(doorSessionManager.getProxyDoorObject(doorName).getStateM() ? " is open" : " is closed");
                } catch (BusException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
