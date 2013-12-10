#ifndef _ALLJOYN_PROTECTED_AUTH_LISTENER_H
#define _ALLJOYN_PROTECTED_AUTH_LISTENER_H
/**
 * @file
 * This file defines a wrapper class for ajn::AuthListener that protects against asynchronous
 * deregistration of the listener instance.
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
#error Only include ProtectedAuthListener.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Thread.h>

#include <alljoyn/AuthListener.h>

#include <alljoyn/Status.h>


namespace ajn {

/**
 * This class adds a level of indirection to an AuthListener so the actual AuthListener can
 * asynchronously be set or removed safely. If the
 */
class ProtectedAuthListener : public AuthListener {

  public:

    ProtectedAuthListener() : listener(NULL), refCount(0) { }

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~ProtectedAuthListener()
    {
        Set(NULL);
    }

    /**
     * Set the listener. If one of internal listener callouts is currently being called this
     * function will block until the callout returns.
     */
    void Set(AuthListener* listener);

    /**
     * Simply wraps the call of the same name to the inner AuthListener
     */
    bool RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials);

    /**
     * Simply wraps the call of the same name to the inner AuthListener
     */
    bool VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials);

    /**
     * Simply wraps the call of the same name to the inner AuthListener
     */
    void SecurityViolation(QStatus status, const Message& msg);

    /**
     * Simply wraps the call of the same name to the inner AuthListener
     */
    void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success);

  private:

    /*
     * The inner listener that is being protected.
     */
    AuthListener* listener;

    qcc::Mutex lock;

    /*
     * Reference count so we know when the inner listener is no longer in use.
     */
    volatile int32_t refCount;
};

}

#endif
