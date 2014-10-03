#ifndef _ALLJOYN_AUTHMECHPIN_H
#define _ALLJOYN_AUTHMECHPIN_H
/**
 * @file
 *
 * This file defines the class for the AllJoyn Pincode based authentication mechanism *
 */

/******************************************************************************
 * Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

#ifndef __cplusplus
#error Only include AuthMechPIN.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Crypto.h>

#include "AuthMechanism.h"

namespace ajn {

/**
 * Derived class for the ALLJOYN_PIN_KEYX authentication mechanism
 */
class AuthMechPIN : public AuthMechanism {
  public:

    /**
     * Returns the static name for this authentication method
     * @return string "ALLJOYN_PIN_KEYX"
     * @see AuthMechPIN GetName
     */
    static const char* AuthName() { return "ALLJOYN_PIN_KEYX"; }

    /**
     * Returns the name for this authentication method
     * returns the same result as @b AuthMechPIN::AuthName;
     * @return the static name for the Pin authentication mechanism.
     */
    const char* GetName() { return AuthName(); }

    /**
     * Initialize this authentication mechanism.
     *
     * @param authRole  Indicates if the authentication method is initializing as a challenger or a responder.
     * @param authPeer  The bus name of the remote peer that is being authenticated.
     *
     * @return ER_OK if the authentication mechanism was succesfully initialized
     *         otherwise an error status.
     */
    QStatus Init(AuthRole authRole, const qcc::String& authPeer);

    /**
     * Function of type AuthMechanismManager::AuthMechFactory. The listener cannot be NULL for
     * this authentication mechanism.
     *
     * @param keyStore   The key store available to this authentication mechanism.
     * @param listener   The listener to register
     *
     * @return  An object of class AuthMechPIN
     */
    static AuthMechanism* Factory(KeyStore& keyStore, ProtectedAuthListener& listener) { return new AuthMechPIN(keyStore, listener); }

    /**
     * Client initiates the conversation by sending a random nonce
     */
    qcc::String InitialResponse(AuthResult& result);

    /**
     * Client's response to a challenge from the server
     */
    qcc::String Response(const qcc::String& challenge, AuthResult& result);

    /**
     * Server's challenge to be sent to the client
     * @param response Response used by server
     * @param result Server Challenge sent to the client
     */
    qcc::String Challenge(const qcc::String& response, AuthResult& result);

    /**
     * Indicates that this authentication mechanism is interactive and requires application or user
     * input.
     *
     * @return  Always returns true for this authentication mechanism.
     */
    bool IsInteractive() { return true; }

    ~AuthMechPIN() { }

  private:

    /**
     * Objects must be created using the factory function
     */
    AuthMechPIN(KeyStore& keyStore, ProtectedAuthListener& listener);

    void ComputeMS(const qcc::String& serverNonce, const qcc::String& pincode);

    qcc::String ComputeVerifier(const char* label);

    qcc::String nonce;

};

}

#endif
