/*
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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
