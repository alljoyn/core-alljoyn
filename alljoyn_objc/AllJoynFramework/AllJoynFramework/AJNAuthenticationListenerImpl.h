////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <alljoyn/AuthListener.h>
#import <alljoyn/Message.h>
#import "AJNObject.h"
#import "AJNAuthenticationListener.h"

/** Internal class that binds an objective-c authentication listener delegate to the AllJoyn C++ authentication listener
 */
class AJNAuthenticationListenerImpl : public ajn::AuthListener {
  protected:

    /**
     * Objective C delegate called when one of the below virtual functions
     * is called.
     */
    __weak id<AJNAuthenticationListener> m_delegate;

  public:

    /**
     * Constructor for the AJN authentication listener implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNAuthenticationListenerImpl(id<AJNAuthenticationListener> aDelegate);

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNAuthenticationListenerImpl();

    /**
     * Authentication mechanism requests user credentials. If the user name is not an empty string
     * the request is for credentials for that specific user. A count allows the listener to decide
     * whether to allow or reject multiple authentication attempts to the same peer.
     *
     * @param authMechanism  The name of the authentication mechanism issuing the request.
     * @param peerName       The name of the remote peer being authenticated.  On the initiating
     *                       side this will be a well-known-name for the remote peer. On the
     *                       accepting side this will be the unique bus name for the remote peer.
     * @param authCount      Count (starting at 1) of the number of authentication request attempts made.
     * @param userName       The user name for the credentials being requested.
     * @param credMask       A bit mask identifying the credentials being requested. The application
     *                       may return none, some or all of the requested credentials.
     * @param[out] credentials    The credentials returned.
     *
     * @return  The caller should return true if the request is being accepted or false if the
     *          requests is being rejected. If the request is rejected the authentication is
     *          complete.
     */
    virtual bool RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials);

    /**
     * Authentication mechanism requests verification of credentials from a remote peer.
     *
     * @param authMechanism  The name of the authentication mechanism issuing the request.
     * @param peerName       The name of the remote peer being authenticated.  On the initiating
     *                       side this will be a well-known-name for the remote peer. On the
     *                       accepting side this will be the unique bus name for the remote peer.
     * @param credentials    The credentials to be verified.
     *
     * @return  The listener should return true if the credentials are acceptable or false if the
     *          credentials are being rejected.
     */
    virtual bool VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials);

    /**
     * Optional method that if implemented allows an application to monitor security violations. This
     * function is called when an attempt to decrypt an encrypted messages failed or when an unencrypted
     * message was received on an interface that requires encryption. The message contains only
     * header information.
     *
     * @param status  A status code indicating the type of security violation.
     * @param msg     The message that cause the security violation.
     */
    virtual void SecurityViolation(QStatus status, const ajn::Message& msg);

    /**
     * Reports successful or unsuccessful completion of authentication.
     *
     * @param authMechanism  The name of the authentication mechanism that was used or an empty
     *                       string if the authentication failed.
     * @param peerName       The name of the remote peer being authenticated.  On the initiating
     *                       side this will be a well-known-name for the remote peer. On the
     *                       accepting side this will be the unique bus name for the remote peer.
     * @param success        true if the authentication was successful, otherwise false.
     */
    virtual void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success);

};