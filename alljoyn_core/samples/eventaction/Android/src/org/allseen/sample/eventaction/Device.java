/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

package org.allseen.sample.eventaction;

import java.util.LinkedList;
import java.util.List;

import org.allseen.sample.event.tester.ActionDescription;
import org.allseen.sample.event.tester.EventDescription;


public class Device {
	private int sessionId;
	private String sessionName;
	private String path;
	private String friendlyName;

	private List<ActionDescription> actions = new LinkedList<ActionDescription>();
	private List<EventDescription> events = new LinkedList<EventDescription>();

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

	public List<EventDescription> getEvents() {
		return events;
	}

	public void addEvent(EventDescription event) {
		event.setSessionName(sessionName);
		this.events.add(event);
	}
	
	public List<ActionDescription> getActions() {
		return actions;
	}

	public void addAction(ActionDescription action) {
		//ensure correct sessionName has been set for the action
		action.setSessionName(sessionName);
		action.setFriendlyName(friendlyName);
		this.actions.add(action);
	}
}
