/**
 * @file
 *
 * This file implements a AuthListener subclass for use by the C API
 */

/******************************************************************************
 * Copyright (c) 2009-2014 AllSeen Alliance. All rights reserved.
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

#include <alljoyn/AuthListener.h>
#include <alljoyn_c/AuthListener.h>
#include <assert.h>
#include <stdio.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

using namespace qcc;
using namespace std;

namespace ajn {
/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of bus related events.
 */
class AuthListenerCallbackC : public AuthListener {
  public:
    AuthListenerCallbackC(const alljoyn_authlistener_callbacks* callbacks_in, const void* context_in)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callbacks, callbacks_in, sizeof(alljoyn_authlistener_callbacks));
        context = context_in;
    }

    virtual bool RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount,
                                    const char* userName, uint16_t credMask, Credentials& credentials)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        assert(callbacks.request_credentials != NULL && "request_credentials callback must be specified");
        bool ret = (callbacks.request_credentials(context, authMechanism, peerName, authCount, userName,
                                                  credMask, (alljoyn_credentials)(&credentials)) == QCC_TRUE ? true : false);
        return ret;
    }

    virtual bool VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        bool ret = AuthListener::VerifyCredentials(authMechanism, peerName, credentials);
        if (callbacks.verify_credentials != NULL) {
            ret = (callbacks.verify_credentials(context, authMechanism, peerName, (alljoyn_credentials)(&credentials)) == QCC_TRUE ? true : false);
        }
        return ret;
    }

    virtual void SecurityViolation(QStatus status, const Message& msg)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.security_violation != NULL) {
            callbacks.security_violation(context, status, (alljoyn_message)(&msg));
        }
    }

    virtual void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        assert(callbacks.authentication_complete != NULL && "authentication_complete callback must be specified");
        callbacks.authentication_complete(context, authMechanism, peerName, (success == true ? QCC_TRUE : QCC_FALSE));
    }
  private:
    alljoyn_authlistener_callbacks callbacks;
    const void* context;
};

class AuthListenerAsyncCallbackC : public AuthListener {
  public:
    AuthListenerAsyncCallbackC(const alljoyn_authlistenerasync_callbacks* callbacks_in, const void* context_in)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        memcpy(&callbacks, callbacks_in, sizeof(alljoyn_authlistener_callbacks));
        context = context_in;
    }

    virtual QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer,
                                            uint16_t authCount, const char* userName, uint16_t credMask,
                                            void* authContext)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        assert(callbacks.request_credentials != NULL && "request_credentials callback must be specified");
        QStatus ret = callbacks.request_credentials(context, (alljoyn_authlistener) this, authMechanism, authPeer, authCount, userName, credMask, authContext);
        return ret;
    }

    virtual QStatus VerifyCredentialsAsync(const char* authMechanism, const char* authPeer,
                                           const Credentials& credentials, void* authContext) {
        QCC_DbgTrace(("%s", __FUNCTION__));
        QStatus ret = ER_NOT_IMPLEMENTED;
        if (callbacks.verify_credentials != NULL) {
            ret = callbacks.verify_credentials(context, (alljoyn_authlistener) this, authMechanism, authPeer, (alljoyn_credentials)(&credentials), authContext);
        }
        return ret;
    }

    virtual void SecurityViolation(QStatus status, const Message& msg)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.security_violation != NULL) {
            callbacks.security_violation(context, status, (alljoyn_message)(&msg));
        }
    }

    virtual void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        assert(callbacks.authentication_complete != NULL && "authentication_complete callback must be specified");
        callbacks.authentication_complete(context, authMechanism, peerName, (success == true ? QCC_TRUE : QCC_FALSE));
    }
  private:
    alljoyn_authlistenerasync_callbacks callbacks;
    const void* context;
};

}

struct _alljoyn_authlistener_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_authlistener AJ_CALL alljoyn_authlistener_create(const alljoyn_authlistener_callbacks* callbacks, const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_authlistener) new ajn::AuthListenerCallbackC(callbacks, context);
}

void AJ_CALL alljoyn_authlistener_destroy(alljoyn_authlistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(listener != NULL && "listener parameter must not be NULL");
    delete (ajn::AuthListenerCallbackC*)listener;
}

alljoyn_authlistener AJ_CALL alljoyn_authlistenerasync_create(const alljoyn_authlistenerasync_callbacks* callbacks, const void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_authlistener) new ajn::AuthListenerAsyncCallbackC(callbacks, context);
}

void AJ_CALL alljoyn_authlistenerasync_destroy(alljoyn_authlistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(listener != NULL && "listener parameter must not be NULL");
    delete (ajn::AuthListenerAsyncCallbackC*)listener;
}

QStatus AJ_CALL alljoyn_authlistener_requestcredentialsresponse(alljoyn_authlistener listener, void* authContext, QCC_BOOL accept, alljoyn_credentials credentials)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListenerAsyncCallbackC*)listener)->RequestCredentialsResponse(authContext, (accept == QCC_TRUE) ? true : false, *((ajn::AuthListener::Credentials*)credentials));
}

QStatus AJ_CALL alljoyn_authlistener_verifycredentialsresponse(alljoyn_authlistener listener, void* authContext, QCC_BOOL accept)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListenerAsyncCallbackC*)listener)->VerifyCredentialsResponse(authContext, (accept == QCC_TRUE) ? true : false);
}
struct _alljoyn_credentials_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_credentials AJ_CALL alljoyn_credentials_create()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_credentials) new ajn::AuthListener::Credentials();
}

void AJ_CALL alljoyn_credentials_destroy(alljoyn_credentials cred)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(cred != NULL && "cred parameter must not be NULL");
    delete (ajn::AuthListener::Credentials*)cred;
}

QCC_BOOL AJ_CALL alljoyn_credentials_isset(const alljoyn_credentials cred, uint16_t creds)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListener::Credentials*)cred)->IsSet(creds) == true ? QCC_TRUE : QCC_FALSE;
}

void AJ_CALL alljoyn_credentials_setpassword(alljoyn_credentials cred, const char* pwd)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::AuthListener::Credentials*)cred)->SetPassword(pwd);
}

void AJ_CALL alljoyn_credentials_setusername(alljoyn_credentials cred, const char* userName)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::AuthListener::Credentials*)cred)->SetUserName(userName);
}

void AJ_CALL alljoyn_credentials_setcertchain(alljoyn_credentials cred, const char* certChain)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::AuthListener::Credentials*)cred)->SetCertChain(certChain);
}

void AJ_CALL alljoyn_credentials_setprivatekey(alljoyn_credentials cred, const char* pk)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::AuthListener::Credentials*)cred)->SetPrivateKey(pk);
}

void AJ_CALL alljoyn_credentials_setlogonentry(alljoyn_credentials cred, const char* logonEntry)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::AuthListener::Credentials*)cred)->SetLogonEntry(logonEntry);
}

void AJ_CALL alljoyn_credentials_setexpiration(alljoyn_credentials cred, uint32_t expiration)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::AuthListener::Credentials*)cred)->SetExpiration(expiration);
}

const char* AJ_CALL alljoyn_credentials_getpassword(const alljoyn_credentials cred)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListener::Credentials*)cred)->GetPassword().c_str();
}

const char* AJ_CALL alljoyn_credentials_getusername(const alljoyn_credentials cred)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListener::Credentials*)cred)->GetUserName().c_str();
}

const char* AJ_CALL alljoyn_credentials_getcertchain(const alljoyn_credentials cred)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListener::Credentials*)cred)->GetCertChain().c_str();
}

const char* AJ_CALL alljoyn_credentials_getprivateKey(const alljoyn_credentials cred)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListener::Credentials*)cred)->GetPrivateKey().c_str();
}

const char* AJ_CALL alljoyn_credentials_getlogonentry(const alljoyn_credentials cred)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListener::Credentials*)cred)->GetLogonEntry().c_str();
}

uint32_t AJ_CALL alljoyn_credentials_getexpiration(const alljoyn_credentials cred)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AuthListener::Credentials*)cred)->GetExpiration();
}

void AJ_CALL alljoyn_credentials_clear(alljoyn_credentials cred)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::AuthListener::Credentials*)cred)->Clear();
}
