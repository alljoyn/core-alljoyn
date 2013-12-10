/**
 * @file
 * SASLEngine is a utility class that provides authentication functions
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_SASLENGINE_H
#define _ALLJOYN_SASLENGINE_H

#include <qcc/platform.h>

#include <set>

#include <qcc/String.h>
#include <qcc/GUID.h>
#include <qcc/KeyBlob.h>
#include <qcc/Mutex.h>

#include <alljoyn/BusAttachment.h>

#include "AuthMechanism.h"

namespace ajn {

/**
 * %SASLEngine is a utility class that implements the state machine for SASL-based authentication
 * mechanisms
 */
class SASLEngine {
  public:

    /** Authentication state   */

    typedef enum {
        ALLJOYN_SEND_AUTH_REQ,   ///< Initial responder state,
        ALLJOYN_WAIT_FOR_AUTH,   ///< Initial challenger state
        ALLJOYN_WAIT_FOR_BEGIN,
        ALLJOYN_WAIT_FOR_DATA,
        ALLJOYN_WAIT_FOR_OK,
        ALLJOYN_WAIT_FOR_REJECT,
        ALLJOYN_WAIT_EXT_RESPONSE, ///< Wait for a response to an extension command */
        ALLJOYN_AUTH_SUCCESS,      ///< Authentication was successful - conversation it over
        ALLJOYN_AUTH_FAILED        ///< Authentication failed - conversation it over
    } AuthState;

    /**
     * %ExtCommmandHandler is a class for implementing an entension command handler for commands
     * that are outside of the normal SASL command set.
     */
    class ExtensionHandler {
      public:
        /**
         * The expected behavior depends on whether the SASL role is RESPONDER or CHALLENGER.
         *
         * For a RESPONDER the SASLCallout is first called with an empty string. This prompts the
         * responder to provide an extension command to send. The next call delivers the
         * CHALLENGER's response and responder either returns an empty string to terminate the
         * extension exchange or a new extension command to continue the exchange.
         *
         * For a CHALLENGER the each call contains the responder's extension command and the return
         * value is the challenger's response. If the challenger responds with an empty string an
         * ERROR will be sent to the responder.
         *
         * @param sasl    The sasl engine instance.
         * @param extCmd  The extension command string or an empty string.
         *
         * @return  A command/response string or an empty string.
         */
        virtual qcc::String SASLCallout(SASLEngine& sasl, const qcc::String& extCmd) = 0;

        /*
         * Destructor
         */
        virtual ~ExtensionHandler() { }
    };

    /**
     * Constructor
     *
     * @param bus           The bus
     * @param authRole      Challenger or responder end of the authentication conversation
     * @param mechanisms    The mechanisms to use for this authentication conversation
     * @param authPeer      A string identifying the peer being authenticated
     * @param listener      Listener for handling password and other authentication related requests.
     * @param extHandler    The an optional handler for extension commands.
     */
    SASLEngine(BusAttachment& bus, AuthMechanism::AuthRole authRole, const qcc::String& mechanisms, const char* authPeer, ProtectedAuthListener& listener, ExtensionHandler* extHandler = NULL);

    /**
     * Destructor
     */
    ~SASLEngine();

    /**
     * Advance to the next step in the authentication conversation
     *
     * @param authIn   The authentication string received from the remote endpoint
     * @param authOut  The authentication string to send to the remote endpoint
     * @param state    Returns the current state of the conversation. The conversation is complete
     *                 when the state is ALLJOYN_AUTHENTICATED.
     *
     * @return ER_OK if the conversation has moved forward,
     *         ER_BUS_AUTH_FAIL if converstation ended with an failure to authenticate
     *         ER_BUS_NOT_AUTHENTICATING if the conversation is complete
     */
    QStatus Advance(qcc::String authIn, qcc::String& authOut, AuthState& state);

    /**
     * Returns the name of the authentication last mechanism that was used. If the authentication
     * conversation is complete this is the authentication mechanism that succeeded or failed.
     */
    qcc::String GetMechanism() { return (authMechanism) ? authMechanism->GetName() : ""; }

    /**
     * Get the identifier string received at the end of a succesful authentication conversation
     */
    const qcc::String& GetRemoteId() { return remoteId; };

    /**
     * Set the identifier string to be sent at the end of a succesful authentication conversation
     */
    void SetLocalId(const qcc::String& id) { localId = id; };

    /**
     * Get the master secret from authentication mechanisms that negotiate one.
     *
     * @param secret The master secret key blob.
     *
     * @return   - ER_OK on success
     *           - ER_BUS_KEY_UNAVAILABLE if there is no master secret to get.
     */
    QStatus GetMasterSecret(qcc::KeyBlob& secret) {
        return (authState != ALLJOYN_AUTH_SUCCESS) ? ER_BUS_KEY_UNAVAILABLE : authMechanism->GetMasterSecret(secret);
    }

    /**
     * Get the role of this SASL engine instance.
     *
     * @return  Returns the role of this SASL engine instance.
     */
    AuthMechanism::AuthRole GetRole() { return authRole; }

    /**
     * Returns true if the authentication mechanism resulted in mutual authentication or false if
     * the authentication only authenticated the RESPONDER to the CHALLENGER.
     *
     * @return Returns true if the authentication was mutual.
     */
    bool AuthenticationIsMutual() { return authIsMutual; }

  private:

    /**
     * Copy constructor not defined this class
     */
    SASLEngine(const SASLEngine& other);

    /**
     * Assigment operator not defined for this class
     */
    SASLEngine& operator=(const SASLEngine& other);

    /**
     * Default constructor is private
     */
    SASLEngine();

    /**
     * The bus object
     */
    BusAttachment& bus;

    /**
     * Indicates if this is a challenger or a responder
     */
    AuthMechanism::AuthRole authRole;

    /**
     * Peer that is being authenticated, this is simply passed on to the authentication mechanisms.
     */
    qcc::String authPeer;

    /**
     * Listener for handling interactive authentication methods.
     */
    ProtectedAuthListener& listener;

    /**
     * Set of available authentication method names
     */
    std::set<qcc::String> authSet;

    /**
     * Count of number of times the state machine has been advanced.
     */
    uint16_t authCount;

    /**
     * Current authentication mechanism
     */
    AuthMechanism* authMechanism;

    /**
     * Current state machine state
     */
    AuthState authState;

    /**
     * Identifier string received from remote authenticated endpoint
     */
    qcc::String remoteId;

    /**
     * Identifier string to send to remote authenticated endpoint
     */
    qcc::String localId;

    /**
     * Extension handler if present.
     */
    ExtensionHandler* extHandler;

    /**
     * Records if a mutual one-side authentication was used.
     */
    bool authIsMutual;

    /**
     * Internal methods
     */

    QStatus Response(qcc::String& inStr, qcc::String& outStr);
    QStatus Challenge(qcc::String& inStr, qcc::String& outStr);
    QStatus NewAuthRequest(qcc::String& authCmd);
};

}

#endif
