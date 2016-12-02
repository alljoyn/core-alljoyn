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

package org.alljoyn.bus.samples.sharedks.client.a;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.Secure;

@BusInterface(name = "org.alljoyn.bus.samples.secure.SecureInterface")
/*
 * The @Secure annotation marks requests any method calls on this interface to first authenticate
 * then encrypt the data between the AllJoyn peers.
 */
@Secure
public interface SecureInterface {

    @BusMethod
    String Ping(String inStr) throws BusException;
}