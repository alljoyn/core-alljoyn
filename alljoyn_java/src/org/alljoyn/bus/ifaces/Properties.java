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

package org.alljoyn.bus.ifaces;

import java.util.Map;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusSignal;

/**
 * The standard org.freedesktop.DBus.Properties interface that can be
 * implemented by bus objects to expose a generic "setter/getter" interface for
 * user-defined properties on DBus.
 */
@BusInterface(name = "org.freedesktop.DBus.Properties")
public interface Properties {

    /**
     * Gets a property that exists on a named interface of a bus object.
     *
     * @param iface the interface that the property exists on
     * @param propName the name of the property
     * @return the value of the property
     * @throws BusException if the named property doesn't exist
     */
    @BusMethod
    Variant Get(String iface, String propName) throws BusException;

    /**
     * Sets a property that exists on a named interface of a bus object.
     *
     * @param iface the interface that the property exists on
     * @param propName the name of the property
     * @param value the value for the property
     * @throws BusException if the named property doesn't exist or cannot be set
     */
    @BusMethod
    void Set(String iface, String propName, Variant value) throws BusException;

    /**
     * Gets all properties for a given interface.
     *
     * @param iface the interface
     * @return a Map of name/value associations
     * @throws BusException if request cannot be honored
     */
    @BusMethod(signature = "s", replySignature = "a{sv}")
    Map<String, Variant> GetAll(String iface) throws BusException;

    /**
     * Notifies others about changes to properties.
     *
     * @param iface the interface
     * @param changedProps a map of property names an their new values
     * @param invalidatedProps a list of property names whose values are invalidated
     * @throws BusException indicating failure sending PropertiesChanged signal
     */
    @BusSignal(signature = "sa{sv}as")
    void PropertiesChanged(String iface, Map<String, Variant> changedProps, String [] invalidatedProps) throws BusException;
}