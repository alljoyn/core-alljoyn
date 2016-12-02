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

package org.alljoyn.about.client;

import java.util.Map;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.services.common.BusObjectDescription;
import org.alljoyn.services.common.ClientBase;

/**
 * An interface for retrieval of remote IoE device's About data.
 * Encapsulates the AboutTransport BusInterface
 * @deprecated see org.alljoyn.bus.AboutProxy
 */
@Deprecated
public interface AboutClient extends ClientBase
{
    /**
     * Get the language that is used for Announcements.
     * @deprecated see org.alljoyn.bus.AboutProxy
     * @return a String representing the language. IETF language tags specified by  RFC 5646.
     * @throws BusException indicating failure getting the default language
     */
    @Deprecated
    public String getDefaultLanguage() throws BusException;

    /**
     * Get the languages that are supported by the device.
     * @deprecated see org.alljoyn.bus.AboutProxy
     * @return a String array of languages. IETF language tags specified by  RFC 5646.
     * @throws BusException indicating failure getting the list of languages supported by the device
     */
    @Deprecated
    public String[] getLanguages() throws BusException;

    /**
     * Return all the configuration fields based on the language tag.
     * @deprecated see org.alljoyn.bus.AboutProxy
     * @param languageTag IETF language tags specified by  RFC 5646
     * @return All the configuration fields based on the language tag. 
     *         If language tag is not specified (i.e. ""), fields based on device's
     *         default language are returned
     * @throws BusException indicating failure getting the AboutData for the specified language
     * @see AboutKeys
     */
    @Deprecated
    public Map <String, Object> getAbout(String languageTag) throws BusException;

    /**
     * Returns the Bus Interfaces and Bus Objects supported by the device.
     * @deprecated see org.alljoyn.bus.AboutProxy
     * @return the array of object paths and the list of all interfaces available
     *         at the given object path.
     * @throws BusException indicating failure to get the BusObjectDescriptions
     */
    @Deprecated
    public BusObjectDescription[] getBusObjectDescriptions() throws BusException;
}