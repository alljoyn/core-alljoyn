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

import java.util.HashMap;
import java.util.Map;

/**
 * Abstract base definition used by interface, method, property, signal, and arg defs.
 */
public abstract class BaseDef {

    /* Common annotation name constants */

    /** Deprecated annotation name (used in methods, signals, properties). Valid values: true, false. */
    public static final String ANNOTATION_DEPRECATED                    = "org.freedesktop.DBus.Deprecated";

    /** NoReply annotation name (used in methods). Valid values: true, false. */
    public static final String ANNOTATION_METHOD_NOREPLY                = "org.freedesktop.DBus.Method.NoReply";

    /** Signal-emission annotation names (used in signals). Valid values: true, false. */
    public static final String ANNOTATION_SIGNAL_SESSIONLESS            = "org.alljoyn.Bus.Signal.Sessionless";
    public static final String ANNOTATION_SIGNAL_SESSIONCAST            = "org.alljoyn.Bus.Signal.Sessioncast";
    public static final String ANNOTATION_SIGNAL_UNICAST                = "org.alljoyn.Bus.Signal.Unicast";
    public static final String ANNOTATION_SIGNAL_GLOBAL_BROADCAST       = "org.alljoyn.Bus.Signal.GlobalBroadcast";

    /** EmitsChangedSignal annotation name (used in properties). Valid values: false, true, invalidates, const. */
    public static final String ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL = "org.freedesktop.DBus.Property.EmitsChangedSignal";

    /** VariantTypes annotation name (used in arguments within methods and signals). */
    public static final String ANNOTATION_ARG_VARIANT_TYPES             = "org.alljoyn.Bus.Arg.VariantTypes";

    /** Secure annotation name (used in interfaces). Valid values: inherit, required, off. */
    public static final String ANNOTATION_SECURE                        = "org.alljoyn.Bus.Secure";

    /** DocString annotation name-prefix. Full name will include a language tag suffix; e.g. DocString.en */
    public static final String ANNOTATION_DOC_STRING                    = "org.alljoyn.Bus.DocString";


    final private String name;
    final private Map<String,String> annotationList;  // annotation name/values


    /**
     * Constructor.
     *
     * @param name the name of this object.
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    protected BaseDef(String name) {
        if (name == null) {
            throw new IllegalArgumentException("Null name");
        }
        this.name = name;
        this.annotationList = new HashMap<String,String>();
    }

    /**
     * @return the name of this object.
     */
    public String getName() {
        return name;
    }

    /**
     * @return the list of annotations defined for this object.
     */
    public Map<String,String> getAnnotationList() {
        return annotationList;
    }

    public void setAnnotationList(Map<String,String> annotations) {
        annotationList.clear();
        annotationList.putAll(annotations);
    }

    /**
     * Retrieve the value of the named annotation.
     *
     * @param name the annotation name
     * @return the matching annotation value; null if not found.
     */
    public String getAnnotation(String name) {
        return annotationList.get(name);
    }

    /**
     * Add the given annotation name/value. If already present then old value is replaced.
     *
     * @param name the annotation name
     * @param value the annotation value
     */
    public void addAnnotation(String name, String value) {
        annotationList.put(name,value);
    }

    /**
     * Retrieve the description (a DocString annotation value) for the given language tag.
     *
     * @param languageTag IETF abbreviated language tag.
     * @return the description for the given language.
     */
    public String getDescription(String languageTag) {
        String suffix = (languageTag==null || languageTag.isEmpty()) ? "" : "."+languageTag;
        return annotationList.get(ANNOTATION_DOC_STRING+suffix);
    }

    /**
     * Set the description for the given language.
     *
     * @param description the description text rendered in the given language.
     * @param languageTag IETF abbreviated language tag.
     */
    public void setDescription(String description, String languageTag) {
        String suffix = (languageTag==null || languageTag.isEmpty()) ? "" : "."+languageTag;
        annotationList.put(ANNOTATION_DOC_STRING+suffix, description);
    }

}