/**
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
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
    public InterfaceDef getInterface(String ifaceName) {
        return getInterface(getInterfaces(), ifaceName);
    }

    /**
     * Search the interfaces definition list for the interface matching the given
     * interface name.
     *
     * @param interfaces the list of interfaces to search.
     * @param ifaceName the name of the interface definition to find.
     * @return the matching instance of InterfaceDef, or null if not found.
     */
    public static InterfaceDef getInterface(List<InterfaceDef> interfaces, String ifaceName) {
        if (interfaces == null || ifaceName == null) return null;
        for (InterfaceDef ifaceDef : interfaces) {
            if (ifaceDef.getName().equals(ifaceName)) {
                return ifaceDef;
            }
        }
        return null;  // no match found
    }

    /** @return the definition of the matching method, or null if not found. */
    public MethodDef getMethod(String ifaceName, String methodName, String signature) {
        return getMethod(getInterfaces(), ifaceName, methodName, signature);
    }

    /**
     * Search the interfaces definition list for the method matching the given
     * interface name and method name. The search will be narrowed further if
     * a method signature is given.
     *
     * @param interfaces the list of interfaces to search.
     * @param ifaceName the name of the interface of interest.
     * @param methodName the name of the method definition to find.
     * @param signature the method signature of interest. Ignored if null.
     * @return the matching instance of MethodDef, or null if not found.
     */
    public static MethodDef getMethod(List<InterfaceDef> interfaces, String ifaceName,
                String methodName, String signature) {
        if (interfaces == null || ifaceName == null) return null;
        for (InterfaceDef ifaceDef : interfaces) {
            if (ifaceDef.getName().equals(ifaceName)) {
                for (MethodDef methodDef : ifaceDef.getMethods()) {
                    if (methodDef.getName().equals(methodName)) {
                        if (signature == null || methodDef.getSignature().equals(signature)) {
                            return methodDef;
                        }
                    }
                }
            }
        }
        return null;  // no match found
    }

    /** @return the definition of the matching signal, or null if not found. */
    public SignalDef getSignal(String ifaceName, String signalName, String signature) {
        return getSignal(getInterfaces(), ifaceName, signalName, signature);
    }

    /**
     * Search the interfaces definition list for the signal matching the given
     * interface name and signal name. The search will be narrowed further if
     * a signal signature is given.
     *
     * @param interfaces the list of interfaces to search.
     * @param ifaceName the name of the interface of interest.
     * @param signalName the name of the signal definition to find.
     * @param signature the signal signature of interest. Ignored if null.
     * @return the matching instance of SignalDef, or null if not found.
     */
    public static SignalDef getSignal(List<InterfaceDef> interfaces, String ifaceName,
                String signalName, String signature) {
        if (interfaces == null || ifaceName == null) return null;
        for (InterfaceDef ifaceDef : interfaces) {
            if (ifaceDef.getName().equals(ifaceName)) {
                for (SignalDef signalDef : ifaceDef.getSignals()) {
                    if (signalDef.getName().equals(signalName)) {
                        if (signature == null || signalDef.getSignature().equals(signature)) {
                            return signalDef;
                        }
                    }
                }
            }
        }
        return null;  // no match found
    }

    /** @return the definition of the matching property, or null if not found. */
    public PropertyDef getProperty(String ifaceName, String propertyName) {
        return getProperty(getInterfaces(), ifaceName, propertyName);
    }

    /**
     * Search the interfaces definition list for the property matching the given
     * interface name and property name.
     *
     * @param interfaces the list of interfaces to search.
     * @param ifaceName the name of the interface of interest.
     * @param propertyName the name of the property definition to find.
     * @return the matching instance of PropertyDef, or null if not found.
     */
    public static PropertyDef getProperty(List<InterfaceDef> interfaces, String ifaceName,
                String propertyName) {
        if (interfaces == null || ifaceName == null) return null;
        for (InterfaceDef ifaceDef : interfaces) {
            if (ifaceDef.getName().equals(ifaceName)) {
                for (PropertyDef propertyDef: ifaceDef.getProperties()) {
                    if (propertyDef.getName().equals(propertyName)) {
                        return propertyDef;
                    }
                }
            }
        }
        return null;  // no match found
    }

}