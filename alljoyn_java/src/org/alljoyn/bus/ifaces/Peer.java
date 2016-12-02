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
 * The standard org.freedesktop.DBus.Peer interface.
 */
@BusInterface(name = "org.freedesktop.DBus.Peer")
public interface Peer {

    /**
     * Send a ping message to a remote connection and get a method reply in
     * response.
     *
     * @throws BusException indicating failure to make Ping method call
     */
    @BusMethod
    void Ping() throws BusException;

    /**
     * Get the DBus machine id.
     * The machine id corresponds to the AllJoyn router's guid in this
     * implementation.
     *
     * @return guid of local AllJoyn router
     * @throws BusException indicating failure to make GetMachineId method call
     */
    @BusMethod
    String GetMachineId() throws BusException;
}