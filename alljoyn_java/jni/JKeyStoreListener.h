/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 *
 ******************************************************************************/
#ifndef _ALLJOYN_JKEYSTORELISTENER_H
#define _ALLJOYN_JKEYSTORELISTENER_H

#include <jni.h>

/**
 * The C++ class that imlements the KeyStoreListener functionality.
 *
 * For historical reasons, the KeyStoreListener follows a different pattern than
 * most of the listeners found in the bindings. Typically there is a one-to-one
 * correspondence between the methods of the C++ listener objects and the Java
 * listener objects.  That is not the case here.
 *
 * The C++ object has two methods, LoadRequest and StoreRequest, which take a
 * reference to a C++ KeyStore object.  The Java bindings break these requests
 * out into more primitive operations.  The upside is that this approach is
 * thought to correspond more closely to the "Java Way."  The downsides are that
 * Java clients work differently than other clients, and by breaking the operations
 * up into more primitive calls, we have created possible consistency problems.
 *
 * A LoadRequest callback to the C++ object is implemented as the following call
 * sequence:
 *
 * - Call into the Java client KeyStoreListener.getKeys() to get the keys from
 *   a local KeyStore, typically a filesystem operation.
 * - Call into the Java client KeyStoreListener.getPassword() to get the
 *   password used to encrypt the keys.  This is remembered somehow, probably
 *   needing a filesystem operation to recall.
 * - Call into the Bindings' BusAttachment.encode() to encode the keys byte
 *   array as UTF-8 characters.  This is a quick local operation.
 * - Call into the C++ KeyStoreListener::PutKeys() to give the encoded keys
 *   and password back to AllJoyn which passes them on to the authentication
 *   engine.
 *
 * The KeyStore and KeyStoreListener are responsible for ensuring the
 * consistency of the information, in what might be a farily complicated
 * way.  Here in the bindings we don't attempt this, but trust that what we
 * get will make sense.
 *
 * A StoreRequest callback to the C++ object is implemented as one call into
 * the client Java object, but the keys are provided as a byte array instead
 * of as a reference to a key store object, and the method name called is
 * changed from the C++ version.
 *
 * - Call into C++ KeyStoreListener::GetKeys to get the newly updated keys
 *   from AllJoyn.
 * - Call into the Java client KeyStoreListener.putKeys() to save the keys
 *   into the local KeyStore, probably using a filesystem operation.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.
 *
 * Think of the weak reference as the counterpart to the handle pointer found in
 * the Java objects that need to call into C++.  Java objects use the handle to
 * get at the C++ objects, and C++ objects use a weak object reference to get at
 * the Java objects.
 *
 * This object translates C++ callbacks from the KeyStoreListener to its Java
 * counterpart.  Because of this, the constructor performs reflection on the
 * provided Java object to determine the methods that need to be called.  When
 * The callback from C++ is executed, we make corresponding Java calls using
 * the weak reference to the java object and the reflection information we
 * discovered in the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 */
class JKeyStoreListener : public ajn::KeyStoreListener {
  public:
    JKeyStoreListener(jobject jlistener);
    ~JKeyStoreListener();
    QStatus LoadRequest(ajn::KeyStore& keyStore);
    QStatus StoreRequest(ajn::KeyStore& keyStore);
  private:
    JKeyStoreListener(const JKeyStoreListener& other);
    JKeyStoreListener& operator =(const JKeyStoreListener& other);
    jweak jkeyStoreListener;
    jmethodID MID_getKeys;
    jmethodID MID_getPassword;
    jmethodID MID_putKeys;
    jmethodID MID_encode;
};

#endif
