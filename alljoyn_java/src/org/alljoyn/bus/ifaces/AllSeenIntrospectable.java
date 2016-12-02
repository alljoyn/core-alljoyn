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

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;

/**
 * The org.allseen.Introspectable interface provides introspection xml
 * that contains description elements that document the object and it's various sub-elements.
 */
@BusInterface(name = "org.allseen.Introspectable")
public interface AllSeenIntrospectable {

   /**
     * Gets the description languages that are supported for the object.
     *
     * @return array of supported language names.
     * @throws BusException indicating failure when calling GetDescriptionLanguages method
     */
    @BusMethod
    String[] GetDescriptionLanguages() throws BusException;

    /**
     * Gets the introspect with description for the object.
     * @param language to return in the description.
     * @return introspect with description.
     * @throws BusException indicating failure when calling IntrospectWithDescription method
     */
    @BusMethod
    String IntrospectWithDescription(String language) throws BusException;
}