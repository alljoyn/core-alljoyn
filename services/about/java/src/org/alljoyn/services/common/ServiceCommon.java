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

import java.util.List;

import org.alljoyn.about.transport.AboutTransport;
import org.alljoyn.services.common.utils.GenericLogger;

/**
 * A base class for Services
 */
public interface ServiceCommon
{
    /**
     * The list of BusObjects that are registered for this service, and the
     * BusInterfaces that they implement.
     * @return the list of BusObjectDescription
     * @see AboutTransport#Announce(short, short, BusObjectDescription[], java.util.Map)
     */
    public List<BusObjectDescription> getBusObjectDescriptions();

    /**
     * Set the platform specific logger
     * @param logger a Java logger utility
     */
    void setLogger(GenericLogger logger);

    /**
     * Is the service running in a client mode. Note: a service can run in both
     * modes simultaneously
     * @return true if the service is running in a client mode
     */
    boolean isClientRunning();

    /**
     * Is the service running in a server mode. Note: a service can run in both
     * modes simultaneously
     * @return true if the service is running in a server mode
     */
    boolean isServerRunning();

}
