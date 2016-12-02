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
 * AllJoyn ERROR message exceptions.  AllJoyn ERROR messages are sent as
 * replies to method calls to signal that an exceptional failure has
 * occurred.
 */
public class ErrorReplyBusException extends BusException {

    private final Status status;

    private final String name;

    private final String message;

    /**
     * Constructs an ErrorReplyBusException with an error name and message from
     * a status code.
     *
     * @param status the status code for the error
     */
    public ErrorReplyBusException(Status status) {
        super("org.alljoyn.Bus.ErStatus");
        this.status = status;
        this.name = null;
        this.message = null;
    }

    /**
     * Constructs an ErrorReplyBusException with an error name from
     * an error reply message.
     *
     * @param name the error name header field
     */
    public ErrorReplyBusException(String name) {
        super(name);
        this.status = Status.BUS_REPLY_IS_ERROR_MESSAGE;
        this.name = name;
        this.message = null;
    }

    /**
     * Constructs an ErrorReplyBusException with an error name and message from
     * an error reply message.
     *
     * @param name the error name header field
     * @param message the error message
     */
    public ErrorReplyBusException(String name, String message) {
        super(name);
        this.status = Status.BUS_REPLY_IS_ERROR_MESSAGE;
        this.name = name;
        this.message = message;
    }

    /**
     * Gets the error status.
     *
     * @return the status
     */
    public Status getErrorStatus() { return status; }

    /**
     * Gets the error name.
     *
     * @return the name
     */
    public String getErrorName() { return name; }

    /**
     * Gets the (optional) error message.
     *
     * @return the message
     */
    public String getErrorMessage() { return message; }

    /**
     * serialVersionUID is recommended for all serializable classes.
     *
     * This class is not expected to be serialized. This is added to address
     * build warnings.
     */
    private static final long serialVersionUID = -3417085194459513418L;
}