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
package org.alljoyn.bus;

import junit.framework.TestCase;

import java.io.File;

public class PermissionConfiguratorTest extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    private PermissionConfigurator permissionConfigurator;
    private BusAttachment busAttachment;

    public void setUp() throws Exception {
        busAttachment = new BusAttachment("PermissionConfiguratorTest");
        busAttachment.connect();
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

    public void testBasic() throws Exception {
        busAttachment.registerAuthListener("ALLJOYN_ECDHE_NULL", null);
        permissionConfigurator = busAttachment.getPermissionConfigurator();
        assertEquals(permissionConfigurator.getApplicationState(), PermissionConfigurator.ApplicationState.NOT_CLAIMABLE);
    }
}

