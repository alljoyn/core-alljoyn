/*
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.ifaces;

import java.util.Arrays;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.annotation.BusSignalHandler;

import junit.framework.TestCase;

public class DBusProxyObjTest extends TestCase {
    public DBusProxyObjTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment bus;

    private DBusProxyObj dbus;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        dbus = bus.getDBusProxyObj();
    }

    public void tearDown() throws Exception {
        dbus = null;

        bus.disconnect();
        bus = null;
    }

    public void testListNames() throws Exception {
        String[] names = dbus.ListNames();
        // all DBus proxyBojects should have the org.freeDesktop.DBus name
        // and org.alljoyn.Bus name as well as org.alljoyn.Daemon and org.alljoyn.sl
        // we only check for the first two.
        assertTrue(Arrays.asList(names).contains("org.freedesktop.DBus"));
        assertTrue(Arrays.asList(names).contains("org.alljoyn.Bus"));
    }

    public void testListActivatableNames() throws Exception {
        String[] names = dbus.ListActivatableNames();
        assertNotNull(names);
    }

    public void testRequestReleaseName() throws Exception {
        String name = "org.alljoyn.bus.ifaces.testRequestReleaseName";
        DBusProxyObj.RequestNameResult res1 = dbus.RequestName(name, DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res1);
        DBusProxyObj.ReleaseNameResult res2 = dbus.ReleaseName(name);
        assertEquals(DBusProxyObj.ReleaseNameResult.Released, res2);
    }

    public void testRequestNullName() throws Exception {
        boolean thrown = false;
        try {
            /* This shows up an ER_ALLJOYN_BAD_VALUE_TYPE log error. */
            dbus.RequestName(null, DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        } catch (BusException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }

    public void testNameHasOwner() throws Exception {
         assertFalse(dbus.NameHasOwner("org.alljoyn.bus.ifaces.DBusProxyObjTest"));
         assertTrue(dbus.NameHasOwner("org.alljoyn.Bus"));
    }

    public void testStartServiceByName() throws Exception {
        boolean thrown = false;
        try {
            DBusProxyObj.StartServiceByNameResult res = dbus.StartServiceByName("UNKNOWN_SERVICE", 0);
            fail("StartServiceByName returned " + res.name() + " expected ErrorReplyBusException");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }

    public void testGetNameOwner() throws Exception {
        boolean thrown = false;
        try {
            String owner = dbus.GetNameOwner("name");
            fail("Call to GetNameOwner returned " + owner + " expected ErrorReplyBusException.");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }

    public void testGetConnectionUnixUser() throws Exception {
        if ( System.getProperty("os.name").startsWith("Windows")){
            /*
             * In windows there is no UnixUser.  Calling the DBus method
             * GetConnectionUnixUser will result in a ErrorReplyBusEception when
             * running in windows.
             */
            String name = "org.alljoyn.bus.ifaces.testGetConnectionUnixUser";
            DBusProxyObj.RequestNameResult res1 = dbus.RequestName(name, DBusProxyObj.REQUEST_NAME_NO_FLAGS);
            assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res1);
            boolean thrown = false;
            try {
                int uid = dbus.GetConnectionUnixUser(name);
                fail("Got ConnectionUnixUser " + uid + " Expected ErrorReplyBusExcpetion.");
            } catch (ErrorReplyBusException ex) {
                thrown=true;
            } finally {
                assertTrue(thrown);
            }

            DBusProxyObj.ReleaseNameResult res2 = dbus.ReleaseName(name);
            assertEquals(DBusProxyObj.ReleaseNameResult.Released, res2);
        } else {
            String name = "org.alljoyn.bus.ifaces.testGetConnectionUnixUser";
            DBusProxyObj.RequestNameResult res1 = dbus.RequestName(name, DBusProxyObj.REQUEST_NAME_NO_FLAGS);
            assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res1);

            int uid = dbus.GetConnectionUnixUser(name);
            assertTrue(uid > 0);

            DBusProxyObj.ReleaseNameResult res2 = dbus.ReleaseName(name);
            assertEquals(DBusProxyObj.ReleaseNameResult.Released, res2);
        }
    }

    public void testGetConnectionUnixUserNoName() throws Exception {
        boolean thrown = false;
        try {
            int uid = dbus.GetConnectionUnixUser("name");
            fail("Got ConnectionUnixUser " + uid + " Expected ErrorReplyBusExcpetion.");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }

    public void testGetConnectionUnixProcessID() throws Exception {
        if ( System.getProperty("os.name").startsWith("Windows")){
            /*
             * In windows there is no UnixUser.  Calling the DBus method
             * GetConnectionUnixProcessID will result in a ErrorReplyBusEception
             * when running in windows.
             */
            String name = "org.alljoyn.bus.ifaces.testGetConnectionUnixProcessID";
            DBusProxyObj.RequestNameResult res1 = dbus.RequestName(name, DBusProxyObj.REQUEST_NAME_NO_FLAGS);
            assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res1);
            boolean thrown = false;
            try {
                int pid = dbus.GetConnectionUnixProcessID(name);
                fail("Got ConnectionUnixProcessID " + pid + " Expected ErrorReplyBusExcpetion.");
            } catch (ErrorReplyBusException ex) {
                thrown = true;
            } finally {
                assertTrue(thrown);
            }

            DBusProxyObj.ReleaseNameResult res2 = dbus.ReleaseName(name);
            assertEquals(DBusProxyObj.ReleaseNameResult.Released, res2);
        } else {
            String name = "org.alljoyn.bus.ifaces.testGetConnectionUnixProcessID";
            DBusProxyObj.RequestNameResult res1 = dbus.RequestName(name, DBusProxyObj.REQUEST_NAME_NO_FLAGS);
            assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res1);

            int pid = dbus.GetConnectionUnixProcessID(name);
            assertTrue(pid > 0);

            DBusProxyObj.ReleaseNameResult res2 = dbus.ReleaseName(name);
            assertEquals(DBusProxyObj.ReleaseNameResult.Released, res2);
        }
    }

    public void testGetConnectionUnixProcessIDNoName() throws Exception {
        boolean thrown = false;
        try {
            int pid = dbus.GetConnectionUnixProcessID("name");
            fail("Got ConnectionUnitProcessID " + pid + " expected ErrorReplyBusException");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }

    public void testAddRemoveMatch() throws Exception {
        dbus.AddMatch("type='signal'");
        dbus.RemoveMatch("type='signal'");
    }

    /*
     * Ignored because DBus daemon returns both a METHOD_RET and ERROR
     * message for this.  The ERROR message is discarded due to the
     * METHOD_RET (will see ER_ALLJOYN_UNMATCHED_REPLY_SERIAL in output),
     * so no exception.
     *
     * AllJoyn does not return an error.
     */
    /*
    public void testRemoveUnknownMatch() throws Exception {
        boolean thrown = false;
        try {
            dbus.RemoveMatch("type='signal'");
        } catch (BusException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }
    */

    public void testGetId() throws Exception {
        String id = dbus.GetId();
        // since the id is always a random GUID string of type 
        // fe438dc401d2834ecd4f65cf7857196e we have no way of knowing what that
        // string will be until runtime.  We will check that the string is not
        // empty and that it contains more than 4 letters. I don't know if the
        // length is always the same for that reason I am checking the length is
        // at least 4.
        assertFalse(id.equals(""));
        assertTrue(id.length() > 4);
    }

    private String newOwner;
    private String nameAcquired;

    @BusSignalHandler(iface="org.freedesktop.DBus", signal="NameOwnerChanged")
    public void nameOwnerChanged(String name, String oldOwner, String newOwner) throws BusException {
        this.newOwner = newOwner;
        synchronized (this) {
            notify();
        }
    }

    @BusSignalHandler(iface="org.freedesktop.DBus", signal="NameLost")
    public void nameLost(String name) throws BusException {
        if (nameAcquired.equals(name)) {
            nameAcquired = "";
        }
        synchronized (this) {
            notify();
        }
    }

    @BusSignalHandler(iface="org.freedesktop.DBus", signal="NameAcquired")
    public void nameAcquired(String name) throws BusException {
        nameAcquired = name;
        synchronized (this) {
            notify();
        }
    }

    public void testNameSignals() throws Exception {
        Status status = bus.registerSignalHandlers(this);
        if (Status.OK != status) {
            throw new BusException("Cannot register signal handler");
        }

        String name = "org.alljoyn.bus.ifaces.testNameSignals";
        newOwner = "";
        nameAcquired = "";
        int flags = DBusProxyObj.REQUEST_NAME_ALLOW_REPLACEMENT;
        DBusProxyObj.RequestNameResult res1 = dbus.RequestName(name, flags);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res1);
        synchronized (this) {
            long start = System.currentTimeMillis();
            while (newOwner.equals("") || !nameAcquired.equals(name)) {
                wait(1000);
                assertTrue("timed out waiting for name signals", (System.currentTimeMillis() - start) < 1000);
            }
        }

        DBusProxyObj.ReleaseNameResult res2 = dbus.ReleaseName(name);
        assertEquals(DBusProxyObj.ReleaseNameResult.Released, res2);
        synchronized (this) {
            long start = System.currentTimeMillis();
            while (!newOwner.equals("") || !nameAcquired.equals("")) {
                wait(1000);
                assertTrue("timed out waiting for name signals", (System.currentTimeMillis() - start) < 1000);
            }
        }
        bus.unregisterSignalHandlers(this);
    }

    public void testListQueuedOwners() throws Exception {
        String name = "org.alljoyn.bus.ifaces.testListQueuedOwners";
        String[] queuedNames;
        Status status = Status.OK;

        BusAttachment bus2;
        bus2 = new BusAttachment(getClass().getName());
        status = bus2.connect();
        assertEquals(Status.OK, status);

        BusAttachment bus3;
        bus3 = new BusAttachment(getClass().getName());
        status = bus3.connect();
        assertEquals(Status.OK, status);

        BusAttachment bus4;
        bus4 = new BusAttachment(getClass().getName());
        status = bus4.connect();
        assertEquals(Status.OK, status);

        /*
         * Test that no errors are returned when calling ListQueuedOwners when
         * there are no name owners
         */
        queuedNames = dbus.ListQueuedOwners(name);

        assertEquals(queuedNames.length, 0);

        /*
         * Test that no names are returned when only the primary owner has the
         * name.
         */
        int flags = BusAttachment.ALLJOYN_NAME_FLAG_ALLOW_REPLACEMENT;
        status = bus.requestName(name, flags);
        assertEquals(Status.OK, status);

        queuedNames = dbus.ListQueuedOwners(name);

        assertEquals(queuedNames.length, 0);


        /*
         * Test that names that already have a primary owner are being queued
         */
        flags = 0;
        status = bus2.requestName(name, flags);
        assertEquals(Status.DBUS_REQUEST_NAME_REPLY_IN_QUEUE, status);

        flags = 0;
        status = bus3.requestName(name, flags);
        assertEquals(Status.DBUS_REQUEST_NAME_REPLY_IN_QUEUE, status);

        queuedNames = dbus.ListQueuedOwners(name);

        assertEquals(queuedNames.length, 2);
        assertEquals(queuedNames[0], bus2.getUniqueName());
        assertEquals(queuedNames[1], bus3.getUniqueName());

        /*
         * Test that the ALLJOYN_NAME_FLAG_ALLOW_REPLACEMENT affecting the queue
         * as it should
         */
        flags = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING;
        status = bus4.requestName(name, flags);
        assertEquals(Status.OK, status);

        queuedNames = dbus.ListQueuedOwners(name);

        assertEquals(queuedNames.length, 3);
        assertEquals(queuedNames[0], bus.getUniqueName());
        assertEquals(queuedNames[1], bus2.getUniqueName());
        assertEquals(queuedNames[2], bus3.getUniqueName());

        //cleanup
        bus2.releaseName(name);
        bus3.releaseName(name);
        bus4.releaseName(name);
    }
}
