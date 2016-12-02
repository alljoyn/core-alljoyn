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

package org.alljoyn.about;

import java.util.List;

import org.alljoyn.about.transport.AboutTransport;
import org.alljoyn.services.common.BusObjectDescription;

/**
 * An interface that publishes Announcements
 * @see AboutTransport#Announce(short, short, BusObjectDescription[], java.util.Map)
 * @deprecated
 */
@Deprecated
public interface Announcer
{
    /**
     * cause an Announcement to be emitted.
     * @deprecated org.alljoyn.bus.BusAttachment.announce
     */
    @Deprecated
    public void announce();

    /**
     * whether Announcements are emitted.
     * @return true if Announcements are emitted.
     * @deprecated
     */
    @Deprecated
    public boolean isAnnouncing();

    /**
     * set whether Announcements are emitted.
     * @param enable enable Announcement emitting.
     * @deprecated
     */
    @Deprecated
    public void setAnnouncing(boolean enable);

    /**
     * Any service who registered a BusObject in the common BusAttachment,
     * should save it here so that we can announce it
     * and include it in the About contents.
     * @param descriptions list of BusObjectDescriptions to be announced
     * @deprecated
     */
    @Deprecated
    public void addBusObjectAnnouncements(List<BusObjectDescription> descriptions);

    /**
     * Remove the passed BusObjectDescriptions from the Announcement
     * @param descriptions list of BusOBjectDescriptions to remove from the announcement
     * @deprecated
     */
    @Deprecated
    public void removeBusObjectAnnouncements(List<BusObjectDescription> descriptions);

}