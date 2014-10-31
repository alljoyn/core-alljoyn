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
package org.alljoyn.bus;

import java.util.Map;

import org.alljoyn.bus.ifaces.About;

/**
 * Get proxy access to the org.alljoyn.About interface.
 *
 * This class enables the user to interact with the remote About BusObject
 */
public class AboutProxy {
    /**
     * AboutProxy constructor
     *
     * @param bus the BusAttachment that owns this ProxyBusObject
     * @param busName the unique or well-known name of the remote AllJoyn BusAttachment
     * @param sessionId the session ID this ProxyBusObject will use
     */
    public AboutProxy(BusAttachment bus, String busName, int sessionId) {
        ProxyBusObject aboutProxy = bus.getProxyBusObject(busName, About.OBJ_PATH, sessionId, new Class<?>[] {About.class});
        proxy = aboutProxy.getInterface(About.class);
    }

    /**
     * Get a list of object paths and the interfaces that are being announced
     *
     * @return
     * An array of AboutObjectDescriptions. Each AboutObjectDescription contains
     * the path of a remote BusObject and a list of interfaces contained at that
     * path.
     * @throws BusException
     *   throws an exception indicating failure to make the remote method call
     */
    public AboutObjectDescription[] getObjectDescription() throws BusException {
        return proxy.getObjectDescription();
    }

    /**
     * Get all of the AboutData from the remote BusAttachment. Not all AboutData
     * is contained in the Announced AboutData. This method can be used to get
     * all of the AboutData for a specified language.
     *
     * @param languageTag a String formated as an IETF language tag specified by RFC 5646
     * @return
     * A map containing the fields specified in the About Interface
     *
     * @throws BusException
     *   throws an exception indicating failure to make the remote method call
     */
    public Map<String, Variant> getAboutData(String languageTag) throws BusException {
        return proxy.getAboutData(languageTag);
    }

    /**
     * Get the version of the remote About interface
     * @return the version of the remote About interface
     * @throws BusException
     *   throws an exception indicating failure to make the remote method call
     */
    public short getVersion() throws BusException {
        return proxy.getVersion();
    }

    About proxy;
}
