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

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.Variant;
import org.alljoyn.config.transport.ConfigTransport;
import org.alljoyn.services.common.ClientBaseImpl;
import org.alljoyn.services.common.ServiceAvailabilityListener;
import org.alljoyn.services.common.utils.TransportUtil;

/**
 * A default implementation of the ConfigClient interface
 */ 
public class ConfigClientImpl extends ClientBaseImpl implements ConfigClient
{
	public final static String TAG = ConfigClientImpl.class.getName();
	
	public ConfigClientImpl(String deviceName, BusAttachment bus, ServiceAvailabilityListener serviceAvailabilityListener, short port)
	{
		super(deviceName, bus, serviceAvailabilityListener, ConfigTransport.OBJ_PATH, ConfigTransport.class, port);
	}

	@Override
	public short getVersion() throws BusException {
		ProxyBusObject proxyObj = getProxyObject();
		// We make calls to the methods of the AllJoyn object through one of its interfaces.
		ConfigTransport configTransport =  proxyObj.getInterface(ConfigTransport.class);
		return configTransport.getVersion();
	}

	@Override
	public Map<String, Object> getConfig(String languageTag) throws BusException
	{
		ProxyBusObject proxyObj = getProxyObject();
		// We make calls to the methods of the AllJoyn object through one of its interfaces. */
		ConfigTransport configTransport =  proxyObj.getInterface(ConfigTransport.class);
		Map<String, Variant> configMap = configTransport.GetConfigurations(languageTag);
		return TransportUtil.fromVariantMap(configMap);
	}

	@Override
	public void setConfig(Map<String, Object> config, String languageTag) throws BusException
	{
		ProxyBusObject proxyObj = getProxyObject();
		// We make calls to the methods of the AllJoyn object through one of its interfaces. */
		ConfigTransport configTransport =  proxyObj.getInterface(ConfigTransport.class);
		configTransport.UpdateConfigurations(languageTag, TransportUtil.toVariantMap(config));
	}

	@Override
	public void setPasscode(String daemonRealm, char[] newPasscode)
			throws BusException
	{
		ProxyBusObject proxyObj = getProxyObject();
		// We make calls to the methods of the AllJoyn object through one of its interfaces. */
		ConfigTransport configTransport =  proxyObj.getInterface(ConfigTransport.class);
		configTransport.SetPasscode(daemonRealm, TransportUtil.toByteArray(newPasscode));
	}

	@Override
	public void restart() throws BusException
	{
		ProxyBusObject proxyObj = getProxyObject();
		// We make calls to the methods of the AllJoyn object through one of its interfaces. 
		ConfigTransport configTransport =  proxyObj.getInterface(ConfigTransport.class);
		configTransport.Restart();
	}

	@Override
	public void factoryReset() throws BusException
	{
		ProxyBusObject proxyObj = getProxyObject();
		// We make calls to the methods of the AllJoyn object through one of its interfaces. */
		ConfigTransport configTransport =  proxyObj.getInterface(ConfigTransport.class);
		configTransport.FactoryReset();
	}

	@Override
	public void ResetConfigurations(String language, String[] fieldsToRemove)throws BusException {
		
		ProxyBusObject proxyObj = getProxyObject();
		// We make calls to the methods of the AllJoyn object through one of its interfaces. */
		ConfigTransport configTransport =  proxyObj.getInterface(ConfigTransport.class);
		configTransport.ResetConfigurations(language, fieldsToRemove);
		
	}
}
