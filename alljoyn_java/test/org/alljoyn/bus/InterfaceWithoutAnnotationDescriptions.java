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
 * This bus interface is similar to InterfaceWithAnnotationDescriptions,
 * except that all "DocString" BusAnnotations are omitted. These
 * description annotations will instead be added manually via the
 * unit tests.
 */
@BusInterface
public interface InterfaceWithoutAnnotationDescriptions {

    @BusMethod(name="Ping")
    public String ping(String str) throws BusException;

    @BusProperty
    public String getStringProp() throws BusException;

    @BusProperty
    public void setStringProp(String value) throws BusException;

}