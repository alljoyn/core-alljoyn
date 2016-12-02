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

import java.util.Map;

import org.alljoyn.bus.Variant;

/**
 * A listener to configuration changes that were triggered by remote clients. 
 */
public interface ConfigChangeListener
{
	/**	
	 * A callback that informs the listener that the configuration has changed for a specific language
	 * @param newConfiguration
	 * @param languageTag the language for which fields were updated
	 */
	public void onConfigChanged(Map<String, Variant> newConfiguration, String languageTag);
	
	/**
	 * A callback that informs the listener that some configuration fields were removed for a specific language
	 * @param language the language for which fields were removed
	 * @param fieldNames the fields that were removed
	 */
	public void onResetConfiguration(String language, String[] fieldNames);
}