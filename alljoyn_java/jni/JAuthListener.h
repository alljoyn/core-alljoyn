/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 *
 ******************************************************************************/
#ifndef _ALLJOYN_JAUTHLISTENER_H
#define _ALLJOYN_JAUTHLISTENER_H

#include <jni.h>
#include <alljoyn/AuthListener.h>

class JBusAttachment;

/**
 * The C++ class that imlements the AuthListener functionality.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.  See the member variable
 * jbusListener for this reference.  The bindings hold separate strong references
 * to prevent the listener from being garbage collected in the presence of the
 * anonymous class idiom.
 *
 * Think of the weak reference as the counterpart to the handle pointer found in
 * the Java objects that need to call into C++.  Java objects use the handle to
 * get at the C++ objects, and C++ objects use a weak object reference to get at
 * the Java objects.
 *
 * This object translates C++ callbacks from the AuthListener to its Java
 * counterpart.  Because of this, the constructor performs reflection on the
 * provided Java object to determine the methods that need to be called.  When
 * The callback from C++ is executed, we make corresponding Java calls using the
 * weak reference to the java object and the reflection information we
 * discovered in the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 */
class JAuthListener : public ajn::AuthListener {
  public:
    JAuthListener(JBusAttachment* ba, jobject jlistener);
    ~JAuthListener();
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount,
                            const char* userName, uint16_t credMask, Credentials& credentials);
    bool VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials);
    void SecurityViolation(QStatus status, const ajn::Message& msg);
    void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success);
  private:
    JAuthListener(const JAuthListener& other);
    JAuthListener& operator =(const JAuthListener& other);

    JBusAttachment* busPtr;
    jweak jauthListener;
    jmethodID MID_requestCredentials;
    jmethodID MID_verifyCredentials;
    jmethodID MID_securityViolation;
    jmethodID MID_authenticationComplete;
};

#endif