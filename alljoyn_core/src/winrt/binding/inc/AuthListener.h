/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#pragma once

#include <alljoyn/AuthListener.h>
#include <alljoyn/Message.h>
#include <qcc/ManagedObj.h>
#include <Status_CPP0x.h>
#include <alljoyn/Status.h>

namespace AllJoyn {

public ref class AuthContext sealed {
  private:
    friend ref class AuthListener;
    friend class _AuthListener;
    AuthContext(void* authContext) : _authContext(authContext) { }
    void* _authContext;
};

ref class Credentials;
ref class BusAttachment;
ref class Message;

/// <summary>
/// Authentication mechanism requests user credentials. If the user name is not an empty string
/// the request is for credentials for that specific user. A count allows the listener to decide
/// whether to allow or reject multiple authentication attempts to the same peer.
/// </summary>
/// <param name="authMechanism">The name of the authentication mechanism issuing the request.</param>
/// <param name="peerName">
/// The name of the remote peer being authenticated.  On the initiating
/// side this will be a well-known-name for the remote peer. On the
/// accepting side this will be the unique bus name for the remote peer.
/// </param>
/// <param name="authCount">Count (starting at 1) of the number of authentication request attempts made.</param>
/// <param name="userName">The user name for the credentials being requested.</param>
/// <param name="credMask">A bit mask identifying the credentials being requested.
/// The application may return none, some or all of the requested credentials.
/// </param>
/// <param name="authContext">Callback context for associating the request with the returned credentials.</param>
/// <returns>
///Return ER_OK if the request is handled.
/// </returns>
public delegate QStatus AuthListenerRequestCredentialsAsyncHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, uint16_t authCount, Platform::String ^ userName, uint16_t credMask, AuthContext ^ authContext);

/// <summary>
///Authentication mechanism requests verification of credentials from a remote peer.
/// </summary>
/// <param name="authMechanism">The name of the authentication mechanism issuing the request.</param>
/// <param name="peerName">The name of the remote peer being authenticated. On the initiating
/// side this will be a well-known-name for the remote peer. On the
/// accepting side this will be the unique bus name for the remote peer.</param>
/// <param name="credentials">The credentials to be verified.</param>
/// <param name="authContext">Callback context for associating the request with the verification response.</param>
/// <returns>
///Return ER_OK if the request is handled.
/// </returns>
public delegate QStatus AuthListenerVerifyCredentialsAsyncHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, Credentials ^ credentials, AuthContext ^ authContext);

/// <summary>
///Optional method that if implemented allows an application to monitor security violations. This
///function is called when an attempt to decrypt an encrypted messages failed or when an unencrypted
///message was received on an interface that requires encryption. The message contains only
///header information.
/// </summary>
/// <param name="status">A status code indicating the type of security violation.</param>
/// <param name="msg">The message that cause the security violation.</param>
public delegate void AuthListenerSecurityViolationHandler(QStatus status, Message ^ msg);

/// <summary>
///Reports successful or unsuccessful completion of authentication.
/// </summary>
/// <param name="authMechanism">The name of the authentication mechanism that was used or an empty
/// string if the authentication failed.</param>
/// <param name="peerName">The name of the remote peer being authenticated.  On the initiating
/// side this will be a well-known-name for the remote peer. On the
/// accepting side this will be the unique bus name for the remote peer.</param>
/// <param name="success">true if the authentication was successful, otherwise false.</param>
public delegate void AuthListenerAuthenticationCompleteHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success);

ref class __AuthListener {
  private:
    friend ref class AuthListener;
    friend class _AuthListener;
    __AuthListener();
    ~__AuthListener();

    event AuthListenerRequestCredentialsAsyncHandler ^ RequestCredentials;
    event AuthListenerVerifyCredentialsAsyncHandler ^ VerifyCredentials;
    event AuthListenerSecurityViolationHandler ^ SecurityViolation;
    event AuthListenerAuthenticationCompleteHandler ^ AuthenticationComplete;
    property BusAttachment ^ Bus;
};

class _AuthListener : protected ajn::AuthListener {
  protected:
    friend class qcc::ManagedObj<_AuthListener>;
    friend ref class AuthListener;
    friend ref class BusAttachment;
    _AuthListener(BusAttachment ^ bus);
    ~_AuthListener();

    QStatus DefaultAuthListenerRequestCredentialsAsyncHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, uint16_t authCount, Platform::String ^ userName, uint16_t credMask, AuthContext ^ authContext);
    QStatus DefaultAuthListenerVerifyCredentialsAsyncHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, AllJoyn::Credentials ^ credentials, AuthContext ^ authContext);
    void DefaultAuthListenerSecurityViolationHandler(AllJoyn::QStatus status, Message ^ msg);
    void DefaultAuthListenerAuthenticationCompleteHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success);
    ::QStatus RequestCredentialsAsync(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, void* authContext);
    ::QStatus VerifyCredentialsAsync(const char* authMechanism, const char* peerName, const ajn::AuthListener::Credentials& credentials, void* authContext);
    void SecurityViolation(::QStatus status, const ajn::Message& msg);
    void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success);

    __AuthListener ^ _eventsAndProperties;
};

/// <summary>
/// Class to allow authentication mechanisms to interact with the user or application.
/// </summary>
public ref class AuthListener sealed {
  public:
    AuthListener(BusAttachment ^ bus);

    /// <summary>
    ///Respond to a call to RequestCredentialsAsync.
    /// </summary>
    /// <param name="authContext">Context that was passed in the call out to RequestCredentialsAsync.</param>
    /// <param name="accept">Returns true to accept the credentials request or false to reject it.</param>
    /// <param name="credentials">The credentials being returned if accept is true.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    void RequestCredentialsResponse(AuthContext ^ authContext, bool accept, Credentials ^ credentials);

    /// <summary>
    ///Respond to a call to VerifyCredentialsAsync.
    /// </summary>
    /// <param name="authContext">Context that was passed in the call out to VerifyCredentialsAsync.</param>
    /// <param name="accept">Returns true to accept the credentials or false to reject it.</param>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// - #ER_OK if successful
    /// - An error status otherwise
    /// </exception>
    void VerifyCredentialsResponse(AuthContext ^ authContext, bool accept);

    /// <summary>
    ///Called when user credentials are requested.
    /// </summary>
    event AuthListenerRequestCredentialsAsyncHandler ^ RequestCredentials
    {
        Windows::Foundation::EventRegistrationToken add(AuthListenerRequestCredentialsAsyncHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        QStatus raise(Platform::String ^ authMechanism, Platform::String ^ peerName, uint16_t authCount, Platform::String ^ userName, uint16_t credMask, AuthContext ^ authContext);
    }

    /// <summary>
    ///Called when a remote peer requests verification of credentials.
    /// </summary>
    event AuthListenerVerifyCredentialsAsyncHandler ^ VerifyCredentials
    {
        Windows::Foundation::EventRegistrationToken add(AuthListenerVerifyCredentialsAsyncHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        QStatus raise(Platform::String ^ authMechanism, Platform::String ^ peerName, AllJoyn::Credentials ^ credentials, AuthContext ^ authContext);
    }

    /// <summary>
    ///Called when an attempt to decrypt an encrypted messages failed or when an unencrypted
    ///message was received on an interface that requires encryption.
    /// </summary>
    event AuthListenerSecurityViolationHandler ^ SecurityViolation
    {
        Windows::Foundation::EventRegistrationToken add(AuthListenerSecurityViolationHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(QStatus status, Message ^ msg);
    }

    /// <summary>
    ///Called upon successful or unsuccessful completion of authentication.
    /// </summary>
    event AuthListenerAuthenticationCompleteHandler ^ AuthenticationComplete
    {
        Windows::Foundation::EventRegistrationToken add(AuthListenerAuthenticationCompleteHandler ^ handler);
        void remove(Windows::Foundation::EventRegistrationToken token);
        void raise(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success);
    }

    property BusAttachment ^ Bus
    {
        BusAttachment ^ get();
    }

  private:
    friend ref class BusAttachment;
    AuthListener(const qcc::ManagedObj<_AuthListener>* listener);
    ~AuthListener();

    qcc::ManagedObj<_AuthListener>* _mListener;
    _AuthListener* _listener;
};

}
// AuthListener.h
