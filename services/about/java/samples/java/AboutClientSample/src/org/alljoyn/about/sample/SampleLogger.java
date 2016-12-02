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

package org.alljoyn.about.sample;

import org.alljoyn.services.common.utils.GenericLogger;

/*
 * empty logger added to override the generic logger
 * This is empty because we don't want to see all the logs from the About
 * that would normally be logged by default.
 */
public class SampleLogger implements GenericLogger {

    @Override
    public void debug(String TAG, String msg) { }

    @Override
    public void info(String TAG, String msg) { }

    @Override
    public void warn(String TAG, String msg) { }

    @Override
    public void error(String TAG, String msg) { }

    @Override
    public void fatal(String TAG, String msg) { }
}