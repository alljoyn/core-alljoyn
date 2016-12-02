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

/**
 * C++ counterpart is PingListener
 * This is named AutoPingListener instead of PingListener,
 * because there is an AllJoyn Java OnPingListener.
 *
 */

public interface AutoPingListener {

    /**
     * Destination Lost: called when the destination becomes unreachable.
     * Called once.
     *
     * @param  group Pinging group name
     * @param  destination Destination that was pinged
     */
    void destinationLost(String group, String destination);

    /**
     * Destination Found: called when the destination becomes reachable.
     * Called once.
     *
     * @param  group Pinging group name
     * @param  destination Destination that was pinged
     */
    void destinationFound(String group, String destination);
}