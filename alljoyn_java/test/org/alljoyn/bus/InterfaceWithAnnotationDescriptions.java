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

import org.alljoyn.bus.annotation.BusAnnotation;
import org.alljoyn.bus.annotation.BusAnnotations;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;

/**
 * This bus interface is an example of an interface that contains
 * interface/method/property "DocString" annotation descriptions.
 */
@BusInterface
@BusAnnotations({
 @BusAnnotation(name="org.alljoyn.Bus.DocString.en", value="English interface description"),
 @BusAnnotation(name="org.alljoyn.Bus.DocString.fr", value="French interface description") })
public interface InterfaceWithAnnotationDescriptions {

    @BusMethod(name="Ping")
    @BusAnnotations({
     @BusAnnotation(name="org.alljoyn.Bus.DocString.en", value="English member description"),
     @BusAnnotation(name="org.alljoyn.Bus.DocString.fr", value="French member description") })
    public String ping(String str) throws BusException;

    @BusProperty
    @BusAnnotations({
     @BusAnnotation(name="org.alljoyn.Bus.DocString.en", value="English property description"),
     @BusAnnotation(name="org.alljoyn.Bus.DocString.fr", value="French property description") })
    public String getStringProp() throws BusException;

    @BusProperty
    public void setStringProp(String value) throws BusException;

}