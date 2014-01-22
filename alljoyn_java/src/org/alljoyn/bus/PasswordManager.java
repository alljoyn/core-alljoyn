/*
 * Copyright (c) 2013, 2014, AllSeen Alliance. All rights reserved.
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
