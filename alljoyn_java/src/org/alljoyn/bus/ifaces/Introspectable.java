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

package org.alljoyn.bus.ifaces;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;

/**
 * The standard org.freedesktop.DBus.Introspectable interface.  This
 * interface allows bus objects to be introspected for the interfaces
 * that they implement and the child objects that they may contain.
 */
@BusInterface(name = "org.freedesktop.DBus.Introspectable")
public interface Introspectable {

    /**
     * Gets the XML introspection data for the object.
     * The schema for the DBus introspection data is described in the
     * DBus specification.
     *
     * @return XML introspection data for the object
     * @throws BusException indicating failure to make Introspect method call
     */
    @BusMethod
    String Introspect() throws BusException;
}