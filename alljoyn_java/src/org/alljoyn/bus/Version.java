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

public class Version {
    /**
     * Gives the version of AllJoyn Library
     *
     * @return the version of the AllJoyn Library
     */
    public static native String get();
    /**
     * Gives build information of AllJoyn Library
     *
     * @return build information of the AllJoyn Library
     */
    public static native String getBuildInfo();
    /**
     * Gives the version of AllJoyn Library as a single number
     *
     * @return the version of the AllJoyn Library as a single number
     */
    public static native int getNumeric();
}