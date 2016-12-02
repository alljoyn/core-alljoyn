/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

package org.alljoyn.services.common;

import org.alljoyn.bus.ErrorReplyBusException;

/**
 * A BusException that is thrown when a remote client uses an unsupported
 * language to get/set data.
 * @deprecated
 */
@Deprecated
public class LanguageNotSupportedException extends ErrorReplyBusException {
    private static final long serialVersionUID = -8150015063069797494L;

    /**
     * Constructor
     * @deprecated
     */
    @Deprecated
    public LanguageNotSupportedException() {
        super("org.alljoyn.Error.LanguageNotSupported", "The language specified is not supported");
    }
}