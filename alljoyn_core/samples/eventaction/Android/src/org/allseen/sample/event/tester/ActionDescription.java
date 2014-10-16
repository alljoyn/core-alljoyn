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
