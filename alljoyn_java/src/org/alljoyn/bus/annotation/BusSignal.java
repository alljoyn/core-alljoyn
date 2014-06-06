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
 * is defined to be a AllJoyn signal.
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface BusSignal {

    /**
     * Override of signal name.
     * The default AllJoyn signal name is the Java method name.
     */
    String name() default "";

    /**
     * Input signature for signal.
     *
     * @see Signature
     */
    String signature() default "";

    /**
     * Output signature for signal.
     *
     * @see Signature
     */
    String replySignature() default "";

    /**
     * Description for this signal
     */
    String description() default "";

    /**
     * Set to true to document that this signal will be sent sessionless.
     * Note that this does not cause the signal to be sent sessionless,
     * just documents it as such.
     */
    boolean sessionless() default false;

    /** Deprecated annotate flag. */
    int ANNOTATE_DEPRECATED = 2;

    /**
     * Annotate introspection data for method.
     * The annotation may be the flag ANNOTATE_DEPRECATED.  See
     * org.freedesktop.DBus.Deprecated in the D-Bus Specification.
     */
    int annotation() default 0;
}
