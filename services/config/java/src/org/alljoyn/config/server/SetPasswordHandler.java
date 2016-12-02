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

package org.alljoyn.config.server;

/**
 * A handler of passpword change requests coming from remote peer clients.
 */
public interface SetPasswordHandler {

	/**
	 * Handler of passpword change requests coming from remote peer clients.
	 * @param peerName
	 * @param password
	 */
	public void setPassword(String peerName, char[] password);

}