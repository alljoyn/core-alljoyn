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
 * Place holder for the case where an Action needs to have its own set of extended values.
 * Allows ability to store an array for Descriptions and use instanceOf to determine type.
 */
public class ActionDescription extends Description {
	private String friendlyName;

	public String getFriendlyName() {
		return friendlyName;
	}

	public void setFriendlyName(String friendlyName) {
		this.friendlyName = friendlyName;
	}
	
	public String toString() {
		String ret = "";
		ret += this.getDescription()+",";
		ret += this.getSessionName()+",";
		ret += this.getPath()+",";
		ret += this.getIface()+",";
		ret += this.getMemberName()+",";
		ret += this.getSignature()+",";
		ret += this.getFriendlyName();
		
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
		this.setFriendlyName(elements[6]);
	}
}