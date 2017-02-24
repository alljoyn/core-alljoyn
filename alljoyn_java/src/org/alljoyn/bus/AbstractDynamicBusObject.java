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

import java.lang.reflect.Method;
import java.util.List;

import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.MethodDef;
import org.alljoyn.bus.defs.SignalDef;
import org.alljoyn.bus.defs.PropertyDef;

/**
 * Convenience abstract DynamicBusObject that implements default handler Methods,
 * allowing subclasses to only implement those bus method, signal, and property handlers
 * that they expect to receive calls on (regardless of what the full bus object definition
 * specifies).
 *
 * The default implementation for getMethodHandler() returns Method methodReceived(Object... args),
 * which is a generic handler for any bus method received. Subclasses can override
 * methodReceived in order to provide a concreate implementation if needed.
 *
 * The default implementation for getSignalHandler() returns Method signalReceived(Object... args),
 * which is a generic handler for any bus signal received. Subclasses can override
 * signalReceived in order to provide a concrete implementation if needed.
 *
 * The default implementation for getPropertyHandler() returns the getPropertyReceived()
 * and setPropertyReceived(Object) Methods, which are generic handlers for any get/set bus
 * property received. Subclasses can override getPropertyReceived and/or setPropertyReceived
 * in order to provide a concrete implementation if needed.
 *
 * Subclasses can also override getMethodHandler, getSignalHandler, or getPropertyHandler
 * in order to have them return different handler Methods than the usual methodReceived,
 * signalReceived, getPropertyReceived, and setPropertyReceived repectively.
 */
public abstract class AbstractDynamicBusObject implements DynamicBusObject  {

    private final BusAttachment mBus;
    private final BusObjectInfo mInfo;

    /**
     * Construct abstract dynamic bus object.
     *
     * @param bus the bus attachment with which this object is registered.
     * @param objectDef the bus object's interface(s) definition.
     */
    public AbstractDynamicBusObject(BusAttachment bus, BusObjectInfo objectDef) {
        mBus = bus;
        mInfo = objectDef;
    }

    /**
     * Construct abstract dynamic bus object.
     *
     * @param bus the bus attachment with which this object is registered.
     * @param path the object path.
     * @param interfaces the bus object's contained interfaces.
     * @throws IllegalArgumentException if interfaces argument is null.
     */
    public AbstractDynamicBusObject(BusAttachment bus, String path, List<InterfaceDef> interfaces) {
        mBus = bus;
        mInfo = new BusObjectInfo(path, interfaces);
    }

    /**
     * @return the bus attachment with which this bus object is registered.
     */
    public BusAttachment getBus() {
        return mBus;
    }

    @Override
    public String getPath() {
        return mInfo.getPath();
    }

    @Override
    public List<InterfaceDef> getInterfaces() {
        return mInfo.getInterfaces();
    }

    /**
     * Default implementation. Returns the methodReceived() Method, which is
     * a generic implementation that handles all bus methods received by the
     * bus object.
     *
     * If a subclass supports receipt of bus methods, it should override
     * methodReceived() or getMethodHandler() in order to provide a concrete
     * implementation. If a subclass does not define bus methods or does not
     * receive bus method calls, then the default implementation need not be
     * overridden.
     *
     * @return the methodReceived(Object... args) Method if the given bus method
     *         criteria matches a defined bus method in the dynamic interface
     *         definitions provided at construction time. Otherwise, return null.
     */
    @Override
    public Method getMethodHandler(String interfaceName, String methodName, String signature) {
        try {
            MethodDef methodDef = mInfo.getMethod(interfaceName, methodName, signature);
            if (methodDef != null) {
                return getClass().getMethod("methodReceived", new Class[]{Object[].class});
            }
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        }
        return null; // specified bus method is not defined
    }

    /**
     * Default implementation. Returns the signalReceived() Method, which is
     * a generic implementation that handles all bus signals received by the
     * bus object.
     *
     * If a subclass supports receipt of bus signals, it should override
     * signalReceived() or getSignalHandler() in order to provide a concrete
     * implementation. If a subclass does not define bus signals or does not
     * receive bus signal calls, then the default implementation need not be
     * overridden.
     *
     * @return the signalReceived(Object... args) Method if the given bus signal
     *         criteria matches a defined bus signal in the dynamic interface
     *         definitions provided at construction time. Otherwise, return null.
     */
    @Override
    public Method getSignalHandler(String interfaceName, String signalName, String signature) {
        try {
            SignalDef signalDef = mInfo.getSignal(interfaceName, signalName, signature);
            if (signalDef != null) {
                return getClass().getMethod("signalReceived", new Class[]{Object[].class});
            }
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        }
        return null; // specified bus signal is not defined
    }

    /**
     * Default implementation. Returns the getPropertyReceived() and/or
     * setPropertyReceived() Methods, which are generic implementations that
     * handle all get/set bus properties received by the bus object.
     *
     * If a subclass supports receipt of get/set bus properties, it should
     * override one or more of getPropertyReceived(), setPropertyReceived(),
     * or getPropertyHandler() in order to provide concrete get and/or set
     * implementations. If a subclass does not define bus properties or does
     * not receive get/set bus property calls, then the default implementation
     * need not be overridden.
     *
     * @return the getPropertyReceived() and setPropertyReceived(Object) Methods
     *         (in a 2-element array) if the given bus property criteria matches
     *         a defined bus property in the dynamic interface definitions provided
     *         at construction time. Otherwise, return null.
     *         If the matching property is write-only, then the index 0 element
     *         returned will be null (no getter).
     *         If the matching property is read-only, then the index 1 element
     *         returned will be null (no setter).
     */
    @Override
    public Method[] getPropertyHandler(String interfaceName, String propertyName) {
        try {
            PropertyDef propertyDef = mInfo.getProperty(interfaceName, propertyName);
            if (propertyDef != null) {
                boolean canGet = propertyDef.isReadAccess() || propertyDef.isReadWriteAccess();
                boolean canSet = propertyDef.isWriteAccess() || propertyDef.isReadWriteAccess();
                return new Method[] {
                    canGet ? getClass().getMethod("getPropertyReceived") : null,
                    canSet ? getClass().getMethod("setPropertyReceived", Object.class) : null
                };
            }
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        }
        return null; // specified bus property is not defined
    }

    /**
     * Default bus method implementation Method. Handles all bus methods
     * received by this bus object (assuming the default getMethodHandler()
     * is not overridden). BusAttachment getMessageContext() can be used
     * to determine the interface name, member name, sender, etc. for the
     * received bus method call.
     *
     * If a subclass supports receipt of bus methods, it should override
     * methodReceived() or getMethodHandler() in order to provide a
     * concrete implementation.
     *
     * @throws BusException with message 'Not implemented'.
     */
    public Object methodReceived(Object... args) throws BusException {
        throw new ErrorReplyBusException("Not implemented");
    }

    /**
     * Default bus signal implementation Method. Handles all bus signals
     * received by this bus object (assuming the default getSignalHandler()
     * is not overridden). BusAttachment getMessageContext() can be used
     * to determine the received bus signal's interface name, member name,
     * sender, etc.
     *
     * If a subclass supports receipt of bus signals, it should override
     * signalReceived() or getSignalHandler() in order to provide a
     * concrete implementation.
     *
     * @throws BusException with message 'Not implemented'.
     */
    public void signalReceived(Object... args) throws BusException {
        throw new ErrorReplyBusException("Not implemented");
    }

    /**
     * Default get-property implementation Method. Handles all get-properties
     * received by this bus object (assuming the default getPropertyHandler()
     * is not overridden). BusAttachment getMessageContext() can be used
     * to determine the interface name, member name, sender, etc. for the
     * received bus get-property call.
     *
     * If a subclass supports receipt of get-property calls, it should override
     * getPropertyReceived() or getPropertyHandler() in order to provide a
     * concrete implementation.
     *
     * @throws BusException with message 'Not implemented'.
     */
    public Object getPropertyReceived() throws BusException {
        throw new ErrorReplyBusException("Not implemented");
    }

    /**
     * Default set-property implementation Method. Handles all set-properties
     * received by this bus object (assuming the default getPropertyHandler()
     * is not overridden). BusAttachment getMessageContext() can be used
     * to determine the interface name, member name, sender, etc. for the
     * received bus set-property call.
     *
     * If a subclass supports receipt of set-property calls, it should override
     * setPropertyReceived() or getPropertyHandler() in order to provide a
     * concrete implementation.
     *
     * @throws BusException with message 'Not implemented'.
     */
    public void setPropertyReceived(Object arg) throws BusException {
        throw new ErrorReplyBusException("Not implemented");
    }
}