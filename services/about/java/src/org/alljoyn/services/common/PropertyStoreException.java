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

/**
 * An exception that is thrown by the PropertyStore when illegal arguments are given in set/get methods.
 * @see PropertyStore
 * @deprecated
 */
@Deprecated
public class PropertyStoreException extends Exception {
    private static final long serialVersionUID = -8488311339426012157L;

    /**
     * The given key is not supported
     */
    public final static int UNSUPPORTED_KEY = 0;

    /**
     * The given language is not supported
     */
    public final static int UNSUPPORTED_LANGUAGE = 1;

    /**
     * Trying to set a read-only field
     */
    public final static int ILLEGAL_ACCESS = 2;

    /**
     * Trying to set a field to an invalid
     */
    public final static int INVALID_VALUE = 3;

    private int m_reason;

    @Deprecated
    public PropertyStoreException(int reason)
    {
        m_reason = reason;
    }

    /**
     * The reason for failure
     * @deprecated
     * @return reason for failure
     */
    @Deprecated
    public int getReason()
    {
        return m_reason;
    }
}