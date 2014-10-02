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
