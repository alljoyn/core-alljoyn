/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 *
 */
package org.alljoyn.bus.samples.simpleclient;

public class BusNameItem {

    public BusNameItem(String busName, boolean isFound)
	{
        this.busName = busName;
        this.sessionId = 0;
        this.isFound = isFound;
        uniqueId = (long) busName.hashCode();
    }

    public String getBusName() {
        return busName;
    }

    public boolean isConnected() {
        return (sessionId != 0);
    }

    public int getSessionId() {
        return sessionId;
    }

    public void setSessionId(int sessionId) {
        this.sessionId = sessionId;
    }

    public long getId() {
        return uniqueId;
    }

    public boolean isFound() {
    	return isFound;
    }
    
    public void setIsFound(boolean isFound) {
    	this.isFound = isFound;
    }
    
    private String busName;
    private int sessionId;
    private boolean isFound;
    private long uniqueId;
}