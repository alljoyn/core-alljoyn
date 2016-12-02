/**
 * This is an Android sample on how to use the AllJoyn properties.
 * This is the Bus Interface description.  It contains two set properties.
 * and two get properties.
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
package org.alljoyn.bus.samples.properties_client;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusProperty;

/*
 * BusInterface with the well-known name org.alljoyn.bus.samples.properties.
 */
@BusInterface (name = "org.alljoyn.bus.samples.properties")
public interface PropertiesInterface {
    /*
     * The BusProperty annotation signifies that this function should be used as part of the AllJoyn
     * interface. A BusProperty is always a method that starts with get or set.  The set method
     * always takes a single value.  while the get method always returns a single value. The single
     * value can be a complex data type such as an array or an Object.
     *
     * All methods that use the BusProperty annotation can throw a BusException and should indicate
     * this fact.
     */
    @BusProperty
    public String getBackGroundColor()throws BusException;

    @BusProperty
    public void setBackGroundColor(String color) throws BusException;

    @BusProperty
    public int getTextSize() throws BusException;

    @BusProperty
    public void setTextSize(int size) throws BusException;
}