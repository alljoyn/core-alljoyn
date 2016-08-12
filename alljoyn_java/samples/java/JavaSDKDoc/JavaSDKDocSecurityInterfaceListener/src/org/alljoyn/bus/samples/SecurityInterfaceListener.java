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
import java.util.Arrays;

import org.alljoyn.bus.AboutListener;
import org.alljoyn.bus.AboutObjectDescription;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.SecurityApplicationProxy;
import org.alljoyn.bus.AuthListener;

public class SecurityInterfaceListener {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static BusAttachment mBus;

    private static SecurityApplicationProxy mSecurityApplicationProxy;

    private static boolean isJoined = false;
    private static Mutable.IntegerValue mSessionId;
    private static String mBusName;

    private static class MyAboutListener implements AboutListener {
        public void announced(String busName, int version, short port, AboutObjectDescription[] objectDescriptions, Map<String, Variant> aboutData) {

            mBusName = busName;
            SessionOpts sessionOpts = new SessionOpts();
            sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
            sessionOpts.isMultipoint = false;
            sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
            sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

            mSessionId = new Mutable.IntegerValue();

            mBus.enableConcurrentCallbacks();

            Status status = mBus.joinSession(busName, port, mSessionId, sessionOpts, new SessionListener());
            if (status != Status.OK) {
                return;
            }
            System.out.println(String.format("BusAttachement.joinSession successful sessionId = %d", mSessionId.value));

            isJoined = true;
        }
    }

    public static void main(String[] args) {
        mBus = new BusAttachment(SecurityInterfaceListener.class.getName(), BusAttachment.RemoteMessage.Receive);

        Status status = mBus.connect();
        if (status != Status.OK) {
            return;
        }
        System.out.println("BusAttachment.connect successful on " + System.getProperty("org.alljoyn.bus.address"));

        AboutListener listener = new MyAboutListener();
        mBus.registerAboutListener(listener);

        status = mBus.registerAuthListener("ALLJOYN_ECDHE_NULL", new AuthListener() {
            public boolean requested(String mechanism, String peerName, int count, String userName,
                    AuthRequest[] requests) {
                return true;
            }

            public void completed(String mechanism, String peerName, boolean authenticated) {
            }
        });
        if (status != Status.OK) {
            return;
        }

        status = mBus.whoImplements(null);
        if (status != Status.OK) {
            return;
        }
        System.out.println("BusAttachment.whoImplements successful " + "com.example.about.feature.interface.sample");

        while(!isJoined) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                System.out.println("Program interupted");
            }
        }
        mSecurityApplicationProxy = new SecurityApplicationProxy(mBus, mBusName, mSessionId.value);

        try {
            System.out.println("Version Number Secure Interface " + mSecurityApplicationProxy.getVersion());
            System.out.println("Application State " + mSecurityApplicationProxy.getApplicationState());
            System.out.println("Manifest Template Digest " + mSecurityApplicationProxy.getManifestTemplateDigest());
            System.out.println("EccPublicKey " + mSecurityApplicationProxy.getEccPublicKey());
            System.out.println("ManufacturerCertificate " + Arrays.toString(mSecurityApplicationProxy.getManufacturerCertificate()));
            System.out.println("ManifestTemplate " + Arrays.toString(mSecurityApplicationProxy.getManifestTemplate()));
            System.out.println("Claim Capabilities " + mSecurityApplicationProxy.getClaimCapabilities());
            System.out.println("Claim Capability Additional Info " + mSecurityApplicationProxy.getClaimCapabilityAdditionalInfo());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}

