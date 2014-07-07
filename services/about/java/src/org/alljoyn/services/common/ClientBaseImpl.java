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

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

public abstract class ClientBaseImpl implements ClientBase
{
    public final static String TAG = ClientBaseImpl.class.getName();

    /**
     * Bus Attachment
     */
    protected BusAttachment m_bus;

    /**
     * The PeerName to connect
     */
    protected String m_peerName;

    /**
     * The id of the established session with the remote device
     */
    protected Integer m_sessionId;

    /**
     * The secure interface pin code
     */
    protected char[] m_pinCode; // default is null !!!

    /**
     * Event listener for session related events
     */
    protected ServiceAvailabilityListener m_serviceAvailabilityListener;

    /**
     * Connection port number
     */
    protected short m_port;

    /**
     * Reference to the {@link PropertyStore}
     */
    protected PropertyStore m_propertyStore;

    /**
     * The name of the remote object
     */
    protected String m_objectPath;

    /**
     * The reflection of the AllJoyn {@link BusObject} interfaces that the remote object implements
     */
    protected Class<?>[] m_interfaceClassArr;

    /**
     * Whether this client object has a connected session to the remote service
     */
    protected boolean m_isConnected = false;

    /**
     * Constructor
     * @param peerName The PeerName to connect
     * @param bus {@link BusAttachment}
     * @param serviceAvailabilityListener Listener to receive session related events
     * @param objectPath The name of the remote object
     * @param interfaceClass The reflection of the AllJoyn {@link BusObject}
     *                       interface that the remote object implements
     * @param port port number to connect
     * @deprecated
     */
    @Deprecated
    public ClientBaseImpl( String peerName,
            BusAttachment bus,
            ServiceAvailabilityListener serviceAvailabilityListener,
            String objectPath,
            Class<?> interfaceClass,
            short port)
    {
        this(peerName, bus, serviceAvailabilityListener, objectPath, port);

        if ( interfaceClass != null ) {
            this.m_interfaceClassArr = new Class<?>[]{interfaceClass};
        }
        else {
            throw new IllegalArgumentException("InterfaceClass can't be null");
        }
    }//Constructor

    /**
     * Constructor
     * @param peerName The PeerName to connect
     * @param bus {@link BusAttachment}
     * @param serviceAvailabilityListener Listener to receive session related events
     * @param objectPath The name of the remote object
     * @param interfaceClassArr The reflection of the AllJoyn {@link BusObject}
     *                          interfaces that the remote object implements.
     * The array is used in {@link ProxyBusObject} creation
     * @param port port number to connect
     */
    public ClientBaseImpl( String peerName,
            BusAttachment bus,
            ServiceAvailabilityListener serviceAvailabilityListener,
            String objectPath,
            Class<?>[] interfaceClassArr,
            short port) {

        this(peerName, bus, serviceAvailabilityListener, objectPath, port);

        if ( interfaceClassArr == null || interfaceClassArr.length == 0 ) {
            throw new IllegalArgumentException("InterfaceClass array can't be empty");
        }
        else {
            this.m_interfaceClassArr = interfaceClassArr;
        }
    }//Constructor


    /**
     * Internal Constructor
     * @param peerName The PeerName to connect
     * @param bus {@link BusAttachment}
     * @param serviceAvailabilityListener Listener to receive session related events
     * @param objectPath The name of the remote object
     * @param port port number to connect
     */
    private ClientBaseImpl( String peerName,
            BusAttachment bus,
            ServiceAvailabilityListener serviceAvailabilityListener,
            String objectPath,
            short port) {

        if ( peerName != null ) {
            this.m_peerName = peerName;
        } else {
            throw new IllegalArgumentException("PeerName can't be null");
        }

        if ( !setBusAttachment(bus) ) {
            throw new IllegalArgumentException("Illegal BusAttachment received");
        }

        if ( objectPath != null) {
            this.m_objectPath = objectPath;
        }
        else {
            throw new IllegalArgumentException("ObjectPath can't be null");
        }

        this.m_port = port;
        m_serviceAvailabilityListener = serviceAvailabilityListener;
    }//Constructor

    /**
     * @see org.alljoyn.services.common.ClientBase#initBus(org.alljoyn.bus.BusAttachment)
     */
    @Override
    public void initBus(BusAttachment busAttachment) throws Exception {
        if ( !setBusAttachment(busAttachment) ) {
            throw new Exception("Illegal BusAttachment received");
        }
    }

    /**
     * @see org.alljoyn.services.common.ClientBase#getVersion()
     */
    @Override
    public abstract short getVersion() throws BusException;

    /**
     * Create and return {@link ProxyBusObject} of the remote object
     * @return {@link ProxyBusObject}
     * @throws BusException
     */
    protected ProxyBusObject getProxyObject() throws BusException
    {
        if (!m_isConnected) {
            throw new BusException("Session is not connected, need to check isConnected, and reconnect.");
        }

        ProxyBusObject m_proxyBusObj = m_bus.getProxyBusObject( getPeerName(),
                getObjectPath(),
                getSessionId(),
                getObjClassArr());
        return m_proxyBusObj;
    }

    /**
     * @see org.alljoyn.services.common.ClientBase#getPeerName()
     */
    @Override
    public String getPeerName()
    {
        return m_peerName;
    }

    /**
     * @see org.alljoyn.services.common.ClientBase#getSessionId()
     */
    @Override
    public int getSessionId()
    {
        return m_sessionId;
    }

    /**
     * The function provides the object with the sessionId of the previously established session <br>
     * This is instead of calling the connect method to establish a new session. <br>
     * Providing sessionId of NULL changes the internal object state to be disconnected,
     * otherwise the state of the object is set to be connected.
     *
     * @param sessionId The session id to be set to the object
     */
    public void setSessionId(Integer sessionId) {
        if ( sessionId == null ) {
            m_isConnected = false;
        }
        else {
            m_isConnected = true;
        }

        this.m_sessionId = sessionId;
    }//setSessionId

    /**
     * @return The name of the remote object
     */
    public String getObjectPath()
    {
        return m_objectPath;
    }

    /**
     * @deprecated
     * @return The reflection of the AllJoyn {@link BusObject} interface that the remote object implements. <br>
     * The Class object is retrieved from an internal interface class array. <br>
     * To receive the whole array is better to use the getObjClassArr() method
     */
    @Deprecated
    public Class<?> getObjectClass()
    {
        if ( m_interfaceClassArr == null || m_interfaceClassArr.length == 0 ) {
            return null;
        }

        return m_interfaceClassArr[0];
    }//getObjectClass

    /**
     * @return Array of the AllJoyn {@link BusObject} interfaces classes that the remote object implements
     */
    public Class<?>[] getObjClassArr()
    {
        return m_interfaceClassArr;
    }//getObjClassArr

    /**
     * @see org.alljoyn.services.common.ClientBase#disconnect()
     */
    @Override
    public void disconnect()
    {
        if (m_isConnected)
        {
            Status status = m_bus.leaveSession(getSessionId());
            if (status == Status.OK)
            {
                m_isConnected = false;
            }
        }
    }

    /**
     * @see org.alljoyn.services.common.ClientBase#connect()
     */
    @Override
    public Status connect() {

        SessionOpts sessionOpts        = createSessionOpts();
        Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

        Status status = m_bus.joinSession(getPeerName(), m_port, sessionId, sessionOpts, new SessionListener()
        {
            @Override
            public void sessionLost(int sessionId, int reason)
            {
                if (getSessionId() == sessionId)
                {
                    m_isConnected = false;
                    connectionLost();
                }
            }

            @Override
            public void sessionMemberAdded(int sessionId, String uniqueName)
            {
            }

            public void sessionMemberRemoved(int sessionId, String uniqueName)
            {
            }

        });

        if (Status.OK == status || Status.ALLJOYN_JOINSESSION_REPLY_ALREADY_JOINED == status)
        {
            this.m_isConnected = true;
            this.m_sessionId   = sessionId.value;
        }
        else{
            this.m_isConnected = false;
        }

        return status;
    }

    /**
     * The method is called by the {@link SessionListener#sessionLost(int)}
     */
    public void connectionLost()
    {
        m_isConnected = false;
        if (m_serviceAvailabilityListener != null)
        {
            m_serviceAvailabilityListener.connectionLost();
        }
    }

    /**
     * @see org.alljoyn.services.common.ClientBase#isConnected()
     */
    @Override
    public boolean isConnected(){
        return m_isConnected;
    }

    //=======================================//

    /**
     * Creates the {@link SessionOpts} object
     * @return the opts
     */
    protected SessionOpts createSessionOpts() {
        SessionOpts sessionOpts   = new SessionOpts();
        sessionOpts.traffic       = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint  = false;
        sessionOpts.proximity     = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports    = SessionOpts.TRANSPORT_ANY;
        return sessionOpts;
    }//createSessionOpts

    /**
     * Set the {@link BusAttachment} object to the instance,
     * if the received {@link BusAttachment} is legal.<br>
     *      The {@link BusAttachment} is legal if it's Not NULL, connected to a daemon; <br>
     *      The current object haven't received a {@link BusAttachment} previously or <br>
     *      the previously received {@link BusAttachment} has the same unique name
     *      as the passed in {@link BusAttachment}
     * @param bus The {@link BusAttachment} to set
     * @return TRUE if the {@link BusAttachment} was set successful otherwise FALSE
     */
    private boolean setBusAttachment(BusAttachment bus) {

        if ( bus == null ) {
            return false;
        }

        if ( !bus.isConnected() ) {
            return false;
        }

        if ( m_bus == null || m_bus.getUniqueName().equals(bus.getUniqueName()) ) {
            this.m_bus = bus;
            return true;
        }
        else {
            return false;
        }
    }//isLegalBusAttachment
}
