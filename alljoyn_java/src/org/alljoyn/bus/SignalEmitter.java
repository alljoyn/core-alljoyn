/*
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

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;

import org.alljoyn.bus.ifaces.Properties;
import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.SignalDef;
import org.alljoyn.bus.defs.ArgDef;
import org.alljoyn.bus.GenericInterface;

/**
 * A helper proxy used by BusObjects to send signals.  A SignalEmitter
 * instance can be used to send any signal from a given AllJoyn interface.
 */
public class SignalEmitter {

    private static final int GLOBAL_BROADCAST = 0x20;
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
     * Constructs a SignalEmitter. If using a dynamic bus object, the emitter
     * will use the GenericInterface.class to relay supported bus signals.
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

        Class<?>[] interfaces;
        Class<?>[] interfacesNew;

        // Check if using dynamic interface definitions (versus static AllJoyn class annotations)
        InvocationHandler handler;
        if (source instanceof DynamicBusObject) {
            interfacesNew = new Class<?>[] {GenericInterface.class};

            // Add support for Properties.PropertiesChanged() signal
            List<InterfaceDef> ifaceDefs = ((DynamicBusObject)source).getInterfaces();
            ifaceDefs.add( buildPropertiesSignalInterface() );

            // Create emit handler that relies on dynamic interface definitions
            handler = new DynamicEmitter(ifaceDefs);
        } else {
            interfaces = source.getClass().getInterfaces();
            interfacesNew = Arrays.copyOf(interfaces, interfaces.length + 1);

            // Add support for Properties.PropertiesChanged() signal
            interfacesNew[interfaces.length] = Properties.class;

            // Create emit handler that relies on AllJoyn class annotations
            handler = new Emitter();
        }

        proxy = Proxy.newProxyInstance(source.getClass().getClassLoader(), interfacesNew, handler);
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

    /** @returns interface definition containing the 'PropertiesChanged' signal of the Properties bus interface. */
    private InterfaceDef buildPropertiesSignalInterface() {
        String interfaceName = InterfaceDescription.getName(Properties.class);
        InterfaceDef interfaceDef = new InterfaceDef(interfaceName);

        SignalDef signalDef = new SignalDef("PropertiesChanged", "sa{sv}as", interfaceName);
        signalDef.addArg( new ArgDef("iface", "s") );
        signalDef.addArg( new ArgDef("changedProps", "a{sv}") );
        signalDef.addArg( new ArgDef("invalidatedProps", "as") );

        interfaceDef.addSignal( signalDef );
        return interfaceDef;
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
     *
     * @deprecated March 2015 for 15.04 release
     */
    @Deprecated
    public void setCompressHeader(boolean compress) {
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


    /**
     * Emits bus signals that are based on dynamic SignalDef definitions.
     *
     * The handler receives an invoke request of GenericInterface.signal(interfaceName, signalName, args...).
     * The {interfaceName, signalName, args...} arguments represent the actual bus signal to be emitted.
     *
     * The DynamicEmitter is initialized with a list of permitted InterfaceDef (and underlying SignalDef
     * definitions). For a requested bus signal to emitted, the corresponding dynamic SignalDef must be
     * present in this list.
     */
    private class DynamicEmitter implements InvocationHandler {

        final private List<InterfaceDef> interfaceDefs;

        /**
         * Constructor.
         *
         * @param interfaceDefs the definitions that describe the interfaces whose signals can be sent.
         * @throws IllegalArgumentException invalid argument was specified.
         */
        DynamicEmitter(List<InterfaceDef> interfaceDefs) {
            if (interfaceDefs == null) {
                throw new IllegalArgumentException("Null interfaceDefs");
            }
            this.interfaceDefs = interfaceDefs;
        }

        /**
         * Send a bus signal based on the specified args and their corresponding SignalDef definition.
         *
         * @param proxy The proxy being called (instance of GenericInterface).
         * @param method The proxy Method being called. Should be GenericInterface.signal(...)
         * @param args an array containing {interfaceName, signalName, args...}, that represent the
         *             actual bus signal to be emitted. Must correspond to a known SignalDef.
         * @return null (bus signals are void operations)
         * @throws IllegalArgumentException invalid proxy Method was specified.
         * @throws BusException if the native call to signal throws one.
         */
        public Object invoke(Object proxy, Method method, Object[] args) throws BusException {
            // Check that the called method is one of the accepted GenericInterface operations
            if (!method.getDeclaringClass().equals(GenericInterface.class) || !method.getName().equals("signal")) {
                throw new IllegalArgumentException(
                        "Invalid emitter method. This signal emitter only supports GenericInterface signal calls.");
            }

            String interfaceName = (String)getArg(args, 0, null);
            String signalName = (String)getArg(args, 1, null);

            // Check that the requested bus signal corresponds to a known SignalDef, and if so emit it
            SignalDef signalDef = BusObjectInfo.getSignal(interfaceDefs,
                    interfaceName, signalName, null);
            if (signalDef != null) {
                Object[] signalArgs = (Object[])getArg(args, 2, new Object[]{});
                signal(source,
                       destination,
                       sessionId,
                       signalDef.getInterfaceName(),
                       signalDef.getName(),
                       signalDef.getSignature(),
                       signalArgs,
                       timeToLive,
                       flags,
                       msgContext);
            }
            return null;
        }

        /** Return the indexed arg. If invalid index, return the given default value. */
        private Object getArg(Object[] args, int index, Object defaultValue) {
            if (index < 0) return defaultValue;
            return (args != null && args.length >= index+1) ? args[index] : defaultValue;
        }
    }

}