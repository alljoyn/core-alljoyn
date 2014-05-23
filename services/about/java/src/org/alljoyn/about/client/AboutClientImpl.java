/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

public class AboutClientImpl extends ClientBaseImpl implements AboutClient
{
    public final static String TAG = AboutClientImpl.class.getName();

    public AboutClientImpl(String deviceName, BusAttachment bus, ServiceAvailabilityListener serviceAvailabilityListener, short port)
    {
        super(deviceName, bus, serviceAvailabilityListener,AboutTransport.OBJ_PATH, new Class<?>[]{AboutTransport.class}, port);
    }

    @Override
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
    public Map<String, Object> getAbout(String languageTag) throws BusException
    {
        ProxyBusObject proxyObj = getProxyObject();
        // We make calls to the methods of the AllJoyn object through one of its interfaces.
        AboutTransport aboutTransport =  proxyObj.getInterface(AboutTransport.class);
        Map<String, Variant> aboutMap = aboutTransport.GetAboutData(languageTag);
        return TransportUtil.fromVariantMap(aboutMap);
    }

    @Override
    public BusObjectDescription[] getBusObjectDescriptions() throws BusException
    {
        ProxyBusObject proxyObj = getProxyObject();
        // We make calls to the methods of the AllJoyn object through one of its interfaces.
        AboutTransport aboutTransport =  proxyObj.getInterface(AboutTransport.class);
        BusObjectDescription[] busObjectDescriptions = aboutTransport.GetObjectDescription();
        return busObjectDescriptions;
    }

    @Override
    public  short getVersion() throws BusException
    {
        ProxyBusObject proxyObj = getProxyObject();
        // We make calls to the methods of the AllJoyn object through one of its interfaces.
        AboutTransport aboutTransport =  proxyObj.getInterface(AboutTransport.class);
        return aboutTransport.getVersion();
    }

    /**
     * @see org.alljoyn.services.common.ClientBaseImpl#connect()
     */
    @Override
    public Status connect() {
        AboutServiceImpl service = (AboutServiceImpl)AboutServiceImpl.getInstance();
        return super.connect();
    }//connect
}
