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

package org.allseen.sample.event.tester;

/*
 * Extends Description to allow the assignment that a signal is designated
 * as not requiring a session.  This is filled in by parsing the
 * introspectionWithDescription xml returned.
 */
public class EventDescription extends Description {
	private boolean isSessionless;

	public boolean isSessionless() {
	    return isSessionless;
    }

	public void setSessionless(boolean isSessionless) {
	    this.isSessionless = isSessionless;
	}
	
	public String toString() {
		String ret = "";
		ret += this.getDescription()+",";
		ret += this.getSessionName()+",";
		ret += this.getPath()+",";
		ret += this.getIface()+",";
		ret += this.getMemberName()+",";
		ret += this.getSignature()+",";
		ret += this.isSessionless() ? "1" : "0";
		
		return ret;
	}
	
	public void createFromString(String s) {
		String elements[] = s.split("\\,");
		this.setDescription(elements[0]);
		this.setSessionName(elements[1]);
		this.setPath(elements[2]);
		this.setIface(elements[3]);
		this.setMemberName(elements[4]);
		this.setSignature(elements[5]);
		this.setSessionless(elements[6].compareTo("1") == 0);
	}
}