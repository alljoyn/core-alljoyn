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
 * Base class of AllJoyn exceptions.
 */
public class BusException extends java.lang.Exception {

    /** Constructs a default BusException. */
    public BusException() {
        super();
    }

    /**
     * Constructs a BusException with a user-defined message.
     *
     * @param msg user-defined message
     */
    public BusException(String msg) {
        super(msg);
    }

    /**
     * Constructs a chained BusException with a user-defined message.
     *
     * @param msg user-defined message
     * @param cause the cause of this exception
     */
    public BusException(String msg, Throwable cause) {
        super(msg, cause);
    }

    /** Prints a line to the native log. */
    private static native void logln(String line);

    /** Logs a Throwable to the native log. */
    static void log(Throwable th) {
        String prefix = "";
        do {
            logln(prefix + th.getClass().getName()
                  + (th.getMessage() == null ? "" : ": " + th.getMessage()));
            StackTraceElement[] stack = th.getStackTrace();
            for (int frame = 0; frame < stack.length; ++frame) {
                logln("    at " + stack[frame]);
            }
            th = th.getCause();
            prefix = "Caused by: ";
        } while (th != null);
    }

    /**
     * serialVersionUID is recommended for all serializable classes.
     *
     * This class is not expected to be serialized. This is added to address
     * build warnings.
     */
    private static final long serialVersionUID = 2268095363311316097L;
}
