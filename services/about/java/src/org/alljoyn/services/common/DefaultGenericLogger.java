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

import org.alljoyn.services.common.utils.GenericLogger;

/**
 * An Android implementation of the {@link GenericLogger} class
 * @deprecated
 */
@Deprecated
public class DefaultGenericLogger implements GenericLogger {

    @Override
    @Deprecated
    public void warn(String TAG, String msg)
    {
        System.out.println(new StringBuilder(TAG).append(" Warning: ").append(msg).toString());
    }

    @Override
    @Deprecated
    public void info(String TAG, String msg)
    {
        System.out.println(new StringBuilder(TAG).append(" Info: ").append(msg).toString());
    }

    @Override
    @Deprecated
    public void fatal(String TAG, String msg)
    {
        System.out.println(new StringBuilder(TAG).append(" WTF: ").append(msg).toString());
    }

    @Override
    @Deprecated
    public void error(String TAG, String msg)
    {
        System.out.println(new StringBuilder(TAG).append(" Error: ").append(msg).toString());
    }

    @Override
    @Deprecated
    public void debug(String TAG, String msg)
    {
        System.out.println(new StringBuilder(TAG).append(" Debug: ").append(msg).toString());
    }
}