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

import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Map;
import java.util.HashMap;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.MessageContext;
import org.alljoyn.bus.DynamicBusObject;
import org.alljoyn.bus.GenericInterface;
import org.alljoyn.bus.InterfaceDescription;
import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.SignalDef;
import org.alljoyn.bus.defs.ArgDef;

import java.lang.reflect.Method;

import junit.framework.TestCase;

public class SignalEmitterTest extends TestCase {

    private static final String DYN_PATH_NAME   = "/dynamic/ping";
    private static final String DYN_IFACE_NAME  = "org.alljoyn.bus.PingInterface";
    private static final String DYN_SIGNAL_NAME = "Ping";

    private DynamicEmitter dynamicEmitter;
    private int receivedGetSignalHandler; // relates to DynamicBusObject.getSignalHandler() called
    private int receivedSignal;           // relates to SignalEmitterTest.signalHandler() called
    private int receivedSignalInDynBusObj; // relates to DynamicEmitter.receiveSignal() called

    public SignalEmitterTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    private int uniquifier = 0;
    private String genUniqueName(BusAttachment bus) {
        return "test.x" + bus.getGlobalGUIDString() + ".x" + uniquifier++;
    }

    private BusAttachment bus;
    private BusAttachment clientBus;

    public class Participant {
        private String name;
        private BusAttachment pbus;
        private Emitter emitter;
        private int received;

        private Map<String,Integer> hostedSessions;

        public Participant(String _name) throws Exception {
            name = _name;
            received = 0;
            hostedSessions = new HashMap<String,Integer>();
            pbus = new BusAttachment(getClass().getName() + name);
            Status status = pbus.connect();
            assertEquals(Status.OK, status);
            SessionOpts sessionOpts = new SessionOpts();
            sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
            sessionOpts.isMultipoint = false;
            sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
            sessionOpts.transports = SessionOpts.TRANSPORT_ANY;
            Mutable.ShortValue sessionPort = new Mutable.ShortValue((short) 42);
            assertEquals(Status.OK, pbus.bindSessionPort(sessionPort, sessionOpts, new SessionPortListener() {
                public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts opts) {return true;}
                public void sessionJoined(short sessionPort, int id, String joiner) {
                    hostedSessions.put(joiner, id);
                }
            }));
            // Request name from bus
            int flag = BusAttachment.ALLJOYN_REQUESTNAME_FLAG_REPLACE_EXISTING | BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE;
            assertEquals(Status.OK, pbus.requestName(name, flag));
            // Advertise same bus name
            assertEquals(Status.OK, pbus.advertiseName(name, SessionOpts.TRANSPORT_ANY));

            // Create bus object
            emitter = new Emitter();
            emitter.setEmitter(Emitter.ALL_HOSTED);
            status = pbus.registerBusObject(emitter, "/emitter");
            assertEquals(Status.OK, status);

            // Register signal handler
            status = pbus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                    this, getClass().getMethod("signalHandler", String.class));
            assertEquals(Status.OK, status);
        }

        public String getName() {
            return name;
        }

        public boolean hasJoiner(String joinerName) {
            return hostedSessions.containsKey(joinerName);
        }

        public void join(Participant other) throws Exception {
            SessionOpts sessionOpts = new SessionOpts();
            sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
            sessionOpts.isMultipoint = false;
            sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
            sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

            Mutable.IntegerValue sessionId = new Mutable.IntegerValue(0);
            Status status = pbus.joinSession(other.getName(), (short)42, sessionId,
                    sessionOpts, new SessionListener());
            assertEquals(Status.OK, status);

            int count = 0;
            while (count < 10 && !other.hasJoiner(pbus.getUniqueName())) {
                ++count;
                Thread.sleep(100);
            }
            assertTrue(count != 10);
        }

        public void find(String other) {
            pbus.findAdvertisedName(other);
        }

        public void signalHandler(String string) throws BusException {
            pbus.getMessageContext();
            ++received;
        }

        public void checkReceived(int expected) throws Exception {
            assertEquals(expected, received);
            received = 0;
        }

        public void emit() throws BusException {
            emitter.emit("sessioncast");
        }
    }

    public class Emitter implements EmitterInterface,
                                    BusObject {

        private SignalEmitter local;
        private SignalEmitter global;
        private SignalEmitter allHosted;
        private SignalEmitter emitter;

        public static final int GLOBAL = 0;
        public static final int LOCAL = 1;
        public static final int ALL_HOSTED = 2;

        public Emitter() {
            local = new SignalEmitter(this);
            global = new SignalEmitter(this, SignalEmitter.GlobalBroadcast.On);
            allHosted = new SignalEmitter(this, BusAttachment.SESSION_ID_ALL_HOSTED, SignalEmitter.GlobalBroadcast.Off);
            emitter = local;
        }

        public void emit(String string) throws BusException {
            emitter.getInterface(EmitterInterface.class).emit(string);
        }

        public void setTimeToLive(int timeToLive) {
            local.setTimeToLive(timeToLive);
            global.setTimeToLive(timeToLive);
            allHosted.setTimeToLive(timeToLive);
        }

        public void setCompressHeader(boolean compress) {
            local.setCompressHeader(compress);
            global.setCompressHeader(compress);
            allHosted.setCompressHeader(compress);
        }

        public void setSessionlessFlag(boolean isSessionless) {
            local.setSessionlessFlag(isSessionless);
            global.setSessionlessFlag(isSessionless);
        }

        public void setEmitter(int type) {
            switch (type) {
                case GLOBAL:
                    emitter = global;
                    break;
                case LOCAL:
                    emitter = local;
                    break;
                case ALL_HOSTED:
                    emitter = allHosted;
                    break;
                default:
                    emitter = local;
            }
        }

        public MessageContext getMessageContext() {
            return local.getMessageContext();
        }

        public Status cancelSessionlessSignal(int serialNum) {
            return local.cancelSessionlessSignal(serialNum);
        }
    }

    private Emitter emitter;

    /** DynamicEmitter is a combination of a signal emitter and a dynamic bus object that has signals to emit. */
    public class DynamicEmitter implements DynamicBusObject {
        private BusAttachment ownerBus; // the bus that this dynamic object is registered to
        private MessageContext ctx;     // message context from most recent receiveSignal (consumer-side)
        private BusObjectInfo info;

        public DynamicEmitter(BusAttachment bus, BusObjectInfo busObjDefinition) {
            ownerBus = bus;
            info = busObjDefinition;
        }

        @Override
        public BusAttachment getBus() {
            return ownerBus;
        }

        @Override
        public String getPath() {
            return info.getPath();
        }

        @Override
        public List<InterfaceDef> getInterfaces() {
            return info.getInterfaces();
        }

        /**
         * Return the signal handler Method that matches the given interface name, signal name and signature.
         */
        @Override
        public Method getSignalHandler(String interfaceName, String signalName, String signature) {
            receivedGetSignalHandler++;

            try {
                // generic handler (supports any number of input parameters)
                return getClass().getDeclaredMethod( "receiveSignal", new Class[]{Object[].class} );
            } catch (java.lang.NoSuchMethodException ex) {
                ex.printStackTrace();
                return null;
            }
        }

        @Override
        public Method getMethodHandler(String interfaceName, String methodName, String signature) {return null;}

        @Override
        public Method[] getPropertyHandler(String interfaceName, String propertyName) {return null;}

        /**
         * Signal handler for a signal received on the consumer-side.
         *
         * Note: The client must register its signal handler methods on its bus using one of
         * the dynamic interface registration methods in BusAttachment. When the bus object
         * has multiple interface and/or signal definitions, the most convenient registration
         * method is registerSignalHandlers(DynamicBusObject).
         */
        public void receiveSignal(Object... args) throws BusException {
            receivedSignalInDynBusObj++;

            ctx = ownerBus.getMessageContext();
            String msg = Arrays.toString(args).replaceAll("\\[|\\]", "");;
            System.out.println("DynamicEmitter.receiveSignal() received " +
                ctx.interfaceName + "." + ctx.memberName + ": " + msg);
        }

        /** Used by consumer-side, the message context of the most recent received signal. */
        public MessageContext getMessageContext() {
            return ctx;
        }

        /**
         * Helper method to emit a signal. Used by producer-side.
         * @throws BusException if signal to be emitted is not defined by the dynamic bus object.
         */
        public void emitSignal(String ifaceName, String signalName, Object... args) throws BusException {
            SignalEmitter emitter = new SignalEmitter(this);
            emitter.setSessionlessFlag(true);
            emitter.setTimeToLive(0);

            // DynamicBusObject signals are always emitted using GenericInterface.signal()
            GenericInterface emitterIntf = emitter.getInterface(GenericInterface.class);
            emitterIntf.signal(ifaceName, signalName, args);
        }
    }

    /* Build an interface definition that contains the signal Ping(string). */
    private static BusObjectInfo buildBusObjectInfo(String path) {
        SignalDef signalDef = new SignalDef(DYN_SIGNAL_NAME, "s", DYN_IFACE_NAME);
        signalDef.addArg( new ArgDef("string", "s", ArgDef.DIRECTION_OUT) );

        InterfaceDef interfaceDef = new InterfaceDef(DYN_IFACE_NAME, true, null);
        interfaceDef.addSignal(signalDef);

        List<InterfaceDef> ifaces = new ArrayList<InterfaceDef>();
        ifaces.add(interfaceDef);
        return new BusObjectInfo(path, ifaces);
    }

    /* Build an interface definition for client-side that contains the signal Ping(string),
       and has matching criteria for signal handling. */
    private static BusObjectInfo buildClientBusObjectInfo(String rule, String source) {
        SignalDef signalDef = new SignalDef(DYN_SIGNAL_NAME, "s", DYN_IFACE_NAME, rule, source);
        signalDef.addArg(new ArgDef("string", "s", ArgDef.DIRECTION_OUT));

        InterfaceDef interfaceDef = new InterfaceDef(DYN_IFACE_NAME, true, null);
        interfaceDef.addSignal(signalDef);

        List<InterfaceDef> ifaces = new ArrayList<InterfaceDef>();
        ifaces.add(interfaceDef);
        return new BusObjectInfo("", ifaces);
    }

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        clientBus = new BusAttachment(getClass().getName() + "-client");
        status = clientBus.connect();
        assertEquals(Status.OK, status);

        emitter = new Emitter();
        status = bus.registerBusObject(emitter, "/emitter");
        assertEquals(Status.OK, status);

        assertEquals(Status.OK, bus.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
                                                          this, getClass().getMethod("signalHandler",
                                                                                     String.class)));

        /* Add rule to receive non-session based signals */
        status = bus.addMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals");
        }

        receivedSignal = 0;
        receivedSignalInDynBusObj= 0;
        receivedGetSignalHandler = 0;
    }

    public void tearDown() throws Exception {
        bus.unregisterBusObject(emitter);
        emitter = null;

        /* Remove rule to receive non-session based signals */
        Status status = bus.removeMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        if (Status.OK != status) {
            throw new GameException("Cannot remove rule to receive signals");
        }

        if (dynamicEmitter != null) {
            bus.unregisterBusObject(dynamicEmitter);
            dynamicEmitter = null;

            // Remove rule to receive non-session based signals
            status = clientBus.removeMatch("type='signal',interface='" + DYN_IFACE_NAME + "',member='" + DYN_SIGNAL_NAME + "'");
            if (Status.OK != status) {
                status = clientBus.removeMatch("type='signal',interface='" + DYN_IFACE_NAME + "'");
                if (Status.OK != status) {
                    throw new GameException("Cannot remove rule to receive signals for "
                        + DYN_IFACE_NAME + " : " + status.name());
                }
            }
        }

        bus.unregisterSignalHandler(this, getClass().getMethod("signalHandler", String.class));

        bus.disconnect();
        bus = null;
        clientBus.disconnect();
        clientBus = null;
    }

    public void signalHandler(String string) throws BusException {
        receivedSignal++;

        MessageContext ctx = bus.getMessageContext();
        System.out.println("SignalEmitterTest.signalHandler() received " +
                ctx.interfaceName + "." + ctx.memberName + ": " + string);
    }

    public void testTimeToLive() throws Exception {
        emitter.setTimeToLive(1);
        emitter.emit("timeToLiveOn");

        emitter.setTimeToLive(0);
        emitter.emit("timeToLiveOff");

        // TODO: how to verify?
    }

    public void testCompressHeader() throws Exception {
        emitter.setCompressHeader(true);
        emitter.emit("compressHeaderOn1");
        emitter.emit("compressHeaderOn2");

        emitter.setCompressHeader(false);
        emitter.emit("compressHeaderOff");

        // TODO: how to verify?
    }

    public void testMessageContext() throws Exception {
        emitter.setEmitter(Emitter.LOCAL);
        emitter.setCompressHeader(false);
        emitter.setSessionlessFlag(true);
        emitter.setTimeToLive(0);
        emitter.emit("sessionless1");
        MessageContext ctx = emitter.getMessageContext();
        assertEquals("/emitter", ctx.objectPath);
        assertEquals("org.alljoyn.bus.EmitterInterface", ctx.interfaceName);
        assertEquals("Emit", ctx.memberName);
        assertEquals("", ctx.destination);
        assertEquals(bus.getUniqueName(), ctx.sender);
        assertEquals("s", ctx.signature);
    }

    public void testCancelSessionless() throws Exception {
        emitter.setCompressHeader(false);
        emitter.setSessionlessFlag(true);
        emitter.setTimeToLive(0);
        emitter.emit("sessionless2");
        int serial = emitter.getMessageContext().serial;
        Status status = emitter.cancelSessionlessSignal(serial);
        assertEquals(Status.OK, status);
        status = emitter.cancelSessionlessSignal(58585858);
        assertEquals(Status.BUS_NO_SUCH_MESSAGE, status);
    }

    public synchronized void testGlobalBroadcast() throws Exception {
        // TODO fix this text
//        /* Set up another daemon to receive the global broadcast signal. */
//        AllJoynDaemon daemon = new AllJoynDaemon();
//        AllJoynProxyObj alljoyn = bus.getAllJoynProxyObj();
//        assertEquals(AllJoynProxyObj.ConnectResult.Success, alljoyn.Connect(daemon.remoteAddress()));
//
//        System.setProperty("org.alljoyn.bus.address", daemon.address());
//        BusAttachment otherConn = new BusAttachment(getClass().getName(), BusAttachment.RemoteMessage.Receive);
//        assertEquals(Status.OK, otherConn.connect());
//        assertEquals(Status.OK, otherConn.registerSignalHandler("org.alljoyn.bus.EmitterInterface", "Emit",
//                                                                this, getClass().getMethod("signalHandler",
//                                                                                           String.class)));
//
//        /* Emit the signal from this daemon. */
//        signalsHandled = 0;
//        emitter.setEmitter(Emitter.GLOBAL);
//        emitter.Emit("globalBroadcastOn");
//        emitter.setEmitter(Emitter.LOCAL);
//        emitter.Emit("globalBroadcastOff");
//        Thread.currentThread().sleep(100);
//        assertEquals(1, signalsHandled);
//
//        otherConn.unregisterSignalHandler(this, getClass().getMethod("signalHandler", String.class));
//        otherConn.disconnect();
//        alljoyn.Disconnect(daemon.remoteAddress());
//        daemon.stop();
    }

    public void testSessionCast() throws Exception {
        String AA = genUniqueName(bus);
        String BB = genUniqueName(bus);
        String CC = genUniqueName(bus);

        Participant A = new Participant(AA);
        Participant B = new Participant(BB);
        Participant C = new Participant(CC);

        A.find(BB);
        A.find(CC);
        B.find(AA);
        B.find(CC);
        C.find(AA);
        C.find(BB);


        /* no sessions yet */
        A.emit();
        B.emit();
        C.emit();
        Thread.sleep(1000);
        A.checkReceived(0);
        B.checkReceived(0);
        C.checkReceived(0);

        /* set up some sessions */
        B.join(A);
        C.join(A);
        C.join(B);
        A.emit();
        B.emit();
        Thread.sleep(1000);
        A.checkReceived(0);
        B.checkReceived(1);
        C.checkReceived(2);

        C.emit();
        Thread.sleep(1000);
        A.checkReceived(0);
        B.checkReceived(0);
        C.checkReceived(0);
    }

    public void testDynamicSignalDef_registerHandler() throws Exception {
        Status status;

        // Create and register the dynamic bus object (producer-side)
        BusObjectInfo busObjInfo = buildBusObjectInfo(DYN_PATH_NAME);
        dynamicEmitter = new DynamicEmitter(bus, busObjInfo); // models a SignalEmitter and a DynamicBusObject
        status = bus.registerBusObject(dynamicEmitter, busObjInfo.getPath());
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetSignalHandler);

        // Register the client signal handler (consumer-side) for a specific signal (matches any source path by default)
        InterfaceDef ifaceDef = busObjInfo.getInterface(DYN_IFACE_NAME);
        status = clientBus.registerSignalHandler(ifaceDef, DYN_SIGNAL_NAME, this,
                              getClass().getMethod("signalHandler",String.class));
        assertEquals(Status.OK, status);

        // Add rule to receive non-session based signals (Note: can omit member field to match all signals on interface)
        status = clientBus.addMatch("type='signal',interface='" + DYN_IFACE_NAME + "',member='" + DYN_SIGNAL_NAME + "'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals for DynamicEmitter");
        }

        // Test the signal emitter that is using a dynamic bus object (producer-side sends signal)
        dynamicEmitter.emitSignal(DYN_IFACE_NAME, DYN_SIGNAL_NAME, "registerHandler");

        Thread.sleep(1000);
        assertEquals(1, receivedSignal);
        assertEquals(0, receivedSignalInDynBusObj);

        clientBus.unregisterSignalHandler(this, getClass().getMethod("signalHandler", String.class));
    }

    public void testDynamicSignalDef_registerHandler_withRule() throws Exception {
        Status status;

        // Create and register the dynamic bus object (producer-side)
        BusObjectInfo busObjInfo = buildBusObjectInfo(DYN_PATH_NAME);
        dynamicEmitter = new DynamicEmitter(bus, busObjInfo); // models a SignalEmitter and a DynamicBusObject
        status = bus.registerBusObject(dynamicEmitter, busObjInfo.getPath());
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetSignalHandler);

        // Register the client signal handler (consumer-side) for a specific signal, and with rule expected to match
        InterfaceDef ifaceDef = busObjInfo.getInterface(DYN_IFACE_NAME);
        Method m = getClass().getMethod("signalHandler",String.class);
        String rule = "path='" + DYN_PATH_NAME + "'";
        status = clientBus.registerSignalHandlerWithRule(ifaceDef, DYN_SIGNAL_NAME, this, m, rule);
        assertEquals(Status.OK, status);

        // Add rule to receive non-session based signals (Note: can omit member field to match all signals on interface)
        status = clientBus.addMatch("type='signal',interface='" + DYN_IFACE_NAME + "',member='" + DYN_SIGNAL_NAME + "'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals for DynamicEmitter");
        }

        // Test the signal emitter that is using a dynamic bus object (producer-side sends signal)
        dynamicEmitter.emitSignal(DYN_IFACE_NAME, DYN_SIGNAL_NAME, "registerHandler_withRule");

        Thread.sleep(1000);
        assertEquals(1, receivedSignal);
        assertEquals(0, receivedSignalInDynBusObj);

        clientBus.unregisterSignalHandler(this, m);
    }

    public void testDynamicSignalDef_registerHandler_withBogusRule() throws Exception {
        Status status;

        // Create and register the dynamic bus object (producer-side)
        BusObjectInfo busObjInfo = buildBusObjectInfo(DYN_PATH_NAME);
        dynamicEmitter = new DynamicEmitter(bus, busObjInfo); // models a SignalEmitter and a DynamicBusObject
        status = bus.registerBusObject(dynamicEmitter, busObjInfo.getPath());
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetSignalHandler);

        // Register the client signal handler (consumer-side) for a specific signal, and with rule not expected to match
        InterfaceDef ifaceDef = busObjInfo.getInterface(DYN_IFACE_NAME);
        Method m = getClass().getMethod("signalHandler",String.class);
        status = clientBus.registerSignalHandlerWithRule(ifaceDef, DYN_SIGNAL_NAME, this, m, "path='/bogusPath'");
        assertEquals(Status.OK, status);

        // Add rule to receive non-session based signals (Note: can omit member field to match all signals on interface)
        status = clientBus.addMatch("type='signal',interface='" + DYN_IFACE_NAME + "',member='" + DYN_SIGNAL_NAME + "'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals for DynamicEmitter");
        }

        // Test the signal emitter that is using a dynamic bus object (producer-side sends signal)
        dynamicEmitter.emitSignal(DYN_IFACE_NAME, DYN_SIGNAL_NAME, "registerHandler_withBogusRule");

        Thread.sleep(1000);
        assertEquals(0, receivedSignal);  // signal not received since filtering by non-matching rule
        assertEquals(0, receivedSignalInDynBusObj);

        clientBus.unregisterSignalHandler(this, m);
    }

    public void testDynamicSignalDef_registerHandlers() throws Exception {
        Status status;

        // Create and register the dynamic bus object (producer-side)
        BusObjectInfo busObjInfo = buildBusObjectInfo(DYN_PATH_NAME);
        dynamicEmitter = new DynamicEmitter(bus, busObjInfo); // models a SignalEmitter and a DynamicBusObject
        status = bus.registerBusObject(dynamicEmitter, busObjInfo.getPath());
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetSignalHandler);

        // Register the client signal handlers (consumer-side) for all defined signals (matches any source path by default)
        DynamicBusObject busObjHandler = new DynamicEmitter(
            clientBus, buildClientBusObjectInfo("","")); // empty rule and source path
        status = clientBus.registerSignalHandlers(busObjHandler);
        assertEquals(Status.OK, status);

        // Add rule to receive non-session based signals (Note: omit member field to match all signals on interface)
        status = clientBus.addMatch("type='signal',interface='" + DYN_IFACE_NAME + "'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals for DynamicEmitter");
        }

        // Test the signal emitter that is using a dynamic bus object (producer-side sends signal)
        dynamicEmitter.emitSignal(DYN_IFACE_NAME, DYN_SIGNAL_NAME, "registerHandlers");

        Thread.sleep(1000);
        assertEquals(0, receivedSignal);
        assertEquals(1, receivedSignalInDynBusObj);

        MessageContext ctx = ((DynamicEmitter)busObjHandler).getMessageContext();
        assertEquals(DYN_PATH_NAME, ctx.objectPath);
        assertEquals(DYN_IFACE_NAME, ctx.interfaceName);
        assertEquals(DYN_SIGNAL_NAME, ctx.memberName);
        assertEquals("s", ctx.signature);

        clientBus.unregisterSignalHandlers(busObjHandler);
    }

    public void testDynamicSignalDef_registerHandlers_withSourcePath() throws Exception {
        Status status;

        // Create and register the dynamic bus object (producer-side)
        BusObjectInfo busObjInfo = buildBusObjectInfo(DYN_PATH_NAME);
        dynamicEmitter = new DynamicEmitter(bus, busObjInfo); // models a SignalEmitter and a DynamicBusObject
        status = bus.registerBusObject(dynamicEmitter, busObjInfo.getPath());
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetSignalHandler);

        // Register the client signal handlers (consumer-side) for all defined signals, and with specified source path
        DynamicBusObject busObjHandler = new DynamicEmitter(
            clientBus, buildClientBusObjectInfo("",DYN_PATH_NAME)); // empty rule, non-empty source path
        status = clientBus.registerSignalHandlers(busObjHandler);
        assertEquals(Status.OK, status);

        // Add rule to receive non-session based signals (Note: omit member field to match all signals on interface)
        status = clientBus.addMatch("type='signal',interface='" + DYN_IFACE_NAME + "'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals for DynamicEmitter");
        }

        // Test the signal emitter that is using a dynamic bus object (producer-side sends signal)
        dynamicEmitter.emitSignal(DYN_IFACE_NAME, DYN_SIGNAL_NAME, "registerHandlers_withSourcePath");

        Thread.sleep(1000);
        assertEquals(0, receivedSignal);
        assertEquals(1, receivedSignalInDynBusObj);

        MessageContext ctx = ((DynamicEmitter)busObjHandler).getMessageContext();
        assertEquals(DYN_PATH_NAME, ctx.objectPath);
        assertEquals(DYN_IFACE_NAME, ctx.interfaceName);
        assertEquals(DYN_SIGNAL_NAME, ctx.memberName);
        assertEquals("s", ctx.signature);

        clientBus.unregisterSignalHandlers(busObjHandler);
    }

    public void testDynamicSignalDef_registerHandlers_withBogusSourcePath() throws Exception {
        Status status;

        // Create and register the dynamic bus object (producer-side)
        BusObjectInfo busObjInfo = buildBusObjectInfo(DYN_PATH_NAME);
        dynamicEmitter = new DynamicEmitter(bus, busObjInfo); // models a SignalEmitter and a DynamicBusObject
        status = bus.registerBusObject(dynamicEmitter, busObjInfo.getPath());
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetSignalHandler);

        // Register the client signal handlers (consumer-side) for all defined signals, and with unknown source path
        DynamicBusObject busObjHandler = new DynamicEmitter(
            clientBus, buildClientBusObjectInfo("","/bogusEmitter")); // empty rule, bogus source path
        status = clientBus.registerSignalHandlers(busObjHandler);
        assertEquals(Status.OK, status);

        // Add rule to receive non-session based signals (Note: omit member field to match all signals on interface)
        status = clientBus.addMatch("type='signal',interface='" + DYN_IFACE_NAME + "'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals for DynamicEmitter");
        }

        // Test the signal emitter that is using a dynamic bus object (producer-side sends signal)
        dynamicEmitter.emitSignal(DYN_IFACE_NAME, DYN_SIGNAL_NAME, "registerHandlers_withBogusSourcePath");

        Thread.sleep(1000);
        assertEquals(0, receivedSignal);
        assertEquals(0, receivedSignalInDynBusObj); // signal not received since producer's DynamicEmitter has wrong path

        clientBus.unregisterSignalHandlers(busObjHandler);
    }

    public void testDynamicSignalDef_registerHandlers_withRule() throws Exception {
        Status status;

        // Create and register the dynamic bus object (producer-side)
        BusObjectInfo busObjInfo = buildBusObjectInfo(DYN_PATH_NAME);
        dynamicEmitter = new DynamicEmitter(bus, busObjInfo); // models a SignalEmitter and a DynamicBusObject
        status = bus.registerBusObject(dynamicEmitter, busObjInfo.getPath());
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetSignalHandler);

        // Register the client signal handlers (consumer-side) for all defined signals, and with rule expected to match
        String rule = "path='" + DYN_PATH_NAME + "'";
        DynamicBusObject busObjHandler = new DynamicEmitter(
            clientBus, buildClientBusObjectInfo(rule,"")); // non-empty rule, empty source path
        status = clientBus.registerSignalHandlers(busObjHandler);
        assertEquals(Status.OK, status);

        // Add rule to receive non-session based signals (Note: omit member field to match all signals on interface)
        status = clientBus.addMatch("type='signal',interface='" + DYN_IFACE_NAME + "'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals for DynamicEmitter");
        }

        // Test the signal emitter that is using a dynamic bus object (producer-side sends signal)
        dynamicEmitter.emitSignal(DYN_IFACE_NAME, DYN_SIGNAL_NAME, "registerHandlers_withRule");

        Thread.sleep(1000);
        assertEquals(0, receivedSignal);
        assertEquals(1, receivedSignalInDynBusObj);

        MessageContext ctx = ((DynamicEmitter)busObjHandler).getMessageContext();
        assertEquals(DYN_PATH_NAME, ctx.objectPath);
        assertEquals(DYN_IFACE_NAME, ctx.interfaceName);
        assertEquals(DYN_SIGNAL_NAME, ctx.memberName);
        assertEquals("s", ctx.signature);

        clientBus.unregisterSignalHandlers(busObjHandler);
    }

    public void testDynamicSignalDef_registerHandlers_withBogusRule() throws Exception {
        Status status;

        // Create and register the dynamic bus object (producer-side)
        BusObjectInfo busObjInfo = buildBusObjectInfo(DYN_PATH_NAME);
        dynamicEmitter = new DynamicEmitter(bus, busObjInfo); // models a SignalEmitter and a DynamicBusObject
        status = bus.registerBusObject(dynamicEmitter, busObjInfo.getPath());
        assertEquals(Status.OK, status);
        assertEquals(1, receivedGetSignalHandler);

        // Register the client signal handlers (consumer-side) for all defined signals, and with rule not expected to match
        String rule = "path='/bogusPath'";
        DynamicBusObject busObjHandler = new DynamicEmitter(
            clientBus, buildClientBusObjectInfo(rule,"")); // bogus rule, empty source path
        status = clientBus.registerSignalHandlers(busObjHandler);
        assertEquals(Status.OK, status);

        // Add rule to receive non-session based signals (Note: omit member field to match all signals on interface)
        status = clientBus.addMatch("type='signal',interface='" + DYN_IFACE_NAME + "'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals for DynamicEmitter");
        }

        // Test the signal emitter that is using a dynamic bus object (producer-side sends signal)
        dynamicEmitter.emitSignal(DYN_IFACE_NAME, DYN_SIGNAL_NAME, "registerHandlers_withBogusRule");

        Thread.sleep(1000);
        assertEquals(0, receivedSignal);
        assertEquals(0, receivedSignalInDynBusObj);  // signal not received since filtering by non-matching rule

        clientBus.unregisterSignalHandlers(busObjHandler);
    }

}
