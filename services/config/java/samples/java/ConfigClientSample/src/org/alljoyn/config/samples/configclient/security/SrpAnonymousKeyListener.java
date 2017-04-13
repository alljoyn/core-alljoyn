/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

package org.alljoyn.config.samples.configclient.security;

import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.alljoyn.bus.AuthListener;
import org.alljoyn.bus.BusAttachment;

/**
 * A default implementation of alljoyn AuthListener.
 * The application will register this listener with the bus, passing itself as a password handler.
 *
 * When the bus requires authentication with a remote device, it will let the password handler
 * (the application) handle it. When the bus receives a result of an authentication attempt with
 * a remote device, it will let the password handler (the application) handle it.
 * @see AuthPasswordHandler
 */
public class SrpAnonymousKeyListener implements AuthListener {

    private static final String TAG = "SrpAnonymousKeyListener";

    public static final char [] DEFAULT_PINCODE = new char[]{'0','0','0','0','0','0'};

    private AuthPasswordHandler mPasswordHandler;
    private List<String> mAuthMechanisms; // supported authentication mechanisms

    /**
     * Constructor. Uses auth mechanisms {"ALLJOYN_SRP_KEYX", "ALLJOYN_ECDHE_PSK"}.
     * @param passwordHandler
     */
    public SrpAnonymousKeyListener(AuthPasswordHandler passwordHandler) {
        mPasswordHandler = passwordHandler;

        mAuthMechanisms = new ArrayList<String>(2);
        mAuthMechanisms.add("ALLJOYN_SRP_KEYX");
        mAuthMechanisms.add("ALLJOYN_ECDHE_PSK");
    }

    /**
     * Constructor
     * @param passwordHandler
     * @param authMechanisms Array of authentication mechanisms
     */
    public SrpAnonymousKeyListener(AuthPasswordHandler passwordHandler, String[] authMechanisms) {
        if (authMechanisms == null) {
            throw new IllegalArgumentException("authMechanisms is undefined");
        }
        mPasswordHandler = passwordHandler;
        mAuthMechanisms = Arrays.asList(authMechanisms);
        Log.d(TAG, "Supported authentication mechanisms: '" + mAuthMechanisms + "'");
    }

    @Override
    public boolean requested(String mechanism, String peer, int count, String userName,  AuthRequest[] requests) {
        Log.i(TAG, " ** " + "requested, mechanism = " + mechanism + " peer = " + peer);
        if (!mAuthMechanisms.contains(mechanism) || !(requests[0] instanceof PasswordRequest)) {
            return false;
        }

        char [] pinCode = DEFAULT_PINCODE;
        if (mPasswordHandler != null && mPasswordHandler.getPassword(peer) != null) {
            pinCode = mPasswordHandler.getPassword(peer);
        }
        ((PasswordRequest)requests[0]).setPassword(pinCode);
        return true;
    }

    @Override
    public void completed(String mechanism, String authPeer, boolean authenticated) {
        mPasswordHandler.completed(mechanism, authPeer, authenticated);
    }

    /**
     * @return AuthMechanisms used by the class
     */
    public String[] getAuthMechanisms() {
        return mAuthMechanisms.toArray(new String[mAuthMechanisms.size()]);
    }

    /**
     * @return Returns AuthMechanisms used by the class as a String required by the
     * {@link BusAttachment#registerAuthListener(String, AuthListener)}
     */
    public String getAuthMechanismsAsString() {
        return mAuthMechanisms.toString().replaceAll("[\\[\\],]", "");
    }

}