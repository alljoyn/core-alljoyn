/**
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

import java.lang.ref.WeakReference;

import junit.framework.TestCase;

import org.alljoyn.bus.annotation.BusSignalHandler;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import org.alljoyn.bus.ifaces.Introspectable;

public class BusAttachmentTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

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
                "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n" +
                        "\"http://standards.freedesktop.org/dbus/introspect-1.0.dtd\">\n" +
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

        boolean thrown = false;
        try {
            proxy.ping("hello");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
            assertEquals(Status.BUS_REPLY_IS_ERROR_MESSAGE, ex.getErrorStatus());
            assertEquals("org.alljoyn.Bus.ErStatus", ex.getErrorName());
            /*
             * in release mode the error code is returned in debug mode the
             * actual text for the error is returned.
             */
            assertTrue("ER_OS_ERROR".equals(ex.getErrorMessage()) || "0x0004".equals(ex.getErrorMessage()));
        }
        assertTrue(thrown);

        thrown = false;
        try {
            proxy.ping("hello");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
            assertEquals(Status.BUS_REPLY_IS_ERROR_MESSAGE, ex.getErrorStatus());
            assertEquals("org.alljoyn.bus.ExceptionService.Error1", ex.getErrorName());
            assertEquals("", ex.getErrorMessage());
        }
        assertTrue(thrown);

        thrown = false;
        try {
            proxy.ping("hello");
        } catch (ErrorReplyBusException ex) {
            thrown = true;
            assertEquals(Status.BUS_REPLY_IS_ERROR_MESSAGE, ex.getErrorStatus());
            assertEquals("org.alljoyn.bus.ExceptionService.Error2", ex.getErrorName());
            assertEquals("Message", ex.getErrorMessage());
        }
        assertTrue(thrown);

        thrown = false;
        try {
            proxy.ping("hello");
        } catch (BusException ex) {
            thrown = true;
            assertEquals("org.alljoyn.Bus.ErStatus", ex.getMessage());
        }
        assertTrue(thrown);
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
}
