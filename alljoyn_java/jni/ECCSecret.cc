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
#include <qcc/Debug.h>
#include <qcc/CryptoECC.h>
#include <qcc/Crypto.h>

#include "alljoyn_jni_helper.h"
#include "alljoyn_java.h"

#define QCC_MODULE "ALLJOYN_JAVA"

using namespace ajn;
using namespace qcc;

/*
 * Class:     org_alljoyn_bus_common_ECCSecret
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_ECCSecret_create(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * Create a new C++ backing object for the Java ECCSecret.
     */
    ECCSecret* eccsecret = new ECCSecret();
    if (!eccsecret) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    SetHandle(thiz, eccsecret);
    if (jenv->ExceptionCheck()) {
        /*
         * delete the ECCSecret.
         */
        QCC_DbgPrintf(("%s: exception", __FUNCTION__));
        delete eccsecret;
        eccsecret = NULL;
    }
}

/*
 * Class:     org_alljoyn_bus_common_ECCSecret
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_ECCSecret_destroy(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ECCSecret* eccsecret = GetHandle<ECCSecret*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QCC_ASSERT(eccsecret);
    delete eccsecret;

    SetHandle(thiz, NULL);
    return;
}

/*
 * Class:     org_alljoyn_bus_common_ECCSecret
 * Method:    derivePreMasterSecret
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_ECCSecret_derivePreMasterSecret(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ECCSecret* eccsecret = GetHandle<ECCSecret*>(thiz);
    if (jenv->ExceptionCheck() || eccsecret == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return NULL;
    }

    uint8_t pmsecret[Crypto_SHA256::DIGEST_SIZE];

    QStatus status = eccsecret->DerivePreMasterSecret(pmsecret, sizeof(pmsecret));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    return ToJByteArray(pmsecret, sizeof(pmsecret));
}
