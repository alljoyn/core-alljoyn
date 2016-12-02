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
 * AllJoyn marshalling related exceptions.  Marshalling exceptions occur when a
 * Java type cannot be marshalled into an AllJoyn type and vice versa.
 * <p>
 * When a Java type cannot be marshalled into an AllJoyn type, the message reported
 * will be similar to {@code cannot marshal class java.lang.Byte into 'a{ss}'} .
 * When an AllJoyn type cannot be marshalled into a Java type, the message reported
 * will be similar to {@code cannot marshal '(i)' into byte}.
 */
public class MarshalBusException extends BusException {

    /** Constructs a default MarshalBusException. */
    public MarshalBusException() {
        super();
    }

    /**
     * Constructs a MarshalBusException with a user-defined message.
     *
     * @param msg user-defined message
     */
    public MarshalBusException(String msg) {
        super(msg);
    }

    /**
     * Constructs a chained MarshalBusException with a user-defined message.
     *
     * @param msg user-defined message
     * @param cause the cause of this exception
     */
    public MarshalBusException(String msg, Throwable cause) {
        super(msg, cause);
    }

    /**
     * serialVersionUID is recommended for all serializable classes.
     *
     * This class is not expected to be serialized. This is added to address
     * build warnings.
     */
    private static final long serialVersionUID = 8113152326929117111L;
}