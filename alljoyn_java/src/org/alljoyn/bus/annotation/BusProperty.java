/*
 * Copyright (c) 2009-2011,2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.annotation;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Indicates that a particular member of an AllJoyn exportable interface
 * is defined to be a AllJoyn property.  In addition to the annotation,
 * property methods must be of the form "{@code getProperty}"
 * and/or "{@code setProperty}".  In this case, "Property" is the
 * AllJoyn name of the property and it is both a read and write property.
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface BusProperty {

    /**
     * Override of property name.
     * The default property name is the method name without the "get"
     * or "set" prefix.  For example, if the method is getState() then
     * the default property name is "State".
     *
     * @return name specified in the BusProperty annotation
     */
    String name() default "";

    /**
     * Signature of property.
     *
     * @see Signature
     *
     * @return signature specified in the BusProperty annotation
     */
    String signature() default "";

    /**
     * Description of this property
     *
     * @return descroption specified in the BusProperty annotation
     */
    String description() default "";

    /** EmitChangedSignal annotate flag. */
    int ANNOTATE_EMIT_CHANGED_SIGNAL = 1;

    /** EmitChangedSignal annotate flag for invalidation notifications. */
    int ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES = 2;

    /**
     * Annotate introspection data for method.
     * The annotation may be the flag ANNOTATE_EMIT_CHANGED_SIGNAL or
     * ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES.  See
     * org.freedesktop.DBus.Property.EmitsChangedSignal in the D-Bus
     * Specification.
     *
     * @return annotation EmitChagedSignal annotate flag specified in the BusProperty annotation
     */
    int annotation() default 0;
}
