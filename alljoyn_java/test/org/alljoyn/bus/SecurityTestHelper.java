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

package org.alljoyn.bus;

import java.io.File;

public class SecurityTestHelper {
    public static Status registerAuthListener(BusAttachment bus) throws Exception {
        return registerAuthListener(bus, openAuthListener, defaultPCListener);
    }

    public static Status registerAuthListener(BusAttachment bus, PermissionConfigurationListener customPCListener) throws Exception {
        return registerAuthListener(bus, openAuthListener, customPCListener);
    }

    public static Status registerAuthListener(BusAttachment bus, AuthListener mAuthListener, PermissionConfigurationListener pclistener) throws Exception {
        Status status = Status.OK;
        if (System.getProperty("os.name").startsWith("Windows")) {
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", mAuthListener, null, false, pclistener);
        } else if (System.getProperty("java.vm.name").startsWith("Dalvik")) {
            /*
             * on some Android devices File.createTempFile trys to create a file in
             * a location we do not have permission to write to.  Resulting in a
             * java.io.IOException: Permission denied error.
             * This code assumes that the junit tests will have file IO permission
             * for /data/data/org.alljoyn.bus
             */
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", mAuthListener,
                            "/data/data/org.alljoyn.bus/files/" + bus.getUniqueName().replaceAll(":", "") + ".ks", false, pclistener);
        } else {
            status = bus.registerAuthListener("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", mAuthListener,
                            File.createTempFile(bus.getUniqueName().replaceAll(":", ""), "ks").getAbsolutePath(), false, pclistener);
        }
        return status;
    }

    public static AuthListener openAuthListener = new AuthListener() {
        @Override
        public boolean requested(String mech, String authPeer, int count, String userName, AuthRequest[] requests) {
            return true;
        }

        @Override
        public void completed(String mech, String peer, boolean authenticated) {}
    };

    public static PermissionConfigurationListener defaultPCListener = new PermissionConfigurationListener() {

        public Status factoryReset() {
            return Status.OK;
        }

        public void policyChanged() {}

        public void startManagement() {}

        public void endManagement() {}
    };
}
