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

import java.util.Map;

/**
 * Interface implemented by AllJoyn users to receive About interface related
 * events
 */
public interface AboutListener {
    /**
     * Called by the bus when an announcement containing information about the
     * application and interfaces is found
     *
     * @param busName   well know name of the remote BusAttachment
     * @param version   version of the Announce signal from the remote About Object
     * @param port      SessionPort used by the announcer
     * @param objectDescriptions a list of object paths and interfaces in the announcement
     * @param aboutData a dictionary of key/value pairs of the AboutData
     */
    void announced(String busName, int version, short port, AboutObjectDescription[] objectDescriptions, Map<String, Variant> aboutData);
}