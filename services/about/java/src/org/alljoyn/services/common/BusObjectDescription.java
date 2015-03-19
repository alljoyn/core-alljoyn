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

package org.alljoyn.services.common;

import org.alljoyn.bus.annotation.Position;

/**
 * Descriptions of the BusObjects that make up the service.
 * Pairing a BusObject with the BusInterfaces that it implements.
 * @deprecated see org.alljoyn.bus.AboutObjectDescription
 */
@Deprecated
public class BusObjectDescription {

    /**
     * The object path of the BusObject 
     * @deprecated see org.alljoyn.bus.AboutObjectDescription
     */
    @Deprecated
    @Position(0) public String path;
    /**
     * An array of strings where each element of the array names an AllJoyn
     * interface that is implemented by the given BusObject.
     * @deprecated see org.alljoyn.bus.AboutObjectDescription
     */
    @Deprecated
    @Position(1) public String[] interfaces;

    /**
     * Return the BusObject path
     * @deprecated see org.alljoyn.bus.AboutObjectDescription
     * @return the BusObject path
     */
    @Deprecated
    public String getPath()
    {
        return path;
    }

    /**
     * Set the BusObject path
     * @deprecated see org.alljoyn.bus.AboutObjectDescription
     * @param path the BusObject path
     */
    @Deprecated
    public void setPath(String path)
    {
        this.path = path;
    }

    /**
     * @deprecated see org.alljoyn.bus.AboutObjectDescription
     * @return Returns the interfaces that are implemented by this BusObject
     */
    @Deprecated
    public String[] getInterfaces()
    {
        return interfaces;
    }

    /**
     * Set the interfaces that are implemented by this BusObject
     * @deprecated see org.alljoyn.bus.AboutObjectDescription
     * @param interfaces the interfaces that are implemented by this BusObject
     */
    @Deprecated
    public void setInterfaces(String[] interfaces)
    {
        this.interfaces = interfaces;
    }

    @Override
    @Deprecated
    public String toString() {

        StringBuilder s = new StringBuilder("");
        s = s.append("path: ").append(path).append(".\n");
        s.append("interfaces: ");
        for (int i = 0; i < interfaces.length; i++){
            s.append(interfaces[i]);
            if(i != interfaces.length-1)
                s.append(",");
            else
                s.append(".\n");
        }

        return s.toString();
    }
}