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

package org.alljoyn.about;

/**
 * The exception is thrown when an internal error has occurred in the {@link AboutService}
 * @deprecated
 */
@Deprecated
public class AboutServiceException extends RuntimeException {
    private static final long serialVersionUID = -6407441278632848769L;

    @Deprecated
    public AboutServiceException() {
    }

    /**
     * @deprecated
     * @param message user-defined message
     */
    @Deprecated
    public AboutServiceException(String message) {
        super(message);
    }

    /**
     * @deprecated
     * @param cause the cause of this exception
     */
    @Deprecated
    public AboutServiceException(Throwable cause) {
        super(cause);
    }

    /**
     * @deprecated
     * @param message user-defined message
     * @param cause the cause of this exception
     */
    @Deprecated
    public AboutServiceException(String message, Throwable cause) {
        super(message, cause);
    }

}