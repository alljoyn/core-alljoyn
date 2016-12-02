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

package org.alljoyn.services.common.utils;

/**
 * A generic Java logger utility, that needs to be implemented per platform
 * @deprecated
 */
@Deprecated
public interface GenericLogger {

    /**
     * Debug level message
     * @deprecated
     * @param TAG Tag to be added to the message, i.e. class that writes the message
     * @param msg message to be sent to the logger
     */
    @Deprecated
    public void debug(String TAG, String msg);

    /**
     * Info level message
     * @deprecated
     * @param TAG Tag to be added to the message, i.e. class that writes the message
     * @param msg message to be sent to the logger
     */
    @Deprecated
    public void info(String TAG, String msg);

    /**
     * Warn level message
     * @deprecated
     * @param TAG Tag to be added to the message, i.e. class that writes the message
     * @param msg message to be sent to the logger
     */
    @Deprecated
    public void warn(String TAG, String msg);

    /**
     * Error level message
     * @deprecated
     * @param TAG Tag to be added to the message, i.e. class that writes the message
     * @param msg message to be sent to the logger
     */
    @Deprecated
    public void error(String TAG, String msg);

    /**
     * Fatal level message
     * @deprecated
     * @param TAG Tag to be added to the message, i.e. class that writes the message
     * @param msg message to be sent to the logger
     */
    @Deprecated
    public void fatal(String TAG, String msg);
}