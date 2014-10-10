/*
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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
