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

package org.allseen.sample.action.tester;


/**
 * Base class to hold the information that makes up a description.
 * This stores the data that can then be used to register for a
 * Signal (Event) or a Method (Action).
 */
public class Description {
	/**
	 * Contains the description to de displayed
	 */
	private String description;
	
	/**
	 * Placeholder for the list of argument descriptions if there are any
	 * Currently this application does not take into account AllJoyn Methods
	 * or Signals that contain arguments.  It is possible but this simple sample
	 * does not take this into account.
	 */
	//private String[] argDescriptions;

	/**
	 * Name of the member which is the Signal or Method function name
	 */
	private String memberName;
	
	/**
	 * Name of the interface that this description was found
	 */
	private String iface;
	
	/**
	 * Path where the interface has been implemented
	 */
	private String path;
	
	/**
	 * The argument signature of the member
	 */
	private String signature = "";

	/**
	 * The uniqueId or wellknown name of the remote application that this description
	 * was found on.  Allows ability to connect or introspect again if needed.
	 */
	private String sessionName;

	/**
	 * Returns the string representing the description found through introspectionWithDescriptions
	 * @return
	 */
	public String getDescription() {
	    return description;
    }
	
	/**
	 * Sets the description string
	 */
	public void setDescription(String description) {
	    this.description = description;
    }

	/**
	 * Returns the string representing the path
	 * @return
	 */
	public String getPath() {
	    return path;
    }

	/**
	 * Sets the path value found through introspectionWithDescriptions
	 */
	public void setPath(String path) {
	    this.path = path;
    }

	/**
	 * Returns the string representing the interface name
	 * @return
	 */
	public String getIface() {
	    return iface;
    }

	/**
	 * Sets the interface value found through introspectionWithDescriptions
	 */
	public void setIface(String iface) {
	    this.iface = iface;
    }

	/**
	 * Returns the string representing the memberName
	 * @return
	 */
	public String getMemberName() {
	    return memberName;
    }

	/**
	 * Sets the memberName value found through introspectionWithDescriptions
	 */
	public void setMemberName(String memberName) {
	    this.memberName = memberName;
    }

	/**
	 * Returns the string representing the sessionName that will be used in a
	 * JoinSession method call or ProxyBusObject AllJoyn API's
	 * @return
	 */
	public String getSessionName() {
	    return sessionName;
    }

	/**
	 * Sets the sessionName value found through introspectionWithDescriptions
	 */
	public void setSessionName(String sessionName) {
	    this.sessionName = sessionName;
    }

	/**
	 * Returns the string representing the signature
	 * @return
	 */
	public String getSignature() {
	    return signature;
    }

	/**
	 * Sets the signature value found through introspectionWithDescriptions
	 */
	public void setSignature(String signature) {
	    this.signature = signature;
    }

	/**
	 * Adds a signature type to the signature string.  Due to introspection xml containing
	 * multiple "arg" tags in which together make up the signature.
	 */
	public void addSignature(String signature) {
		if(this.signature == null)
			this.signature = "";
		this.signature += signature;
	}
}
