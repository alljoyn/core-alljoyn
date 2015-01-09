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

import org.alljoyn.bus.BusObject;

/**
 * Indicates that a method should receive AllJoyn signals.
 * <p>
 * {@code iface} and {@code signal} may be either the DBus interface and signal name, or the Java interface and method
 * name. For the following interface definition:
 *
 * <blockquote>
 * <pre>
 * package org.myapp;
 *
 * &#064;BusInterface(name = &quot;org.sample.MyInterface&quot;)
 * public interface IMyInterface
 * {
 *     &#064;BusSignal(name = &quot;MySignal&quot;)
 *     public void EmitMySignal(String str)
 *         throws BusException;
 * }
 *
 * </pre>
 * </blockquote>
 * <p>
 * either of the following may be used to annotate a signal handler:
 * </p>
 * <blockquote>
 * <pre>
 * &#064;BusSignalHandler(iface = &quot;org.sample.MyInterface&quot;, signal = &quot;MySignal&quot;)
 * public void handleSignal(String str)
 * {
 * }
 *
 * &#064;BusSignalHandler(iface = &quot;org.myapp.IMyInterface&quot;, signal = &quot;EmitMySignal&quot;)
 * public void handleSignal(String str)
 * {
 * }
 * </pre>
 * </blockquote>
 * <p>
 * The first example may be used succesfully when {@code IMyInterface} is known to the BusAttachment via a previous call
 * to {@link org.alljoyn.bus.BusAttachment#registerBusObject(BusObject, String)} or
 * {@link org.alljoyn.bus.BusAttachment#getProxyBusObject(String, String, int, Class[])}.
 * </p>
 * <p>
 * The second example may be used succesfully when {@code IMyInterface} is unknown to the BusAttachment.
 * </p>
 * <p>
 * The rule and source properties are mutually exclusive: first the rule property is checked. If it is non-empty, The
 * signal handler will be registered with this rule. If it is empty, the rule will be registered with the source
 * property.
 * </p>
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface BusSignalHandler {

    /**
     * The interface name of the signal.
     *
     * @return iface name specified in the BusSignalHandler annotation
     */
    String iface();

    /**
     * The signal name of the signal.
     *
     * @return signal name specified in the BusSignalHandler annotation
     */
    String signal();

    /**
     * The object path of the emitter of the signal or unspecified for all
     * paths.
     *
     * @return source object path specified in the BusSignalHandler annotation
     */
    String source() default "";

    /**
     * The rule that will be used as a filter to invoke this signal handler
     *
     * @return rule specified in the BusSignalHandler annotation
     */
    String rule() default "";
}
