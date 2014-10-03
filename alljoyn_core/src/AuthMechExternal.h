#ifndef _ALLJOYN_AUTHMECHEXTERNAL_H
#define _ALLJOYN_AUTHMECHEXTERNAL_H
/**
 * @file
 * This file defines the class for the DBUS EXTERNAL authentication method
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
#error Only include AuthMechExternal.h in C++ code.
#endif

#include <qcc/platform.h>

#include <qcc/Mutex.h>
#include <qcc/Util.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include "AuthMechanism.h"


namespace ajn {

/**
 * Derived class for the DBUS External authentication method
 *
 * %AuthMechExternal inherits from AuthMechanism
 */
class AuthMechExternal : public AuthMechanism {
  public:

    /**
     * Returns the static name for this authentication method
     *
     * @return string "EXTERNAL"
     *
     * @see AuthMechExternal::GetName
     */
    static const char* AuthName() { return "EXTERNAL"; }

    /**
     * Returns the name for this authentication method
     *
     * returns the same result as \b AuthMechExternal::AuthName;
     *
     * @return the static name for the anonymous authentication mechanism.
     *
     */
    const char* GetName() { return AuthName(); }

    /**
     * Function of type MKeyStoreAuthMechanismManager::AuthMechFactory
     *
     * @param keyStore   The key store available to this authentication mechanism.
     * @param listener   The listener to register (listener is not used by AuthMechExternal)
     *
     * @return An object of class AuthMechExternal
     */
    static AuthMechanism* Factory(KeyStore& keyStore, ProtectedAuthListener& listener) { return new AuthMechExternal(keyStore, listener); }

    /**
     * Client sends the user id in the initial response
     *
     * @param result  Returns:
     *                - ALLJOYN_AUTH_OK        if the authentication is complete,
     *                - ALLJOYN_AUTH_CONTINUE  if the authentication is incomplete
     *
     * @return the user id
     *
     */
    qcc::String InitialResponse(AuthResult& result) { result = ALLJOYN_AUTH_CONTINUE; return qcc::U32ToString(qcc::GetUid()); }

    /**
     * Responses flow from clients to servers.
     *        EXTERNAL always responds with OK
     * @param challenge Challenge provided by the server; the external authentication
     *                  mechanism will ignore this parameter.
     * @param result    Return ALLJOYN_AUTH_OK
     *
     * @return an empty string.
     */
    qcc::String Response(const qcc::String& challenge, AuthResult& result) { result = ALLJOYN_AUTH_OK; return ""; }

    /**
     * Server's challenge to be sent to the client
     *
     * the external authentication mechanism always responds with an empty
     * string and AuthResult of ALLJOYN_AUTH_OK when InitialChallenge is called.
     *
     * @param result Returns ALLJOYN_AUTH_OK
     *
     * @return an empty string
     *
     * @see AuthMechanism::InitialChallenge
     */
    qcc::String InitialChallenge(AuthResult& result) { result = ALLJOYN_AUTH_OK; return ""; }

    /**
     * Server's challenge to be sent to the client.
     *        EXTERNAL doesn't send anything after the initial challenge
     * @param response Response used by server
     * @param result Returns ALLJOYN_AUTH_OK.
     *
     * @return an empty string
     *
     * @see AuthMechanism::Challenge
     */
    qcc::String Challenge(const qcc::String& response, AuthResult& result) { result = ALLJOYN_AUTH_OK; return ""; }

    ~AuthMechExternal() { }

  private:

    AuthMechExternal(KeyStore& keyStore, ProtectedAuthListener& listener) : AuthMechanism(keyStore, listener) { }

};

}

#endif
