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

package org.alljoyn.about.icon;

import org.alljoyn.about.transport.IconTransport;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.services.common.ClientBaseImpl;
import org.alljoyn.services.common.ServiceAvailabilityListener;

@Deprecated
public class AboutIconClientImpl extends ClientBaseImpl implements AboutIconClient
{
    @Deprecated
    public final static String TAG = AboutIconClientImpl.class.getName();

    @Deprecated
    public AboutIconClientImpl(String peerName, BusAttachment bus, ServiceAvailabilityListener serviceAvailabilityListener, short port)
    {
        super(peerName, bus, serviceAvailabilityListener, IconTransport.OBJ_PATH, new Class<?>[] { IconTransport.class }, port);
    }

    @Override
    @Deprecated
    public short getVersion() throws BusException {
        ProxyBusObject proxyObj = getProxyObject();
        /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
        IconTransport aboutIconTransport =  proxyObj.getInterface(IconTransport.class);
        return aboutIconTransport.getVersion();
    }

    @Override
    @Deprecated
    public String getMimeType() throws BusException {
        ProxyBusObject proxyObj = getProxyObject();
        /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
        IconTransport aboutIconTransport =  proxyObj.getInterface(IconTransport.class);
        return aboutIconTransport.getMimeType();
    }

    @Override
    @Deprecated
    public int getSize() throws BusException {
        ProxyBusObject proxyObj = getProxyObject();
        /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
        IconTransport aboutIconTransport =  proxyObj.getInterface(IconTransport.class);
        return aboutIconTransport.getSize();
    }

    @Override
    @Deprecated
    public String GetUrl() throws BusException {
        ProxyBusObject proxyObj = getProxyObject();
        /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
        IconTransport aboutIconTransport =  proxyObj.getInterface(IconTransport.class);
        return aboutIconTransport.GetUrl();
    }

    @Override
    @Deprecated
    public byte[] GetContent() throws BusException {
        ProxyBusObject proxyObj = getProxyObject();
        /* We make calls to the methods of the AllJoyn object through one of its interfaces. */
        IconTransport aboutIconTransport =  proxyObj.getInterface(IconTransport.class);
        return aboutIconTransport.GetContent();
    }


}