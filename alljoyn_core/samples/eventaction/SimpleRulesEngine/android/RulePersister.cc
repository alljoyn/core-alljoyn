/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include "RulePersister.h"

void RulePersister::saveRule(Rule* rule)
{
    JNIEnv* env;
    jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
    if (JNI_EDETACHED == jret) {
        vm->AttachCurrentThread(&env, NULL);
    }

    jclass jcls = env->GetObjectClass(jobj);
    jmethodID mid = env->GetMethodID(jcls, "saveRule", "(Ljava/lang/String;)V");
    if (mid == 0) {
        LOGTHIS("Failed to get Java saveRule");
    } else {
        jstring jRule = env->NewStringUTF(rule->toString().c_str());
        env->CallVoidMethod(jobj, mid, jRule);
        env->DeleteLocalRef(jRule);
    }
    if (JNI_EDETACHED == jret) {
        vm->DetachCurrentThread();
    }
}

void RulePersister::loadRules()
{
    JNIEnv* env;
    jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
    if (JNI_EDETACHED == jret) {
        vm->AttachCurrentThread(&env, NULL);
    }

    jclass jcls = env->GetObjectClass(jobj);
    jmethodID mid = env->GetMethodID(jcls, "loadRules", "()V");
    if (mid == 0) {
        LOGTHIS("Failed to get Java loadRules");
    } else {
        env->CallVoidMethod(jobj, mid);
    }
    if (JNI_EDETACHED == jret) {
        vm->DetachCurrentThread();
    }
}

void RulePersister::clearRules()
{
    JNIEnv* env;
    jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
    if (JNI_EDETACHED == jret) {
        vm->AttachCurrentThread(&env, NULL);
    }

    jclass jcls = env->GetObjectClass(jobj);
    jmethodID mid = env->GetMethodID(jcls, "clearRules", "()V");
    if (mid == 0) {
        LOGTHIS("Failed to get Java clearRules");
    } else {
        env->CallVoidMethod(jobj, mid);
    }
    if (JNI_EDETACHED == jret) {
        vm->DetachCurrentThread();
    }
}
