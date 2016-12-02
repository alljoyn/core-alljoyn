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

/*
 * A version of simple interface with the same name, but different method count.
 */
@BusInterface(name="org.alljoyn.bus.SimpleInterface")
public interface SimpleInterfaceC {

    @BusMethod(name="Ping", signature="s", replySignature="s")
    public String ping(String inStr) throws BusException;

    @BusMethod(name="Pong", signature="s", replySignature="s")
    public String pong(String inStr) throws BusException;
}
