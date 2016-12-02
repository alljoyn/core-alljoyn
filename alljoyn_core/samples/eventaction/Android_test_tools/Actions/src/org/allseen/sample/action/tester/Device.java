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

package org.allseen.sample.action.tester;

import java.util.LinkedList;
import java.util.List;


public class Device {
	private int sessionId;
	private String sessionName;
	private String path;
	private String friendlyName;

	private List<ActionDescription> actions = new LinkedList<ActionDescription>();

	public Device(String sessionName, int sessionId, String friendlyName, String path) {
		this.setSessionId(sessionId);
		this.setSessionName(sessionName);
		this.setFriendlyName(friendlyName);
		this.setPath(path);
	}

	public int getSessionId() {
		return sessionId;
	}

	public void setSessionId(int sessionId) {
		this.sessionId = sessionId;
	}

	public String getSessionName() {
		return sessionName;
	}

	public void setSessionName(String sessionName) {
		this.sessionName = sessionName;
	}

	public String getPath() {
		return path;
	}

	public void setPath(String path) {
		this.path = path;
	}

	public String getFriendlyName() {
		return friendlyName;
	}

	public void setFriendlyName(String friendlyName) {
		this.friendlyName = friendlyName;
	}

	public List<ActionDescription> getActions() {
		return actions;
	}

	public void addAction(ActionDescription action) {
		//ensure correct sessionName has been set for the action
		action.setSessionName(sessionName);
		this.actions.add(action);
	}
}