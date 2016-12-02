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
 * Class to allow the user or application to set credentials used for the authentication
 * of thin clients.
 * Before invoking connect() to BusAttachment, the application should call SetCredentials
 * if it expects to be able to communicate to/from thin clients.
 * The bundled router will start advertising the name as soon as it is started and MUST have
 * the credentials set to be able to authenticate any thin clients that may try to use the
 * bundled router to communicate with the app.
 */
public class PasswordManager {

    /**
     * Set credentials used for the authentication of thin clients.
     *
     * @param authMechanism  Mechanism to use for authentication.
     *
     * @param password  Password to use for authentication.
     *
     * @return
     * <ul>
     * <li>OK if credentials was successfully set.</li>
     * </ul>
     */
    public static native Status setCredentials(String authMechanism, String password);

}