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
 * A version of simple interface with the same name, but different method signature.
 */
@BusInterface(name="org.alljoyn.bus.SimpleInterface")
public interface SimpleInterfaceB {

    @BusMethod(name="Ping", signature="i", replySignature="i")
    public int ping(int inInt) throws BusException;
}
