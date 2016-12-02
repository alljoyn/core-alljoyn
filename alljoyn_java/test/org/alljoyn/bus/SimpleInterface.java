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

/**
 * SimpleInterface is an example of an interface that is published
 * onto alljoyn by org.alljoyn.bus.samples.simple.Service and is subscribed
 * to by org.alljoyn.bus.samples.simple.Client.
 */
@BusInterface
public interface SimpleInterface {

    /**
     * Echo a string.
     *
     * @param inStr   The string to be echoed by the service.
     * @return  The echoed string.
     */
    @BusMethod(name="Ping", signature="s", replySignature="s")
    public String ping(String inStr) throws BusException;
}
