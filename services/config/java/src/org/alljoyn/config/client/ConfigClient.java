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

package org.alljoyn.config.client;

import java.util.Map;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.bus.BusException;
import org.alljoyn.services.common.ClientBase;

/**
 * An interface for retrieval and setting of remote IoE device's Config data.
 * Encapsulates the ConfigTransport BusInterface
 */
public interface ConfigClient extends ClientBase
{	
	/**
	 * Return all the updatable configuration fields based on the language tag.  If language tag is not specified (i.e. ""), configuration fields based on device's default language are returned. 
	 * @param languageTag IETF language tags specified by  RFC 5646
	 * @return All the configuration fields based on the language tag.
	 * @throws BusException. If a language tag is not supported by the device, Alljoyn error org.alljoyn.Error.LanguageNotSupported is returned.
	 * @see AboutKeys
	 */
	public Map <String, Object> getConfig(String languageTag) throws BusException;
	
	/**
	 * Update the configuration data with the given map.  Only the fields listed in the map will be updated.  
	 * Whenever there is an error, the AllJoyn error code org.alljoyn.Error.InvalidValue will be returned and the error message will contain the field name of the invalid field.
	 * @param config the configuration map. The change will only apply to keys that are inside the map
	 * @param languageTag IETF language tags specified by  RFC 5646
	 * @throws BusException. If a language tag is not supported by the device, Alljoyn error org.alljoyn.Error.LanguageNotSupported is returned. Whenever there is an error, the AllJoyn error code org.alljoyn.Error.InvalidValue will be returned
	 */
	public void setConfig(Map <String, Object> config ,String languageTag) throws BusException;
	
	/**
	 * Change the passphrase.  The passcode is used for secure interfaces.  Both fields are user-entered.
	 * @param daemonRealm passcode+DaemonRealm is used by the client to authenticate with the restricted daemon.
	 * @param newPasscode the new passcode
	 * @throws BusException. If this feature is not allowed by the OEM, the AllJoyn error code org.alljoyn.Error.FeatureNotAvailable will be returned.
	 */
	public void setPasscode(String daemonRealm, char[] newPasscode) throws BusException;
	
	/**
	 * This method restarts or power cycles the device.
	 * @throws BusException
	 */
	public void restart() throws BusException;
	
	/**
	 * Direct the device to disconnect from the personal AP, clear all previously configured data, and start the soft AP mode.  
	 * Some device may not support this feature.   
	 * @throws BusException. A FeatureNotAvailable exception will be thrown if the device doesn't support the feature
	 */
	public void factoryReset() throws BusException;
	
	/**
	 * Remove/Clear the configuration data specified by the field list.  
	 * Whenever there is an error, the AllJoyn error code org.alljoyn.Error.InvalidValue will be returned and the error message will contain the field name of the invalid field. 
	 * @param language IETF language tags specified by  RFC 5646
	 * @param fieldsToRemove the fields to remove for this language
	 * @throws BusException. If a language tag is not supported by the device, Alljoyn error org.alljoyn.Error.LanguageNotSupported is returned. Whenever there is an error, the AllJoyn error code org.alljoyn.Error.InvalidValue will be returned
	 */
	public void ResetConfigurations (String language, String[] fieldsToRemove) throws BusException;

}
