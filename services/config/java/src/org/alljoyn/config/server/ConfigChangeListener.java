/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
