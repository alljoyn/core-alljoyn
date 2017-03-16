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

#include <jni.h>
#include <alljoyn/PermissionConfigurationListener.h>
#include <qcc/Debug.h>

#include "JPermissionConfigurationListener.h"
#include "alljoyn_jni_helper.h"

#define QCC_MODULE "ALLJOYN_JAVA"

JPermissionConfigurationListener::JPermissionConfigurationListener(jobject jlistener) : jpcListener(NULL)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    JNIEnv* env = GetEnv();

    if (!jlistener) {
        Throw("java/lang/NullPointerException", "PermissionConfigurationListener object is null");
        return;
    }

    QCC_DbgPrintf(("%s: Taking global reference to listener %p", __FUNCTION__, jlistener));
    jpcListener = env->NewGlobalRef(jlistener);
    if (!jpcListener) {
        QCC_LogError(ER_FAIL, ("%s: Can't create new global reference", __FUNCTION__));
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jlistener);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("%s: Can't GetObjectClass()", __FUNCTION__));
        return;
    }

    MID_factoryReset = env->GetMethodID(clazz, "factoryReset", "()Lorg/alljoyn/bus/Status;");
    if (!MID_factoryReset) {
        QCC_DbgPrintf(("%s: Can't find factoryReset", __FUNCTION__));
    }

    MID_policyChanged = env->GetMethodID(clazz, "policyChanged", "()V");
    if (!MID_policyChanged) {
        QCC_DbgPrintf(("%s: Can't find policyChanged", __FUNCTION__));
    }

    MID_startManagement = env->GetMethodID(clazz, "startManagement", "()V");
    if (!MID_startManagement) {
        QCC_DbgPrintf(("%s: Can't find startManagement", __FUNCTION__));
    }

    MID_endManagement = env->GetMethodID(clazz, "endManagement", "()V");
    if (!MID_endManagement) {
        QCC_DbgPrintf(("%s: Can't find endManagement", __FUNCTION__));
    }
}

JPermissionConfigurationListener::~JPermissionConfigurationListener()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (jpcListener) {
        QCC_DbgPrintf(("%s: Releasing global reference to listener %p", __FUNCTION__, jpcListener));
        GetEnv()->DeleteGlobalRef(jpcListener);
        jpcListener = NULL;
    }
}

QStatus JPermissionConfigurationListener::FactoryReset()
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * This call out to the listener means that the DestinationFound method
     * must be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("%s: Call out to listener object and method", __FUNCTION__));
    jobject status = env->CallObjectMethod(jpcListener, MID_factoryReset);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return ER_FAIL;
    }

    jfieldID fid = env->GetFieldID(CLS_Status, "errorCode", "I");
    if (!fid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find errorCode field ID in Status", __FUNCTION__));
        return ER_FAIL;
    }

    jint jerrorCode = env->GetIntField(status, fid);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Can't get int errorcode from Status", __FUNCTION__));
        return ER_FAIL;
    }

    return static_cast<QStatus>(jerrorCode);
}

void JPermissionConfigurationListener::PolicyChanged()
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * This call out to the listener means that the DestinationFound method
     * must be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("%s: Call out to listener object and method", __FUNCTION__));
    env->CallVoidMethod(jpcListener, MID_policyChanged);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
    }

}

void JPermissionConfigurationListener::StartManagement()
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * This call out to the listener means that the DestinationFound method
     * must be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("%s: Call out to listener object and method", __FUNCTION__));
    env->CallVoidMethod(jpcListener, MID_startManagement);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
    }

}

void JPermissionConfigurationListener::EndManagement()
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * This call out to the listener means that the DestinationFound method
     * must be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("%s: Call out to listener object and method", __FUNCTION__));
    env->CallVoidMethod(jpcListener, MID_endManagement);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
    }

}
