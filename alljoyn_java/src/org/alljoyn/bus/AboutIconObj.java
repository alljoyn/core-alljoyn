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

import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.AboutIcon;
import org.alljoyn.bus.ifaces.Icon;

/**
 * AboutIconObj is an AllJoyn BusObject that implements the org.alljoyn.Icon
 * standard interface. Applications that provide AllJoyn IoE services to receive
 * info about the Icon of the service.
 */
public class AboutIconObj implements Icon, BusObject{
    /**
     * version of the org.alljoyn.Icon interface
     */
    public static final short VERSION = 1;

    /**
     * Create an About Icon BusObject
     *
     * @param bus BusAttachment instance associated with this BusObject
     * @param icon instance of an AboutIcon which holds the icon image
     */
    AboutIconObj(BusAttachment bus, AboutIcon icon) {
        if(bus == null) {
            throw new NullPointerException("bus must not be null");
        }
        if(icon == null) {
            throw new NullPointerException("icon must not be null");
        }
        m_icon = icon;
        bus.registerBusObject(this, Icon.OBJ_PATH);
    }

    /**
     * @return The about Icon interface version.
     */
    @Override
    public short getVersion() throws BusException {
        return VERSION;
    }

    /**
     * @return A string indicating the image MimeType
     */
    @Override
    public String getMimeType() throws BusException {
        return m_icon.getMimeType();
    }

    /**
     * @return The size of the icon content.
     */
    @Override
    public int getSize() throws BusException {
        if(m_icon.getContent() == null) {
            return 0;
        }
        return m_icon.getContent().length;
    }


    /**
     * @return The URL indicating the cloud location of the icon
     */
    @Override
    public String getUrl() throws BusException {
        return m_icon.getUrl();
    }

    /**
     * @return an array of bytes containing the binary content for the icon
     */
    @Override
    public byte[] getContent() throws BusException {
        if (m_icon.getContent() == null) {
            return new byte[0];
        }
        return m_icon.getContent();
    }

    /**
     * AboutIcon used by the AboutIcon BusObject
     */
    private AboutIcon m_icon;
}
