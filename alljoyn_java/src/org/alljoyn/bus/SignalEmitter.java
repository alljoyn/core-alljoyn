/*
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

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Arrays;

import org.alljoyn.bus.ifaces.Properties;

/**
 * A helper proxy used by BusObjects to send signals.  A SignalEmitter
 * instance can be used to send any signal from a given AllJoyn interface.
 */
public class SignalEmitter {

    private static final int GLOBAL_BROADCAST = 0x20;
    private static final int COMPRESSED = 0x40;
    private static final int SESSIONLESS = 0x10;

    protected BusObject source;
    private final String destination;
    private final int sessionId;
    private int timeToLive;
    private int flags;
    private final Object proxy;
    private final MessageContext msgContext;

    /** Controls behavior of broadcast signals ({@code null} desintation). */
    public enum GlobalBroadcast {

        /**
         * Broadcast signals will not be forwarded across bus-to-bus
         * connections.  This is the default.
         */
        Off,

        /**
         * Broadcast signals will be forwarded across bus-to-bus
         * connections.
         */
        On
    }

    /**
     * Constructs a SignalEmitter.
     *
     * @param source the source object of any signals sent from this emitter
     * @param destination well-known or unique name of destination for signal
     * @param sessionId A unique SessionId for this AllJoyn session instance,
     *                  or BusAttachment.SESSION_ID_ALL_HOSTED to emit on all
     *                  sessions hosted by the BusAttachment.
     * @param globalBroadcast whether to forward broadcast signals
     *                        across bus-to-bus connections
     */
    public SignalEmitter(BusObject source, String destination, int sessionId, GlobalBroadcast globalBroadcast) {
        this.source = source;
        this.destination = destination;
        this.sessionId = sessionId;
        this.flags = (globalBroadcast == GlobalBroadcast.On)
                ? this.flags | GLOBAL_BROADCAST
                : this.flags & ~GLOBAL_BROADCAST;

        Class<?>[] interfaces = source.getClass().getInterfaces();
        Class<?>[] interfacesNew = Arrays.copyOf(interfaces, interfaces.length + 1);
        interfacesNew[interfaces.length] = Properties.class;
        proxy = Proxy.newProxyInstance(source.getClass().getClassLoader(), interfacesNew, new Emitter());
        msgContext = new MessageContext();
    }

    /**
     * Construct a SignalEmitter used for broadcasting to a session
     *
     * @param source the source object of any signals sent from this emitter
     * @param sessionId A unique SessionId for this AllJoyn session instance
     * @param globalBroadcast whether to forward broadcast signals
     *                        across bus-to-bus connections
     */
    public SignalEmitter(BusObject source, int sessionId, GlobalBroadcast globalBroadcast) {
        this(source, null, sessionId, globalBroadcast);
    }

    /**
     * Constructs a SignalEmitter used for broadcasting.
     *
     * @param source the source object of any signals sent from this emitter
     * @param globalBroadcast whether to forward broadcast signals
     *                        across bus-to-bus connections
     */
    public SignalEmitter(BusObject source, GlobalBroadcast globalBroadcast) {
        this(source, null, BusAttachment.SESSION_ID_ANY, globalBroadcast);
    }

    /**
     * Constructs a SignalEmitter used for local broadcasting.
     *
     * @param source the source object of any signals sent from this emitter
     */
    public SignalEmitter(BusObject source) {
        this(source, null, BusAttachment.SESSION_ID_ANY, GlobalBroadcast.Off);
    }

    /** Sends the signal. */
    private native void signal(BusObject busObj, String destination, int sessionId, String ifaceName,
                               String signalName, String inputSig, Object[] args, int timeToLive,
                               int flags, MessageContext ctx) throws BusException;

    private class Emitter implements InvocationHandler {

        @Override
        public Object invoke(Object proxy, Method method, Object[] args) throws BusException {
            for (Class<?> i : proxy.getClass().getInterfaces()) {
                for (Method m : i.getMethods()) {
                    if (method.getName().equals(m.getName())) {
                        signal(source,
                               destination,
                               sessionId,
                               InterfaceDescription.getName(i),
                               InterfaceDescription.getName(m),
                               InterfaceDescription.getInputSig(m),
                               args,
                               timeToLive,
                               flags,
                               msgContext);
                    }
                }
            }
            return null;
        }
    }

    /**
     * Sets the time-to-live of future signals sent from this emitter.
     *
     * @param timeToLive if non-zero this specifies the useful lifetime for signals sent
     *                   by this emitter. The units are in milliseconds for
     *                   non-sessionless signals and seconds for sessionless signals. If
     *                   delivery of the signal is delayed beyond the timeToLive due to
     *                   network congestion or other factors the signal may be
     *                   discarded. There is no guarantee that expired signals will not
     *                   still be delivered.
     */
    public void setTimeToLive(int timeToLive) {
        this.timeToLive = timeToLive;
    }

    /**
     * Enables header compression of future signals sent from this emitter.
     *
     * @param compress if {@code true} compress header for destinations that can handle
     *                 header compression
     */
    public void setCompressHeader(boolean compress) {
        this.flags = compress ? this.flags | COMPRESSED : this.flags & ~COMPRESSED;
    }

    /**
     * Sets the signal to be sent out as a sessionless signal
     *
     * @param sessionless if {@code true} the signal is set to be sent out as a sessionless
     *                       signal
     *
     */
    public void setSessionlessFlag(boolean sessionless) {
        this.flags = sessionless ? this.flags | SESSIONLESS : this.flags & ~SESSIONLESS;
    }

    /**
     * Get the MessageContext of the last signal sent from this emitter.
     *
     * @return  MessageContext of the last signal sent from this emitter.
     */
    public MessageContext getMessageContext() {
        return msgContext;
    }

    private native Status cancelSessionlessSignal(BusObject busObject, int serialNum);

    /**
     * Cancel a sessionless signal sent from this SignalEmitter
     *
     * @param serialNum   Serial number of message to cancel
     * @return
     * <ul>
     * <li>OK if request completed successfully.</li>
     * <li>BUS_NO_SUCH_MESSAGE if message with given serial number could not be located.</li>
     * <li>BUS_NOT_ALLOWED if message with serial number was sent by a different sender.</li>
     * </ul>
     */
    public Status cancelSessionlessSignal(int serialNum) {
        return cancelSessionlessSignal(source, serialNum);
    }

    /**
     * Gets a proxy to the interface that emits signals.
     *
     * @param <T> any class implementation of a interface annotated with AllJoyn interface annotations
     * @param intf the interface of the bus object that emits the signals
     *
     * @return the proxy implementing the signal emitter
     */
    public <T> T getInterface(Class<T> intf) {
        @SuppressWarnings(value = "unchecked")
        T p = (T) proxy;
        return p;
    }
}
