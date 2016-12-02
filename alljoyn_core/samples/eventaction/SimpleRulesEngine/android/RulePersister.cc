/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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