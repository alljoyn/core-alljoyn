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
 * is defined to be a AllJoyn method.
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface BusMethod {

    /**
     * Override of method name.
     * The default AllJoyn method name is the Java method name.
     *
     * @return name specified in the BusMethod annotation
     */
    String name() default "";

    /**
     * Input signature for method.
     *
     * @see Signature
     *
     * @return signature specified in the BusMethod annotation
     */
    String signature() default "";

    /**
     * Output signature for method.
     *
     * @see Signature
     *
     * @return replySignature specified in the BusMethod annotation
     */
    String replySignature() default "";

    /**
     * Description of this method
     *
     * @return description specified in the BusMethod annotation
     */
    String description() default "";

    /** No reply annotate flag. */
    int ANNOTATE_NO_REPLY = 1;

    /** Deprecated annotate flag. */
    int ANNOTATE_DEPRECATED = 2;

    /**
     * Annotate introspection data for method.
     * The annotation is the exclusive OR of the flags ANNOTATE_NO_REPLY and
     * ANNOTATE_DEPRECATED.  See org.freedesktop.DBus.Deprecated and
     * org.freedesktop.DBus.Method.NoReply in the D-Bus Specification.
     *
     * @return annotation annotate flag specified in the BusMethod annotation
     */
    int annotation() default 0;
}
