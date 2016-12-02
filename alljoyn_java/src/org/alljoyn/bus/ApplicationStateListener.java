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

import org.alljoyn.bus.common.KeyInfoNISTP256;

public interface ApplicationStateListener {

    /**
     * Handler for the org.allseen.Bus.Application's State sessionless signal.
     *
     * @param busName          unique name of the remote BusAttachment that
     *                             sent the State signal
     * @param publicKeyInfo the application public key
     * @param state the application state
     */
    void state(String busName, KeyInfoNISTP256 publicKeyInfo, PermissionConfigurator.ApplicationState state);

}