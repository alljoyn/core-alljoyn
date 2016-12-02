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

package org.alljoyn.about.transport;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;

/**
 * Definition of the Icon BusInterface
 * @deprecated see org.alljoyn.bus.ifaces.Icon
 */
@Deprecated
@BusInterface (name = IconTransport.INTERFACE_NAME)
public interface IconTransport extends BusObject
{
    public static final String INTERFACE_NAME = "org.alljoyn.Icon";
    public final static String OBJ_PATH = "/About/DeviceIcon";

    /**
     * @deprecated see org.alljoyn.bus.ifaces.Icon
     * @return Interface version
     * @throws BusException indicating failure to obtain Version property
     */
    @Deprecated
    @BusProperty(signature="q")
    public short getVersion() throws BusException;

    /**
     * @deprecated see org.alljoyn.bus.ifaces.Icon
     * @return Mime type for the icon
     * @throws BusException indicating failure to obtain MimeType property
     */
    @Deprecated
    @BusProperty(signature="s")
    public String getMimeType() throws BusException;

    /**
     * @deprecated see org.alljoyn.bus.ifaces.Icon
     * @return Size of the icon
     * @throws BusException indicating failure to obtain Size property
     */
    @Deprecated
    @BusProperty(signature="u")
    public int getSize() throws BusException;

    /**
     * Returns the URL if the icon is hosted on the cloud
     * @deprecated see org.alljoyn.bus.ifaces.Icon
     * @return the URL if the icon is hosted on the cloud
     * @throws BusException indicating failure to make GetUrl method call
     */
    @Deprecated
    @BusMethod(replySignature="s")
    public String GetUrl() throws BusException;

    /**
     * Returns binary content for the icon
     * @deprecated see org.alljoyn.bus.ifaces.Icon
     * @return binary content for the icon
     * @throws BusException indicating failure to make GetContent method call
     */
    @Deprecated
    @BusMethod(replySignature="ay")
    public byte[] GetContent() throws BusException;
}