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

package org.alljoyn.bus;

/**
 * AllJoyn annotation related exceptions.
 * This exception will occur if
 * <ul>
 * <li>a field of a user-defined type is not annotated with its position,
 * <li>a Java data type that is not supported is used,
 * <li>an Enum data type is not annotated with a valid AllJoyn type
 * </ul>
 */
public class AnnotationBusException extends BusException {

    /** Constructs a default AnnotationBusException. */
    public AnnotationBusException() {
        super();
    }

    /**
     * Constructs a AnnotationBusException with a user-defined message.
     *
     * @param msg user-defined message
     */
    public AnnotationBusException(String msg) {
        super(msg);
    }

    /**
     * serialVersionUID is recommended for all serializable classes.
     *
     * This class is not expected to be serialized. This is added to address
     * build warnings.
     */
    private static final long serialVersionUID = 8416013838658817217L;
}