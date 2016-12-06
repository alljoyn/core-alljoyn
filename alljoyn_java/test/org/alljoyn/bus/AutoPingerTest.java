/*
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
import java.util.Set;
import java.util.HashSet;
import org.alljoyn.bus.BusAttachment.RemoteMessage;

public class AutoPingerTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment busAttachment;
    private AutoPinger autoPinger;

    public void setUp() throws Exception {
        busAttachment = new BusAttachment("AutoPingerTestBusAttachment");
        assertEquals(Status.OK, busAttachment.connect());
        autoPinger = new AutoPinger(busAttachment);
    }

    /** this test function attempts to mirror the c++ test function */
    public void testBasic() {
        BusAttachment clientBus = new BusAttachment("app", RemoteMessage.Ignore);
        assertEquals(Status.OK, clientBus.connect());
        TestPingListener testListener = new TestPingListener();
        autoPinger.addPingGroup("testgroup", testListener, 1);

        String uniqueName = clientBus.getUniqueName();
        assertEquals(Status.BUS_PING_GROUP_NOT_FOUND, autoPinger.addDestination("badgroup", uniqueName));
        assertEquals(Status.OK, autoPinger.addDestination("testgroup", uniqueName));
        assertEquals(Status.OK, autoPinger.addDestination("testgroup", uniqueName));

        testListener.WaitUntilFound(uniqueName);
        clientBus.disconnect();
        testListener.WaitUntilLost(uniqueName);

        assertEquals(Status.BUS_PING_GROUP_NOT_FOUND, autoPinger.removeDestination("badgroup", uniqueName));
        assertEquals(Status.OK, autoPinger.removeDestination("testgroup", uniqueName));
        assertEquals(Status.OK, autoPinger.removeDestination("testgroup", uniqueName));

        assertEquals(Status.BUS_PING_GROUP_NOT_FOUND, autoPinger.setPingInterval("badgroup", 2));
        assertEquals(Status.OK, autoPinger.setPingInterval("testgroup", 2)); /* no real test on updated interval */

        autoPinger.pause();
        autoPinger.pause();
        autoPinger.resume();
        autoPinger.resume();

        clientBus.connect();
        uniqueName = clientBus.getUniqueName();
        assertEquals(Status.OK, autoPinger.addDestination("testgroup", uniqueName));

        testListener.WaitUntilFound(uniqueName);
        autoPinger.removePingGroup("badgroup");
        autoPinger.removePingGroup("testgroup");

        clientBus.disconnect();
    }

    /** this test function attempts to mirror the c++ test function */
    public void testMultibus() {
        final int G = 2;
        final int N = 5;
        TestPingListener[] testListener = {new TestPingListener(), new TestPingListener()};
        String groupNames[] = { "evengroup", "oddgroup" };

        for (int i = 0; i < G; i++) {
            autoPinger.addPingGroup(groupNames[i], testListener[i], 1);
        }

        BusAttachment[] serviceBuses = new BusAttachment[N];
        String[] uniqueNames = new String[N];

        for (int i = 0; i < serviceBuses.length; i++) {
            serviceBuses[i] = new BusAttachment("test");
            serviceBuses[i].connect();
            uniqueNames[i] = serviceBuses[i].getUniqueName();
            autoPinger.addDestination(groupNames[i % G], uniqueNames[i]);
        }

        for (int i = 0; i < serviceBuses.length; i++) {
            testListener[i % G].WaitUntilFound(uniqueNames[i]);
            serviceBuses[i].disconnect();
        }

        for (int i = 0; i < serviceBuses.length; i++) {
            testListener[i % G].WaitUntilLost(uniqueNames[i]);
        }

    }

    public void testAutoPinger() {
        autoPinger.pause();
        try {
            Thread.sleep(1000);
        } catch (Exception e) {}
        autoPinger.resume();
        autoPinger.addPingGroup("group1", new AutoPingListener() {
            public void destinationLost(String group, String destination) {
                System.out.println("group " + group + ", destination " + destination);
            }

            public void destinationFound(String group, String destination) {
                System.out.println("group " + group + ", destination " + destination);
            }
        }, 2);
    }

    public class TestPingListener implements AutoPingListener {

        private static final int MAX_RETRIES = 1000;
        private Set<String> found;
        private Set<String> lost;

        public TestPingListener() {
            found = new HashSet<String>();
            lost = new HashSet<String>();
        }

        void WaitUntilFound(String destination) {
            int retries = 0;
            System.out.println("Wait until we see " + destination);
            while (retries < MAX_RETRIES && !found.contains(destination)) {
                try {
                    Thread.sleep(10);
                } catch (Exception e) {}
                retries++;
            }
            assertFalse(MAX_RETRIES == retries);
        }

        void WaitUntilLost(String destination) {
            int retries = 0;
            System.out.println("Wait until we see " + destination);
            while (retries < MAX_RETRIES && !lost.contains(destination)) {
                try {
                    Thread.sleep(10);
                } catch (Exception e) {}
                retries++;
            }
            assertFalse(MAX_RETRIES == retries);
        }

        public void destinationLost(String group, String destination) {
            System.out.println("group " + group + ", destination " + destination);
            lost.add(destination);
        }

        public void destinationFound(String group, String destination) {
            System.out.println("group " + group + ", destination " + destination);
            found.add(destination);
        }
    }
}
