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

import org.alljoyn.bus.BusException;
import org.alljoyn.services.common.ClientBase;

/**
 * An interface for retrieval of remote IoE device's Icon.
 * Encapsulates the IconTransport BusInterface
 * @deprecated see org.alljoyn.bus.AboutIconProxy
 */
@Deprecated
public interface AboutIconClient extends ClientBase
{
    public String getMimeType() throws BusException;
    public int getSize() throws BusException;
    public String GetUrl() throws BusException;
    public byte[] GetContent() throws BusException;
}