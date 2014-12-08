/*
 * Copyright (c) 2009-2011,2014 AllSeen Alliance. All rights reserved.
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
 * Indicates that a given Java interface is also an AllJoyn interface.
 * AllJoyn interfaces contain AllJoyn methods and AllJoyn signals which are
 * exposed to external applications.
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
public @interface BusInterface {

    /**
     * Override of the default interface name.
     * The default AllJoyn interface name is the fully qualified name of
     * the Java interface.  The override may be used to specify the
     * explicit AllJoyn interface name.
     *
     * @return name specified by the BusInterface annotation
     */
    String name() default "";

    /**
     * specify if the interface is announced
     * possible values are "true" or "false"
     * defaults to "false"
     *
     * @return announced value specified in the BusInterface annotation
     */
    String announced() default "false";

    /**
     * This interface's description language
     *
     * @return descriptionLangauge specified in the BusInterface annotation
     */
    String descriptionLanguage() default "";

    /**
     * This interface's description
     *
     * @return description specified in the BusInterface annotation
     */
    String description() default "";

    /**
     * Class name of an org.alljoyn.bus.Translator instance used
     * to translate this interface's descriptions
     *
     * @return descriptionTranslator specified in the BusInterface annotation
     */
    String descriptionTranslator() default "";
}
