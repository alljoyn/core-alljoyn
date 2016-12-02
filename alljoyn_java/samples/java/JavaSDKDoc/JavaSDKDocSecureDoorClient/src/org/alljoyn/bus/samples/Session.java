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
package org.alljoyn.bus.samples;

import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;

public class Session {
    private Mutable.IntegerValue sessionId;
    private Door doorProxy;

    public Session(Mutable.IntegerValue sessionId, Door door) {
        this.sessionId = sessionId;
        doorProxy = door;
    }

    public Mutable.IntegerValue getSessionId() {
        return sessionId;
    }

    public Door getDoorInterface() {
        return doorProxy;
    }
}