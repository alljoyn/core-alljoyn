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

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Status;

/**
 * A base class for service clients.
 * @deprecated
 */
@Deprecated
public interface ClientBase
{
    /**
     * Stop the AllJoyn session with the device.
     * @deprecated
     */
    @Deprecated
    public void disconnect();

    /**
     * Start an AllJoyn session with the device.
     * @deprecated
     * @return AllJoyn Bus Status
     */
    @Deprecated
    public Status connect();

    /**
     * Is there an open session with the device.
     * @deprecated
     * @return true if there is a session with the device.
     */
    @Deprecated
    public boolean isConnected();

    /**
     * Interface version
     * @deprecated
     * @return Interface version
     * @throws BusException indicating failure obtain Version property
     */
    @Deprecated
    public short getVersion() throws BusException;

    /**
     * The peer device's bus name, as advertised by Announcements
     * @deprecated
     * @return Unique bus name
     */
    @Deprecated
    public String getPeerName();

    /**
     * The id of the open session with the peer device.
     * @deprecated
     * @return AllJoyn session id
     */
    @Deprecated
    public int getSessionId();

    /**
     * Initialize client by passing the BusAttachment
     * @deprecated
     * @param busAttachment BusAttachment associated with this ClientBase instance
     * @throws Exception error indicating failure to initialize the client
     */
    @Deprecated
    void initBus(BusAttachment busAttachment) throws Exception;

}