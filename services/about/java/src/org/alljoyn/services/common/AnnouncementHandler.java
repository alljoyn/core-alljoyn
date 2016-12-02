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

import java.util.Map;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.about.transport.AboutTransport;
import org.alljoyn.bus.Variant;


/**
 * An interface to be implemented in order to receive Announcements
 * @deprecated org.alljoyn.bus.AboutListener
 */
@Deprecated
public interface AnnouncementHandler
{
    /**
     * Handle a received About Announcement
     * @deprecated org.alljoyn.bus.AboutListener
     * @param serviceName the peer's AllJoyn bus name
     * @param port the peer's bound port for accepting About session connections
     * @param objectDescriptions the peer's BusInterfaces and BusObjects
     * @param aboutData a map of peer's properties.
     * @see AboutKeys
     * @see AboutTransport#Announce(short, short, BusObjectDescription[], java.util.Map)
     */
    @Deprecated
    public void onAnnouncement(String serviceName, short port, BusObjectDescription[] objectDescriptions, Map<String, Variant> aboutData);

    /**
     * Handle a loss of connectivity with this bus name
     * @deprecated
     * @param serviceName the AllJoyn bus name of the lost peer
     */
    @Deprecated
    public void onDeviceLost(String serviceName);
}