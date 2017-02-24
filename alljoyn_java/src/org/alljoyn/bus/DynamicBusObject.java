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

import org.alljoyn.bus.defs.InterfaceDef;

/**
 * An interface that can be implemented by bus objects that discover their
 * supported signal/method/property members at runtime.
 *
 * Any java.lang.reflect.Method instances that are returned by getMethodHandler(),
 * getSignalHandler(), and getPropertyHandler() must also be implemented by the bus
 * object implementing this interface.
 */
public interface DynamicBusObject extends BusObject {

    /**
     * @return the bus object's path.
     */
    public String getPath();

    /**
     * @return the bus object's interface definitions.
     */
    public List<InterfaceDef> getInterfaces();

    /**
     * Return the Method implementation of the bus method handler that matches
     * the given interface name and method name.
     *
     * Note: getMethodHandler() is called by AllJoyn when this bus object is being
     * registered with the bus attachment. If null is returned at that time, the
     * bus object registration will fail with Status.BUS_CANNOT_ADD_HANDLER.
     * However, if the bus object has no bus methods defined in its list of
     * interface definitions, then this operation can safely be stubbed out
     * with a no-op implementation that returns null.
     *
     * @param interfaceName the name of the bus interface
     * @param methodName the name of the bus method
     * @param signature the input signature of the bus method
     * @return the Method instance of the implementation handler for the named
     *         bus method, or null if no such bus method is defined in the bus
     *         object's list of interface definitions.
     */
    public Method getMethodHandler(String interfaceName, String methodName, String signature);

    /**
     * Return the Method implementation of the bus signal handler that matches
     * the given interface name and signal name.
     *
     * Note: getSignalHandler() is called by AllJoyn when this bus object is being
     * registered with the bus attachment. If null is returned at that time, the
     * bus object registration will fail with Status.BUS_CANNOT_ADD_HANDLER.
     * However, if the bus object has no bus signals defined in its list of
     * interface definitions, then this operation can safely be stubbed out
     * with a no-op implementation that returns null.
     *
     * @param interfaceName the name of the bus interface
     * @param signalName the name of the bus signal
     * @param signature the input signature of the bus signal
     * @return the Method instance of the implementation handler for the named
     *         bus signal, or null if no such bus signal is defined in the bus
     *         object's list of interface definitions.
     */
    public Method getSignalHandler(String interfaceName, String signalName, String signature);

    /**
     * Return the get and set Method implementation of the bus property handlers
     * that match the given interface name and property name.
     *
     * Note: getPropertyHandler() is called by AllJoyn when this bus object is being
     * registered with the bus attachment. If null is returned at that time, the
     * bus object registration will fail with Status.BUS_CANNOT_ADD_HANDLER.
     * However, if the bus object has no bus properties defined in its list of
     * interface definitions, then this operation can safely be stubbed out
     * with a no-op implementation that returns null.
     *
     * Note: The set Method implementation must be defined as a void return, otherwise
     * the bus object registration will fail with Status.BUS_CANNOT_ADD_HANDLER.
     *
     * @param interfaceName the name of the bus interface
     * @param propertyName the name of the bus property
     * @return a two-element array containing the Method instance of the get and
     *         set implementation handlers for the named bus property, or null if
     *         no such bus property is defined in the bus object's list of interface
     *         definitions. The get Method is at index 0 and the set Method is at
     *         index 1 of the array. If the property is write-only, then the index 0
     *         element will be null. If the property is read-only, then the index 1
     *         element will be null. If the property is read-write, then both elements
     *         of the array must be populated with non-null Method instances.
     */
    public Method[] getPropertyHandler(String interfaceName, String propertyName);

}