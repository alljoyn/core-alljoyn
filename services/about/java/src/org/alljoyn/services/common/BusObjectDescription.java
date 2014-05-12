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

package org.alljoyn.services.common;

import org.alljoyn.bus.annotation.Position;

/**
 * Descriptions of the BusObjects that make up the service.
 * Pairing a BusObject with the BusInterfaces that it implements.
 */
public class BusObjectDescription {

    /* The object path of the BusObject */
    @Position(0) public String path;
    /* 
     * An array of strings where each element of the array names an AllJoyn
     * interface that is implemented by the given BusObject.
     */
    @Position(1) public String[] interfaces;

    /**
     * Return the BusObject path
     * @return the BusObject path
     */
    public String getPath()
    {
        return path;
    }

    /**
     * Set the BusObject path
     * @param path the BusObject path
     */
    public void setPath(String path)
    {
        this.path = path;
    }

    /**
     * @return Returns the interfaces that are implemented by this BusObject
     */
    public String[] getInterfaces()
    {
        return interfaces;
    }

    /**
     * Set the interfaces that are implemented by this BusObject
     * @param interfaces the interfaces that are implemented by this BusObject
     */
    public void setInterfaces(String[] interfaces)
    {
        this.interfaces = interfaces;
    }

    @Override
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