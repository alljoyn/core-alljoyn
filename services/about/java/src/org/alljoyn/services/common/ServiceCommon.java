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

import java.util.List;

import org.alljoyn.about.transport.AboutTransport;
import org.alljoyn.services.common.utils.GenericLogger;

/**
 * A base class for Services
 * @deprecated
 */
@Deprecated
public interface ServiceCommon
{
    /**
     * The list of BusObjects that are registered for this service, and the
     * BusInterfaces that they implement.
     * @deprecated
     * @return the list of BusObjectDescription
     * @see AboutTransport#Announce(short, short, BusObjectDescription[], java.util.Map)
     */
    @Deprecated
    public List<BusObjectDescription> getBusObjectDescriptions();

    /**
     * Set the platform specific logger
     * @deprecated
     * @param logger a Java logger utility
     */
    @Deprecated
    void setLogger(GenericLogger logger);

    /**
     * Is the service running in a client mode. Note: a service can run in both
     * modes simultaneously
     * @deprecated
     * @return true if the service is running in a client mode
     */
    @Deprecated
    boolean isClientRunning();

    /**
     * Is the service running in a server mode. Note: a service can run in both
     * modes simultaneously
     * @deprecated
     * @return true if the service is running in a server mode
     */
    @Deprecated
    boolean isServerRunning();

}