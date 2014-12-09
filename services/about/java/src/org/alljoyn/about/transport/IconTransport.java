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

package org.alljoyn.about.transport;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;

/**
 * Definition of the Icon BusInterface
 */
@BusInterface (name = IconTransport.INTERFACE_NAME)
public interface IconTransport extends BusObject
{
    public static final String INTERFACE_NAME = "org.alljoyn.Icon";
    public final static String OBJ_PATH = "/About/DeviceIcon";

    /**
     * @return Interface version
     * @throws BusException indicating failure to obtain Version property
     */
    @BusProperty(signature="q")
    public short getVersion() throws BusException;

    /**
     * @return Mime type for the icon
     * @throws BusException indicating failure to obtain MimeType property
     */
    @BusProperty(signature="s")
    public String getMimeType() throws BusException;

    /**
     * @return Size of the icon
     * @throws BusException indicating failure to obtain Size property
     */
    @BusProperty(signature="u")
    public int getSize() throws BusException;

    /**
     * Returns the URL if the icon is hosted on the cloud
     * @return the URL if the icon is hosted on the cloud
     * @throws BusException indicating failure to make GetUrl method call
     */
    @BusMethod(replySignature="s")
    public String GetUrl() throws BusException;

    /**
     * Returns binary content for the icon
     * @return binary content for the icon
     * @throws BusException indicating failure to make GetContent method call
     */
    @BusMethod(replySignature="ay")
    public byte[] GetContent() throws BusException;
}
