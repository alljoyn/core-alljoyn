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

import java.util.Map;
import java.util.HashMap;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.Status;

import junit.framework.TestCase;

public class SignalEmitterTest extends TestCase {
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

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
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
    }

    public void tearDown() throws Exception {
        bus.unregisterBusObject(emitter);
        emitter = null;

        /* Remove rule to receive non-session based signals */
        Status status = bus.addMatch("type='signal',interface='org.alljoyn.bus.EmitterInterface',member='Emit'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals");
        }

        bus.disconnect();
        bus = null;
    }

    public void signalHandler(String string) throws BusException {
        bus.getMessageContext();
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
}
