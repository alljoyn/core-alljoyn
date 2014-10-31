/*
 * Copyright (c) 2014 AllSeen Alliance. All rights reserved.
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
 */
package org.alljoyn.bus;

import org.alljoyn.bus.AboutIcon;
import org.alljoyn.bus.ifaces.Icon;

public class AboutIconProxy implements Icon{

    /**
     * AboutProxy constructor
     *
     * @param bus the BusAttachment that owns this ProxyBusObject
     * @param busName the unique or well-known name of the remote AllJoyn BusAttachment
     * @param sessionId the session ID this ProxyBusObject will use
     */
    public AboutIconProxy(BusAttachment bus, String busName, int sessionId) {
        ProxyBusObject aboutIconProxy = bus.getProxyBusObject(busName, Icon.OBJ_PATH, sessionId, new Class<?>[] {Icon.class});
        proxy = aboutIconProxy.getInterface(Icon.class);
    }

    /**
     * fill in an AboutIcon by making remote method calls to obtain the
     * Information from a remote AboutIcon BusObject
     *
     * @return An AboutIcon
     *
     * @throws BusException indicating there was a failure trying to obtain the
     * data needed to create an AboutIcon
     */
    public AboutIcon getAboutIcon() throws BusException {
        return new AboutIcon(getMimeType(), getUrl(), getContent());
    }

    /**
     * @return the version of the remote AboutIcon BusObject
     * @throws BusException indicating there was a failure obtaining the version
     * of the remote interface
     */
    @Override
    public short getVersion() throws BusException {
        return proxy.getVersion();
    }

    /**
     * @return the MimeType of the icon from the remote AboutIcon BusObject
     * @throws BusException indicating there was a failure obtaining the MimeType
     */
    @Override
    public String getMimeType() throws BusException {
        return proxy.getMimeType();
    }

    /**
     * @return the size of the icon content from the remote AboutIcon BusObject
     * @throws BusException indication there was a failure obtaining the content
     *         size
     */
    @Override
    public int getSize() throws BusException {
        return proxy.getSize();
    }

    /**
     * @return the URL indicating the cloud location of the icon from the remote
     *         AboutIcon BusObject
     * @throws BusException indicating there was a failure obtaining the URL
     */
    @Override
    public String getUrl() throws BusException {
        return proxy.getUrl();
    }

    /**
     * @return an array of bytes containing the binary content for the icon from
     *         the remote AboutIcon BusObject
     * @throws BusException indicating there was a failure obtaining the icon
     *         content.
     */
    @Override
    public byte[] getContent() throws BusException {
        return proxy.getContent();
    }

    /**
     * An org.alljoyn.Icon ProxyBusObject
     */
    private Icon proxy;

}
