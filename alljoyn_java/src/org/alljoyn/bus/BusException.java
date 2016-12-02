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