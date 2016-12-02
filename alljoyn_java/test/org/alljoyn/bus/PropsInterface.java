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
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;

/**
 * PropsInterface is an example of an interface that is published onto
 * alljoyn by org.alljoyn.bus.samples.props.Service and is subscribed
 * to by org.alljoyn.bus.samples.props.Client.  The interface contains
 * two read/write properties: 'StringProp' and 'IntProp'.  The 'IntProp'
 * also notifies everyone of changes to its value via the
 * org.freedesktop.DBus.Properties.PropertiesChanged signal.
 */
@BusInterface
public interface PropsInterface {
    @BusMethod(name="Ping")
    public String ping(String str) throws BusException;
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
    @BusProperty(annotation=BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    public int getIntProp() throws BusException;

    /**
     * Set the property named 'IntProp' to the value.  Setting this property
     * will cause a change notification signal to be sent.
     *
     * @param value The new value of 'IntProp'.
     */
    @BusProperty(annotation=BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    public void setIntProp(int value) throws BusException;
}
