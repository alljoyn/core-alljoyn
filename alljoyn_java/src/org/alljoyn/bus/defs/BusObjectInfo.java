/**
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
 */

package org.alljoyn.bus.defs;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Bus Object info used to describe a bus path and interface definitions.
 */
public class BusObjectInfo {

    final private String path;
    final private List<InterfaceDef> interfaces;


    /**
     * Constructor.
     *
     * @param path the BusObject path.
     * @param interfaces the BusObject's contained interfaces.
     */
    public BusObjectInfo( String path, List<InterfaceDef> interfaces ) {
        if (path == null) {
            throw new IllegalArgumentException("Null path");
        }
        if (interfaces == null) {
            throw new IllegalArgumentException("Null interfaces");
        }
        this.path = path;
        this.interfaces = interfaces;
    }

    /**
     * @return the BusObject path.
     */
    public String getPath() {
        return path;
    }

    /**
     * @return the BusObject's contained interface definitions.
     */
    public List<InterfaceDef> getInterfaces() {
        return interfaces;
    }

    /**
     * Compares path and then only the names of interfaces (does not do a deep compare of interfaces).
     *
     * @param o the BusObject to compare.
     * @return whether this BusObject equals the given BusObject instance.
     */
    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        BusObjectInfo that = (BusObjectInfo) o;

        if (!path.equals(that.path)) return false;
        if (interfaces.size() != that.interfaces.size()) return false;

        // Sort and test whether interfaces lists are equal (use copy to preserve original order)
        List<InterfaceDef> thisInterfaces = new ArrayList<InterfaceDef>(interfaces);
        List<InterfaceDef> thatInterfaces = new ArrayList<InterfaceDef>(that.interfaces);
        Collections.sort(thisInterfaces, new Comparator<InterfaceDef>() {
            public int compare(InterfaceDef one, InterfaceDef two){
                return one.getName().compareTo(two.getName());
            }
        });
        Collections.sort(thatInterfaces, new Comparator<InterfaceDef>() {
            public int compare(InterfaceDef one, InterfaceDef two){
                return one.getName().compareTo(two.getName());
            }
        });
        return thisInterfaces.equals(thatInterfaces);
    }

    @Override
    public int hashCode() {
        int result = path.hashCode();
        result = 31 * result + interfaces.hashCode();
        return result;
    }

}
