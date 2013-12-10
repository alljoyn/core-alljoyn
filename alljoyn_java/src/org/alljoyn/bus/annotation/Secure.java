/*
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
 * Indicates that a given interface is secured by an authentication mechanism.
 *
 * Valid value declarations are:
 * <ol>
 *   <li>required(default value) - interface methods on the interface can only be
 *   called by an authenticated peer</li>
 *   <li>off - if security is off interface authentication is never required
 *   even when implemented by a secure object.</li>
 *   <li>inherit - if the Secure annotation is omitted the interface will use a
 *   security of inherit.  If security is inherit or security is not specified for an
 *   interface the interface inherits the security of the objects that implements
 *   the interface.</li>
 *   <li>if an unknown value is given inherit will be used</li>
 * </ol>
 * @see org.alljoyn.bus.BusAttachment#registerAuthListener(String, AuthListener)
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
public @interface Secure {
    String value() default "required";
}
