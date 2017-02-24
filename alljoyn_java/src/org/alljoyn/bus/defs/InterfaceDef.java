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

import java.util.List;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Collections;

/**
 * Interface definition used to describe the interface.
 * Annotations commonly used: DocString, Secure.
 */
public class InterfaceDef extends BaseDef {

    /**
     * Values for the Secure annotation. One of: "inherit", "true", or "off".
     * Indicates that a given interface is to be secured by an authentication mechanism. If the Secure
     * annotation is absent, or an unknown value is given, the interface will use a security of inherit.
     */
    public enum SecurityPolicy {
        /** Inherit the security of the object that implements the interface */
        INHERIT("inherit"),
        /** Security is required for an interface */
        TRUE("true"),
        /** Security does not apply to this interface */
        OFF("off");

        final public String text;

        SecurityPolicy(String text) {
            this.text = text;
        }

        public static SecurityPolicy fromString(String text) {
            for (SecurityPolicy item: SecurityPolicy.values()) {
                if (item.text.equalsIgnoreCase(text)) {
                    return item;
                }
            }
            return null;
        }
    }

    final private List<MethodDef> methods;
    final private List<SignalDef> signals;
    final private List<PropertyDef> properties;

    private boolean announced;
    final private String introspectXml; // xml description of the interface (optional)


    /**
     * Constructor.
     *
     * @param name the name of the bus interface.
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    public InterfaceDef(String name) {
        this(name, false, null);
    }

    /**
     * Constructor.
     *
     * @param name the name of the bus interface.
     * @param isAnnounced whether the interface is announced
     * @param introspectXml [optional] XML describing the interface and its methods, signals, properties.
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    public InterfaceDef(String name, boolean isAnnounced, String introspectXml) {
        super(name);
        this.methods = new ArrayList<MethodDef>();
        this.signals = new ArrayList<SignalDef>();
        this.properties = new ArrayList<PropertyDef>();
        this.announced = isAnnounced;
        this.introspectXml = introspectXml;
    }

    /**
     * @return whether the bus interface is secure (indicated via a Secure annotation).
     */
    public boolean isSecureRequired() {
        String value = getAnnotation(ANNOTATION_SECURE);
        return value != null && value.equalsIgnoreCase(SecurityPolicy.TRUE.text);
    }

    /**
     * Returns whether the security policy is inherit (indicated via a Secure annotation).
     * If the Secure annotation is absent, or an unknown value is present, the interface
     * will also use a security of inherit.
     *
     * @return whether the bus interface will inherit the security of the object that
     *         implements the interface.
     */
    public boolean isSecureInherit() {
        String value = getAnnotation(ANNOTATION_SECURE);
        return value == null || value.equalsIgnoreCase(SecurityPolicy.INHERIT.text) ||
                !(value.equalsIgnoreCase(SecurityPolicy.TRUE.text)
                        || value.equalsIgnoreCase(SecurityPolicy.OFF.text)
                        || value.equalsIgnoreCase("false"));  // accepts 'false' as an alias for 'off'
    }

    /**
     * @return whether the bus interface is announced.
     */
    public boolean isAnnounced() {
        return announced;
    }

    public void setAnnounced(boolean isAnnounced) {
        this.announced = isAnnounced;
    }

    /**
     * @return the interface's contained method definitions.
     */
    public List<MethodDef> getMethods() {
        return methods;
    }

    public void setMethods(List<MethodDef> list) {
        methods.clear();
        methods.addAll(list);
    }

    public void addMethod(MethodDef method) {
        methods.add(method);
    }

    /**
     * @return the interface's contained signal definitions.
     */
    public List<SignalDef> getSignals() {
        return signals;
    }

    public void setSignals(List<SignalDef> list) {
        signals.clear();
        signals.addAll(list);
    }

    public void addSignal(SignalDef signal) {
        signals.add(signal);
    }

    /**
     * @return the interface's contained property definitions.
     */
    public List<PropertyDef> getProperties() {
        return properties;
    }

    public void setProperties(List<PropertyDef> list) {
        properties.clear();
        properties.addAll(list);
    }

    public void addProperty(PropertyDef property) {
        properties.add(property);
    }

    /**
     * @return the xml description of the interface. Null if not provided.
     */
    public String getIntrospectXml() {
        return introspectXml;
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("InterfaceDef {name=");
        builder.append(getName());
        for ( MethodDef m : methods ) {
            builder.append("\n\t" + m.toString());
        }
        for ( SignalDef s : signals ) {
            builder.append("\n\t" + s.toString());
        }
        for ( PropertyDef p : properties ) {
            builder.append("\n\t" + p.toString());
        }
        builder.append("}");
        return builder.toString();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        InterfaceDef that = (InterfaceDef) o;

        if (!getName().equals(that.getName())) return false;
        if (methods.size() != that.methods.size()) return false;
        if (signals.size() != that.signals.size()) return false;
        if (properties.size() != that.properties.size()) return false;

        // Sort and test whether methods lists are equal (use copy to preserve original order)
        List<MethodDef> thisMethods = new ArrayList<MethodDef>(methods);
        List<MethodDef> thatMethods = new ArrayList<MethodDef>(that.methods);
        Collections.sort(thisMethods, new Comparator<MethodDef>() {
            public int compare(MethodDef one, MethodDef two){
                return one.getName().compareTo(two.getName());
            }
        });
        Collections.sort(thatMethods, new Comparator<MethodDef>() {
            public int compare(MethodDef one, MethodDef two){
                return one.getName().compareTo(two.getName());
            }
        });
        if (!thisMethods.equals(thatMethods)) return false;

        // Sort and test whether signals lists are equal (use copy to preserve original order)
        List<SignalDef> thisSignals = new ArrayList<SignalDef>(signals);
        List<SignalDef> thatSignals = new ArrayList<SignalDef>(that.signals);
        Collections.sort(thisSignals, new Comparator<SignalDef>() {
            public int compare(SignalDef one, SignalDef two){
                return one.getName().compareTo(two.getName());
            }
        });
        Collections.sort(thatSignals, new Comparator<SignalDef>() {
            public int compare(SignalDef one, SignalDef two){
                return one.getName().compareTo(two.getName());
            }
        });
        if (!thisSignals.equals(thatSignals)) return false;

        // Sort and test whether properties lists are equal (use copy to preserve original order)
        List<PropertyDef> thisProperties = new ArrayList<PropertyDef>(properties);
        List<PropertyDef> thatProperties = new ArrayList<PropertyDef>(that.properties);
        Collections.sort(thisProperties, new Comparator<PropertyDef>() {
            public int compare(PropertyDef one, PropertyDef two){
                return one.getName().compareTo(two.getName());
            }
        });
        Collections.sort(thatProperties, new Comparator<PropertyDef>() {
            public int compare(PropertyDef one, PropertyDef two){
                return one.getName().compareTo(two.getName());
            }
        });
        return thisProperties.equals(thatProperties);
    }

    @Override
    public int hashCode() {
        int result = getName().hashCode();
        result = 31 * result + methods.hashCode();
        result = 31 * result + signals.hashCode();
        result = 31 * result + properties.hashCode();
        return result;
    }

}