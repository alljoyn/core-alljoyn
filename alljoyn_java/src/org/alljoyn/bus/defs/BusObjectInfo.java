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
public class BusObjectInfo extends BaseDef {

    final private List<InterfaceDef> interfaces;


    /**
     * Constructor.
     *
     * @param path the BusObject path.
     * @param interfaces the BusObject's contained interfaces.
     */
    public BusObjectInfo(String path, List<InterfaceDef> interfaces) {
        super(path);
        if (interfaces == null) {
            throw new IllegalArgumentException("Null interfaces");
        }
        this.interfaces = interfaces;
    }

    /**
     * @return the BusObject path.
     */
    public String getPath() {
        return getName();
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

        if (!getPath().equals(that.getPath())) return false;
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
        int result = getPath().hashCode();
        result = 31 * result + interfaces.hashCode();
        return result;
    }

    /** @return the definition of the matching interface, or null if not found. */
    public InterfaceDef getDefinition(String ifaceName) {
        return BusObjectInfo.getDefinition( getInterfaces(), ifaceName, null, InterfaceDef.class );
    }

    /** @return the definition of the matching interface or member, or null if not found. */
    public <T extends BaseDef> T getDefinition(String ifaceName, String memberName, Class<T> definitionType) {
        return BusObjectInfo.getDefinition( getInterfaces(), ifaceName, memberName, definitionType );
    }

    /**
     * Search the interfaces definition list for the definition instance having
     * the given interface name and member name. The type of definition returned
     * will corresponding with the given definitionType class.
     *
     * If a member name is not provided, then return the definition for the
     * given interface name (InterfaceDef). If a member name is specified,
     * then return the given interface's associated member definition (MethodDef,
     * SignalDef, or PropertyDef). If the interface and member are not found,
     * then return null.
     *
     * @param interfaces the list of interfaces to search.
     * @param ifaceName the name of the interface definition to find.
     * @param membername the name of the member definition (method, signal, property)
     *                   to find.
     * @param definitionType the type of definition to find. One of: InterfaceDef.class,
     *                       MethodDef.class, SignalDef.class, PropertyDef.class
     * @return the matching definition (instance of InterfaceDef, MethodDef, SignalDef,
     *         PropertyDef), or null if not found.
     */
    public static <T extends BaseDef> T getDefinition(
                List<InterfaceDef> interfaces, String ifaceName,
                String memberName, Class<T> definitionType) {
        if (interfaces == null || ifaceName == null) return null; // exit early

        for (InterfaceDef ifaceDef : interfaces) {
            if (ifaceDef.getName().equals(ifaceName)) {
                if (InterfaceDef.class.equals(definitionType)) {
                    return definitionType.cast(ifaceDef);
                } else if (MethodDef.class.equals(definitionType)) {
                    for (MethodDef methodDef : ifaceDef.getMethods()) {
                        if (methodDef.getName().equals(memberName)) {
                            return definitionType.cast(methodDef);
                        }
                    }
                } else if (SignalDef.class.equals(definitionType)) {
                    for (SignalDef signalDef : ifaceDef.getSignals()) {
                        if (signalDef.getName().equals(memberName)) {
                            return definitionType.cast(signalDef);
                        }
                    }
                } else if (PropertyDef.class.equals(definitionType)) {
                    for (PropertyDef propertyDef : ifaceDef.getProperties()) {
                        if (propertyDef.getName().equals(memberName)) {
                            return definitionType.cast(propertyDef);
                        }
                    }
                }
            }
        }
        return null;  // no match found
    }

}
