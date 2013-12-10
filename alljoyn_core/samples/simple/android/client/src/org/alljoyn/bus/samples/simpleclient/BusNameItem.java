/*
 * Copyright (c) 2010 - 2011, AllSeen Alliance. All rights reserved.
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
