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

import org.alljoyn.bus.defs.InterfaceDef;

/**
 * An interface that can be implemented by bus objects that discover their
 * supported signal/method/property members at runtime.
 *
 * Any java.lang.reflect.Method instances that are returned by getMethod(),
 * getSignal(), and getProperty() must also be implemented by the bus object
 * implementing this interface.
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
     * Return the matching Method Call implementation for the given named bus method.
     *
     * @return the Method instance for the given named bus method, or null if no match found.
     */
    public Method getMethodMember( String interfaceName, String methodName, String signature );

    /**
     * Return the matching Signal implementation for the given named bus signal.
     *
     * @return the Method instance for the given named bus signal, or null if no match found.
     */
    public Method getSignalMember( String interfaceName, String signalName, String signature );

    /**
     * Return the matching Get and Set Property implementation for the given named bus property.
     *
     * @return an array containing the property's get (at index 0) and set (at index 1) Method instances.
     */
    public Method[] getPropertyMember( String interfaceName, String propertyName );

}
