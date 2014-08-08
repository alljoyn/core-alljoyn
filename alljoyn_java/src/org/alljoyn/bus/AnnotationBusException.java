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
