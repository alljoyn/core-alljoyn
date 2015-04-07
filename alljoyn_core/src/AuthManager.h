#ifndef _ALLJOYN_AUTHMANAGER_H
#define _ALLJOYN_AUTHMANAGER_H
/**
 * @file
 * This file defines the authentication mechanism manager
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
#error Only include AuthManager.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>
#include <qcc/Debug.h>

#include <map>

#include "AuthMechanism.h"
#include "KeyStore.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_AUTH"

namespace ajn {

/**
 * This class manages authentication mechanisms
 */
class AuthManager {
  public:

    /**
     * Constructor
     */
    AuthManager(KeyStore& keyStore) : keyStore(keyStore) { }

    /**
     * Type for an factory for an authentication mechanism. AllJoynAuthentication mechanism classes
     * provide a function of this type when registering with the AllJoynAuthentication mechanism manager.
     *
     * @param keyStore  The key store for keys and other security credentials required for the
     *                  authentication mechanism.
     *
     * @param listener  Provides callouts for authentication mechanisms that interact with the user or
     *                  application.
     */
    typedef AuthMechanism* (*AuthMechFactory)(KeyStore& keyStore, ProtectedAuthListener& listener);

    /**
     * Registers an authentication mechanism factory function and associates it with a
     * specific authentication mechanism name.
     */
    void RegisterMechanism(AuthMechFactory factory, const char* mechanismName)
    {
        authMechanisms[mechanismName] = factory;
    }

    /**
     * Unregisters an authentication mechanism factory function
     */
    void UnregisterMechanism(const char* mechanismName)
    {
        AuthMechFactoryMap::iterator it = authMechanisms.find(mechanismName);
        if (it != authMechanisms.end()) {
            authMechanisms.erase(it);
        }
    }

    /**
     * Filter out mechanisms with names not listed in the string.
     */
    size_t FilterMechanisms(const qcc::String& list)
    {
        size_t num = 0;
        std::map<qcc::StringMapKey, AuthMechFactory>::iterator it;

        for (it = authMechanisms.begin(); it != authMechanisms.end(); it++) {
            if (list.find(it->first.c_str()) == qcc::String::npos) {
                authMechanisms.erase(it);
                it = authMechanisms.begin();
                num = 0;
            } else {
                ++num;
            }
        }
        return num;
    }

    /**
     * Check that the list of names are registered mechanisms.
     */
    QStatus CheckNames(qcc::String list)
    {
        QStatus status = ER_OK;
        while (!list.empty()) {
            size_t pos = list.find_first_of(' ');
            qcc::String name = list.substr(0, pos);
            if (name == "ALLJOYN_ECDHE_NULL") {
            } else if (name == "ALLJOYN_ECDHE_PSK") {
            } else if (name == "ALLJOYN_ECDHE_ECDSA") {
            } else if (name == "GSSAPI") {
            } else if (!authMechanisms.count(name)) {
                status = ER_BUS_INVALID_AUTH_MECHANISM;
                QCC_LogError(status, ("Unknown authentication mechanism %s", name.c_str()));
                break;
            }
            list.erase(0, (pos == qcc::String::npos) ? pos : pos + 1);
        }
        return status;
    }

    /**
     * Returns an authentication mechanism object for the requested authentication mechanism.
     *
     * @param mechanismName String representing the name of the authentication mechanism
     *
     * @param listener  Required for authentication mechanisms that interact with the user or
     *                  application.
     *
     * @return An object that implements the requested authentication mechanism or NULL if there is
     *         no such object. Note this function will also return NULL if the authentication
     *         mechanism requires a listener and none has been provided.
     */
    AuthMechanism* GetMechanism(const qcc::String& mechanismName, ProtectedAuthListener& listener)
    {
        std::map<qcc::StringMapKey, AuthMechFactory>::iterator it = authMechanisms.find(mechanismName);
        if (authMechanisms.end() != it) {
            return (it->second)(keyStore, listener);
        } else {
            return NULL;
        }
    }


  private:
    /* Private assigment operator - does nothing */
    AuthManager operator=(const AuthManager&);

    /**
     * Reference to the keyStore
     */
    KeyStore& keyStore;

    /**
     * Maps authentication mechanisms names to factory functions
     */
    typedef std::map<qcc::StringMapKey, AuthMechFactory> AuthMechFactoryMap;
    AuthMechFactoryMap authMechanisms;
};

}

#undef QCC_MODULE

#endif
