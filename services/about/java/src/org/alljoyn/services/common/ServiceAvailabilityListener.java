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

package org.alljoyn.services.common;

/**
 * Listens to connection losses with a peer device
 * @deprecated
 */
@Deprecated
public interface ServiceAvailabilityListener
{
    /**
     * Fired when a connection to the peer device was lost.
     * @deprecated
     */
    @Deprecated
    public void connectionLost();
}