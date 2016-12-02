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
package org.alljoyn.bus.common;

import org.alljoyn.bus.BusException;

public class ECCSecret {

    /** Allocate native resources. */
    private native void create();

    /** Release native resources. */
    private synchronized native void destroy();

    /**
     * Default Constructor;
     */
    public ECCSecret() {
        create();
    }

    /**
     * Let the Java garbage collector release resources.
     */
    @Override
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }
    /**
     * Derives the PreMasterSecret.
     * Current implementaiton uses SHA256 HASH KDF.
     * @return premaster secret.
     * @throws BusException
     *      ER_OK if the pre-master secret is successfully computed and put in pbPreMasterSecret.
     *      ER_FAIL otherwise.
     *      Other error status.
     */
    public native byte[] derivePreMasterSecret() throws BusException;

    /** The native connection handle. */
    private long handle;
}