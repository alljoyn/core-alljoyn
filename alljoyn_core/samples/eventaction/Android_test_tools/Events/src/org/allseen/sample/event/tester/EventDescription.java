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
}