/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */

package org.alljoyn.bus;

import org.alljoyn.bus.BusException;

/**
 * An interface that can be implemented by bus objects to provide a generic
 * signal, method call, and get/set property operation for a bus interface.
 *
 * A generic API can be used to invoke dynamic bus interface operations, since
 * their members are discovered at runtime (e.g. via bus introspection), rather
 * than being statically implemented within class definitions.
 */
public interface GenericInterface {

    /**
     * Generic signal operation used as part of an AllJoyn bus interface.
     *
     * Note: When used in conjunction with dynamic interface definitions, the runtime input
     * arg types passed in should be based on the input arguments defined by the SignalDef
     * definition corresponding to the given signal name.
     */
    public void signal(String interfaceName, String signalName, Object... args) throws BusException; 

    /**
     * Generic method call operation used as part of an AllJoyn bus interface.
     *
     * Note: When used in conjunction with dynamic interface definitions, the runtime input
     * arg types passed in, and the runtime return type, should be based on the input and
     * output arguments defined by the MethodDef definition corresponding to the given
     * method name.
     */
    public Object methodCall(String interfaceName, String methodName, Object... args) throws BusException;

    /**
     * Generic get property operation used as part of an AllJoyn bus interface.
     *
     * Note: When used in conjunction with dynamic interface definitions, the runtime
     * return type should be based on the type field defined by the PropertyDef 
     * definition corresponding to the given property name.
     */
    public Object getProperty(String interfaceName, String propertyName) throws BusException;

    /**
     * Generic set property operation used as part of an AllJoyn bus interface.
     *
     * Note: When used in conjunction with dynamic interface definitions, the runtime input
     * arg type passed in should be based on the type field defined by the PropertyDef 
     * definition corresponding to the given property name.
     */
    public void setProperty(String interfaceName, String propertyName, Object arg) throws BusException;

}