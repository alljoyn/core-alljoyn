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
#ifndef _ALLJOYN_ALLJOYN_JNI_HELPER_H
#define _ALLJOYN_ALLJOYN_JNI_HELPER_H

#include <jni.h>
#include <stdlib.h>
#include <alljoyn/MsgArg.h>

extern jclass CLS_BusException;
extern jclass CLS_Status;

extern jclass CLS_ECCPublicKey;
extern jclass CLS_ECCPrivateKey;
extern jclass CLS_ECCSignature;
extern jclass CLS_KeyInfoNISTP256;
extern jclass CLS_JAVA_UTIL_UUID;
extern jclass CLS_ErrorReplyBusException;
extern jclass CLS_BusException;
extern jclass CLS_AboutDataListener;
extern jclass CLS_PermissionConfigurator;
extern jclass CLS_PermissionConfiguratorApplicationState;
extern jclass CLS_CertificateX509CertificateType;
extern jclass CLS_CertificateX509;
extern jclass CLS_CertificateId;

extern jmethodID MID_ECCPublicKey_cnstrctr;
extern jmethodID MID_ECCPrivateKey_cnstrctr;
extern jmethodID MID_ECCSignature_cnstrctr;
extern jmethodID MID_PermissionConfigurator_cnstrctr;
extern jmethodID MID_KeyInfoNISTP256_cnstrctr;
extern jmethodID MID_KeyInfoNISTP256_setPublicKey;

extern jfieldID FID_ECCPrivateKey_d;
extern jfieldID FID_ECCPublicKey_x;
extern jfieldID FID_ECCPublicKey_y;
extern jfieldID FID_ECCSignature_r;
extern jfieldID FID_ECCSignature_s;

extern jobject PermissionConfiguratorApplicationState_NOT_CLAIMABLE;
extern jobject PermissionConfiguratorApplicationState_CLAIMABLE;
extern jobject PermissionConfiguratorApplicationState_CLAIMED;
extern jobject PermissionConfiguratorApplicationState_NEED_UPDATE;

extern jobject CertificateX509Type_UNRESTRICTED;
extern jobject CertificateX509Type_IDENTITY;
extern jobject CertificateX509Type_MEMBERSHIP;
extern jobject CertificateX509Type_INVALID;

/**
 * Marshal an Object into a MsgArg.
 *
 * @param[in] signature the signature of the Object
 * @param[in] jarg the Object
 * @param[in] arg the MsgArg to marshal into
 * @return the marshalled MsgArg or NULL if the marshalling failed.  This will
 *         be the same as @param arg if marshalling succeeded.
 */
ajn::MsgArg* Marshal(const char* signature, jobject jarg, ajn::MsgArg* arg);

/**
 * Get a valid JNIEnv pointer.
 *
 * A JNIEnv pointer is only valid in an associated JVM thread.  In a callback
 * function (from C++), there is no associated JVM thread, so we need to obtain
 * a valid JNIEnv.  This is a helper function to make that happen.
 *
 * @return The JNIEnv pointer valid in the calling context.
 */
JNIEnv* GetEnv(jint* result = 0);

/**
 * Set the native C++ handle of a given Java object.
 *
 * If we have an object that has a native counterpart, we need a way to get at
 * the native object from the Java object.  We do this by storing the native
 * pointer as an opaque handle in a Java field named "handle".  We use Java
 * reflection to determine the field out and set the handle value.
 *
 * @param jobj The Java object which needs to have its handle set.
 * @param handle The pointer to the C++ object which is the handle value.
 *
 * @warning May throw an exception.
 */
void SetHandle(jobject jobj, void* handle);

/**
 * Helper function to throw an exception
 */
void Throw(const char* name, const char* msg);

/**
 * A helper class to wrap local references ensuring proper release.
 */
template <class T>
class JLocalRef {
  public:
    JLocalRef() : jobj(NULL) { }
    JLocalRef(const T& obj) : jobj(obj) { }
    ~JLocalRef() { if (jobj) { GetEnv()->DeleteLocalRef(jobj); } }
    JLocalRef& operator=(T obj)
    {
        if (jobj) {
            GetEnv()->DeleteLocalRef(jobj);
        }
        jobj = obj;
        return *this;
    }
    operator T() { return jobj; }
    T move()
    {
        T ret = jobj;
        jobj = NULL;
        return ret;
    }
  private:
    T jobj;
};

/**
 * Get the native C++ handle of a given Java object.
 *
 * If we have an object that has a native counterpart, we need a way to get at
 * the native object from the Java object.  We do this by storing the native
 * pointer as an opaque handle in a Java field named "handle".  We use Java
 * reflection to pull the field out and return the handle value.
 *
 * Think of this handle as the counterpart to the object reference found in
 * the C++ objects that need to call into Java.  Java objects use the handle to
 * get at the C++ objects, and C++ objects use an object reference to get at
 * the Java objects.
 *
 * @return The handle value as a pointer.  NULL is a valid value.
 *
 * @warning This method makes native calls which may throw exceptions.  In the
 *          usual idiom, exceptions must be checked for explicitly by the caller
 *          after *every* call to GetHandle.  Since NULL is a valid value to
 *          return, validity of the returned pointer must be checked as well.
 */
template <typename T>
T GetHandle(jobject jobj)
{
    JNIEnv* env = GetEnv();
    if (!jobj) {
        Throw("java/lang/NullPointerException", "failed to get native handle on null object");
        return NULL;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jobj);
    jfieldID fid = env->GetFieldID(clazz, "handle", "J");
    void* handle = NULL;
    if (fid) {
        handle = (void*)env->GetLongField(jobj, fid);
    }

    return reinterpret_cast<T>(handle);
}

jbyteArray ToJByteArray(const uint8_t* byteArray, size_t len);

/**
 * Users of this function need to free the memory of the returned pointer
 * after using it.
 *
 * @return a byte array
 *
 */
uint8_t* ToByteArray(jbyteArray jbArray);

/*
 * Note that some JNI calls do not set the returned value to NULL when
 * an exception occurs.  In that case we must explicitly set the
 * reference here to NULL to prevent calling DeleteLocalRef on an
 * invalid reference.
 *
 * The list of such functions used in this file is:
 * - CallObjectMethod
 * - CallStaticObjectMethod
 * - GetObjectArrayElement
 */
jobject CallObjectMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...);

/**
 * Helper function to wrap StringUTFChars to ensure proper release of resource.
 *
 * @warning NULL is a valid value, so exceptions must be checked for explicitly
 * by the caller after constructing the JString.
 */
class JString {
  public:
    JString(jstring s);
    virtual ~JString();
    const char* c_str() { return str; }
  protected:
    jstring jstr;
    const char* str;
  private:
    JString(const JString& other);
    JString& operator =(const JString& other);
};

/**
 * A scoped JNIEnv pointer to ensure proper release.
 */
class JScopedEnv {
  public:
    JScopedEnv();
    ~JScopedEnv();
    JNIEnv* operator->() { return env; }
    operator JNIEnv*() { return env; }
  private:
    JScopedEnv(const JScopedEnv& other);
    JScopedEnv& operator =(const JScopedEnv& other);

    JNIEnv* env;
    jint detached;
};

jobject GetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index);
#endif