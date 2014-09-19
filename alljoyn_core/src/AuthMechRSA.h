#ifndef _ALLJOYN_AUTHMECHRSA_H
#define _ALLJOYN_AUTHMECHRSA_H
/**
 * @file
 * This file defines the class for the KeyStore PeerGroup RSA authentication mechanism
 */

/******************************************************************************
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
 ******************************************************************************/

#ifndef __cplusplus
#error Only include AuthMechRSA.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Crypto.h>

#include "AuthMechanism.h"

namespace ajn {

/**
 * Derived class for the KeyStore PeerGroup RSA authentication mechanism
 *
 * %AuthMechRSA inherits from AuthMechanism
 */
class AuthMechRSA : public AuthMechanism, qcc::Crypto_RSA::PassphraseListener {
  public:

    /**
     * Returns the static name for this authentication method
     *
     * @return string "ALLJOYN_RSA_KEYX"
     *
     * @see AuthMechRSA::GetName
     */
    static const char* AuthName() { return "ALLJOYN_RSA_KEYX"; }

    /**
     * Returns the name for this authentication method
     *
     * returns the same result as @b AuthMechRSA::AuthName;
     *
     * @return the static name for the anonymous authentication mechanism.
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
     * Function of type MKeyStoreAuthMechanismManager::AuthMechFactory
     *
     * @param keyStore   The key store available to this authentication mechanism.
     * @param listener   The listener to register
     *
     * @return An object of class AuthMechRSA
     */
    static AuthMechanism* Factory(KeyStore& keyStore, ProtectedAuthListener& listener) { return new AuthMechRSA(keyStore, listener); }

    /**
     * Initial response from the client.
     *
     * @param result Returns ALLJOYN_AUTH_CONTINUE
     *
     * @return the initial response.
     */
    qcc::String InitialResponse(AuthResult& result);

    /**
     * Client's response to a challenge from the server
     *
     * @param challenge Challenge provided by the server
     * @param result clients response to a challenge
     *
     * @return Client's response to a challenge from the server
     *
     */
    qcc::String Response(const qcc::String& challenge, AuthResult& result);

    /**
     * Server's challenge to be sent to the client
     *
     * @param response Response used by server
     * @param result Server Challenge sent to the client
     *
     * @returns Server's challenge to be sent to the client
     */
    qcc::String Challenge(const qcc::String& response, AuthResult& result);

    ~AuthMechRSA() { }

  private:

    /**
     * Step in the authentication conversation
     */
    uint8_t step;

    /**
     * Callback to request a passphrase.
     *
     * @param passphrase  Returns the passphrase.
     * @param toWrite     If true indicates the passphrase is being used to write a new key.
     */
    bool GetPassphrase(qcc::String& passphrase, bool toWrite);

    /**
     * Objects must be created using the factory function
     */
    AuthMechRSA(KeyStore& keyStore, ProtectedAuthListener& listener);

    /**
     * Compute the master secret.
     */
    void ComputeMS(qcc::KeyBlob& premasterSecret);

    /**
     * Compute the verifier string.
     */
    qcc::String ComputeVerifier(const char* label);

    /**
     * State for local and remote sides of conversation.
     */
    struct Context {
        qcc::Crypto_RSA rsa;
        qcc::String certChain;
        qcc::String rand;
    };

    /**
     * Hash of all the challenges and responses used for final verification.
     */
    qcc::Crypto_SHA1 msgHash;

    /**
     * Local context.
     */
    Context local;

    /**
     * Remote context.
     */
    Context remote;

};

}

#endif
