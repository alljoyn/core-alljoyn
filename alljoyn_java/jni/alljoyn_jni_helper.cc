/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 *
 ******************************************************************************/
#include <jni.h>

#include "alljoyn_jni_helper.h"

jbyteArray ToJByteArray(const uint8_t* byteArray, size_t len)
{
    JNIEnv* env = GetEnv();
    jbyteArray array = env->NewByteArray(len);
    env->SetByteArrayRegion(array, 0, len, reinterpret_cast<const jbyte*>(byteArray));
    return array;
}

uint8_t* ToByteArray(jbyteArray array)
{
    JNIEnv* env = GetEnv();
    int len = env->GetArrayLength(array);
    if (len == 0) {
        return NULL;
    }
    uint8_t* buf = new uint8_t[len];
    env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte*>(buf));
    return buf;
}