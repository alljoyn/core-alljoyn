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

/**
 * BasicProperty is an example of an interface with a bus property.
 */
@BusInterface
public interface BasicProperty {

    @BusProperty(annotation=BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    String getName() throws BusException;

    @BusProperty(annotation=BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    void setName(String value) throws BusException;

}