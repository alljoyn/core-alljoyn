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

import java.util.Vector;

import org.allseen.sample.event.tester.ActionDescription;
import org.allseen.sample.event.tester.EventDescription;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class Rules {
	private EventDescription event;
	private Vector<ActionDescription> actions = new Vector<ActionDescription>();
	
	public Rules() {}
	
	public Rules(EventDescription event) {
		this.event = event;
	}
	
	public EventDescription getEvent() {
		return event;
	}
	
	public Vector<ActionDescription> getActions() {
		return actions;
	}
	
	public void addAction(ActionDescription action) {
		actions.add(action);
	}
	
	public void removeAction(ActionDescription action) {
		actions.remove(action);
	}

	public void removeAllActions() {
		actions.clear();
	}
	
	public String toString() {
		JSONObject json = new JSONObject();
		try{
			json.put("event", event.toString());
			JSONArray array = new JSONArray();
			for(ActionDescription a : this.actions) {
				array.put(a.toString());
			}
			json.put("actions", array);
		}catch(Exception e) {
			e.printStackTrace();
		}
		return json.toString();
	}
	
	public void createFromString(String s) {
		try {
			JSONObject json = new JSONObject(s);
			event = new EventDescription();
			event.createFromString((String)json.get("event"));
			JSONArray array = json.getJSONArray("actions");
			for(int i = 0; i < array.length(); i++) {
				ActionDescription a = new ActionDescription();
				a.createFromString(array.getString(i));
				actions.add(a);
			}
		} catch (JSONException e) {
			e.printStackTrace();
		}
		
	}

}
