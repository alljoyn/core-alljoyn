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

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.Status;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import static junit.framework.Assert.*;
import junit.framework.TestCase;

public class SignalEmitterTest extends TestCase {
    public SignalEmitterTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    private BusAttachment bus;

    public class Emitter implements EmitterInterface,
                                    BusObject {

        private SignalEmitter local;
        private SignalEmitter global;
        private SignalEmitter emitter;

        public Emitter() {
            local = new SignalEmitter(this);
            global = new SignalEmitter(this, SignalEmitter.GlobalBroadcast.On);
            emitter = local;
        }

        public void emit(String string) throws BusException {
            emitter.getInterface(EmitterInterface.class).emit(string);
        }

        public void setTimeToLive(int timeToLive) {
            local.setTimeToLive(timeToLive);
            global.setTimeToLive(timeToLive);
        }

        public void setCompressHeader(boolean compress) {
            local.setCompressHeader(compress);
            global.setCompressHeader(compress);
        }

        public void setSessionlessFlag(boolean isSessionless) {
            local.setSessionlessFlag(isSessionless);
            global.setSessionlessFlag(isSessionless);
        }

        public void setGlobalBroadcast(boolean globalBroadcast) {
            emitter = globalBroadcast ? global : local;
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

    private boolean signalReceived = false;
    private int signalsHandled = 0;
    private MessageContext rxMessageContext;

    public void signalHandler(String string) throws BusException {
        rxMessageContext = bus.getMessageContext();
        ++signalsHandled;
        signalReceived = true;
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
        emitter.setGlobalBroadcast(false);
        emitter.setCompressHeader(false);
        emitter.setSessionlessFlag(true);
        emitter.setTimeToLive(0);
        signalReceived = false;
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
        signalReceived = false;
        emitter.emit("sessionless2");
        int serial = emitter.getMessageContext().serial;
        Status status = emitter.cancelSessionlessSignal(serial);
        assertEquals(Status.OK, status);
        status = emitter.cancelSessionlessSignal(58585858);
        assertEquals(Status.BUS_NO_SUCH_MESSAGE, status);
    }

    public void testGlobalBroadcast() throws Exception {
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
//        emitter.setGlobalBroadcast(true);
//        emitter.Emit("globalBroadcastOn");
//        emitter.setGlobalBroadcast(false);
//        emitter.Emit("globalBroadcastOff");
//        Thread.currentThread().sleep(100);
//        assertEquals(1, signalsHandled);
//
//        otherConn.unregisterSignalHandler(this, getClass().getMethod("signalHandler", String.class));
//        otherConn.disconnect();
//        alljoyn.Disconnect(daemon.remoteAddress());
//        daemon.stop();
    }
}
