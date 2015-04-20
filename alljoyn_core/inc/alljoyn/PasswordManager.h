#ifndef _ALLJOYN_PASSWORDMANAGER_H
#define _ALLJOYN_PASSWORDMANAGER_H
/**
 * @file
 * This file defines the PasswordManager class that provides the interface to
 * set credentials used for the authentication of thin clients.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#error Only include PasswordManager.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <alljoyn/Status.h>
namespace ajn {

/**
 * Class to allow the user or application to set credentials used for the authentication
 * of thin clients.
 * Before invoking Connect() to BusAttachment, the application should call SetCredentials
 * if it expects to be able to communicate to/from thin clients.
 * The bundled router will start advertising the name as soon as it is started and MUST have
 * the credentials set to be able to authenticate any thin clients that may try to use the
 * bundled router to communicate with the app.
 */

class PasswordManager {
  public:
    /**
     * @brief Set credentials used for the authentication of thin clients.
     *
     * @param authMechanism  Mechanism to use for authentication.
     * @param password       Password to use for authentication.
     *
     * @return   Returns ER_OK if the credentials was successfully set.
     */
    static QStatus AJ_CALL SetCredentials(qcc::String authMechanism, qcc::String password) {
        *PasswordManager::authMechanism = authMechanism;
        *PasswordManager::password = password;
        return ER_OK;
    }

    /// @cond ALLJOYN_DEV
    /**
     * @internal
     * Get the password set by the user/app
     *
     * @return Returns the password set by the user/app.
     */
    static qcc::String AJ_CALL GetPassword() { return *password; }

    /**
     * @internal
     * Get the authMechanism set by the user/app
     *
     * @return Returns the authMechanism set by the user/app.
     */
    static qcc::String AJ_CALL GetAuthMechanism() { return *authMechanism; }
    /// @endcond
  private:
    static void Init();
    static void Shutdown();
    friend class StaticGlobals;

    static qcc::String* authMechanism;           /**< The auth mechanism selected by the user/app */
    static qcc::String* password;                /**< The password selected by the user/app */
};

}
#endif
