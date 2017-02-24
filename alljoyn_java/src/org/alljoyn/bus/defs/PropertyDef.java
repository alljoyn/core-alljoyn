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

/**
 * Property definition used to describe an interface property.
 * Annotations commonly used: DocString, EmitsChangedSignal, Deprecated.
 */
public class PropertyDef extends BaseDef {

    /* Property access permission values */
    public static final String ACCESS_READ = "read";
    public static final String ACCESS_WRITE = "write";
    public static final String ACCESS_READWRITE = "readwrite";

    /**
     * Values for the EmitsChangedSignal annotation. One of: "false", "true", "invalidates", "const".
     */
    public enum ChangedSignalPolicy {
        /** Emit no signal on property change */
        FALSE("false"),
        /** Emit signal denoting property change and new value */
        TRUE("true"),
        /** Emit signal denoting property change but not the new value */
        INVALIDATES("invalidates"),
        /** Property never changes and never emits changed signal. Emission same as being 'false'. */
        CONST("const");

        final public String text;

        ChangedSignalPolicy(String text) {
            this.text = text;
        }

        public static ChangedSignalPolicy fromString(String text) {
            for (ChangedSignalPolicy item: ChangedSignalPolicy.values()) {
                if (item.text.equalsIgnoreCase(text)) {
                    return item;
                }
            }
            return null;
        }
    }

    final private String interfaceName;
    final private String type;  // property signature
    final private String access;
    final private int timeout;


    /**
     * Constructor. Timeout defaults to -1, indicating that an implementation dependant
     * default timeout will be used.
     *
     * @param name the name of the bus property (without the "get" or "set" prefix)
     * @param type the type signature of the property (represented by characters: y,b,n,q,i,u,x,t,d,s,o,g,a,r,v).
     *             See https://dbus.freedesktop.org/doc/dbus-specification.html#idp94392448
     * @param access the access permission (one of: read, write, readWrite)
     * @param interfaceName the parent interface name.
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    public PropertyDef(String name, String type, String access, String interfaceName) {
        this(name, type, access, interfaceName, -1);
    }

    /**
     * Constructor.
     *
     * @param name the name of the bus property (without the "get" or "set" prefix)
     * @param type the type signature of the property (represented by characters: y,b,n,q,i,u,x,t,d,s,o,g,a,r,v).
     *             See https://dbus.freedesktop.org/doc/dbus-specification.html#idp94392448
     * @param access the access permission (one of: read, write, readWrite)
     * @param interfaceName the parent interface name.
     * @param timeout reply timeout in milliseconds; -1 indicates that an implementation dependant
     *                default timeout will be used.
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    public PropertyDef(String name, String type, String access, String interfaceName, int timeout) {
        super(name);
        if (type == null || type.isEmpty()) {
            throw new IllegalArgumentException("Null or empty type");
        }
        if (access == null ||
                !(access.equalsIgnoreCase(ACCESS_READ)
                    || access.equalsIgnoreCase(ACCESS_WRITE)
                    || access.equalsIgnoreCase(ACCESS_READWRITE))) {
            throw new IllegalArgumentException("Null or unknown access");
        }
        if (interfaceName == null) {
            throw new IllegalArgumentException("Null interfaceName");
        }
        if (timeout < -1) {
            throw new IllegalArgumentException("Invalid timeout");
        }
        this.interfaceName = interfaceName;
        this.type = type;
        this.access = access;
        this.timeout = timeout;
    }

    /**
     * @return the name of the bus interface.
     */
    public String getInterfaceName() {
        return interfaceName;
    }

    /**
     * Get the property type, represented as a signature.
     *
     * @return the signature of the property (represented by characters: y,b,n,q,i,u,x,t,d,s,o,g,a,r,v).
     */
    public String getType() {
        return type;
    }

    /**
     * Get the property access permission.
     *
     * @return the access permission (one of: 'read', 'write', readWrite').
     */
    public String getAccess() {
        return access;
    }

    /**
     * @return whether this parameter is read-only
     */
    public boolean isReadAccess() {
        return access.equalsIgnoreCase(ACCESS_READ);
    }

    /**
     * @return whether this parameter is write-only
     */
    public boolean isWriteAccess() {
        return access.equalsIgnoreCase(ACCESS_WRITE);
    }

    /**
     * @return whether this parameter is read-write
     */
    public boolean isReadWriteAccess() {
        return access.equalsIgnoreCase(ACCESS_READWRITE);
    }

    /**
     * Timeout specified in milliseconds to wait for a reply.
     * The default value is -1.
     * The value -1 means use the implementation dependent default timeout.
     *
     * @return reply timeout in milliseconds
     */
    public int getTimeout() {
        return timeout;
    }

    /**
     * @return whether the bus signal is deprecated (indicated via a Deprecated annotation).
     */
    public boolean isDeprecated() {
        String value = getAnnotation(ANNOTATION_DEPRECATED);
        return Boolean.parseBoolean(value);
    }

    /**
     * @return whether EmitsChangedSignal is set to 'true' (in the annotation). Indicates whether
     *         the interface should emit a signal on property change, denoting property change and
     *         new value.
     */
    public boolean isEmitsChangedSignal() {
        String value = getAnnotation(ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL);
        return value != null && value.equalsIgnoreCase(ChangedSignalPolicy.TRUE.text);
    }

    /**
     * @return whether EmitsChangedSignal is set to 'invalidates' (in the annotation). Indicates whether
     *         the interface should emit a signal on property change, denoting property change but not
     *         the new value.
     */
    public boolean isEmitsChangedSignalInvalidates() {
        String value = getAnnotation(ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL);
        return value != null && value.equalsIgnoreCase(ChangedSignalPolicy.INVALIDATES.text);
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("PropertyDef {name=");
        builder.append(getName());
        builder.append(", type=");
        builder.append(type);
        builder.append(", access=");
        builder.append(access);
        builder.append(", interfaceName=");
        builder.append(interfaceName);
        builder.append("}");
        return builder.toString();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        PropertyDef that = (PropertyDef) o;

        if (!interfaceName.equals(that.interfaceName)) return false;
        if (!getName().equals(that.getName())) return false;
        if (!type.equals(that.type)) return false;
        return access.equals(that.access);
    }

    @Override
    public int hashCode() {
        int result = interfaceName.hashCode();
        result = 31 * result + getName().hashCode();
        result = 31 * result + type.hashCode();
        result = 31 * result + access.hashCode();
        return result;
    }

}