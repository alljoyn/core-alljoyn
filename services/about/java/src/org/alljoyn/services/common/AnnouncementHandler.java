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

package org.alljoyn.services.common;

import java.util.Map;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.about.transport.AboutTransport;
import org.alljoyn.bus.Variant;


/**
 * An interface to be implemented in order to receive Announcements
 */
public interface AnnouncementHandler
{
    /**
     * Handle a received About Announcement
     * @param serviceName the peer's AllJoyn bus name
     * @param port the peer's bound port for accepting About session connections
     * @param objectDescriptions the peer's BusInterfaces and BusObjects
     * @param aboutData a map of peer's properties.
     * @see AboutKeys
     * @see AboutTransport#Announce(short, short, BusObjectDescription[], java.util.Map)
     */
    public void onAnnouncement(String serviceName, short port, BusObjectDescription[] objectDescriptions, Map<String, Variant> aboutData);

    /**
     * Handle a loss of connectivity with this bus name
     * @param serviceName the AllJoyn bus name of the lost peer
     */
    public void onDeviceLost(String serviceName);
}
