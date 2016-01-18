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

package org.alljoyn.config;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.config.client.ConfigClient;
import org.alljoyn.config.server.ConfigChangeListener;
import org.alljoyn.config.server.FactoryResetHandler;
import org.alljoyn.config.server.PassphraseChangedListener;
import org.alljoyn.config.server.RestartHandler;
import org.alljoyn.config.server.SetPasswordHandler;
import org.alljoyn.services.common.PropertyStore;
import org.alljoyn.services.common.ServiceAvailabilityListener;
import org.alljoyn.services.common.ServiceCommon;

/**
 * An interface for both Config client (consumer) and server (producer).
 * An application may want to implement both, but still use one bus, so for convenience both functionalities are encapsulated here.
 */
public interface ConfigService extends ServiceCommon
{
	/**
	 * The Config protocol version.
	 */
	public static final int PROTOCOL_VERSION = 1;

	/**
	 * Start server mode.  The application creates the BusAttachment
	 * @param configDataStore a map of device/application data values.
	 * @param configChangeListener listener to configuration changes coming from remote client peers.
	 * @param restartHandler handler for restart requests coming from remote client peers.
	 * @param factoryResetHandler handler for factory reset requests coming from remote client peers.
	 * @param passphraseChangeListener listener to password changes coming from remote client peers.
	 * @param bus the AllJoyn bus attachment.
	 * @throws Exception
	 * @see AboutKeys
	 */
    public void startConfigServer(ConfigDataStore configDataStore, ConfigChangeListener configChangeListener, RestartHandler restartHandler, FactoryResetHandler factoryResetHandler,
            PassphraseChangedListener passphraseChangeListener, BusAttachment bus) throws Exception;

	/**
	 * Start server mode.  The application creates the BusAttachment
	 * @param propertyStore a map of device/application properties.
	 * @param configChangeListener listener to configuration changes coming from remote client peers.
	 * @param restartHandler handler for restart requests coming from remote client peers.
	 * @param factoryResetHandler handler for factory reset requests coming from remote client peers.
	 * @param passphraseChangeListener listener to password changes coming from remote client peers.
	 * @param bus the AllJoyn bus attachment.
	 * @throws Exception
	 * @see AboutKeys
	 * @deprecated use {@link startConfigServer(AboutDataListener, ConfigChangeListener, RestartHandler, FactoryResetHandler, PassphraseChangedListener, BusAttachment) startConfigServer} instead
	 */
    @Deprecated
	public void startConfigServer(PropertyStore propertyStore, ConfigChangeListener configChangeListener, RestartHandler restartHandler,
			FactoryResetHandler factoryResetHandler, PassphraseChangedListener passphraseChangeListener, BusAttachment bus) throws Exception;

	/**
	 * Stop server mode.
	 * @throws Exception
	 */
	public void stopConfigServer() throws Exception;

	/**
	 * Start client mode.  The application creates the BusAttachment
	 * @param bus the AllJoyn bus attachment
	 * @throws Exception
	 */
	public void startConfigClient(BusAttachment bus) throws Exception;
	
	/**
	 * Create an Config client for a peer.
	 * @param deviceName the remote device
	 * @param serviceAvailabilityListener listener for connection loss
	 * @param port the peer's bound port of the About server
	 * @return ConfigClient for running a session with the peer
	 * @throws Exception
	 */
	public ConfigClient createFeatureConfigClient(String deviceName, ServiceAvailabilityListener serviceAvailabilityListener, short port) throws Exception;
	
	/**
	 * Stop client mode. Disconnect all sessions.
	 * @throws Exception
	 */
	public void stopConfigClient() throws Exception;
	
	/**
	 * Handler of password change requests coming from remote peer clients.
	 * @param setPasswordHandler the application
	 */
	public void setSetPasswordHandler(SetPasswordHandler setPasswordHandler);
}
