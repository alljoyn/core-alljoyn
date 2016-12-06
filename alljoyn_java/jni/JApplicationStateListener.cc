/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
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

#include <jni.h>
#include <alljoyn/PermissionConfigurationListener.h>
#include <alljoyn/PermissionConfigurator.h>
#include <qcc/Debug.h>
#include <qcc/CryptoECC.h>

#include "JApplicationStateListener.h"
#include "alljoyn_jni_helper.h"

#define QCC_MODULE "ALLJOYN_JAVA"

using namespace ajn;
using namespace qcc;

JApplicationStateListener::JApplicationStateListener(jobject jlistener) : jasListener(NULL)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    JNIEnv* env = GetEnv();

    if (!jlistener) {
        QCC_LogError(ER_FAIL, ("%s: jlistener null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "ApplicationStateListener object is null");
        return;
    }

    QCC_DbgPrintf(("%s: Taking global reference to listener %p", __FUNCTION__, jlistener));
    jasListener = env->NewGlobalRef(jlistener);
    if (!jasListener) {
        QCC_LogError(ER_FAIL, ("%s: Can't create new global reference", __FUNCTION__));
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jlistener);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("%s: Can't GetObjectClass()", __FUNCTION__));
        return;
    }

    MID_state = env->GetMethodID(clazz, "state", "(Ljava/lang/String;Lorg/alljoyn/bus/common/KeyInfoNISTP256;Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;)V");
    if (!MID_state) {
        QCC_DbgPrintf(("%s: Can't find state", __FUNCTION__));
    }
}

JApplicationStateListener::~JApplicationStateListener()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (jasListener) {
        QCC_DbgPrintf(("%s: Releasing global reference to listener %p", __FUNCTION__, jasListener));
        GetEnv()->DeleteGlobalRef(jasListener);
        jasListener = NULL;
    }
}

void JApplicationStateListener::State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * This call out to the listener means that the state  method
     * must be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("%s: Call out to listener object and method", __FUNCTION__));

    jstring jbusName = env->NewStringUTF(busName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    jobject jstate = NULL;
    if (state == PermissionConfigurator::ApplicationState::NOT_CLAIMABLE) {
        jstate = PermissionConfiguratorApplicationState_NOT_CLAIMABLE;
    } else if (state == PermissionConfigurator::ApplicationState::CLAIMABLE) {
        jstate = PermissionConfiguratorApplicationState_CLAIMABLE;
    } else if (state == PermissionConfigurator::ApplicationState::CLAIMED) {
        jstate = PermissionConfiguratorApplicationState_CLAIMED;
    } else if (state == PermissionConfigurator::ApplicationState::NEED_UPDATE) {
        jstate = PermissionConfiguratorApplicationState_NEED_UPDATE;
    }

    const ECCPublicKey* pubKey = publicKeyInfo.GetPublicKey();
    if (pubKey == NULL) {
        QCC_LogError(ER_FAIL, ("%s: pubKey is null", __FUNCTION__));
        return;
    }

    jobject jpublicKeyInfo = env->NewObject(CLS_KeyInfoNISTP256, MID_KeyInfoNISTP256_cnstrctr);

    jbyteArray arrayX = ToJByteArray(pubKey->GetX(), pubKey->GetCoordinateSize());
    jbyteArray arrayY = ToJByteArray(pubKey->GetY(), pubKey->GetCoordinateSize());

    jobject jpublicKey = env->NewObject(CLS_ECCPublicKey, MID_ECCPublicKey_cnstrctr, arrayX, arrayY);

    env->CallVoidMethod(jpublicKeyInfo, MID_KeyInfoNISTP256_setPublicKey, jpublicKey);

    env->CallVoidMethod(jasListener, MID_state, jbusName, jpublicKeyInfo, jstate);
}
