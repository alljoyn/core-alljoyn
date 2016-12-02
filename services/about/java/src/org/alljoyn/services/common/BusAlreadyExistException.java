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
 * An exception is thrown when trying to set the BusAttachment of a service more than once
 * @deprecated
 */
@Deprecated
public class BusAlreadyExistException extends Exception
{
    private static final long serialVersionUID = -9094758961113991316L;

    @Deprecated
    BusAlreadyExistException(String txt)
    {
        super(txt);
    }
}