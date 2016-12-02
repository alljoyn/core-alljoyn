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

package org.alljoyn.about.client;

import java.util.Map;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.about.AboutServiceImpl;
import org.alljoyn.about.transport.AboutTransport;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.services.common.BusObjectDescription;
import org.alljoyn.services.common.ClientBaseImpl;
import org.alljoyn.services.common.ServiceAvailabilityListener;
import org.alljoyn.services.common.utils.TransportUtil;

@Deprecated
public class AboutClientImpl extends ClientBaseImpl implements AboutClient
{
    @Deprecated
    public final static String TAG = AboutClientImpl.class.getName();

    @Deprecated
    public AboutClientImpl(String deviceName, BusAttachment bus, ServiceAvailabilityListener serviceAvailabilityListener, short port)
    {
        super(deviceName, bus, serviceAvailabilityListener,AboutTransport.OBJ_PATH, new Class<?>[]{AboutTransport.class}, port);
    }

    @Override
    @Deprecated
    public String[] getLanguages() throws BusException
    {
        ProxyBusObject proxyObj = getProxyObject();
        // We make calls to the methods of the AllJoyn object through one of its interfaces.
        AboutTransport aboutTransport =  proxyObj.getInterface(AboutTransport.class);
        Map<String, Variant> aboutMap = aboutTransport.GetAboutData("en");
        Map<String, Object> fromVariantMap = TransportUtil.fromVariantMap(aboutMap);
        String[] languages = (String[]) fromVariantMap.get(AboutKeys.ABOUT_SUPPORTED_LANGUAGES);
        if (languages != null && languages.length > 0)
        {
            return languages;
        }
        else
        {
            String defaultLanaguage = (String) fromVariantMap.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
            return new String[]{defaultLanaguage};
        }
    }

    @Override
    @Deprecated
    public String getDefaultLanguage() throws BusException
    {
        ProxyBusObject proxyObj = getProxyObject();
        // We make calls to the methods of the AllJoyn object through one of its interfaces.
        AboutTransport aboutTransport =  proxyObj.getInterface(AboutTransport.class);
        Map<String, Variant> aboutMap = aboutTransport.GetAboutData("en");
        Map<String, Object> fromVariantMap = TransportUtil.fromVariantMap(aboutMap);
        String defaultLanaguage = (String) fromVariantMap.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
        return defaultLanaguage;
    }

    @Override
    @Deprecated
    public Map<String, Object> getAbout(String languageTag) throws BusException
    {
        ProxyBusObject proxyObj = getProxyObject();
        // We make calls to the methods of the AllJoyn object through one of its interfaces.
        AboutTransport aboutTransport =  proxyObj.getInterface(AboutTransport.class);
        Map<String, Variant> aboutMap = aboutTransport.GetAboutData(languageTag);
        return TransportUtil.fromVariantMap(aboutMap);
    }

    @Override
    @Deprecated
    public BusObjectDescription[] getBusObjectDescriptions() throws BusException
    {
        ProxyBusObject proxyObj = getProxyObject();
        // We make calls to the methods of the AllJoyn object through one of its interfaces.
        AboutTransport aboutTransport =  proxyObj.getInterface(AboutTransport.class);
        BusObjectDescription[] busObjectDescriptions = aboutTransport.GetObjectDescription();
        return busObjectDescriptions;
    }

    @Override
    @Deprecated
    public  short getVersion() throws BusException
    {
        ProxyBusObject proxyObj = getProxyObject();
        // We make calls to the methods of the AllJoyn object through one of its interfaces.
        AboutTransport aboutTransport =  proxyObj.getInterface(AboutTransport.class);
        return aboutTransport.getVersion();
    }

    /**
     * @see org.alljoyn.services.common.ClientBaseImpl#connect()
     * @deprecated
     */
    @Override
    @Deprecated
    public Status connect() {
        AboutServiceImpl service = (AboutServiceImpl)AboutServiceImpl.getInstance();
        return super.connect();
    }//connect
}