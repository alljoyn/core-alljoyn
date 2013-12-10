/**
 * @file
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/Debug.h>

#include "ProtectedAuthListener.h"

#include <list>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace ajn;
using namespace qcc;


static const uint32_t ASYNC_AUTH_TIMEOUT = (120 * 1000);

class AuthContext {
  public:
    AuthContext(AuthListener* listener, AuthListener::Credentials* credentials) : listener(listener), accept(false), credentials(credentials) { }

    AuthListener* listener;
    bool accept;
    AuthListener::Credentials* credentials;
    qcc::Event event;
};

class AsyncTracker {
  public:

    static AuthContext* Allocate(AuthListener* listener, AuthListener::Credentials* credentials)
    {
        if (IncrementAndFetch(&refs) == 1) {
            /* Handle race where a remove is in progress */
            while (self) {
                qcc::Sleep(1);
            }
            self = new AsyncTracker();
            QCC_DbgHLPrintf(("Allocated AsyncTracker %#x", self));
        } else {
            /* Handle race where another add is in progress */
            while (!self) {
                qcc::Sleep(1);
            }
            QCC_DbgHLPrintf(("AsyncTracker %#x ref count = %d", self, refs));
        }
        AuthContext* context = new AuthContext(listener, credentials);
        self->lock.Lock();
        self->contexts.push_back(context);
        self->lock.Unlock();
        return context;
    }

    static bool Trigger(AuthContext* context, bool accept, AuthListener::Credentials* credentials)
    {
        bool found = false;
        /* Ensure that self has been Allocated in AsysncTracker::Allocate */
        if (self) {
            if (IncrementAndFetch(&refs) > 1) {
                self->lock.Lock();
                for (std::list<AuthContext*>::iterator it = self->contexts.begin(); it != self->contexts.end(); ++it) {
                    if (*it == context) {
                        self->contexts.erase(it);
                        context->accept = accept;
                        if (accept && credentials && context->credentials) {
                            *context->credentials = *credentials;
                        }
                        /*
                         * Set the event to unblock the waiting thread
                         */
                        context->event.SetEvent();
                        found = true;
                        /*
                         * Decrement to balance increment in AsysncTracker::Allocate
                         */
                        DecrementAndFetch(&refs);
                        break;
                    }
                }
                self->lock.Unlock();
            }
            if (DecrementAndFetch(&refs) == 0) {
                QCC_DbgHLPrintf(("Released AsyncTracker %#x", self));
                delete self;
                self = NULL;
            }
        }
        return found;
    }

    static void Release(AuthContext* context)
    {
        Trigger(context, false, NULL);
        delete context;
    }

    static void RemoveAll(AuthListener* listener)
    {
        /* Ensure that self has been Allocated in AsysncTracker::Allocate */
        if (self) {
            if (IncrementAndFetch(&refs) > 1) {
                self->lock.Lock();
                for (std::list<AuthContext*>::iterator it = self->contexts.begin(); it != self->contexts.end();) {
                    AuthContext* context = *it;
                    if (context->listener == listener) {
                        /*
                         * Set the event to unblock the waiting thread
                         */
                        context->accept = false;
                        context->event.SetEvent();
                        it = self->contexts.erase(it);
                        /*
                         * Decrement to balance increment in AsyncTracker::Allocate
                         */
                        DecrementAndFetch(&refs);
                    } else {
                        ++it;
                    }
                }
                self->lock.Unlock();
            }
            if (DecrementAndFetch(&refs) == 0) {
                delete self;
                self = NULL;
            }
        }
    }

  private:
    std::list<AuthContext*> contexts;
    qcc::Mutex lock;

    static int32_t refs;
    static AsyncTracker* self;
};

int32_t AsyncTracker::refs = 0;
AsyncTracker* AsyncTracker::self = NULL;

void ProtectedAuthListener::Set(AuthListener* listener)
{
    lock.Lock(MUTEX_CONTEXT);
    /*
     * Clear the current listener to prevent any more calls to this listener.
     */
    AuthListener* goner = this->listener;
    this->listener = NULL;
    /*
     * Remove the listener unblocking any threads that might be waiting
     */
    if (goner) {
        AsyncTracker::RemoveAll(goner);
    }
    /*
     * Poll and sleep until the current listener is no longer in use.
     */
    while (refCount) {
        lock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(10);
        lock.Lock(MUTEX_CONTEXT);
    }
    /*
     * Now set the new listener
     */
    this->listener = listener;
    lock.Unlock(MUTEX_CONTEXT);
}

bool ProtectedAuthListener::RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials)
{
    bool ok = false;
    lock.Lock(MUTEX_CONTEXT);
    AuthListener* listener = this->listener;
    ++refCount;
    lock.Unlock(MUTEX_CONTEXT);
    if (listener) {
        AuthContext* context = AsyncTracker::Allocate(listener, &credentials);
        /*
         * First try the asynch implementation
         */
        QStatus status = listener->RequestCredentialsAsync(authMechanism, peerName, authCount, userName, credMask, (void*)context);
        if (status != ER_OK) {
            if (status == ER_NOT_IMPLEMENTED) {
                ok = listener->RequestCredentials(authMechanism, peerName, authCount, userName, credMask, credentials);
            }
        } else {
            ok = (Event::Wait(context->event, ASYNC_AUTH_TIMEOUT) == ER_OK) && context->accept;
        }
        AsyncTracker::Release(context);
    }
    lock.Lock(MUTEX_CONTEXT);
    --refCount;
    lock.Unlock(MUTEX_CONTEXT);
    return ok;
}

bool ProtectedAuthListener::VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials)
{
    bool ok = false;
    lock.Lock(MUTEX_CONTEXT);
    AuthListener* listener = this->listener;
    ++refCount;
    lock.Unlock(MUTEX_CONTEXT);
    if (listener) {
        AuthContext* context = AsyncTracker::Allocate(listener, NULL);
        /*
         * First try the asynch implementation
         */
        QStatus status = listener->VerifyCredentialsAsync(authMechanism, peerName, credentials, (void*)context);
        if (status != ER_OK) {
            if (status == ER_NOT_IMPLEMENTED) {
                ok = listener->VerifyCredentials(authMechanism, peerName, credentials);
            }
        } else {
            ok = (Event::Wait(context->event, ASYNC_AUTH_TIMEOUT) == ER_OK) && context->accept;
        }
        AsyncTracker::Release(context);
    }
    lock.Lock(MUTEX_CONTEXT);
    --refCount;
    lock.Unlock(MUTEX_CONTEXT);
    return ok;
}

void ProtectedAuthListener::SecurityViolation(QStatus status, const Message& msg)
{
    lock.Lock(MUTEX_CONTEXT);
    AuthListener* listener = this->listener;
    ++refCount;
    lock.Unlock(MUTEX_CONTEXT);
    if (listener) {
        listener->SecurityViolation(status, msg);
    }
    lock.Lock(MUTEX_CONTEXT);
    --refCount;
    lock.Unlock(MUTEX_CONTEXT);
}

void ProtectedAuthListener::AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
{
    lock.Lock(MUTEX_CONTEXT);
    AuthListener* listener = this->listener;
    ++refCount;
    lock.Unlock(MUTEX_CONTEXT);
    if (listener) {
        listener->AuthenticationComplete(authMechanism, peerName, success);
    }
    lock.Lock(MUTEX_CONTEXT);
    --refCount;
    lock.Unlock(MUTEX_CONTEXT);
}

/*
 * Static handler for auth credential response
 */
QStatus AuthListener::RequestCredentialsResponse(void* context, bool accept, Credentials& credentials)
{
    return AsyncTracker::Trigger((AuthContext*)context, accept, &credentials) ? ER_OK : ER_TIMEOUT;
}

/*
 * Static handler for auth credential verification
 */
QStatus AuthListener::VerifyCredentialsResponse(void* context, bool accept)
{
    return AsyncTracker::Trigger((AuthContext*)context, accept, NULL) ? ER_OK : ER_TIMEOUT;
}
