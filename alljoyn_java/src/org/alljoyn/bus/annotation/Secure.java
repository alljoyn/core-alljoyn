/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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