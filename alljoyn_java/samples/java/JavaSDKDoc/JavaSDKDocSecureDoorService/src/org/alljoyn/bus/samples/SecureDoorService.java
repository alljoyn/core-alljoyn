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

import java.util.Map;
import java.io.File;
import java.util.Scanner;

import org.alljoyn.bus.AboutObj;
import org.alljoyn.bus.PermissionConfigurationListener;
import org.alljoyn.bus.PermissionConfigurator;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Mutable;

public class SecureDoorService {
    static {
        System.loadLibrary("alljoyn_java");
    }
    private static BusAttachment mBus;

    private static final String defaultManifestXml = "<manifest>" +
    "<node>" +
    "<interface>" +
    "<any>" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" +
    "</any>" +
    "</interface>" +
    "</node>" +
    "</manifest>";

    public static void main(String[] args) throws Exception{
        mBus = new BusAttachment("SecureDoorService", BusAttachment.RemoteMessage.Receive);
        mBus.registerKeyStoreListener(new DoorKeyStoreListener());
        mBus.connect();

        Status status = mBus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA", authListener, null, false, pclistener);
        if (status != Status.OK) {
            System.out.println("registerAuthListener " + status);
            return;
        }
        DoorImpl door = new DoorImpl("kitchen");

        status = mBus.registerBusObject(door, "/sample/security/Door");
        if (status != Status.OK) {
            System.out.println("registerBusObject " + status);
            return;
        }

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        final Mutable.ShortValue contactPort = new Mutable.ShortValue((short) 12345);

        //setting up permission configurator so that service is claimable
        mBus.getPermissionConfigurator().setClaimCapabilities((short) (PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_PSK | PermissionConfigurator.CLAIM_CAPABILITY_CAPABLE_ECDHE_NULL));
        mBus.getPermissionConfigurator().setClaimCapabilityAdditionalInfo((short) (PermissionConfigurator.CLAIM_CAPABILITY_ADDITIONAL_PSK_GENERATED_BY_SECURITY_MANAGER | PermissionConfigurator.CLAIM_CAPABILITY_ADDITIONAL_PSK_GENERATED_BY_APPLICATION));
        mBus.getPermissionConfigurator().setManifestTemplateFromXml(defaultManifestXml);

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
        //using about announcements to be discovered by the security manager
        AboutObj aboutObj = new AboutObj(mBus);
        status = aboutObj.announce(contactPort.value, new MyAboutData());
        if (status != Status.OK) {
            System.out.println("announce " + status);
            return;
        }

        System.out.println("Waiting to be claimed...");

        while (mBus.getPermissionConfigurator().getApplicationState() != PermissionConfigurator.ApplicationState.CLAIMED) {
            Thread.sleep(1000);
        }

        Scanner scanner = new Scanner(System.in);

        while (true) {
            System.out.print("> ");
            String command = scanner.next();

            if (command.equals("q")) {
                break;
            }
        }
    }

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
}

