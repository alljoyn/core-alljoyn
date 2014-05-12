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

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.services.common.utils.GenericLogger;

/**
 * A default implementation of {@link ServiceCommon}.
 */
public abstract class ServiceCommonImpl implements ServiceCommon {
    protected String TAG = "";

    /**
     * The {@link BusAttachment} to use
     */
    private BusAttachment m_bus;

    /**
     * Port number that is advertised by the Announcement signal
     */
    protected static short m_port;

    /**
     * Logger
     */
    protected GenericLogger m_logger;

    /**
     * Gets TRUE if the service is running in the client mode
     */
    private boolean m_isClientRunning;

    /**
     * Gets TRUE if the service is running in the server mode
     */
    private boolean m_isServerRunning;

    /**
     * The daemon should advertise itself "quietly" (directly to the calling port)
     * This is to reply directly to a TC looking for a daemon
     */
    protected static final String DAEMON_NAME_PREFIX= "org.alljoyn.BusNode.";

    /**
     * The daemon should advertise itself "quietly" (directly to the calling port)
     * This is to reply directly to a TC looking for a daemon
     */
    protected static final String DAEMON_QUIET_PREFIX= "quiet@";

    /**
     * Constructor
     */
    public ServiceCommonImpl() {
    }

    /**
     * Set the {@link BusAttachment} for the object
     * @param bus
     * @throws BusAlreadyExistException
     */
    protected void setBus(BusAttachment bus) throws BusAlreadyExistException
    {
        if ( bus == null ) {
            throw new IllegalArgumentException("BusAttachment can't be NULL");
        }

        if ( !bus.isConnected() ) {
            throw new IllegalArgumentException("The received BusAttachment wasn't connected to the daemon");
        }

        if (m_bus == null || m_bus.getUniqueName().equals(bus.getUniqueName())) {
            m_bus = bus;
        }
        else {
            throw new BusAlreadyExistException("The object has been set previously with a BusAttachment");
        }
    }

    /**
     * @return The currently used {@link BusAttachment}
     */
    protected BusAttachment getBus()
    {
        return m_bus;
    }

    /**
     * Starts the service in client mode
     */
    protected void startClient()
    {
        m_isClientRunning = true;
    }

    /**
     * Starts the service in server mode
     */
    protected void startServer()
    {
        m_isServerRunning = true;
    }

    /**
     * Stop the client mode
     */
    protected void stopClient()
    {
        m_isClientRunning = false;

        if (!m_isServerRunning)
        {
            m_bus = null;
        }
    }

    /**
     * Stop the server mode
     */
    protected void stopServer()
    {
        m_isServerRunning = false;

        if (!m_isClientRunning)
        {
            m_bus = null;
        }
    }

    /**
     * @see org.alljoyn.services.common.ServiceCommon#isClientRunning()
     */
    @Override
    public boolean isClientRunning()
    {
        return m_isClientRunning;
    }

    /**
     * @see org.alljoyn.services.common.ServiceCommon#isServerRunning()
     */
    @Override
    public boolean isServerRunning()
    {
        return m_isServerRunning;
    }

    /**
     * @see org.alljoyn.services.common.ServiceCommon#setLogger(org.alljoyn.services.common.utils.GenericLogger)
     */
    @Override
    public void setLogger(GenericLogger logger)
    {
        m_logger = logger;
    }

    /**
     * @return Returns the logger that is set for this object. If the object's 
     *         logger is NULL creates and returns an instance of the {@link DefaultGenericLogger}
     */
    public GenericLogger getLogger()
    {
        if (m_logger == null)
        {
            m_logger =  new DefaultGenericLogger();
        }

        return m_logger;
    }

    /**
     * @see org.alljoyn.services.common.ServiceCommon#getBusObjectDescriptions()
     */
    @Override
    public abstract List<BusObjectDescription> getBusObjectDescriptions();

}
