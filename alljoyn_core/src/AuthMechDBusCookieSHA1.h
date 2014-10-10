#ifndef _ALLJOYN_AUTHMECHDBUSCOOKIESHA1_H
#define _ALLJOYN_AUTHMECHDBUSCOOKIESHA1_H
/**
 * @file
 * This file defines the class for the DBUS Cookie SHA1 authentication method
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
#error Only include AuthMechDBusCookieSHA1.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include "AuthMechanism.h"

namespace ajn {

/**
 * Derived class for the DBUS Cookie SHA1 authentication method
 */
class AuthMechDBusCookieSHA1 : public AuthMechanism {
  public:

    /**
     * Returns the static name for this authentication method
     *
     * @return string "DBUS_COOKIE_SHA1"
     *
     * @see AuthMechDBusCookieSHA1::GetName
     */
    static const char* AuthName() { return "DBUS_COOKIE_SHA1"; }

    /**
     * Returns the name for this authentication method
     *
     * returns the same result as \b AuthMechAuthMechDBusCookieSHA1::AuthName
     *
     * @return the static name for the DBus cookie SHA1 authentication mechanism.
     */
    const char* GetName() { return AuthName(); }

    /**
     * Function of type AuthMechanismManager::AuthMechFactory
     *
     * @param keyStore   The key store available to this authentication mechanism.
     * @param listener   The listener to register (listener is not used by AuthMechDBusCookieSHA1)
     *
     * @return  An object of class AuthMechDBusCookieSHA1
     */
    static AuthMechanism* Factory(KeyStore& keyStore, ProtectedAuthListener& listener) { return new AuthMechDBusCookieSHA1(keyStore, listener); }

    /**
     * Initial response from this client which in this case is the current user name
     *
     * @param result  Returns:
     *                - ALLJOYN_AUTH_OK        if the authentication is complete,
     *                - ALLJOYN_AUTH_CONTINUE  if the authentication is incomplete
     *
     * @return the user name set by the environment variable "USERNAME".
     */
    qcc::String InitialResponse(AuthResult& result);

    /**
     * Client's response to a challenge from the server
     * @param challenge String representing the challenge provided by the server
     * @param result    Returns:
     *                      - ALLJOYN_AUTH_OK  if the authentication is complete,
     *                      - ALLJOYN_AUTH_ERROR    if the response was rejected.
     *
     * @return a string that is in response to the challenge string provided by
     *         the server. This string is not expected to be human readable.
     */
    qcc::String Response(const qcc::String& challenge, AuthResult& result);

    /**
     * Server's challenge to be sent to the client
     *
     * @param response Response used by server
     * @param result Returns:
     *                   - ALLJOYN_AUTH_OK  if the authentication is complete,
     *                   - ALLJOYN_AUTH_CONTINUE if the authentication is incomplete,
     *                   - ALLJOYN_AUTH_ERROR    if the response was rejected.
     * @return a string designed to be used as the the challenge string
     *         when calling \b AuthMechDBusCookieSHA1::Response.
     *         This string is not expected to be human readable.
     */
    qcc::String Challenge(const qcc::String& response, AuthResult& result);

  private:

    AuthMechDBusCookieSHA1(KeyStore& keyStore, ProtectedAuthListener& listener) : AuthMechanism(keyStore, listener) { }

    qcc::String userName;
    qcc::String cookie;
    qcc::String nonce;

};

}

#endif
