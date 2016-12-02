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
import org.alljoyn.bus.annotation.BusSignal;
import org.alljoyn.bus.annotation.BusProperty;

@BusInterface
public interface AnnotationInterface {

    @BusMethod(name="DeprecatedMethod", annotation=BusMethod.ANNOTATE_DEPRECATED)
    public String deprecatedMethod(String str) throws BusException;

    @BusMethod(name="NoReplyMethod", annotation=BusMethod.ANNOTATE_NO_REPLY)
    public void noReplyMethod(String str) throws BusException;

    @BusMethod(name="DeprecatedNoReplyMethod", annotation=BusMethod.ANNOTATE_DEPRECATED | BusMethod.ANNOTATE_NO_REPLY)
    public void deprecatedNoReplyMethod(String str) throws BusException;

    @BusSignal(name="DeprecatedSignal", annotation=BusSignal.ANNOTATE_DEPRECATED)
    public void deprecatedSignal(String str) throws BusException;

    @BusProperty(annotation=BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    public String getChangeNotifyProperty() throws BusException;

    @BusProperty(annotation=BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    public void setChangeNotifyProperty(String str) throws BusException;
}
