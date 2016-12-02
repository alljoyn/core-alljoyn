/**
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

public class NullKeyStoreListener implements KeyStoreListener {
    public byte[] getKeys() { return null; }
    public char[] getPassword() { return "password".toCharArray(); }
    public void putKeys(byte[] keys) {}
}