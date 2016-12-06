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
package org.alljoyn.bus;

import junit.framework.TestCase;

import java.io.File;

import org.alljoyn.bus.common.KeyInfoNISTP256;
import org.alljoyn.bus.common.KeyInfoECC;

public class PermissionConfiguratorTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    private PermissionConfigurator permissionConfigurator;
    private BusAttachment busAttachment;

    private String defaultManifestTemplate = "<manifest>" +
    "<node>" +
    "<interface name=\"org.alljoyn.security2.test\">" + 
    "<method name=\"Up\">" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
    "</method>" +
    "<method name=\"Down\">" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
    "</method>" +
    "</interface>" +
    "<interface name=\"org.allseenalliance.control.Mouse*\">" +
    "<any>" +
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" +
    "</any>" +
    "</interface>" +
    "</node>" +
    "</manifest>";

    public void setUp() throws Exception {
        busAttachment = new BusAttachment("PermissionConfiguratorTest");
        busAttachment.connect();
    }

    public void tearDown() throws Exception {
        busAttachment.disconnect();
        busAttachment.release();
        busAttachment = null;
        permissionConfigurator = null;
    }

    private Status registerAuthListener() throws Exception {
        Status status = Status.OK;
        if (System.getProperty("os.name").startsWith("Windows")) {
            status = busAttachment.registerAuthListener("ALLJOYN_ECDHE_NULL", null, null, false, pclistener);
        } else if (System.getProperty("java.vm.name").startsWith("Dalvik")) {
            /*
             * on some Android devices File.createTempFile trys to create a file in
             * a location we do not have permission to write to.  Resulting in a
             * java.io.IOException: Permission denied error.
             * This code assumes that the junit tests will have file IO permission
             * for /data/data/org.alljoyn.bus
             */
            status = busAttachment.registerAuthListener("ALLJOYN_ECDHE_NULL", null,
                            "/data/data/org.alljoyn.bus/files/alljoyn.ks", false, pclistener);
        } else {
            status = busAttachment.registerAuthListener("ALLJOYN_ECDHE_NULL", null,
                            File.createTempFile("alljoyn", "ks").getAbsolutePath(), false, pclistener);
        }
        return status;
    }

    public void testVeryBasic() throws Exception {
        //Need to register AuthListener to be able to use permissionConfigurator
        permissionConfigurator = busAttachment.getPermissionConfigurator();
        try {
            permissionConfigurator.getApplicationState();
            assertTrue(false);
        } catch (Exception e) {
            assertEquals(e.getMessage(), "ER_" + Status.FEATURE_NOT_AVAILABLE);
        }
    }

    public void testNotClaimable() throws Exception {
        assertEquals(Status.OK, busAttachment.registerAuthListener("ALLJOYN_ECDHE_NULL", null,
                File.createTempFile("alljoyn","ks").getAbsolutePath(), false, pclistener));
        SecurityApplicationProxy sap = new SecurityApplicationProxy(busAttachment,busAttachment.getUniqueName(),(short)0);
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, sap.getApplicationState());
    }

    public void testBasic() throws Exception {
        assertEquals(Status.OK, busAttachment.registerAuthListener("ALLJOYN_ECDHE_NULL", null,
                File.createTempFile("alljoyn","ks").getAbsolutePath(), false, pclistener));

        SecurityApplicationProxy sap = new SecurityApplicationProxy(busAttachment,busAttachment.getUniqueName(),(short)0);
        assertEquals(PermissionConfigurator.ApplicationState.NOT_CLAIMABLE, sap.getApplicationState());

        permissionConfigurator = busAttachment.getPermissionConfigurator();
        permissionConfigurator.setManifestTemplateFromXml(defaultManifestTemplate);

        assertEquals(PermissionConfigurator.ApplicationState.CLAIMABLE, sap.getApplicationState());

        KeyInfoNISTP256 securityManagerKey = permissionConfigurator.getSigningPublicKey();
    }

    private boolean factoryReset = false;
    private boolean policyChanged = false;
    private boolean startManagement = false;
    private boolean endManagement = false;
    private PermissionConfigurationListener pclistener = new PermissionConfigurationListener() {

        public Status factoryReset() {
            factoryReset = true;
            return Status.OK;
        }

        public void policyChanged() {
            policyChanged = true;
        }

        public void startManagement() {
            startManagement = true;
        }

        public void endManagement() {
            endManagement = true;
        }
    };
}
