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

#include "AuthListener.h"

#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <Message.h>
#include <BusAttachment.h>
#include <Credentials.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

AuthListener::AuthListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values for bus attachment
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create auth listener as a managed type
        _mListener = new qcc::ManagedObj<_AuthListener>(bus);
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Reference to T* contained in the managed interface for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

AuthListener::AuthListener(const qcc::ManagedObj<_AuthListener>* listener)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values specified for listener
        if (NULL == listener) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach the listener passed in
        _mListener = new qcc::ManagedObj<_AuthListener>(*listener);
        if (NULL == _mListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Reference to T* contained in the managed interface for convenience
        _listener = &(**_mListener);
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

AuthListener::~AuthListener()
{
    // Delete the interface managed interface to reflect current use count
    if (NULL != _mListener) {
        delete _mListener;
        _mListener = NULL;
        _listener = NULL;
    }
}

void AuthListener::RequestCredentialsResponse(AuthContext ^ authContext, bool accept, Credentials ^ credentials)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values specified in authContext
        if (nullptr == authContext) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Check for invalid values specified in credentials
        if (nullptr == credentials) {
            status = ER_BAD_ARG_3;
            break;
        }
        // Get the ajn Credentials type for the actual call
        ajn::AuthListener::Credentials* creds = credentials->_credentials;
        // Call the real API with parameters
        status = _listener->RequestCredentialsResponse(authContext->_authContext, accept, *creds);
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void AuthListener::VerifyCredentialsResponse(AuthContext ^ authContext, bool accept)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check for invalid values specified in authContext
        if (nullptr == authContext) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Call the real API with parameters
        status = _listener->VerifyCredentialsResponse(authContext->_authContext, accept);
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Windows::Foundation::EventRegistrationToken AuthListener::RequestCredentials::add(AuthListenerRequestCredentialsAsyncHandler ^ handler)
{
    // Adds a handler to the internal ref class for RequestCredentials
    return _listener->_eventsAndProperties->RequestCredentials::add(handler);
}

void AuthListener::RequestCredentials::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Removes a handler to the internal ref class for RequestCredentials
    _listener->_eventsAndProperties->RequestCredentials::remove(token);
}

QStatus AuthListener::RequestCredentials::raise(Platform::String ^ authMechanism, Platform::String ^ peerName, uint16_t authCount, Platform::String ^ userName, uint16_t credMask, AuthContext ^ authContext)
{
    // Invokes a handler to the internal ref class for RequestCredentials
    return _listener->_eventsAndProperties->RequestCredentials::raise(authMechanism, peerName, authCount, userName, credMask, authContext);
}

Windows::Foundation::EventRegistrationToken AuthListener::VerifyCredentials::add(AuthListenerVerifyCredentialsAsyncHandler ^ handler)
{
    // Adds a handler to the internal ref class for VerifyCredentials
    return _listener->_eventsAndProperties->VerifyCredentials::add(handler);
}

void AuthListener::VerifyCredentials::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Removes a handler to the internal ref class for VerifyCredentials
    _listener->_eventsAndProperties->VerifyCredentials::remove(token);
}

QStatus AuthListener::VerifyCredentials::raise(Platform::String ^ authMechanism, Platform::String ^ peerName, AllJoyn::Credentials ^ credentials, AuthContext ^ authContext)
{
    // Invokes a handler to the internal ref class for VerifyCredentials
    return _listener->_eventsAndProperties->VerifyCredentials::raise(authMechanism, peerName, credentials, authContext);
}

Windows::Foundation::EventRegistrationToken AuthListener::SecurityViolation::add(AuthListenerSecurityViolationHandler ^ handler)
{
    // Adds a handler to the internal ref class for SecurityViolation
    return _listener->_eventsAndProperties->SecurityViolation::add(handler);
}

void AuthListener::SecurityViolation::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Removes a handler to the internal ref class for SecurityViolation
    _listener->_eventsAndProperties->SecurityViolation::remove(token);
}

void AuthListener::SecurityViolation::raise(QStatus status, Message ^ msg)
{
    // Invokes a handler to the internal ref class for SecurityViolation
    _listener->_eventsAndProperties->SecurityViolation::raise(status, msg);
}

Windows::Foundation::EventRegistrationToken AuthListener::AuthenticationComplete::add(AuthListenerAuthenticationCompleteHandler ^ handler)
{
    // Adds a handler to the internal ref class for AuthenticationComplete
    return _listener->_eventsAndProperties->AuthenticationComplete::add(handler);
}

void AuthListener::AuthenticationComplete::remove(Windows::Foundation::EventRegistrationToken token)
{
    // Removes a handler to the internal ref class for AuthenticationComplete
    _listener->_eventsAndProperties->AuthenticationComplete::remove(token);
}

void AuthListener::AuthenticationComplete::raise(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success)
{
    // Invokes a handler to the internal ref class for AuthenticationComplete
    _listener->_eventsAndProperties->AuthenticationComplete::raise(authMechanism, peerName, success);
}

BusAttachment ^ AuthListener::Bus::get()
{
    // Get property accessor to the internal ref class for Bus
    return _listener->_eventsAndProperties->Bus;
}

_AuthListener::_AuthListener(BusAttachment ^ bus)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create an instance of the internal ref class
        _eventsAndProperties = ref new __AuthListener();
        // Check for failed allocation
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Set a default handler to the internal ref class for RequestCredentials
        _eventsAndProperties->RequestCredentials += ref new AuthListenerRequestCredentialsAsyncHandler([&] (Platform::String ^ authMechanism, Platform::String ^ peerName, uint16_t authCount, Platform::String ^ userName, uint16_t credMask, AuthContext ^ authContext)->QStatus {
                                                                                                           return DefaultAuthListenerRequestCredentialsAsyncHandler(authMechanism, peerName, authCount, userName, credMask, authContext);
                                                                                                       });
        // Set a default handler to the internal ref class for VerifyCredentials
        _eventsAndProperties->VerifyCredentials += ref new AuthListenerVerifyCredentialsAsyncHandler([&] (Platform::String ^ authMechanism, Platform::String ^ peerName, AllJoyn::Credentials ^ credentials, AuthContext ^ authContext)->QStatus {
                                                                                                         return DefaultAuthListenerVerifyCredentialsAsyncHandler(authMechanism, peerName, credentials, authContext);
                                                                                                     });
        // Set a default handler to the internal ref class for SecurityViolation
        _eventsAndProperties->SecurityViolation += ref new AuthListenerSecurityViolationHandler([&] (AllJoyn::QStatus status, Message ^ msg) {
                                                                                                    DefaultAuthListenerSecurityViolationHandler(status, msg);
                                                                                                });
        // Set a default handler to the internal ref class for AuthenticationComplete
        _eventsAndProperties->AuthenticationComplete += ref new AuthListenerAuthenticationCompleteHandler([&] (Platform::String ^ authMechanism, Platform::String ^ peerName, bool success) {
                                                                                                              DefaultAuthListenerAuthenticationCompleteHandler(authMechanism, peerName, success);
                                                                                                          });
        // Set the internal ref class bus attachment value
        _eventsAndProperties->Bus = bus;
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_AuthListener::~_AuthListener()
{
    _eventsAndProperties = nullptr;
}

QStatus _AuthListener::DefaultAuthListenerRequestCredentialsAsyncHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, uint16_t authCount, Platform::String ^ userName, uint16_t credMask, AuthContext ^ authContext)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert the authMechnaism value to a qcc::String
        qcc::String strAuthMechanism = PlatformToMultibyteString(authMechanism);
        // Check for failed conversion
        if (nullptr != authMechanism && strAuthMechanism.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert the peerName value to a qcc::String
        qcc::String strPeerName = PlatformToMultibyteString(peerName);
        // Check for failed conversion
        if (nullptr != peerName && strPeerName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert the userName value to a qcc::String
        qcc::String strUserName = PlatformToMultibyteString(userName);
        // Check for failed conversion
        if (nullptr != userName && strUserName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Call the real API with parameters
        status = ajn::AuthListener::RequestCredentialsAsync(strAuthMechanism.c_str(), strPeerName.c_str(), authCount, strUserName.c_str(), credMask, authContext->_authContext);
        break;
    }

    return (AllJoyn::QStatus)status;
}

QStatus _AuthListener::DefaultAuthListenerVerifyCredentialsAsyncHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, AllJoyn::Credentials ^ credentials, AuthContext ^ authContext)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert the authMechnaism value to a qcc::String
        qcc::String strAuthMechanism = PlatformToMultibyteString(authMechanism);
        // Check for failed conversion
        if (nullptr != authMechanism && strAuthMechanism.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert the peerName value to a qcc::String
        qcc::String strPeerName = PlatformToMultibyteString(peerName);
        // Check for failed conversion
        if (nullptr != peerName && strPeerName.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Get the ajn Credentials type for the actual call
        ajn::AuthListener::Credentials* creds = credentials->_credentials;
        // Call the real API with parameters
        status = ajn::AuthListener::VerifyCredentialsAsync(strAuthMechanism.c_str(), strPeerName.c_str(), *creds, authContext->_authContext);
        break;
    }

    return (AllJoyn::QStatus)status;
}

void _AuthListener::DefaultAuthListenerSecurityViolationHandler(AllJoyn::QStatus status, Message ^ msg)
{
    //Get the ajn Message type for the actual call
    ajn::Message* m = *(msg->_message);
    // Call the real API with parameters
    ajn::AuthListener::SecurityViolation((::QStatus)(int)status, *m);
}

void _AuthListener::DefaultAuthListenerAuthenticationCompleteHandler(Platform::String ^ authMechanism, Platform::String ^ peerName, bool success)
{
    // For virtual pure functions, do nothing
}

::QStatus _AuthListener::RequestCredentialsAsync(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, void* authContext)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert the authMechnaism value to a Platform::String
        Platform::String ^ strAuthMechanism = MultibyteToPlatformString(authMechanism);
        // Check for failed conversion
        if (nullptr == strAuthMechanism && authMechanism != NULL && authMechanism[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert the peerName value to a Platform::String
        Platform::String ^ strPeerName = MultibyteToPlatformString(peerName);
        // Check for failed conversion
        if (nullptr == strPeerName && peerName != NULL && peerName[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert the userName value to a Platform::String
        Platform::String ^ strUserName = MultibyteToPlatformString(userName);
        // Check for failed conversion
        if (nullptr == strUserName && userName != NULL && userName[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create auth context from unmanaged type
        AuthContext ^ context = ref new AuthContext(authContext);
        // Check for failed allocation
        if (nullptr == context) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Invoke the callback for RequestCredentials through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     status = (::QStatus)_eventsAndProperties->RequestCredentials(strAuthMechanism, strPeerName, authCount, strUserName, credMask, context);
                                                                                                                 }));
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return status;
}

::QStatus _AuthListener::VerifyCredentialsAsync(const char* authMechanism, const char* peerName, const Credentials& credentials, void* authContext)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert the authMechnaism value to a Platform::String
        Platform::String ^ strAuthMechanism = MultibyteToPlatformString(authMechanism);
        // Check for failed conversion
        if (nullptr == strAuthMechanism && authMechanism != NULL && authMechanism[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert the peerName value to a Platform::String
        Platform::String ^ strPeerName = MultibyteToPlatformString(peerName);
        // Check for failed conversion
        if (nullptr == strPeerName && peerName != NULL && peerName[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create credentials from unmanaged type
        AllJoyn::Credentials ^ cred = ref new AllJoyn::Credentials(&credentials);
        // Check for failed allocation
        if (nullptr == cred) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Create auth context from unmanaged type
        AuthContext ^ context = ref new AuthContext(authContext);
        // Check for failed allocation
        if (nullptr == context) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Invoke the callback for VerifyCredentials through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     status = (::QStatus)_eventsAndProperties->VerifyCredentials(strAuthMechanism, strPeerName, cred, context);
                                                                                                                 }));
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }

    return status;
}

void _AuthListener::SecurityViolation(::QStatus status, const ajn::Message& msg)
{
    while (true) {
        // Create message from unmanaged type
        Message ^ message = ref new Message(&msg);
        // Check for failed allocation
        if (nullptr == message) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Invoke the callback for SecurityViolation through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->SecurityViolation((QStatus)(int)status, message);
                                                                                                                 }));
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void _AuthListener::AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Convert the authMechanism value to a Platform::String
        Platform::String ^ strAuthMechanism = MultibyteToPlatformString(authMechanism);
        // Check for failed conversion
        if (nullptr == strAuthMechanism && authMechanism != NULL && authMechanism[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Convert the peerName value to a Platform::String
        Platform::String ^ strPeerName = MultibyteToPlatformString(peerName);
        // Check for failed conversion
        if (nullptr == strPeerName && peerName != NULL && peerName[0] != '\0') {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Invoke the callback for AuthenticationComplete through the dispatcher
        _eventsAndProperties->Bus->_busAttachment->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                     _eventsAndProperties->AuthenticationComplete(strAuthMechanism, strPeerName, success);
                                                                                                                 }));
        break;
    }

    // Bubble up QStatus as a qcc exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

__AuthListener::__AuthListener()
{
    Bus = nullptr;
}

__AuthListener::~__AuthListener()
{
    Bus = nullptr;
}

}
