/**
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

package org.alljoyn.bus;

import java.lang.ref.WeakReference;

import junit.framework.TestCase;

import org.alljoyn.bus.annotation.BusSignalHandler;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Introspectable;
import org.alljoyn.bus.common.KeyInfoNISTP256;

import org.alljoyn.bus.GenericInterface;

import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.MethodDef;
import org.alljoyn.bus.defs.SignalDef;
import org.alljoyn.bus.defs.PropertyDef;
import org.alljoyn.bus.defs.ArgDef;

import java.lang.reflect.Method;
import java.lang.reflect.Type;
import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;

public class BusAttachmentTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    int receivedSimpleServicePing;
    int receivedGetSignalHandler;
    int receivedGetMethodHandler;
    int receivedGetPropertyHandler;

    private int uniquifier = 0;
    private String genUniqueName(BusAttachment bus) {
        return "test.x" + bus.getGlobalGUIDString() + ".x" + uniquifier++;
    }

    public class Emitter implements EmitterInterface, BusObject {
        private SignalEmitter emitter;

        public Emitter() {
            emitter = new SignalEmitter(this);
            emitter.setTimeToLive(1000);
        }
        public void emit(String string) throws BusException {
            emitter.getInterface(EmitterInterface.class).emit(string);
        }
    }

    public class Service implements BusObject {
    }

    public class SimpleService implements SimpleInterface, BusObject {
        public String ping(String str) {
            receivedSimpleServicePing++;
            MessageContext ctx = bus.getMessageContext();
            assertEquals(false, ctx.isUnreliable);
            assertEquals("/simple", ctx.objectPath);
            assertEquals("org.alljoyn.bus.SimpleInterface", ctx.interfaceName);
            assertEquals("Ping", ctx.memberName);
            assertEquals("s", ctx.signature);
            assertEquals("", ctx.authMechanism);
            return str;
        }
    }

    public class SecureService implements SecureInterface, BusObject {
        public String ping(String str) { return str; }
    }

    public class ExceptionService implements SimpleInterface, BusObject {
        private int i;
        public String ping(String str) throws BusException {
            switch (i++) {
            case 0:
                throw new ErrorReplyBusException(Status.OS_ERROR);
            case 1:
                throw new ErrorReplyBusException("org.alljoyn.bus.ExceptionService.Error1");
            case 2:
                throw new ErrorReplyBusException("org.alljoyn.bus.ExceptionService.Error2", "Message");
            case 3:
                throw new BusException("BusException");
            }
            return str;
        }
    }

    public class AnnotationService implements AnnotationInterface,
    BusObject {
        public String deprecatedMethod(String str) throws BusException { return str; }
        public void noReplyMethod(String str) throws BusException {}
        public void deprecatedNoReplyMethod(String str) throws BusException {}
        public void deprecatedSignal(String str) throws BusException {}
        public String getChangeNotifyProperty() throws BusException { return ""; }
        public void setChangeNotifyProperty(String str) throws BusException {}
    }

    private BusAttachment bus;
    private WeakReference<BusAttachment> busRef = new WeakReference<BusAttachment>(bus);
    private BusAttachment otherBus;
    private WeakReference<BusAttachment> otherBusRef = new WeakReference<BusAttachment>(otherBus);
    private int handledSignals1;
    private int handledSignals2;
    private int handledSignals3;
    private int handledSignals4;
    private boolean pinRequested;
    private String address;

    @Override
    public void setUp() throws Exception {
        bus = null;
        otherBus = null;
        address = System.getProperty("org.alljoyn.bus.address", "unix:abstract=alljoyn");

        handledSignals1 = 0;
        handledSignals2 = 0;
        handledSignals3 = 0;
        handledSignals4 = 0;
        pinRequested = false;

        found = false;
        lost = false;

        foundNameA = false;
        foundNameB = false;

        foundName1 = false;
        foundName2 = false;
        foundName3 = false;
        transport2 = 0;

        sessionAccepted = false;
        sessionJoined = false;
        onJoined = false;
        joinSessionStatus = Status.NONE;
        busSessionId = -1;
        otherBusSessionId = -1;

        sessionLost = false;
        sessionLostReason = -1;

        sessionJoinedFlag = false;
        sessionLostFlagA = false;
        sessionMemberAddedFlagA = false;
        sessionMemberRemovedFlagA = false;
        sessionLostFlagB = false;
        sessionMemberAddedFlagB = false;
        sessionMemberRemovedFlagB = false;

        onPinged = false;

        receivedSimpleServicePing = 0;
        receivedGetSignalHandler = 0;
        receivedGetMethodHandler = 0;
        receivedGetPropertyHandler = 0;
    }

    @Override
    public void tearDown() throws Exception {
        System.setProperty("org.alljoyn.bus.address", address);

        if (bus != null) {
            assertTrue(bus.isConnected());
            bus.disconnect();
            assertFalse(bus.isConnected());
            bus.release();
            bus = null;
        }

        if (otherBus != null) {
            otherBus.disconnect();
            otherBus.release();
            otherBus = null;
        }
        /*
         * Each BusAttachment is a very heavy object that creates many file
         * descripters for each BusAttachment.  This will force Java's Garbage
         * collector to remove the BusAttachments 'bus' and 'serviceBus' before
         * continuing on to the next test.
         */
        do{
            System.gc();
            Thread.sleep(5);
        }
        while (busRef.get() != null || otherBusRef.get() != null);
    }

    public synchronized void stopWait() {
        this.notifyAll();
    }

    public class Lambda {
        public boolean func() { return false; }
    }

    public boolean waitForLambda(long waitMs, Lambda expression) throws Exception {
        boolean ret = expression.func();
        long endMs = System.currentTimeMillis() + waitMs;
        while (!ret && (System.currentTimeMillis() <= endMs)) {
            Thread.sleep(5);
            ret = expression.func();
        }
        return ret;
    }

    public void testEmitSignalFromUnregisteredSource() throws Exception {
        boolean thrown = false;
        try {
            Emitter emitter = new Emitter();
            emitter.emit("emit1");
            emitter = null;
        } catch (BusException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }

    }

    public void signalHandler1(String string) throws BusException {
        ++handledSignals1;
    }

    public void signalHandler2(String string) throws BusException {
        ++handledSignals2;
    }

    public void signalHandler3(String string)
        throws BusException
    {
        ++handledSignals3;
    }

    /* As opposed to signalHandler4 this one only increments the value */
    public void signalHandler4Basic(String string)
        throws BusException
    {
        ++handledSignals4;
    }

    public synchronized void testRegisterMultipleSignalHandlersForOneSignal() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        Emitter emitter = new Emitter();
        status = bus.registerBusObject(emitter, "/emitter");
        assertEquals(Status.OK, status);

        handledSignals1 = handledSignals2 = 0;
        status = bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                this, getClass().getMethod("signalHandler1", String.class));
        assertEquals(Status.OK, status);
        status = bus.addMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        assertEquals(Status.OK, status);
        status = bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                this, getClass().getMethod("signalHandler2", String.class));
        assertEquals(Status.OK, status);
        status =
            bus.registerSignalHandlerWithRule("org.alljoyn.bus.EmitterInterface", "Emit", this,
                getClass().getMethod("signalHandler3", String.class), "path='/emitter'");
        assertEquals(Status.OK, status);
        status =
            bus.registerSignalHandlerWithRule("org.alljoyn.bus.EmitterInterface", "Emit", this,
                getClass().getMethod("signalHandler4Basic", String.class), "path='/wrongpath'");
        assertEquals(Status.OK, status); // This signalhandler must never be hit

        //
        // Emit a signal and make sure that both of the handlers we registered
        // receive that signal.
        //
        emitter.emit("emit1");

        //
        // It's hard to say what the minimum time to wait is, but we have to
        // impose a maximum time or we wait forever.  Any conceivable machine
        // whether running on a VM or not should be able to send and receive a
        // signal in ten seconds.  We'll do the wait in increments of one second
        // to minimize the time in the best case.
        //
        for (int i = 0; i < 10; ++i) {
            this.wait(1000);
            //
            // Break out as soon as we see the result we need.
            //
            if (handledSignals1 == 1 && handledSignals2 == 1 && handledSignals3 == 1) {
                break;
            }
        }

        assertEquals(1, handledSignals1);
        assertEquals(1, handledSignals2);
        assertEquals(1, handledSignals3);
        assertEquals(0, handledSignals4);

        handledSignals1 = handledSignals2 = handledSignals3 = 0;

        //
        // Unregister one of the handlers and make sure that the unregistered
        // handler does not receive the signal but the second handler continues
        // to get the signal.
        //
        bus.unregisterSignalHandler(this, getClass().getMethod("signalHandler1", String.class));
        emitter.emit("emit2");

        //
        // We want to wait go make sure that we *don't* see a signal, so we
        // have to wait long enough for any conceivable machine to possible
        // receive one.  We can't really wait until the one is received since
        // the driving thread could be blocked for unknown amounts of time and
        // could simply come back later and deliver the signal.  So this test
        // is hard to do in a minimum time.  Since we pulled a time out of the
        // hat above when we decided that any reasonable machine must be able
        // to do a signal in ten seconds, we should use that here as well.
        //
        this.wait(10000);
        assertEquals(0, handledSignals1);
        assertEquals(1, handledSignals2);
        assertEquals(1, handledSignals3);
        assertEquals(0, handledSignals4); // This must always be zero
        bus.unregisterSignalHandler(this, getClass().getMethod("signalHandler2", String.class));
        status = bus.removeMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        assertEquals(Status.OK, status);
        emitter = null;
        status = null;
    }

    public void testRegisterSignalHandlerNoLocalObject() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        status = bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                this, getClass().getMethod("signalHandler1", String.class));
        assertEquals(Status.OK, status);
        bus.unregisterSignalHandler(this, getClass().getMethod("signalHandler1", String.class));
    }

    public class SignalHandlers {
        //@BusSignalHandler(iface = "org.alljoyn.bus.EmitterFoo", signal = "Emit") // ClassNotFound exception
        public void signalHandler1(String string) throws BusException {
        }

        @BusSignalHandler(iface = "org.alljoyn.bus.EmitterBarInterface", signal = "Emit")
        public void signalHandler2(String string) throws BusException {
        }

        @BusSignalHandler(iface = "org.alljoyn.bus.EmitterBarInterface", signal = "EmitBar")
        public void signalHandler3(String string) throws BusException {
        }
    }

    public void testRegisterSignalHandlersNoLocalObject() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        SignalHandlers handlers = new SignalHandlers();
        status = bus.registerSignalHandlers(handlers);
        assertEquals(Status.OK, status);
    }

    public synchronized void testRegisterSignalHandlerBeforeLocalObject() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        status = bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                this, getClass().getMethod("signalHandler1", String.class));
        assertEquals(Status.OK, status);
        status = bus.addMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        assertEquals(Status.OK, status);

        Emitter emitter = new Emitter();
        status = bus.registerBusObject(emitter, "/emitter");
        assertEquals(Status.OK, status);

        handledSignals1 = 0;
        emitter.emit("emit1");
        this.wait(500);
        assertEquals(1, handledSignals1);
        emitter = null;
        status = bus.removeMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        assertEquals(Status.OK, status);
    }

    public synchronized void testRegisterSourcedSignalHandler() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        Emitter emitter = new Emitter();
        status = bus.registerBusObject(emitter, "/emitter");
        assertEquals(Status.OK, status);

        status = bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                this, getClass().getMethod("signalHandler1", String.class),
                "/emitter");
        assertEquals(Status.OK, status);
        status = bus.addMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        assertEquals(Status.OK, status);
        handledSignals1 = 0;
        emitter.emit("emit1");
        this.wait(500);
        assertEquals(1, handledSignals1);

        bus.unregisterSignalHandler(this, getClass().getMethod("signalHandler1", String.class));
        status = bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                this, getClass().getMethod("signalHandler1", String.class),
                "/doesntexist");
        assertEquals(Status.OK, status);
        handledSignals1 = 0;
        emitter.emit("emit1");
        this.wait(500);
        assertEquals(0, handledSignals1);
        emitter = null;
        status = bus.removeMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        assertEquals(Status.OK, status);
    }

    // suppressing the unused signalHandler3 warning this is used in the test
    // but the compiler does not know.
    @SuppressWarnings("unused")
     private void unusablePrivateFunction(String string) throws BusException {
     }

    public void testRegisterPrivateSignalHandler() throws Exception {
        boolean thrown = false;
        try {
            bus = new BusAttachment(getClass().getName());
            Status status = bus.connect();
            assertEquals(Status.OK, status);

            status = bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                    this, getClass().getMethod("unusablePrivateFunction", String.class));
            assertEquals(Status.OK, status);
        } catch (NoSuchMethodException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }

    public void testReregisterNameAfterDisconnect() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        status = bus.requestName("org.alljoyn.bus.BusAttachmentTest", 0);
        assertEquals(Status.OK, status);

        bus.release();

        bus = new BusAttachment(getClass().getName());
        status = bus.connect();
        assertEquals(Status.OK, status);

        status = bus.requestName("org.alljoyn.bus.BusAttachmentTest", 0);
        assertEquals(Status.OK, status);
    }

    public void testRegisterSameLocalObjectTwice() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        Service service = new Service();
        status = bus.registerBusObject(service, "/testobject");
        assertEquals(Status.OK, status);
        status = bus.registerBusObject(service, "/testobject");
        assertEquals(Status.BUS_OBJ_ALREADY_EXISTS, status);
    }

    public void signalHandler4(String string) throws BusException {
        ++handledSignals4;
        MessageContext ctx = bus.getMessageContext();
        assertEquals(true, ctx.isUnreliable);
        assertEquals("/emitter", ctx.objectPath);
        assertEquals("org.alljoyn.bus.EmitterInterface", ctx.interfaceName);
        assertEquals("Emit", ctx.memberName);
        assertEquals("s", ctx.signature);
        assertEquals("", ctx.authMechanism);
        stopWait();
    }

    public synchronized void testSignalMessageContext() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        Emitter emitter = new Emitter();
        status = bus.registerBusObject(emitter, "/emitter");
        assertEquals(Status.OK, status);

        handledSignals4 = 0;
        status = bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                this, getClass().getMethod("signalHandler4", String.class));
        assertEquals(Status.OK, status);
        status = bus.addMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        assertEquals(Status.OK, status);

        emitter.emit("emit4");
        this.wait(500);
        assertEquals(1, handledSignals4);
        emitter = null;
        status = bus.removeMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        assertEquals(Status.OK, status);
    }

    public void testMethodMessageContext() throws Exception {
        bus = new BusAttachment(getClass().getName());
        SimpleService service = new SimpleService();
        assertEquals(Status.OK, bus.registerBusObject(service, "/simple"));

        assertEquals(Status.OK, bus.connect());
        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                        DBusProxyObj.REQUEST_NAME_NO_FLAGS));
        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                "/simple",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterface.class });
        SimpleInterface proxy = proxyObj.getInterface(SimpleInterface.class);
        proxy.ping("hello");
    }

    /* ALLJOYN-26 */
    public void testRegisterUnknownAuthListener() throws Exception {
        bus = new BusAttachment(getClass().getName());
        bus.registerKeyStoreListener(new NullKeyStoreListener());
        Status status = bus.connect();
        assertEquals(Status.OK, status);
        status = bus.registerAuthListener("ALLJOYN_BOGUS_KEYX", new AuthListener() {
            public boolean requested(String mechanism, String authPeer, int count, String userName,
                    AuthRequest[] requests) {
                return false;
            }
            public void completed(String mechanism, String authPeer, boolean authenticated) {}
        });
        assertEquals(Status.BUS_INVALID_AUTH_MECHANISM, status);
    }

    private AuthListener authListener = new AuthListener() {
        public boolean requested(String mechanism, String authPeer, int count, String userName,
                AuthRequest[] requests) {
            pinRequested = true;
            for (AuthRequest request : requests) {
                if (request instanceof PasswordRequest) {
                    ((PasswordRequest) request).setPassword("123456".toCharArray());
                } else if (request instanceof ExpirationRequest) {
                } else {
                    return false;
                }
            }
            return true;
        }

        public void completed(String mechanism, String authPeer, boolean authenticated) {}
    };

    public void testRegisterAuthListenerBeforeConnect() throws Exception {
        bus = new BusAttachment(getClass().getName());
        bus.registerKeyStoreListener(new NullKeyStoreListener());
        SecureService service = new SecureService();
        Status status = bus.registerBusObject(service, "/secure");
        assertEquals(Status.OK, status);
        status = bus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener);
        assertEquals(Status.OK, status);
        status = bus.connect();
        assertEquals(Status.OK, status);
        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res);

        BusAttachment otherBus = new BusAttachment(getClass().getName());
        otherBus.registerKeyStoreListener(new NullKeyStoreListener());
        status = otherBus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener);
        assertEquals(Status.OK, status);
        status = otherBus.connect();
        assertEquals(Status.OK, status);
        ProxyBusObject proxyObj = otherBus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                "/secure",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SecureInterface.class });
        SecureInterface proxy = proxyObj.getInterface(SecureInterface.class);
        pinRequested = false;
        proxy.ping("hello");
        assertEquals(true, pinRequested);

        otherBus.disconnect();
        otherBus.release();
        otherBus = null;
    }

    public void testBasicSecureConnection() throws Exception {
        bus = new BusAttachment(getClass().getName());
        bus.registerKeyStoreListener(new NullKeyStoreListener());
        Status status = bus.connect();
        assertEquals(status, Status.OK);

        BusAttachment otherBus = new BusAttachment(getClass().getName());
        otherBus.registerKeyStoreListener(new NullKeyStoreListener());
        status = otherBus.connect();
        assertEquals(status, Status.OK);

        status = bus.secureConnection(otherBus.getUniqueName(), false);
        assertEquals(status, Status.BUS_SECURITY_NOT_ENABLED);

        status = bus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener);
        assertEquals(Status.OK, status);

        status = bus.secureConnection(otherBus.getUniqueName(), false);
        assertEquals(status, Status.AUTH_FAIL);

        status = otherBus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener);
        assertEquals(Status.OK, status);

        status = bus.secureConnection(otherBus.getUniqueName(), false);
        assertEquals(status, Status.OK);

        otherBus.disconnect();
        otherBus.release();
        otherBus = null;
    }

    public void testSecureConnectionWithNullBusName() throws Exception {
        bus = new BusAttachment(getClass().getName());
        bus.registerKeyStoreListener(new NullKeyStoreListener());
        Status status = bus.connect();
        assertEquals(status, Status.OK);

        BusAttachment otherBus = new BusAttachment(getClass().getName());
        otherBus.registerKeyStoreListener(new NullKeyStoreListener());
        status = otherBus.connect();
        assertEquals(status, Status.OK);

        status = bus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener);
        assertEquals(Status.OK, status);

        status = otherBus.registerAuthListener("ALLJOYN_SRP_KEYX", authListener);
        assertEquals(Status.OK, status);

        status = bus.secureConnection(null, false);
        assertEquals(status, Status.OK);

        otherBus.disconnect();
        otherBus.release();
        otherBus = null;
    }

    public void testGetUniqueName() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);
        assertFalse(bus.getUniqueName().equals(""));
    }

    public void testMethodSignalAnnotation() throws Exception {
        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());
        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                        DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        AnnotationService service = new AnnotationService();
        assertEquals(Status.OK, bus.registerBusObject(service, "/annotation"));

        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                "/annotation",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { Introspectable.class });

        Introspectable introspectable = proxyObj.getInterface(Introspectable.class);
        String actual = introspectable.Introspect();
        String expected =
                "<!DOCTYPE node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n" +
                        "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\">\n" +
                        "<node>\n" +
                        "  <interface name=\"org.alljoyn.bus.AnnotationInterface\">\n" +
                        "    <method name=\"DeprecatedMethod\">\n" +
                        "      <arg type=\"s\" direction=\"in\"/>\n" +
                        "      <arg type=\"s\" direction=\"out\"/>\n" +
                        "      <annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>\n" +
                        "    </method>\n" +
                        "    <method name=\"DeprecatedNoReplyMethod\">\n" +
                        "      <arg type=\"s\" direction=\"in\"/>\n" +
                        "      <annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>\n" +
                        "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>\n" +
                        "    </method>\n" +
                        "    <signal name=\"DeprecatedSignal\">\n" +
                        "      <arg type=\"s\" direction=\"out\"/>\n" +
                        "      <annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>\n" +
                        "    </signal>\n" +
                        "    <method name=\"NoReplyMethod\">\n" +
                        "      <arg type=\"s\" direction=\"in\"/>\n" +
                        "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>\n" +
                        "    </method>\n" +
                        "    <property name=\"ChangeNotifyProperty\" type=\"s\" access=\"readwrite\">\n" +
                        "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>\n" +
                        "    </property>\n" +
                        "  </interface>\n" +
                        "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n" +
                        "    <method name=\"Introspect\">\n" +
                        "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n" +
                        "    </method>\n" +
                        "  </interface>\n" +
                        "  <interface name=\"org.allseen.Introspectable\">\n" +
                        "    <method name=\"GetDescriptionLanguages\">\n" +
                        "      <arg name=\"languageTags\" type=\"as\" direction=\"out\"/>\n" +
                        "    </method>\n" +
                        "    <method name=\"IntrospectWithDescription\">\n" +
                        "      <arg name=\"languageTag\" type=\"s\" direction=\"in\"/>\n" +
                        "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n" +
                        "    </method>\n" +
                        "    <annotation name=\"org.alljoyn.Bus.Secure\" value=\"off\"/>\n" +
                        "  </interface>\n" +
                        "</node>\n";
        assertEquals(expected, actual);
    }

    public void testNoReplyMethod() throws Exception {
        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());
        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                        DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        AnnotationService service = new AnnotationService();
        assertEquals(Status.OK, bus.registerBusObject(service, "/annotation"));

        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                "/annotation",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { AnnotationInterface.class });
        AnnotationInterface proxy = proxyObj.getInterface(AnnotationInterface.class);
        proxy.noReplyMethod("noreply");
        /* No assert here, the test is to make sure the above call doesn't throw an exception. */
    }

    public void testMethodException() throws Exception {
        bus = new BusAttachment(getClass().getName());
        ExceptionService service = new ExceptionService();
        assertEquals(Status.OK, bus.registerBusObject(service, "/exception"));

        assertEquals(Status.OK, bus.connect());
        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                        DBusProxyObj.REQUEST_NAME_NO_FLAGS));
        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                "/exception",
                BusAttachment.SESSION_ID_ANY,
                new Class<?>[] { SimpleInterface.class });
        SimpleInterface proxy = proxyObj.getInterface(SimpleInterface.class);

        try {
            proxy.ping("hello");
            fail("Failed to receive expected BusException");
        } catch (BusException ex) {
            /*
             * In release mode the error code is returned, in debug mode the
             * actual text for the error is returned.
             */
            assertTrue("ER_OS_ERROR".equals(ex.getMessage()) || "0x0004".equals(ex.getMessage()));
        }

        try {
            proxy.ping("hello");
            fail("Failed to receive expected BusException");
        } catch (ErrorReplyBusException ex) {
            assertEquals(Status.BUS_REPLY_IS_ERROR_MESSAGE, ex.getErrorStatus());
            assertEquals("org.alljoyn.bus.ExceptionService.Error1", ex.getErrorName());
            assertEquals("", ex.getErrorMessage());
        }

        try {
            proxy.ping("hello");
            fail("Failed to receive expected BusException");
        } catch (ErrorReplyBusException ex) {
            assertEquals(Status.BUS_REPLY_IS_ERROR_MESSAGE, ex.getErrorStatus());
            assertEquals("org.alljoyn.bus.ExceptionService.Error2", ex.getErrorName());
            assertEquals("Message", ex.getErrorMessage());
        }

        try {
            proxy.ping("hello");
            fail("Failed to receive expected BusException");
        } catch (BusException ex) {
            assertTrue("ER_FAIL".equals(ex.getMessage()) || "0x0002".equals(ex.getMessage()));
        }
    }

    private boolean found;
    private boolean lost;

    public class FindNewNameBusListener extends BusListener {
        public FindNewNameBusListener(BusAttachment bus) {
            this.bus = bus;
        }

        @Override
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            found = true;
            // stopWait seems to block sometimes so enable concurrency.
            bus.enableConcurrentCallbacks();
            stopWait();
        }

        private BusAttachment bus;
    }

    public synchronized void testFindNewName() throws Exception {
        bus = new BusAttachment(getClass().getName());
        String name = genUniqueName(bus);

        BusListener testBusListener = new FindNewNameBusListener(bus);
        bus.registerBusListener(testBusListener);

        assertEquals(Status.OK, bus.connect());
        found = false;
        lost = false;
        assertEquals(Status.OK, bus.findAdvertisedName(name));

        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());
        int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        assertEquals(Status.OK, otherBus.requestName(name, flag));
        assertEquals(Status.OK, otherBus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        this.wait(5 * 1000);
        assertEquals(true, found);

        assertEquals(Status.OK, bus.cancelFindAdvertisedName(name));
    }

    public class LostExistingNameBusListener extends BusListener {
        public LostExistingNameBusListener(BusAttachment bus) {
            this.bus = bus;
        }

        @Override
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            //System.out.println("Name is :  "+name);
            found = true;
            // stopWait seems to block sometimes, so enable concurrency.
            bus.enableConcurrentCallbacks();
            stopWait();
        }

        @Override
        public void lostAdvertisedName(String name, short transport, String namePrefix) {
            lost = true;
            // stopWait seems to block sometimes, so enable concurrency.
            bus.enableConcurrentCallbacks();
            stopWait();
        }

        private BusAttachment bus;
    }

    public synchronized void testLostExistingName() throws Exception {
        // The client part
        bus = new BusAttachment(getClass().getName());
        String name = genUniqueName(bus);

        assertEquals(Status.OK, bus.connect());

        BusListener testBusListener = new LostExistingNameBusListener(bus);
        bus.registerBusListener(testBusListener);

        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());

        int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        assertEquals(Status.OK, otherBus.requestName(name, flag));
        assertEquals(Status.OK, otherBus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        found = false;
        lost = false;

        assertEquals(Status.OK, bus.findAdvertisedName(name));

        // Advertise a name and then cancel it.
        this.wait(4 * 1000);
        String happy = name + ".happy";
        assertEquals(Status.OK, otherBus.advertiseName(happy, SessionOpts.TRANSPORT_ANY));
        this.wait(4 * 1000);
        assertEquals(true , found);
        assertEquals(Status.OK, otherBus.cancelAdvertiseName(happy, SessionOpts.TRANSPORT_ANY));
        this.wait(20 * 1000);
        assertEquals(true, lost);
    }

    public class FindExistingNameBusListener extends BusListener {
        public FindExistingNameBusListener(BusAttachment bus) {
            this.bus = bus;
        }

        @Override
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            found = true;

            // stopWait seems to block sometimes so we need to enable concurrency
            bus.enableConcurrentCallbacks();

            stopWait();
        }

        @Override
        public void lostAdvertisedName(String name, short transport, String namePrefix) {
            lost = true;
        }

        private BusAttachment bus;
    }

    public synchronized void testFindExistingName() throws Exception {
        bus = new BusAttachment(getClass().getName());
        String name = genUniqueName(bus);

        BusListener testBusListener = new FindExistingNameBusListener(bus);
        bus.registerBusListener(testBusListener);

        assertEquals(Status.OK, bus.connect());

        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());

        int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        assertEquals(Status.OK, otherBus.requestName(name, flag));
        assertEquals(Status.OK, otherBus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        found = false;
        lost = false;

        assertEquals(Status.OK, bus.findAdvertisedName(name));

        this.wait(2 * 1000);
        assertEquals(true, found);
    }


    public void testFindNullName() throws Exception {
        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());
        assertEquals(Status.BAD_ARG_1, bus.findAdvertisedName(null));
    }

    public class FindMultipleNamesBusListener extends BusListener {
        public FindMultipleNamesBusListener(BusAttachment bus) {
            this.bus = bus;
        }

        @Override
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            if (name.equals(nameA)) {
                foundNameA = true;
            }
            if ( name.equals(nameB)) {
                foundNameB = true;
            }
            // stopWait seems to block sometimes, so enable concurrency.
            bus.enableConcurrentCallbacks();
            stopWait();
        }

        private BusAttachment bus;
    }

    private String nameA;
    private String nameB;
    private boolean foundNameA;
    private boolean foundNameB;

    public synchronized void testFindMultipleNames() throws Exception {
        bus = new BusAttachment(getClass().getName());
        nameA = genUniqueName(bus);
        nameB = genUniqueName(bus);

        BusListener testBusListener = new FindMultipleNamesBusListener(bus);
        bus.registerBusListener(testBusListener);

        assertEquals(Status.OK, bus.connect());
        foundNameA = foundNameB = false;

        assertEquals(Status.OK, bus.findAdvertisedName(nameA));
        assertEquals(Status.OK, bus.findAdvertisedName(nameB));

        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());

        assertEquals(Status.OK, otherBus.advertiseName(nameA, SessionOpts.TRANSPORT_ANY));
        assertEquals(Status.OK, otherBus.advertiseName(nameB, SessionOpts.TRANSPORT_ANY));
        this.wait(4 * 1000);
        // check to see if we have found both names if not wait a little longer
        // not thread safe, but worst case situation is it will wait the full 2
        // seconds
        if(!foundNameA || !foundNameB){
            this.wait(4 * 1000);
        }


        assertEquals(true, foundNameA);
        assertEquals(true, foundNameB);

        assertEquals(Status.OK, otherBus.cancelAdvertiseName(nameA, SessionOpts.TRANSPORT_ANY));
        assertEquals(Status.OK, otherBus.cancelAdvertiseName(nameB, SessionOpts.TRANSPORT_ANY));

        assertEquals(Status.OK, bus.cancelFindAdvertisedName(nameB));
        foundNameA = foundNameB = false;
        assertEquals(Status.OK, otherBus.advertiseName(nameA, SessionOpts.TRANSPORT_ANY));
        assertEquals(Status.OK, otherBus.advertiseName(nameB, SessionOpts.TRANSPORT_ANY));
        //this will unlock when name.A is found
        this.wait(2 * 1000);

        assertEquals(Status.OK, bus.cancelFindAdvertisedName(nameA));
        assertEquals(Status.OK, otherBus.cancelAdvertiseName(nameA, SessionOpts.TRANSPORT_ANY));
        assertEquals(Status.OK, otherBus.cancelAdvertiseName(nameB, SessionOpts.TRANSPORT_ANY));
        assertEquals(true, foundNameA);
        assertEquals(false, foundNameB);
    }

    public class FindNamesByTransportListener extends BusListener {
        public FindNamesByTransportListener(BusAttachment bus) {
            this.bus = bus;
        }

        @Override
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            if (name.equals(name1)) {
                foundName1 = true;
            }
            if (name.equals(name2)) {
                foundName2 = true;
                transport2 |= transport;
            }
            if (name.equals(name3)) {
                foundName3 = true;
            }
            // stopWait seems to block sometimes, so enable concurrency.
            bus.enableConcurrentCallbacks();
            stopWait();
        }

        private BusAttachment bus;
    }

    private String name1;
    private String name2;
    private String name3;
    private boolean foundName1;
    private boolean foundName2;
    private boolean foundName3;
    private short transport2;
    private static final short LOCAL = 0x01;
    private static final short TCP = 0x04;
    public synchronized void testFindNameByTransport() throws Exception {
        foundName1 = false;
        foundName2 = false;
        foundName3 = false;
        transport2 = 0;
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        name1 = genUniqueName(bus);
        name2 = genUniqueName(bus);
        name3 = genUniqueName(bus);

        BusListener testBusListener = new FindNamesByTransportListener(bus);
        bus.registerBusListener(testBusListener);

        assertEquals(Status.OK, bus.connect());
        found = false;
        assertEquals(Status.OK, bus.findAdvertisedNameByTransport(name1,TCP));
        assertEquals(Status.OK, bus.findAdvertisedNameByTransport(name2,LOCAL));
        assertEquals(Status.OK, bus.findAdvertisedNameByTransport(name3,LOCAL));
        assertEquals(Status.OK, bus.cancelFindAdvertisedNameByTransport(name3,LOCAL));

        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());

        assertEquals(Status.OK, otherBus.advertiseName(name1, SessionOpts.TRANSPORT_ANY));
        assertEquals(Status.OK, otherBus.advertiseName(name2, SessionOpts.TRANSPORT_ANY));
        assertEquals(Status.OK, otherBus.advertiseName(name3, SessionOpts.TRANSPORT_ANY));

        this.wait(2 * 1000);
        assertEquals(false, foundName1);
        assertEquals(true, foundName2);
        assertEquals(LOCAL, transport2);
        assertEquals(false, foundName3);

    }

    public class CancelFindNameInsideListenerBusListener extends BusListener {
        public CancelFindNameInsideListenerBusListener(BusAttachment bus) {
            this.bus = bus;
        }

        @Override
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            if (!found) {
                found = true;
                // Prepare for blocking call
                bus.enableConcurrentCallbacks();

                assertEquals(Status.OK, bus.cancelFindAdvertisedName(namePrefix));
                stopWait();
            }
        }

        private BusAttachment bus;
    }

    public synchronized void testCancelFindNameInsideListener() throws Exception {
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        String name = genUniqueName(bus);

        BusListener testBusListener = new CancelFindNameInsideListenerBusListener(bus);
        bus.registerBusListener(testBusListener);

        assertEquals(Status.OK, bus.connect());
        found = false;
        assertEquals(Status.OK, bus.findAdvertisedName(name));

        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());

        assertEquals(Status.OK, otherBus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        this.wait(2 * 1000);
        assertEquals(true, found);
    }

    public void testFindSameName() throws Exception {
        bus = new BusAttachment(getClass().getName());
        String name = genUniqueName(bus);

        assertEquals(Status.OK, bus.connect());

        assertEquals(Status.OK, bus.findAdvertisedName(name));
        assertEquals(Status.ALLJOYN_FINDADVERTISEDNAME_REPLY_ALREADY_DISCOVERING, bus.findAdvertisedName(name));
        assertEquals(Status.OK, bus.cancelFindAdvertisedName(name));
    }

    /*
     * this test verifies when bindSessionPort is called when the bus is not
     * connected the BUS_NOT_CONNECTED status is returned.
     *
     * Also test bindSessionPort after connecting with the bus.
     */
    public void testBindSessionPort() throws Exception {
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        Mutable.ShortValue sessionPort = new Mutable.ShortValue((short) 42);

        assertEquals(Status.BUS_NOT_CONNECTED, bus.bindSessionPort(sessionPort, sessionOpts, new SessionPortListener()));

        assertEquals(Status.OK, bus.connect());

        assertEquals(Status.OK, bus.bindSessionPort(sessionPort, sessionOpts, new SessionPortListener()));

    }

    /*
     * test unBindSessionPort by binding the with the sessionPort and instantly
     * unbinding with the session port.
     */
    public void testUnBindSessionPort() throws Exception {
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        Mutable.ShortValue sessionPort = new Mutable.ShortValue((short) 42);

        assertEquals(Status.BUS_NOT_CONNECTED, bus.unbindSessionPort(sessionPort.value));

        assertEquals(Status.OK, bus.connect());

        assertEquals(Status.OK, bus.bindSessionPort(sessionPort, sessionOpts, new SessionPortListener()));
        assertEquals(Status.OK, bus.unbindSessionPort(sessionPort.value));
    }

    private boolean sessionAccepted;
    private boolean sessionJoined;
    private boolean onJoined;
    private Status joinSessionStatus;
    private int busSessionId;
    private int otherBusSessionId;

    public class JoinSessionSessionListener extends SessionListener{

    }

    public synchronized void testJoinSession() throws Exception {

        found = false;
        sessionAccepted = false;
        sessionJoined = false;

        // create new BusAttachment
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        String name = genUniqueName(bus);

        assertEquals(Status.OK, bus.connect());

        // Set up SessionOpts
        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        // User defined sessionPort Number
        Mutable.ShortValue sessionPort = new Mutable.ShortValue((short) 42);

        //bindSessionPort new SessionPortListener
        assertEquals(Status.OK, bus.bindSessionPort(sessionPort, sessionOpts,
                new SessionPortListener(){
            @Override
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                if (sessionPort == 42) {
                    sessionAccepted = true;
                    // stopWait seems to block sometimes, so enable concurrency.
                    bus.enableConcurrentCallbacks();
                    stopWait();
                    return true;
                } else {
                    sessionAccepted = false;
                    return false;
                }
            }

            @Override
            public void sessionJoined(short sessionPort, int id, String joiner) {
                if (sessionPort == 42) {
                    busSessionId = id;
                    sessionJoined = true;
                } else {
                    sessionJoined = false;
                }
            }
        }));
        // Request name from bus
        int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        assertEquals(Status.OK, bus.requestName(name, flag));
        // Advertise same bus name
        assertEquals(Status.OK, bus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        // Create Second BusAttachment
        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());

        // Register BusListener for the foundAdvertisedName Listener
        otherBus.registerBusListener(new BusListener() {
            @Override
            public void foundAdvertisedName(String name, short transport, String namePrefix) {
                found = true;
                SessionOpts sessionOpts = new SessionOpts();
                sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
                sessionOpts.isMultipoint = false;
                sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
                sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

                Mutable.IntegerValue sessionId = new Mutable.IntegerValue(0);
                // Since we are using blocking form of joinSession, we need to enable concurrency
                otherBus.enableConcurrentCallbacks();
                // Join session once the AdvertisedName has been found
                joinSessionStatus = otherBus.joinSession(name, (short)42, sessionId,
                        sessionOpts, new JoinSessionSessionListener());
                otherBusSessionId = sessionId.value;
                stopWait();
            }
        });

        // find the AdvertisedName
        assertEquals(Status.OK, otherBus.findAdvertisedName(name));

        this.wait(5 * 1000);

        assertEquals(true, found);

        this.wait(5 * 1000);
        if(!sessionAccepted || !sessionJoined || (Status.NONE == joinSessionStatus)) {
            this.wait(5 * 1000);
        }

        assertEquals(Status.OK, joinSessionStatus);
        assertEquals(true, sessionAccepted);
        assertEquals(true, sessionJoined);
        assertEquals(busSessionId, otherBusSessionId);
    }

    class JoinSessionAsyncBusListener extends BusListener {
        public String name;

        public JoinSessionAsyncBusListener(String name) {
            this.name = name;
        }

        @Override
        public void foundAdvertisedName(String advertisedName, short transport, String namePrefix) {
            assertEquals(name, advertisedName);
            found = true;
            // stopWait seems to block sometimes, so enable concurrency.
            otherBus.enableConcurrentCallbacks();
            stopWait();
        }
    };

    public synchronized void testJoinSessionAsync() throws Exception {
        /*
         * Setup to create a session, advertise a name on one bus attachment and find the name
         * on another bus attachment.
         */
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        String name = genUniqueName(bus);

        assertEquals(Status.OK, bus.connect());
        assertEquals(Status.OK, bus.bindSessionPort(new Mutable.ShortValue((short) 42), new SessionOpts(),
                new SessionPortListener(){
            @Override
            public boolean acceptSessionJoiner(short sessionPort, String joiner,
                    SessionOpts sessionOpts) {
                assertEquals(42, sessionPort);
                return true;
            }
            @Override
            public void sessionJoined(short sessionPort, int id, String joiner) {
                assertEquals(42, sessionPort);
            }
        }));
        int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING |
                BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        assertEquals(Status.OK, bus.requestName(name, flag));
        assertEquals(Status.OK, bus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());
        JoinSessionAsyncBusListener busListener = new JoinSessionAsyncBusListener(name);
        otherBus.registerBusListener(busListener);

        found = false;
        assertEquals(Status.OK, otherBus.findAdvertisedName(name));
        this.wait(5 * 1000);
        assertEquals(true, found);

        /*
         * The actual test begins here.
         */
        onJoined = false;
        Integer context = new Integer(0xacceeded);
        assertEquals(Status.OK, otherBus.joinSession(name, (short) 42, new SessionOpts(),
                new SessionListener(), new OnJoinSessionListener() {
            @Override
            public void onJoinSession(Status status, int sessionId, SessionOpts opts, Object context) {
                assertEquals(Status.OK, status);
                int i = ((Integer)context).intValue();
                assertEquals(0xacceeded, i);
                onJoined = true;
                // stopWait seems to block sometimes, so enable concurrency.
                otherBus.enableConcurrentCallbacks();
                stopWait();
            }
        }, context));
        this.wait(5 * 1000);
        assertEquals(true, onJoined);
    }

    private boolean sessionLost;
    private int sessionLostReason;

    public class LeaveSessionSessionListener extends SessionListener {
        public LeaveSessionSessionListener(BusAttachment bus) {
        }

        @Override
        public void sessionLost(int sessionId, int reason) {
            sessionLostReason = reason;
            sessionLost = true;
        }
    }

    public synchronized void testLeaveSession() throws Exception {
        found = false;
        sessionAccepted = false;
        sessionJoined = false;
        sessionLost = false;
        sessionLostReason = SessionListener.ALLJOYN_SESSIONLOST_INVALID;
        // create new BusAttachment
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        String name = genUniqueName(bus);
        /*
         * The number '1337' is just a dummy number, the BUS_NOT_CONNECTED
         * status is expected regardless of the number.
         */
        assertEquals(Status.BUS_NOT_CONNECTED, bus.leaveSession(1337));
        assertEquals(Status.OK, bus.connect());

        // Set up SessionOpts
        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        // User defined sessionPort Number
        Mutable.ShortValue sessionPort = new Mutable.ShortValue((short) 42);

        //bindSessionPort new SessionPortListener
        assertEquals(Status.OK, bus.bindSessionPort(sessionPort, sessionOpts, new SessionPortListener() {
            @Override
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                if (sessionPort == 42) {
                    sessionAccepted = true;
                    return true;
                } else {
                    sessionAccepted = false;
                    return false;
                }
            }

            @Override
            public void sessionJoined(short sessionPort, int id, String joiner) {
                if (sessionPort == 42) {
                    busSessionId = id;
                    sessionJoined = true;
                } else {
                    sessionJoined = false;
                }
            }
        }));
        // Request name from bus
        int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        assertEquals(Status.OK, bus.requestName(name, flag));
        // Advertise same bus name
        assertEquals(Status.OK, bus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        // Create Second BusAttachment
        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);

        assertEquals(Status.OK, otherBus.connect());

        // Register BusListener for the foundAdvertisedName Listener
        otherBus.registerBusListener(new BusListener() {
            @Override
            public void foundAdvertisedName(String name, short transport, String namePrefix) {
                SessionOpts sessionOpts = new SessionOpts();
                sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
                sessionOpts.isMultipoint = false;
                sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
                sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

                Mutable.IntegerValue sessionId = new Mutable.IntegerValue(0);

                // Since we are using blocking form of joinSession and setLinKTimeout, we need to enable concurrency
                otherBus.enableConcurrentCallbacks();

                // Join session once the AdvertisedName has been found
                joinSessionStatus = otherBus.joinSession(name, (short)42, sessionId,
                        sessionOpts, new LeaveSessionSessionListener(otherBus));
                otherBusSessionId = sessionId.value;

                // Set a link timeout
                if (joinSessionStatus == Status.OK) {
                    Mutable.IntegerValue timeout = new Mutable.IntegerValue(60);
                    joinSessionStatus = otherBus.setLinkTimeout(sessionId.value, timeout);
                    if (joinSessionStatus == Status.OK) {
                        if ((timeout.value < 60) && (timeout.value != 0)) {
                            joinSessionStatus = Status.FAIL;
                        }
                    }
                }
                // must be last
                found = true;
            }
        });

        // find the AdvertisedName
        assertEquals(Status.OK, otherBus.findAdvertisedName(name));

        // Make sure name was found and session was joined
        assertEquals(true, waitForLambda(4 * 1000, new Lambda() {
                @Override
                public boolean func() { return found && sessionAccepted && sessionJoined && (joinSessionStatus == Status.OK); }
            }));

        assertEquals(busSessionId, otherBusSessionId);
        assertEquals(Status.OK, otherBus.leaveSession(otherBusSessionId));

        found = false;
        sessionAccepted = sessionJoined = false;
        joinSessionStatus = Status.NONE;
        busSessionId = otherBusSessionId = 0;

        otherBus.cancelFindAdvertisedName(name);
        otherBus.findAdvertisedName(name);

        // Wait for found name and session joined
        assertEquals(true, waitForLambda(4 * 1000, new Lambda() {
                @Override
                public boolean func() { return found && sessionAccepted && sessionJoined && (joinSessionStatus == Status.OK); }
            }));

        assertEquals(Status.OK, joinSessionStatus);
        assertEquals(true, sessionAccepted);
        assertEquals(true, sessionJoined);
        assertEquals(busSessionId, otherBusSessionId);

        assertEquals(Status.OK, bus.leaveSession(busSessionId));

        // Wait for session lost
        assertEquals(true, waitForLambda(4 * 1000, new Lambda() {
                public boolean func() { return sessionLost; }
            }));
        assertEquals(SessionListener.ALLJOYN_SESSIONLOST_REMOTE_END_LEFT_SESSION, sessionLostReason);
    }

    private synchronized void testSelfJoin(boolean joinerLeaves)
        throws Exception
    {
        String name = genUniqueName(bus);

        found = false;
        sessionAccepted = false;
        sessionJoined = false;
        sessionLost = false;
        sessionLostReason = SessionListener.ALLJOYN_SESSIONLOST_INVALID;

        /*
         * The number '1337' is just a dummy number, the BUS_NOT_CONNECTED status is expected regardless of the number.
         */
        assertEquals(Status.BUS_NOT_CONNECTED, bus.leaveSession(1337));
        assertEquals(Status.OK, bus.connect());

        // Set up SessionOpts
        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        // User defined sessionPort Number
        Mutable.ShortValue sessionPort = new Mutable.ShortValue((short) 42);

        // bindSessionPort new SessionPortListener
        assertEquals(Status.OK, bus.bindSessionPort(sessionPort, sessionOpts, new SessionPortListener() {
            @Override
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts)
            {
                if (sessionPort == 42) {
                    sessionAccepted = true;
                    return true;
                }
                else {
                    sessionAccepted = false;
                    return false;
                }
            }

            @Override
            public void sessionJoined(short sessionPort, int id, String joiner)
            {
                if (sessionPort == 42) {
                    busSessionId = id;
                    sessionJoined = true;
                    assertEquals(Status.OK,
                        bus.setHostedSessionListener(busSessionId, new LeaveSessionSessionListener(bus)));
                    assertEquals(Status.OK,
                        otherBus.setJoinedSessionListener(busSessionId, new LeaveSessionSessionListener(otherBus)));
                    // Pass null to test if the session listener is cleared
                    assertEquals(Status.OK, otherBus.setJoinedSessionListener(busSessionId, null));
                    // Set a new LeaveSessionSessionListener after clearing the previous one
                    assertEquals(Status.OK,
                        otherBus.setJoinedSessionListener(busSessionId, new LeaveSessionSessionListener(otherBus)));
                }
                else {
                    sessionJoined = false;
                }
            }
        }));
        // Request name from bus
        int flag =
            BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING
                | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
        assertEquals(Status.OK, bus.requestName(name, flag));
        // Advertise same bus name
        assertEquals(Status.OK, bus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

        if (bus != otherBus) {
            assertEquals(Status.OK, otherBus.connect());
        }

        // Register BusListener for the foundAdvertisedName Listener
        otherBus.registerBusListener(new BusListener() {
            @Override
            public void foundAdvertisedName(String name, short transport, String namePrefix)
            {
                SessionOpts sessionOpts = new SessionOpts();
                sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
                sessionOpts.isMultipoint = false;
                sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
                sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

                Mutable.IntegerValue sessionId = new Mutable.IntegerValue(0);

                // Since we are using blocking form of joinSession and setLinKTimeout, we need to enable concurrency
                otherBus.enableConcurrentCallbacks();

                // Join session once the AdvertisedName has been found
                joinSessionStatus =
                    otherBus.joinSession(name, (short) 42, sessionId, sessionOpts, new LeaveSessionSessionListener(
                        otherBus));
                otherBusSessionId = sessionId.value;

                // Set a link timeout
                if (joinSessionStatus == Status.OK) {
                    Mutable.IntegerValue timeout = new Mutable.IntegerValue(60);
                    joinSessionStatus = otherBus.setLinkTimeout(sessionId.value, timeout);
                    if (joinSessionStatus == Status.OK) {
                        if ((timeout.value < 60) && (timeout.value != 0)) {
                            joinSessionStatus = Status.FAIL;
                        }
                    }
                }
                // must be last
                found = true;
            }
        });

        // find the AdvertisedName
        assertEquals(Status.OK, otherBus.findAdvertisedName(name));

        // Make sure name was found and session was joined
        assertEquals(true, waitForLambda(4 * 1000, new Lambda() {
            @Override
            public boolean func()
            {
                return found && sessionAccepted && sessionJoined && (joinSessionStatus == Status.OK);
            }
        }));

        if (joinerLeaves) {
            assertEquals(Status.OK, otherBus.leaveJoinedSession(busSessionId));
            if (bus != otherBus) {
                assertEquals(Status.ALLJOYN_LEAVESESSION_REPLY_NO_SESSION, otherBus.leaveHostedSession(busSessionId));
            }
        }
        else {
            assertEquals(Status.OK, bus.leaveHostedSession(busSessionId));
            if (bus != otherBus) {
                assertEquals(Status.ALLJOYN_LEAVESESSION_REPLY_NO_SESSION, bus.leaveJoinedSession(busSessionId));
            }
            else {
                assertEquals(Status.ALLJOYN_LEAVESESSION_REPLY_NO_SESSION, bus.leaveSession(busSessionId));
            }
        }
    }

    public synchronized void testOtherJoinJoinerLeaves()
        throws Exception
    {
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);

        this.testSelfJoin(true);
    }

    public synchronized void testOtherJoinHostLeaves()
        throws Exception
    {
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);

        this.testSelfJoin(false);
    }

    public synchronized void testSelfJoinJoinerLeaves()
        throws Exception
    {
        bus = otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);

        this.testSelfJoin(true);
    }

    public synchronized void testSelfJoinHostLeaves()
        throws Exception
    {
        bus = otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);

        this.testSelfJoin(false);
    }

    /* ALLJOYN-958 */
    public synchronized void testUnregisterBusListener() throws Exception {
        boolean thrown = false;
        bus = new BusAttachment(getClass().getName());

        BusListener testBusListener = new FindNewNameBusListener(bus);
        bus.registerBusListener(testBusListener);

        assertEquals(Status.OK, bus.connect());

        /* This is the actual test- unregister the bus listener */
        /* The Exception catch will only catch java exceptions and will not catch jni crashes. So, if jni crashes, the whole junit will stop. */
        try {
            bus.unregisterBusListener(testBusListener);
            /* This wait is to ensure that the unregister operation has completed successfully. There are no jni crashes like the one found in ALLJOYN-958 */
            this.wait(1 * 1000);
        } catch (Exception ex) {
            thrown = true;
        } finally {
            assertTrue(!thrown);
        }

    }

    public class RegisteredUnregisteredBusListener extends BusListener {
        public RegisteredUnregisteredBusListener(BusAttachment bus) {
            this.bus = bus;
            this.registered = false;
            this.unregistered = false;
        }

        @Override
        public void listenerRegistered(BusAttachment bus) {
            assertEquals(this.bus, bus);
            registered = true;
        }

        @Override
        public void listenerUnregistered() {
            unregistered = true;
        }

        public boolean registered;
        public boolean unregistered;

        private BusAttachment bus;
    }

    public void testRegisteredUnregisteredBusListener() throws Exception {
        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());

        RegisteredUnregisteredBusListener testBusListener = new RegisteredUnregisteredBusListener(bus);
        bus.registerBusListener(testBusListener);
        assertTrue(testBusListener.registered);
        bus.unregisterBusListener(testBusListener);
        assertTrue(testBusListener.unregistered);
    }

    public void testIsConnected() throws Exception {
        bus = new BusAttachment(getClass().getName());
        assertFalse(bus.isConnected());
        assertEquals(Status.OK, bus.connect());
        assertTrue(bus.isConnected());
    }

    public class RemoveSessionMemberSessionPortListener extends SessionPortListener {
        public RemoveSessionMemberSessionPortListener(BusAttachment bus) { this.bus = bus; }

        @Override
        public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
            if (sessionPort == 42) {
                sessionAccepted = true;
                return true;
            } else {
                sessionAccepted = false;
                return false;
            }
        }
        @Override
        public void sessionJoined(short sessionPort, int id, String joiner) {
                sessionJoinedFlag = true;
                bus.setSessionListener(id,  new SessionListener() {
                    public void sessionLost(final int sessionId, int reason) {
                        sessionLostFlagA = true;
                    }
                    public void sessionMemberAdded(int sessionId, String uniqueName) {
                        sessionMemberAddedFlagA = true;
                    }
                    public void sessionMemberRemoved(int sessionId, String uniqueName) {
                        sessionMemberRemovedFlagA = true;
                    }
            });
        }
        private BusAttachment bus;
    }

    public class RemoveSessionMemberSessionListener extends SessionListener {
            public void sessionLost(final int sessionId, int reason) {
                sessionLostFlagB = true;
            }
            public void sessionMemberAdded(int sessionId, String uniqueName) {
                sessionMemberAddedFlagB = true;
            }
            public void sessionMemberRemoved(int sessionId, String uniqueName) {
                sessionMemberRemovedFlagB = true;
            }
    }

    private boolean sessionJoinedFlag = false;
    private boolean sessionLostFlagA = false;
    private boolean sessionMemberAddedFlagA = false;
    private boolean sessionMemberRemovedFlagA = false;
    private boolean sessionLostFlagB = false;
    private boolean sessionMemberAddedFlagB = false;
    private boolean sessionMemberRemovedFlagB = false;

    public void testRemoveSessionMember() throws Exception {

        /* make sure global flags are initialized */
        sessionJoinedFlag = false;
        sessionLostFlagA = false;
        sessionMemberAddedFlagA = false;
        sessionMemberRemovedFlagA = false;
        sessionLostFlagB = false;
        sessionMemberAddedFlagB = false;
        sessionMemberRemovedFlagB = false;

        BusAttachment busA = new BusAttachment("bus.Aa");
        BusAttachment busB = new BusAttachment("bus.Bb");

        Status status = busA.connect();
        if (Status.OK != status) {
            throw new Exception("BusAttachment.connect() failed with " + status.toString());
        }

        status = busB.connect();
        if (Status.OK != status) {
            throw new Exception("BusAttachment.connect() failed with " + status.toString());
        }

        /* Multi-point session */
        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = true;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        RemoveSessionMemberSessionPortListener sessionPortListener = new RemoveSessionMemberSessionPortListener(busA);
        Mutable.ShortValue port = new Mutable.ShortValue((short) 42);

        assertEquals(Status.OK, busA.bindSessionPort(port, sessionOpts, sessionPortListener));

        RemoveSessionMemberSessionListener sessionListener = new RemoveSessionMemberSessionListener();
        Mutable.IntegerValue sessionId = new Mutable.IntegerValue(0);

        assertEquals(Status.OK, busB.joinSession(busA.getUniqueName(), (short)42, sessionId, sessionOpts, sessionListener));

        //Wait upto 4 seconds all callbacks and listeners to be called.
        assertEquals(true, waitForLambda(4 * 1000, new Lambda() {
                public boolean func() { return sessionJoinedFlag && sessionMemberAddedFlagA && sessionMemberAddedFlagB; }
            }));

    assertEquals(true, sessionJoinedFlag);
    assertEquals(true, sessionMemberAddedFlagA);
    assertEquals(true, sessionMemberAddedFlagB);

        assertEquals(Status.ALLJOYN_REMOVESESSIONMEMBER_NOT_BINDER, busB.removeSessionMember(sessionId.value, busA.getUniqueName()));

        assertEquals(Status.ALLJOYN_REMOVESESSIONMEMBER_NOT_FOUND, busA.removeSessionMember(sessionId.value, busA.getUniqueName()));

        assertEquals(Status.ALLJOYN_REMOVESESSIONMEMBER_NOT_FOUND, busA.removeSessionMember(sessionId.value, ":Invalid"));

        assertEquals(Status.OK, busA.removeSessionMember(sessionId.value, busB.getUniqueName()));

        //Wait upto 2 seconds all callbacks and listeners to be called.
        assertEquals(true, waitForLambda(2 * 1000, new Lambda() {
                public boolean func() { return sessionLostFlagA && sessionLostFlagB && sessionMemberRemovedFlagA && sessionMemberRemovedFlagB; }
            }));

        assertEquals(true,sessionLostFlagA);
        assertEquals(true,sessionLostFlagB);
        assertEquals(true,sessionMemberRemovedFlagA);
        assertEquals(true,sessionMemberRemovedFlagB);

    }

    public void testPing() throws Exception {
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);

        assertEquals(Status.OK, bus.connect());

        assertEquals(Status.OK, bus.ping(bus.getUniqueName(), 1000));
    }


    private boolean onPinged = false;

    public synchronized void testPingAsync() throws Exception {
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, bus.connect());

        onPinged = false;
        Integer context = new Integer(0xdeedfeed);
        assertEquals(Status.OK, bus.ping(bus.getUniqueName(), 1000, new OnPingListener() {
            @Override
            public void onPing(Status status, Object context) {
                synchronized(this) {
                    assertEquals(Status.OK, status);
                    int i = ((Integer)context).intValue();
                    assertEquals(0xdeedfeed, i);
                    onPinged = true;
                }
                // stopWait seems to block sometimes, so enable concurrency.
                bus.enableConcurrentCallbacks();
                stopWait();
                
            }
        }, context));
        this.wait(5 * 1000);
        assertEquals(true, onPinged);
    }

    public synchronized void testPingAsyncOtherBus() throws Exception {
        bus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, bus.connect());

        // Create Second BusAttachment
        otherBus = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, otherBus.connect());

        onPinged = false;
        Integer context = new Integer(0xdeedfeed);
        assertEquals(Status.OK, bus.ping(otherBus.getUniqueName(), 1000, new OnPingListener() {
            @Override
            public void onPing(Status status, Object context) {
                synchronized(this) {
                    assertEquals(Status.OK, status);
                    int i = ((Integer)context).intValue();
                    assertEquals(0xdeedfeed, i);
                    onPinged = true;
                }
                // stopWait seems to block sometimes, so enable concurrency.
                bus.enableConcurrentCallbacks();
                stopWait();
            }
        }, context));
        this.wait(5 * 1000);
        assertEquals(true, onPinged);
    }

    public void testBadObjectPaths() throws Exception {
        bus = new BusAttachment(getClass().getName(),
                BusAttachment.RemoteMessage.Receive);
        assertEquals(Status.OK, bus.connect());

        Emitter emitter = new Emitter();

        assertEquals(Status.BUS_BAD_OBJ_PATH,
                bus.registerBusObject(emitter, "badPath"));
        assertEquals(Status.BUS_BAD_OBJ_PATH,
                bus.registerBusObject(emitter, "/bad.path"));
        assertEquals(Status.BUS_BAD_OBJ_PATH,
                bus.registerBusObject(emitter, "/bad path"));

        assertEquals(Status.OK, bus.registerBusObject(emitter, "/goodpath"));
    }

    public void testGetDynamicProxyBusObject() throws Exception {
        String ifaceName = "org.alljoyn.bus.SimpleInterface";

        // Usual BusAttachment and DBusProxyObj setup

        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());

        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                        DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        SimpleService service = new SimpleService();
        assertEquals(Status.OK, bus.registerBusObject(service, "/simple"));

        // Test getting/calling proxy using dynamic interface definition and GenericInterface invocation

        InterfaceDef interfaceDef = new InterfaceDef(ifaceName);
        MethodDef methodDef = new MethodDef("Ping", "s", "s", ifaceName);
        methodDef.addArg( new ArgDef("inStr", "s") );
        methodDef.addArg( new ArgDef("result", "s", ArgDef.DIRECTION_OUT) );
        interfaceDef.addMethod(methodDef);

        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                "/simple",
                BusAttachment.SESSION_ID_ANY,
                Arrays.asList(new InterfaceDef[]{interfaceDef}),
                false);
        GenericInterface proxy = proxyObj.getInterface(GenericInterface.class);
        String result = (String)proxy.methodCall(ifaceName, "Ping", "hello");

        assertEquals(1, receivedSimpleServicePing);
        assertEquals("hello", result);

        // Test that the bus method args can also be sent as an Object[]

        result = (String)proxy.methodCall(ifaceName, "Ping", new Object[]{"arg array of size 1"});
        assertEquals(2, receivedSimpleServicePing);
        assertEquals("arg array of size 1", result);
    }

    public void testGetDynamicProxyBusObject_invalidInvoke() throws Exception {
        String ifaceName = "org.alljoyn.bus.SimpleInterface";

        // Usual BusAttachment and DBusProxyObj setup

        bus = new BusAttachment(getClass().getName());
        assertEquals(Status.OK, bus.connect());

        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                control.RequestName("org.alljoyn.bus.BusAttachmentTest",
                        DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        SimpleService service = new SimpleService();
        assertEquals(Status.OK, bus.registerBusObject(service, "/simple"));

        // Get proxy using dynamic interface definition and GenericInterface

        InterfaceDef interfaceDef = new InterfaceDef(ifaceName);
        MethodDef methodDef = new MethodDef("Ping", "s", "s", ifaceName);
        methodDef.addArg( new ArgDef("inStr", "s") );
        methodDef.addArg( new ArgDef("result", "s", ArgDef.DIRECTION_OUT) );
        interfaceDef.addMethod(methodDef);

        ProxyBusObject proxyObj = bus.getProxyBusObject("org.alljoyn.bus.BusAttachmentTest",
                "/simple",
                BusAttachment.SESSION_ID_ANY,
                Arrays.asList(new InterfaceDef[]{interfaceDef}),
                false);
        GenericInterface proxy = proxyObj.getInterface(GenericInterface.class);

        // Test proxy method call with invalid bus interface name

        try {
            proxy.methodCall("org.alljoyn.bus.BogusInterface", "Ping", "hello");
            fail("Expected BusException('No such method')");
        } catch (BusException e) {
            assertTrue(e.getMessage().startsWith("No such method"));
        }

        // Test proxy method call with invalid bus method name

        try {
            proxy.methodCall(ifaceName, "BogusMethod", "hello");
            fail("Expected BusException('No such method')");
        } catch (BusException e) {
            assertTrue(e.getMessage().startsWith("No such method"));
        }

        // Test proxy method call with missing bus method arg

        try {
            proxy.methodCall(ifaceName, "Ping"/*, "hello"*/); // omitting Ping's arg
            fail("Expected BusException due to too few args specified");
        } catch (BusException e) {
            assertEquals("ER_BUS_UNEXPECTED_SIGNATURE", e.getMessage());
        }

        // Test proxy method call with too many bus method args

        try {
            proxy.methodCall(ifaceName, "Ping", "expected arg", "unexpected arg");
            fail("Expected ArrayIndexOutOfBoundsException due to too many args specified");
        } catch (java.lang.ArrayIndexOutOfBoundsException e) {
        }
    }

    public void testRegisterDynamicBusObject() {
        // Create bus attachment
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        // Create dynamic bus object
        DynamicBusObject busObj = new DynamicBusObject() {
            BusObjectInfo info = buildInterfaceDefinitions();

            @Override
            public String getPath() { return info.getPath(); }

            @Override
            public List<InterfaceDef> getInterfaces() { return info.getInterfaces(); }

            @Override
            public Method getMethodHandler(String interfaceName, String methodName, String signature) {
                // implementation specific - determine and return the appropriate implementation Method
                receivedGetMethodHandler++;
                return getMethodImplementation(getClass(), "myMethodImpl", new Class[]{});
            }

            @Override
            public Method getSignalHandler(String interfaceName, String signalName, String signature) {
                // implementation specific - determine and return the appropriate implementation Method
                receivedGetSignalHandler++;
                return getMethodImplementation(getClass(), "mySignalImpl", new Class[]{Object.class});
            }

            @Override
            public Method[] getPropertyHandler(String interfaceName, String propertyName) {
                // implementation specific - determine and return the appropriate implementation Method
                receivedGetPropertyHandler++;
                return new Method[] {
                    getMethodImplementation(getClass(), "myGetPropertyImpl", new Class[]{}),
                    getMethodImplementation(getClass(), "mySetPropertyImpl", new Class[]{Object.class}) };
            }

            Object myMethodImpl() throws BusException {return null;}
            void mySignalImpl(Object arg) throws BusException {}
            Object myGetPropertyImpl() throws BusException {return null;}
            void mySetPropertyImpl(Object arg) throws BusException {}
        };

        // Register the dynamic bus object
        status = bus.registerBusObject(busObj, "/test");
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetMethodHandler);
        assertEquals(1, receivedGetSignalHandler);
        assertEquals(1, receivedGetPropertyHandler);
    }

    public void testRegisterDynamicBusObject_invalidGetImplHandlers() {
        // Create bus attachment
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        // Create dynamic bus object
        DynamicBusObject busObj = new DynamicBusObject() {
            BusObjectInfo info = buildInterfaceDefinitions();

            @Override
            public String getPath() { return info.getPath(); }

            @Override
            public List<InterfaceDef> getInterfaces() { return info.getInterfaces(); }

            @Override
            public Method getMethodHandler(String interfaceName, String methodName, String signature) {
                return null;  // invalid because the bus object's interface definitions include a bus method
            }

            @Override
            public Method getSignalHandler(String interfaceName, String signalName, String signature) {
                return null;  // invalid because the bus object's interface definitions include a bus signal
            }

            @Override
            public Method[] getPropertyHandler(String interfaceName, String propertyName) {
                return null;  // invalid because the bus object's interface definitions include a bus property
            }
        };

        // Register the dynamic bus object
        status = bus.registerBusObject(busObj, "/test");
        assertEquals(Status.BUS_CANNOT_ADD_HANDLER, status);
    }

    public void testRegisterDynamicBusObject_validNullGetImplHandlers() {
        // Create bus attachment
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        // Create dynamic bus object
        DynamicBusObject busObj = new DynamicBusObject() {
            BusObjectInfo info;
            {   // Initialize the bus object interface definitions to have only a bus method
                InterfaceDef ifaceDef = new InterfaceDef("org.test.iface");
                ifaceDef.addMethod( new MethodDef("DoSomething", "", "", "org.test.iface") );
                info = new BusObjectInfo("/test", Arrays.asList(ifaceDef) );
            }

            @Override
            public String getPath() { return info.getPath(); }

            @Override
            public List<InterfaceDef> getInterfaces() { return info.getInterfaces(); }

            @Override
            public Method getMethodHandler(String interfaceName, String methodName, String signature) {
                // implementation specific - determine and return the appropriate implementation Method
                receivedGetMethodHandler++;
                return getMethodImplementation(getClass(), "myMethodImpl", new Class[]{});
            }

            @Override
            public Method getSignalHandler(String interfaceName, String signalName, String signature) {
                receivedGetSignalHandler++;
                return null;  // OK because the bus object's interface definitions don't include bus signals (never called)
            }

            @Override
            public Method[] getPropertyHandler(String interfaceName, String propertyName) {
                receivedGetPropertyHandler++;
                return null;  // OK because the bus object's interface definitions don't include bus properties (never called)
            }

            void myMethodImpl() throws BusException {}
        };

        // Register the dynamic bus object
        status = bus.registerBusObject(busObj, "/test");
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetMethodHandler);
        assertEquals(0, receivedGetSignalHandler);
        assertEquals(0, receivedGetPropertyHandler);
    }

    public class GenericService implements DynamicBusObject {
        BusObjectInfo info;

        // Map-key format: path + interfaceName + memberName + memberSignature
        public Map<String,Integer> callbackCount = new HashMap<String,Integer>();
        public Map<String,Object> propertyMap = new HashMap<String,Object>();

        public GenericService(String path, String ifaceName) {
            info = buildInterfaceDefinitions2(path, ifaceName);
        }

        @Override
        public String getPath() { return info.getPath(); }

        @Override
        public List<InterfaceDef> getInterfaces() { return info.getInterfaces(); }

        @Override
        public Method getMethodHandler(String interfaceName, String methodName, String signature) {
            receivedGetMethodHandler++;
            return getMethodImplementation(getClass(), "busMethodImpl", new Class[]{Object[].class});
        }

        @Override
        public Method[] getPropertyHandler(String interfaceName, String propertyName) {
            receivedGetPropertyHandler++;
            return new Method[] {
                getMethodImplementation(getClass(), "busPropertyGetImpl", new Class[]{}),
                getMethodImplementation(getClass(), "busPropertySetImpl", new Class[]{Object.class}) };
        }

        @Override
        public Method getSignalHandler(String interfaceName, String signalName, String signature) {
            receivedGetSignalHandler++;
            return getMethodImplementation(getClass(), "busSignalImpl", new Class[]{Object[].class});
        }

        /** A generic implementation Method used with a bus method with any return type and any input parameters */
        public Object busMethodImpl(Object... args) throws BusException {
            MessageContext ctx = bus.getMessageContext();

            // increment count for the specific bus method received
            String key = ctx.objectPath + ctx.interfaceName + ctx.memberName + ctx.signature;
            Integer count = callbackCount.get(key);
            callbackCount.put(key, count==null ? 1 : ++count);

            MethodDef methodDef = info.getMethod(ctx.interfaceName, ctx.memberName, ctx.signature);
            String replySig = methodDef.getReplySignature();
            if (replySig == null || replySig.isEmpty()) {
                return null;
            } else {
                switch (replySig.charAt(0)) {
                    case MsgArg.ALLJOYN_STRING:
                        return "";
                    case MsgArg.ALLJOYN_BOOLEAN:
                        return false;
                    default:
                        return null;
                }
            }
        }

        /** A generic implementation Method used with a set bus property of any type */
        public void busPropertySetImpl(Object arg) throws BusException {
            MessageContext ctx = bus.getMessageContext();
            String key = ctx.objectPath + ctx.interfaceName + ctx.memberName + ctx.signature;

            // increment count for the specific bus property received
            Integer count = callbackCount.get(key);
            callbackCount.put(key, count==null ? 1 : ++count);

            propertyMap.put(key, arg);
        }

        /** A generic implementation Method used with a get bus property of any type */
        public Object busPropertyGetImpl() throws BusException {
            MessageContext ctx = bus.getMessageContext();
            String key = ctx.objectPath + ctx.interfaceName + ctx.memberName + ctx.signature;

            // increment count for the specific bus property received
            Integer count = callbackCount.get(key);
            callbackCount.put(key, count==null ? 1 : ++count);

            return propertyMap.get(key);
        }

        /** A generic implementation Method used with a bus signal with any input parameters */
        public void busSignalImpl(Object... args) throws BusException {
            // no-op - not needed since interface definition in buildInterfaceDefinitions2() has no bus signals
        }
    }

    public void testDynamicBusObjectContainingGenericImplementationHandlers() throws Exception {
        final String busName = "org.alljoyn.bus.BusAttachmentTest";
        final String objPath = "/genericImplementationHandlers";
        final String ifaceName = "org.alljoyn.bus.DynamicInterfaceForGenericHandlerTest";
        Object result;

        // Setup BusAttachment and use DBusProxyObj to request bus name

        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        DBusProxyObj control = bus.getDBusProxyObj();
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner,
                control.RequestName(busName, DBusProxyObj.REQUEST_NAME_NO_FLAGS));

        // Create and register a dynamic bus object

        GenericService busObj = new GenericService(objPath, ifaceName);
        status = bus.registerBusObject(busObj, objPath);
        assertEquals(Status.OK, status);

        // Get proxy using dynamic interface definition and GenericInterface invocation

        ProxyBusObject proxyObj = bus.getProxyBusObject(busName, objPath,
                BusAttachment.SESSION_ID_ANY,
                busObj.getInterfaces(),
                false);
        GenericInterface proxy = proxyObj.getInterface(GenericInterface.class);

        // Test that bus object registration successfully mapped the implementation methods

        assertEquals(4, receivedGetMethodHandler); // Note: counts based on intf def in buildInterfaceDefinitions2()
        assertEquals(3, receivedGetPropertyHandler);
        assertEquals(0, receivedGetSignalHandler);

        // Test BusMethod: boolean DoSomethingNoInArg()

        proxy.methodCall(ifaceName, "DoSomethingNoInArg");
        assertEquals(1, (int)busObj.callbackCount.get(objPath+ifaceName+"DoSomethingNoInArg"+""));
        result = proxy.methodCall(ifaceName, "DoSomethingNoInArg");
        assertEquals(2, (int)busObj.callbackCount.get(objPath+ifaceName+"DoSomethingNoInArg"+""));
        assertTrue(result instanceof Boolean);

        // Test BusMethod: String DoSomethingOneInArg(String arg1)

        result = proxy.methodCall(ifaceName, "DoSomethingOneInArg", "hello");
        assertEquals(1, (int)busObj.callbackCount.get(objPath+ifaceName+"DoSomethingOneInArg"+"s"));
        assertTrue(result instanceof String);

        // Test BusMethod: void DoSomethingTwoInArgs(String arg1, int arg2)

        proxy.methodCall(ifaceName, "DoSomethingTwoInArgs", "hello", 123);
        assertEquals(1, (int)busObj.callbackCount.get(objPath+ifaceName+"DoSomethingTwoInArgs"+"si"));

        // Test BusMethod: void DoSomethingThreeInArgs(String arg1, boolean arg2, String arg3)

        proxy.methodCall(ifaceName, "DoSomethingThreeInArgs", "hello", true, "again");
        assertEquals(1, (int)busObj.callbackCount.get(objPath+ifaceName+"DoSomethingThreeInArgs"+"sbs"));

        // Test BusMethod: void DoSomethingOneInArg(String arg1) - sending wrong arg1 type
        try {
            proxy.methodCall(ifaceName, "DoSomethingOneInArg", 123); // using int arg rather than the expected String type
            fail("Expected BusException");
        } catch (BusException e) {
        }

        // Test BusProperty: String getStringProperty(), void setStringProperty(String value)

        proxy.setProperty(ifaceName, "StringProperty", "blue");
        assertEquals("blue", (String)busObj.propertyMap.get(objPath+ifaceName+"StringProperty"+"s"));

        result = proxy.getProperty(ifaceName, "StringProperty");
        assertEquals("blue", (String)result);

        // Test BusProperty: boolean getBooleanProperty(), void setBooleanProperty(boolean value)

        proxy.setProperty(ifaceName, "BooleanProperty", true);
        assertEquals(true, ((Boolean)busObj.propertyMap.get(objPath+ifaceName+"BooleanProperty"+"b")).booleanValue());

        result = proxy.getProperty(ifaceName, "BooleanProperty");
        assertEquals(true, ((Boolean)result).booleanValue());

        // Test BusProperty: int getIntegerProperty(), void setIntegerProperty(int value)

        proxy.setProperty(ifaceName, "IntegerProperty", 999);
        assertEquals(999, ((Integer)busObj.propertyMap.get(objPath+ifaceName+"IntegerProperty"+"i")).intValue());

        result = proxy.getProperty(ifaceName, "IntegerProperty");
        assertEquals(999, ((Integer)result).intValue());

        // Test BusProperty: void setIntegerProperty(int value) - sending wrong value type

        try {
            proxy.setProperty(ifaceName, "IntegerProperty", "999"); // using String value rather than the expected int type
            fail("Expected BusException");
        } catch (BusException e) {
        }

        // Test proxyObj getAllProperties()

        Map<String,Variant> properties = proxyObj.getAllProperties(ifaceName);

        assertEquals(3, properties.size());
        assertEquals("blue", (String)properties.get("StringProperty").getObject());
        assertEquals(true, ((Boolean)properties.get("BooleanProperty").getObject()).booleanValue());
        assertEquals(999, ((Integer)properties.get("IntegerProperty").getObject()).intValue());
    }

    /* Build a bus object definition containing interfaces that have methods, signals, and properties. */
    private BusObjectInfo buildInterfaceDefinitions() {
        String path = "/test";
        String ifaceName = "org.alljoyn.bus.TestInterface";

        // BusMethod: boolean DoSomething()
        MethodDef methodDef = new MethodDef("DoSomething", "", "b", ifaceName);
        methodDef.addArg( new ArgDef("result", "b", ArgDef.DIRECTION_OUT) );

        // BusSignal: void EmitSomething(String something)
        SignalDef signalDef = new SignalDef("EmitSomething", "s", ifaceName);
        signalDef.addArg( new ArgDef("something", "s") );

        // BusProperty: String getSomeProperty(), void setSomeProperty(String value)
        PropertyDef propertyDef = new PropertyDef("SomeProperty", "s", PropertyDef.ACCESS_READWRITE, ifaceName);

        InterfaceDef interfaceDef = new InterfaceDef(ifaceName);
        interfaceDef.addMethod(methodDef);
        interfaceDef.addSignal(signalDef);
        interfaceDef.addProperty(propertyDef);

        List<InterfaceDef> ifaceDefs = new ArrayList<InterfaceDef>();
        ifaceDefs.add(interfaceDef);
        return new BusObjectInfo(path, ifaceDefs);
    }

    private BusObjectInfo buildInterfaceDefinitions2(String objPath, String ifaceName) {
        List<InterfaceDef> ifaceDefs = new ArrayList<InterfaceDef>();
        InterfaceDef interfaceDef = new InterfaceDef(ifaceName);

        // BusMethod: boolean DoSomethingNoInArg()
        MethodDef methodDef = new MethodDef("DoSomethingNoInArg", "", "b", ifaceName);
        methodDef.addArg( new ArgDef("result", "b", ArgDef.DIRECTION_OUT) );
        interfaceDef.addMethod(methodDef);

        // BusMethod: String DoSomethingOneInArg(String arg1)
        methodDef = new MethodDef("DoSomethingOneInArg", "s", "s", ifaceName);
        methodDef.addArg( new ArgDef("arg1", "s") );
        methodDef.addArg( new ArgDef("result", "s", ArgDef.DIRECTION_OUT) );
        interfaceDef.addMethod(methodDef);

        // BusMethod: void DoSomethingTwoInArgs(String arg1, int arg2)
        methodDef = new MethodDef("DoSomethingTwoInArgs", "si", "", ifaceName);
        methodDef.addArg( new ArgDef("arg1", "s") );
        methodDef.addArg( new ArgDef("arg2", "i") );
        interfaceDef.addMethod(methodDef);

        // BusMethod: void DoSomethingThreeInArgs(String arg1, boolean arg2, String arg3)
        methodDef = new MethodDef("DoSomethingThreeInArgs", "sbs", "", ifaceName);
        methodDef.addArg( new ArgDef("arg1", "s") );
        methodDef.addArg( new ArgDef("arg2", "b") );
        methodDef.addArg( new ArgDef("arg3", "s") );
        interfaceDef.addMethod(methodDef);

        // BusProperty: String getStringProperty(), void setStringProperty(String value)
        PropertyDef propertyDef = new PropertyDef("StringProperty", "s", PropertyDef.ACCESS_READWRITE, ifaceName);
        interfaceDef.addProperty(propertyDef);

        // BusProperty: boolean getBooleanProperty(), void setBooleanProperty(boolean value)
        propertyDef = new PropertyDef("BooleanProperty", "b", PropertyDef.ACCESS_READWRITE, ifaceName);
        interfaceDef.addProperty(propertyDef);

        // BusProperty: int getIntegerProperty(), void setIntegerProperty(int value)
        propertyDef = new PropertyDef("IntegerProperty", "i", PropertyDef.ACCESS_READWRITE, ifaceName);
        interfaceDef.addProperty(propertyDef);

        ifaceDefs.add(interfaceDef);
        return new BusObjectInfo(objPath, ifaceDefs);
    }

    private Method getMethodImplementation(Class<?> clazz, String methodName, Class<?>... parameterTypes) {
        try {
            return clazz.getDeclaredMethod(methodName, parameterTypes);
        } catch (java.lang.NoSuchMethodException ex) {
            System.out.println("No such method " + ex.getMessage());
            return null;
        }
    }

    /*
     *  TODO
     *  Verify that all of the BusAttachment methods are tested
     *  addMatch, removeMatch
     *  findAdvertiseName, and setDaemonDebug.
     */
    /*
     * Note: The following BusAttachement methods are tested through indirect
     * testing (i.e. there is not a test named for the method but the methods
     * are still being tested)
     * advertisedName - is being tested by testFindNewName, testLostExistingName,
     *                  testFindExistingName, testFindMultipleNames
     * requestName - is being tested by testFindNewName, testLostExistingName,
     *               testFindExistingName, testFindMultipleNames
     * cancelAdvertiseName -  is being tested by the testFindMultipleNames
     * releaseName - is being tested in the tearDown of every test
     */

    public void testRegisterNullApplicationStateListener() throws Exception {
        bus = new BusAttachment(getClass().getName(),
                BusAttachment.RemoteMessage.Receive);
        bus.connect();
        try {
            Status status = bus.registerApplicationStateListener(null);
            fail("Failed to get expected exception: ApplicationStateListener object is null");
        } catch (Exception e) {
            // expected exception
        }
    }

    public void testUnregisterApplicationStateListener() throws Exception {
        bus = new BusAttachment(getClass().getName(),
                BusAttachment.RemoteMessage.Receive);
        bus.connect();
        assertEquals(Status.APPLICATION_STATE_LISTENER_NO_SUCH_LISTENER, bus.unregisterApplicationStateListener(appStateListener));
        assertEquals(Status.APPLICATION_STATE_LISTENER_NO_SUCH_LISTENER, bus.unregisterApplicationStateListener(null));
    }

    public void testRegisterApplicationStateListener() throws Exception {
        bus = new BusAttachment(getClass().getName(),
                BusAttachment.RemoteMessage.Receive);
        bus.connect();

        // Test basic register/unregister
        assertEquals(Status.OK, bus.registerApplicationStateListener(appStateListener));
        assertEquals(Status.OK, bus.unregisterApplicationStateListener(appStateListener));

        // Test duplicate unregister
        assertEquals(Status.APPLICATION_STATE_LISTENER_NO_SUCH_LISTENER, bus.unregisterApplicationStateListener(appStateListener));

        // Test duplicate register
        assertEquals(Status.OK, bus.registerApplicationStateListener(appStateListener));
        assertEquals(Status.APPLICATION_STATE_LISTENER_ALREADY_EXISTS, bus.registerApplicationStateListener(appStateListener));
        assertEquals(Status.OK, bus.unregisterApplicationStateListener(appStateListener));
    }

    private ApplicationStateListener appStateListener = new ApplicationStateListener() {
        public void state(String busName, KeyInfoNISTP256 publicKeyInfo, PermissionConfigurator.ApplicationState state) {
            System.out.println("state callback was called on this bus " + busName);
        }
    };

}