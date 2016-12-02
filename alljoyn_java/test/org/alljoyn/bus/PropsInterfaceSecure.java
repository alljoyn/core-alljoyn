/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */

package org.alljoyn.bus;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.annotation.Secure;

/**
 * PropsInterfaceSecure is an example of a secure interface that is published
 * onto alljoyn by org.alljoyn.bus.samples.props.ServiceSecure and is subscribed
 * to by org.alljoyn.bus.samples.props.Client.  The interface
 * contains two read/write properties: 'StringProp' and 'IntProp'.
 */
@BusInterface
@Secure
public interface PropsInterfaceSecure {
    /**
     * Get the property named 'StringProp'.
     *
     * @return The property value.
     */
    @BusProperty
    public String getStringProp() throws BusException;

    /**
     * Set the property named 'StringProp' to the value.
     *
     * @param value The new value of 'StringProp'.
     */
    @BusProperty
    public void setStringProp(String value) throws BusException;

    /**
     * Get the property named 'IntProp'.
     *
     * @return The property value.
     */
    @BusProperty
    public int getIntProp() throws BusException;

    /**
     * Set the property named 'IntProp' to the value.
     *
     * @param value The new value of 'IntProp'.
     */
    @BusProperty
    public void setIntProp(int value) throws BusException;
}
