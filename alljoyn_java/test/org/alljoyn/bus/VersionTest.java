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

import junit.framework.TestCase;

import org.alljoyn.bus.Version;

public class VersionTest  extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    // Since the version can change there are only a few things we can check
    // without creating complex parsing code.
    // 1. check that the code does not crash when the method is called
    // 2. make sure the value returned is a reasonable. If its a string make
    //    sure its not empty if its a number make sure its not negative.
    public void testGetVersion() {
        assertFalse(Version.get().isEmpty());
    }
    
    public void testGetBuildInfo() {
        assertFalse(Version.getBuildInfo().isEmpty());
    }
    
    public void testGetNumericVersion() {
        assertTrue(Version.getNumeric() >= 0);
    }
}