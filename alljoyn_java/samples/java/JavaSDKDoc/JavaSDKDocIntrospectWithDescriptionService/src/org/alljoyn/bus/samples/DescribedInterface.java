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
package org.alljoyn.bus.samples;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.annotation.BusSignal;
import org.alljoyn.bus.annotation.BusMethod;

@BusInterface (name = "org.samples.described",
                descriptionLanguage="en",
                description="This interface is described")
public interface DescribedInterface {

    @BusMethod (description="This is a method")
    public void myMethod();

    @BusSignal (description="This is a signal")
    public void mySignal();

    @BusProperty(description="This is a property")
    public void setMyProperty(int p);

    @BusProperty(description="This is a property")
    public int getMyProperty();
}