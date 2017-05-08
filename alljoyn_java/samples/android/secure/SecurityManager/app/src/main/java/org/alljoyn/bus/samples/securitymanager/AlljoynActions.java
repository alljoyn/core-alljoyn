/**
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

package org.alljoyn.bus.samples.securitymanager;

import android.content.Context;
import android.util.Log;
import android.os.Message;
import android.os.Handler;

import org.alljoyn.bus.ApplicationStateListener;
import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.PermissionConfigurationListener;
import org.alljoyn.bus.PermissionConfigurator;
import org.alljoyn.bus.SecurityApplicationProxy;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.CryptoECC;
import org.alljoyn.bus.common.ECCPublicKey;
import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.samples.securitymanager.SecurityApplicationStateListener;

import java.io.File;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.jar.Manifest;

public class AlljoynActions {

    static {
        System.loadLibrary("alljoyn_java");
    }

    public static final int FAIL = 0;
    public static final int SUCCESS = 1;

    private static final short PORT = 101;

    private static final String MANIFEST_XML =
        "<manifest>" +
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
    private static final String TAG = "AlljoynAction";

    private BusAttachment bus;
    private UUID managerUuid;
    private UUID managerGroupUuid;
    private KeyInfoNISTP256 managerKey;

    private Context context;
    private Handler errorHandler;
    protected SecurityApplicationStateListener secAppStateListener;

    public AlljoynActions(Context context,
            Handler errorHandler,
            SecurityApplicationStateListener secAppStateListener)
    {
        this.context = context;
        this.errorHandler = errorHandler;
        this.secAppStateListener = secAppStateListener;
    }

    protected Runnable busStart = new Runnable() {
        public void run() {
            bus = new BusAttachment("SecurityManager");
            bus.connect();

            bus.registerAboutListener(new AboutAppNameListener());
            bus.whoImplements(null);

            managerGroupUuid = UUID.randomUUID();
            managerUuid = UUID.randomUUID();

            Status status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA",
                    mAuthListener,
                    context.getFileStreamPath("alljoyn_keystore").getAbsolutePath(),
                    false,
                    mPcListener);

            if (status != Status.OK) {
                notifyHandler(false, "Failed to registerAuthListener " +
                        status.toString() + " Restart Application.");
                return;
            }

            PermissionConfigurator pc = bus.getPermissionConfigurator();
            try {
                managerKey = pc.getSigningPublicKey();
                managerKey.setKeyId(CertificateX509.generateAuthorityKeyId(managerKey.getPublicKey()));
                pc.setManifestTemplateFromXml(MANIFEST_XML);

                CertificateX509 identityCert = CertificateHelper.generateSignedIdentityCertificate(bus,
                        managerKey.getPublicKey());
                String signedManifest = pc.computeThumbprintAndSignManifestXml(identityCert, MANIFEST_XML);
                pc.claim(managerKey,
                        UUID.randomUUID(),
                        managerKey,
                        new CertificateX509[]{identityCert},
                        new String[]{signedManifest});

                CertificateX509 membershipCert = CertificateHelper.generateSignedMembershipCertificate(bus,
                        managerKey.getPublicKey(),
                        managerUuid);

                pc.installMembership(new CertificateX509[]{membershipCert});
            } catch (BusException e) {
                notifyHandler(false, "Failed to claim or installMembership " +
                        e.toString());
                return;
            }
            bus.registerApplicationStateListener(secAppStateListener);

        }
    };

    private AuthListener mAuthListener = new AuthListener() {
        @Override
        public boolean requested(String mechanism, String authPeer, int count, String userName, AuthRequest[] requests) {
            return true;
        }

        @Override
        public void completed(String mechanism, String authPeer, boolean authenticated) {
        }
    };

    public void setPosition(int position) {
        this.position = position;
    }

    private int position;

    private SessionOpts sessionOpts = new SessionOpts();
    protected Runnable claimAndAddMembership = new Runnable() {

        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    (short) PORT,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                notifyHandler(false, "Failed to joinSession " + status.toString());
                return;
            }

            List<KeyInfoNISTP256> tempKeys = new ArrayList<>(secAppStateListener.getMap().keySet());

            SecurityApplicationProxy sap = null;
            try {
                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);

                CertificateX509 identity = CertificateHelper.generateSignedIdentityCertificate(bus,
                        tempKeys.get(position).getPublicKey());
                String signedManifest = bus.getPermissionConfigurator().computeThumbprintAndSignManifestXml(
                        identity,
                        sap.getManifestTemplate());

                sap.claim(managerKey,
                        managerUuid,
                        managerKey,
                        new CertificateX509[]{identity},
                        new String[]{signedManifest});
                bus.secureConnection(null, true);
            } catch (Exception e) {
                notifyHandler(false, "Failed to claim " + e.toString());
                return;
            }

            try {
                sap.endManagement();
            } catch (Exception e) {
                notifyHandler(false, "Failed to endManagement " + e.toString());
                return;
            }

            notifyHandler(true, "Claiming Succeeded");
        }
    };

    protected Runnable installMembership = new Runnable() {
        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    PORT,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                notifyHandler(false, "Failed to joinSession " + status.toString());
                return;
            }

            SecurityApplicationProxy sap = null;
            try {
                status = bus.secureConnection(tempList.get(position).getBusName(), true);
                if (status != Status.OK) {
                    notifyHandler(false, "Failed to secureConnection " + status.toString());
                    return;
                }

                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);

                sap.startManagement();

                CertificateX509 membership = CertificateHelper.generateSignedMembershipCertificate(bus,
                        sap.getEccPublicKey(),
                        managerGroupUuid);
                sap.installMembership(new CertificateX509[]{membership});
                sap.endManagement();
                bus.leaveSession(sessionId.value);
            } catch (BusException e) {
                notifyHandler(false, "Failed to installMembership " + e.toString());
                return;
            }

            notifyHandler(true, "install Membership succeeded");
        }
    };

    protected Runnable installPolicy = new Runnable() {
        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    PORT,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                notifyHandler(false, "Failed to joinSession " + status.toString());
                return;
            }

            SecurityApplicationProxy sap = null;
            try {
                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);

                status = bus.secureConnection(tempList.get(position).getBusName(), true);
                if (status != Status.OK) {
                    notifyHandler(false, "Failed to secureConnection " + status.toString());
                    return;
                }

                sap.startManagement();
                String policy = PolicyHelper.POLICY_BEGIN +
                        PolicyHelper.CA_ACL +
                        PolicyHelper.ACL_ANY +
                        PolicyHelper.ACL_END +
                        PolicyHelper.MEMBER_ACL +
                        PolicyHelper.ACL_ANY +
                        PolicyHelper.ACL_END +
                        PolicyHelper.POLICY_END;

                policy = policy.replace(PolicyHelper.REPLACE_CA,
                        CertificateX509.encodePublicKeyPEM(managerKey.getPublicKey()));
                policy = policy.replace(PolicyHelper.REPLACE_MEMBER,
                        CertificateX509.encodePublicKeyPEM(managerKey.getPublicKey()));
                policy = policy.replace(PolicyHelper.REPLACE_MEMBER_GUID,
                        managerGroupUuid.toString().replace("-", ""));
                sap.updatePolicy(policy);
                sap.endManagement();
                bus.leaveSession(sessionId.value);
            } catch (BusException e) {
                notifyHandler(false, "Failed to installPolicy " + e.toString());
                return;
            }

            notifyHandler(true, "installPolicy succeeded");
        }
    };

    protected Runnable unclaim = new Runnable() {
        @Override
        public void run() {
            Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

            List<OnlineApplication> tempList = new ArrayList<>(secAppStateListener.getMap().values());

            Status status = bus.joinSession(tempList.get(position).getBusName(),
                    PORT,
                    sessionId,
                    sessionOpts,
                    new SessionListener());

            if (status != Status.OK) {
                notifyHandler(false, "Failed to joinSession " + status.toString());
                return;
            }

            SecurityApplicationProxy sap = null;
            try {

                status = bus.secureConnection(tempList.get(position).getBusName(), true);
                if (status != Status.OK) {
                    notifyHandler(false, "Failed to secureConnection " + status.toString());
                    return;
                }

                sap = new SecurityApplicationProxy(bus,
                        tempList.get(position).getBusName(),
                        sessionId.value);
                sap.startManagement();
                sap.reset();

            } catch (BusException e) {
                notifyHandler(false, "Failed to unclaim " + e.toString());
                return;
            }

            notifyHandler(true, "Unclaim Succeeded");
        }
    };

    private PermissionConfigurationListener mPcListener = new PermissionConfigurationListener() {
        @Override
        public Status factoryReset() {
            Log.i(TAG, "factoryReset()");
            return Status.OK;
        }

        @Override
        public void policyChanged() {
            Log.i(TAG, "policyChanged()");
        }

        @Override
        public void startManagement() {
            Log.i(TAG, "startManagement()");
        }

        @Override
        public void endManagement() {
            Log.i(TAG, "endManagement()");
        }
    };

    private void notifyHandler(boolean success, String message) {
        Message msg = errorHandler.obtainMessage(success ? SUCCESS : FAIL, message);
        errorHandler.sendMessage(msg);
    }
}
