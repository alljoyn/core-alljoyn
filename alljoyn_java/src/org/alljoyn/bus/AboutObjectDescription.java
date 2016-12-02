/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

package org.alljoyn.bus;

import org.alljoyn.bus.annotation.Position;

/**
 * Descriptions of a BusObject that makes up the service.
 * Pairing a BusObject with the BusInterfaces that it implements.
 */
public class AboutObjectDescription {

    /** The object path of the BusObject */
    @Position(0) public String path;
    /**
     * An array of strings where each element of the array names an AllJoyn
     * interface that is implemented by the given BusObject.
     */
    @Position(1) public String[] interfaces;
}