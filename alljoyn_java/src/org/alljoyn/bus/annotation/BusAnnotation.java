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
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Bus Methods, Signals, Properties and Interfaces can all have Annotations,
 * which have a name and a value
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
public @interface BusAnnotation {

    /**
     * Override of method name.
     *
     * @return the name specified by the BusAnnotation
     */
    String name();

    /**
     * Override of method value
     * The default value of the annotation
     *
     * @return the value specified by the BusAnnotation
     */
    String value();
}