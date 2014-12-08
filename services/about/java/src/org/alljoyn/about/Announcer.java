/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

package org.alljoyn.about;

import java.util.List;

import org.alljoyn.about.transport.AboutTransport;
import org.alljoyn.services.common.BusObjectDescription;

/**
 * An interface that publishes Announcements
 * @see AboutTransport#Announce(short, short, BusObjectDescription[], java.util.Map)
 */
public interface Announcer
{
    /**
     * cause an Announcement to be emitted.
     */
    public void announce();

    /**
     * whether Announcements are emitted.
     * @return true if Announcements are emitted.
     */
    public boolean isAnnouncing();

    /**
     * set whether Announcements are emitted.
     * @param enable enable Announcement emitting.
     */
    public void setAnnouncing(boolean enable);

    /**
     * Any service who registered a BusObject in the common BusAttachment,
     * should save it here so that we can announce it
     * and include it in the About contents.
     * @param descriptions list of BusObjectDescriptions to be announced
     */
    public void addBusObjectAnnouncements(List<BusObjectDescription> descriptions);

    /**
     * Remove the passed BusObjectDescriptions from the Announcement
     * @param descriptions list of BusOBjectDescriptions to remove from the announcement
     */
    public void removeBusObjectAnnouncements(List<BusObjectDescription> descriptions);

}
