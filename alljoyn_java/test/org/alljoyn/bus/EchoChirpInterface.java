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
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusSignal;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.annotation.Secure;

@BusInterface(name="org.allseen.bus.EchoChirpInterface")
@Secure
public interface EchoChirpInterface {

    @BusMethod
    public String echo(String shout) throws BusException;

    @BusSignal
    public void chirp(String tweet) throws BusException;

    @BusProperty
    public int getProp1() throws BusException;
    @BusProperty
    public void setProp1(int arg) throws BusException;

    @BusProperty
    public int getProp2() throws BusException;
    @BusProperty
    public void setProp2(int arg) throws BusException;

}