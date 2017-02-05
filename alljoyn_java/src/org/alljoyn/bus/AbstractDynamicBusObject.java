/**
 * Copyright AllSeen Alliance. All rights reserved.
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

import java.lang.reflect.Method;
import java.util.List;

import org.alljoyn.bus.defs.BusObjectInfo;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.MethodDef;
import org.alljoyn.bus.defs.SignalDef;
import org.alljoyn.bus.defs.PropertyDef;

/**
 * Convenience abstract DynamicBusObject that implements default no-op handler Methods,
 * allowing subclasses to only implement those bus method, signal, and property handlers
 * that they expect to receive calls on (regardless of what the full bus object definition
 * specifies).
 */
public abstract class AbstractDynamicBusObject implements DynamicBusObject  {

    private final BusObjectInfo mInfo;

    /**
     * Construct abstract dynamic bus object.
     *
     * @param objectDef the bus object's interface(s) definition.
     */
    public AbstractDynamicBusObject(BusObjectInfo objectDef) {
        mInfo = objectDef;
    }

    /**
     * Construct abstract dynamic bus object.
     *
     * @param path the BusObject path.
     * @param interfaces the BusObject's contained interfaces.
     * @throws IllegalArgumentException if interfaces argument is null.
     */
    public AbstractDynamicBusObject(String path, List<InterfaceDef> interfaces) {
        mInfo = new BusObjectInfo(path, interfaces);
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
     * Default no-op implementation for subclasses that do not define bus methods or do not
     * receive bus method calls. Subclasses must override this method if they do define AND
     * receive bus method calls.
     *
     * @return If the bus method is defined then return a no-op Method implementation. Otherwise, return null.
     */
    @Override
    public Method getMethodHandler(String interfaceName, String methodName, String signature) {
        try {
            MethodDef methodDef = mInfo.getMethod(interfaceName, methodName, signature);
            if (methodDef != null) {
                return AbstractDynamicBusObject.class.getDeclaredMethod("defaultMethod", new Class[]{Object[].class});
            }
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        }
        return null; // specified bus method is not defined
    }

    /**
     * Default no-op implementation for subclasses that do not define bus signals or do not
     * receive bus signal calls. Subclasses must override this method if they do define AND
     * receive bus signal calls.
     *
     * @return If the bus signal is defined then return a no-op Method implementation. Otherwise, return null.
     */
    @Override
    public Method getSignalHandler(String interfaceName, String signalName, String signature) {
        try {
            SignalDef signalDef = mInfo.getSignal(interfaceName, signalName, signature);
            if (signalDef != null) {
                return AbstractDynamicBusObject.class.getDeclaredMethod("defaultSignal", new Class[]{Object[].class});
            }
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        }
        return null; // specified bus signal is not defined
    }

    /**
     * Default no-op implementation for subclasses that do not define bus properties or do not
     * receive bus property calls. Subclasses must override this method if they do define AND
     * receive bus property calls.
     *
     * @return If the bus property is defined then return no-op getter/setter Method implementations (2-element array).
     *         If the bus property is not defined, then return null.
     *         If the property is write-only, then the index 0 element will be null (no getter).
     *         If the property is read-only, then the index 1 element will be null (no setter).
     */
    @Override
    public Method[] getPropertyHandler(String interfaceName, String propertyName) {
        try {
            PropertyDef propertyDef = mInfo.getProperty(interfaceName, propertyName);
            if (propertyDef != null) {
                boolean canGet = propertyDef.isReadAccess() || propertyDef.isReadWriteAccess();
                boolean canSet = propertyDef.isWriteAccess() || propertyDef.isReadWriteAccess();
                return new Method[] {
                        canGet ? AbstractDynamicBusObject.class.getDeclaredMethod("defaultGetProperty") : null,
                        canSet ? AbstractDynamicBusObject.class.getDeclaredMethod("defaultSetProperty", Object.class) : null
                };
            }
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        }
        return null; // specified bus property is not defined
    }

    /**
     * Default no-op method implementation called by AllJoyn. Do not override.
     * Subclasses may override getMethodHandler() as needed.
     */
    public Object defaultMethod(Object... args) throws BusException {
        throw new ErrorReplyBusException("Not implemented");
    }

    /**
     * Default no-op signal implementation called by AllJoyn. Do not override.
     * Subclasses may override getSignalHandler() as needed.
     */
    public void defaultSignal(Object... args) throws BusException {
        throw new ErrorReplyBusException("Not implemented");
    }

    /**
     * Default no-op get-property implementation called by AllJoyn. Do not override.
     * Subclasses may override getPropertyHandler() as needed.
     */
    public Object defaultGetProperty() throws BusException {
        throw new ErrorReplyBusException("Not implemented");
    }

    /**
     * Default no-op set-property implementation called by AllJoyn. Do not override.
     * Subclasses may override getPropertyHandler() as needed.
     */
    public void defaultSetProperty(Object arg) throws BusException {
        throw new ErrorReplyBusException("Not implemented");
    }
}
