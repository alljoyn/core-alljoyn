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
#include <qcc/CryptoECC.h>
#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/Debug.h>
#include <string>
#include <algorithm>

#include "alljoyn_java.h"
#include "alljoyn_jni_helper.h"

#define QCC_MODULE "ALLJOYN_JAVA"

using namespace qcc;
using namespace std;

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_create(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * Create a new C++ backing object for the Java CryptoECC.
     */
    Crypto_ECC* cryptoPtr = new Crypto_ECC();
    if (!cryptoPtr) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    SetHandle(thiz, cryptoPtr);
    if (jenv->ExceptionCheck()) {
        /*
         * delete the Crypto_ECC.
         */
        QCC_DbgPrintf(("%s: exception", __FUNCTION__));
        delete cryptoPtr;
        cryptoPtr = NULL;
    }
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_destroy(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QCC_ASSERT(cryptoPtr);
    delete cryptoPtr;

    SetHandle(thiz, NULL);
    return;
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateDHKeyPair
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateDHKeyPair(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    QStatus status = cryptoPtr->GenerateDHKeyPair();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateSPEKEKeyPair
 * Signature: (BILorg/alljoyn/bus/common/GUID128;Lorg/alljoyn/bus/common/GUID128;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateSPEKEKeyPair(JNIEnv* jenv, jobject thiz, jbyteArray jpw, jlong jpwLen, jobject jclientGUID, jobject jserviceGUID)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    jmethodID mid = jenv->GetMethodID(CLS_JAVA_UTIL_UUID, "toString", "()Ljava/lang/String;");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find UUID.toString", __FUNCTION__));
        Throw("java/lang/NoSuchMethodException", NULL);
        return;
    }

    JString cguid((jstring) CallObjectMethod(jenv, jclientGUID, mid));
    QCC_DbgPrintf(("%s: clientGUID %s", __FUNCTION__, cguid.c_str()));
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: clientGUID is null or has not been generated", __FUNCTION__));
        return;
    }

    JString sguid((jstring) CallObjectMethod(jenv, jserviceGUID, mid));
    QCC_DbgPrintf(("%s: serviceGUID %s", __FUNCTION__, sguid.c_str()));
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: serviceGUID is null or has not been generated", __FUNCTION__));
        return;
    }

    String clguid(cguid.c_str());
    String seguid(sguid.c_str());

    std::string cliguid = static_cast<std::string>(clguid);
    std::string serguid = static_cast<std::string>(seguid);

    cliguid.erase(std::remove(cliguid.begin(), cliguid.end(), '-'), cliguid.end());
    serguid.erase(std::remove(serguid.begin(), serguid.end(), '-'), serguid.end());

    clguid = cliguid;
    seguid = serguid;

    GUID128 clientGUID(clguid);
    GUID128 serviceGUID(seguid);

    uint8_t* pw = ToByteArray(jpw);

    QStatus status = cryptoPtr->GenerateSPEKEKeyPair(pw, jpwLen, clientGUID, serviceGUID);

    delete [] pw;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }

}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateSharedSecret
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;Lorg/alljoyn/bus/common/ECCSecret;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateSharedSecret(JNIEnv* jenv, jobject thiz, jobject jeccPublicKey, jobject jeccSecret)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    ECCPublicKey eccPublicKey;

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_x);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_y);

    uint8_t* eccX = ToByteArray(jeccX);
    uint8_t* eccY = ToByteArray(jeccY);

    if (jenv->ExceptionCheck()) {
        delete [] eccX;
        delete [] eccY;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));
    delete [] eccX;
    delete [] eccY;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    ECCSecret* eccsecret = GetHandle<ECCSecret*>(jeccSecret);
    if (jenv->ExceptionCheck() || eccsecret == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    status = cryptoPtr->GenerateSharedSecret(&eccPublicKey, eccsecret);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    getDHPublicKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_getDHPublicKey(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return NULL;
    }

    const ECCPublicKey* eccPublicKey = cryptoPtr->GetDHPublicKey();
    if (!eccPublicKey) {
        QCC_LogError(ER_FAIL, ("%s: ECCPublicKey wasn't generated", __FUNCTION__));
        Throw("java/lang/NullPointerException", "ECCPublicKey wasn't generated");
        return NULL;
    }

    jbyteArray arrayX = ToJByteArray(eccPublicKey->GetX(), eccPublicKey->GetCoordinateSize());
    jbyteArray arrayY = ToJByteArray(eccPublicKey->GetY(), eccPublicKey->GetCoordinateSize());

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, MID_ECCPublicKey_cnstrctr, arrayX, arrayY);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Couldn't make jobject", __FUNCTION__));
        jenv->DeleteLocalRef(retObj);
        jenv->DeleteLocalRef(arrayX);
        jenv->DeleteLocalRef(arrayY);
        return NULL;
    }

    return retObj;
}

/* * Class:     org_alljoyn_bus_common_CryptoECC * Method:    setDHPublicKey
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDHPublicKey(JNIEnv* jenv, jobject thiz, jobject jeccPublicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    ECCPublicKey eccPublicKey;

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_x);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_y);

    uint8_t* eccX = ToByteArray(jeccX);
    uint8_t* eccY = ToByteArray(jeccY);

    if (jenv->ExceptionCheck()) {
        delete [] eccX;
        delete [] eccY;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));
    delete [] eccX;
    delete [] eccY;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    cryptoPtr->SetDHPublicKey(&eccPublicKey);
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    getDHPrivateKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPrivateKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_getDHPrivateKey(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return NULL;
    }

    const ECCPrivateKey* eccPrivateKey = cryptoPtr->GetDHPrivateKey();
    if (!eccPrivateKey) {
        QCC_LogError(ER_FAIL, ("%s: ECCPrivateKey wasn't generated", __FUNCTION__));
        Throw("java/lang/NullPointerException", "ECCPrivateKey wasn't generated");
        return NULL;
    }

    jbyteArray arrayD = jenv->NewByteArray(eccPrivateKey->GetSize());
    jenv->SetByteArrayRegion(arrayD, 0, eccPrivateKey->GetSize(), reinterpret_cast<const jbyte*>(eccPrivateKey->GetD()));

    jobject retObj = jenv->NewObject(CLS_ECCPrivateKey, MID_ECCPrivateKey_cnstrctr, arrayD);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Couldn't make jobject", __FUNCTION__));
        return NULL;
    }

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    setDHPrivateKey
 * Signature: (Lorg/alljoyn/bus/common/ECCPrivateKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDHPrivateKey(JNIEnv* jenv, jobject thiz, jobject jeccPrivateKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    ECCPrivateKey eccPrivateKey;

    jbyteArray jeccD = (jbyteArray) jenv->GetObjectField(jeccPrivateKey, FID_ECCPrivateKey_d);

    uint8_t* eccD = ToByteArray(jeccD);
    if (jenv->ExceptionCheck()) {
        delete [] eccD;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QStatus status = eccPrivateKey.Import(eccD, jenv->GetArrayLength(jeccD));

    delete [] eccD;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    cryptoPtr->SetDHPrivateKey(&eccPrivateKey);
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    getDSAPublicKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_getDSAPublicKey(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return NULL;
    }

    const ECCPublicKey* eccPublicKey = cryptoPtr->GetDSAPublicKey();
    if (!eccPublicKey) {
        QCC_LogError(ER_FAIL, ("%s: ECCPublicKey wasn't generated", __FUNCTION__));
        Throw("java/lang/NullPointerException", "ECCPublicKey wasn't generated");
        return NULL;
    }

    jbyteArray arrayX = ToJByteArray(eccPublicKey->GetX(), eccPublicKey->GetCoordinateSize());
    jbyteArray arrayY = ToJByteArray(eccPublicKey->GetY(), eccPublicKey->GetCoordinateSize());

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, MID_ECCPublicKey_cnstrctr, arrayX, arrayY);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Couldn't make jobject", __FUNCTION__));
        jenv->DeleteLocalRef(retObj);
        jenv->DeleteLocalRef(arrayX);
        jenv->DeleteLocalRef(arrayY);
        return NULL;
    }

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    setDSAPublicKey
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDSAPublicKey(JNIEnv* jenv, jobject thiz, jobject jeccPublicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    ECCPublicKey eccPublicKey;

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_x);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_y);

    uint8_t* eccX = ToByteArray(jeccX);
    uint8_t* eccY = ToByteArray(jeccY);

    if (jenv->ExceptionCheck()) {
        delete [] eccX;
        delete [] eccY;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));
    delete [] eccX;
    delete [] eccY;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    cryptoPtr->SetDSAPublicKey(&eccPublicKey);
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    getDSAPrivateKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPrivateKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_getDSAPrivateKey(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return NULL;
    }

    const ECCPrivateKey* eccPrivateKey = cryptoPtr->GetDSAPrivateKey();
    if (!eccPrivateKey) {
        QCC_LogError(ER_FAIL, ("%s: ECCPrivateKey wasn't generated", __FUNCTION__));
        Throw("java/lang/NullPointerException", "ECCPrivateKey wasn't generated");
        return NULL;
    }

    jbyteArray arrayD = jenv->NewByteArray(eccPrivateKey->GetSize());
    jenv->SetByteArrayRegion(arrayD, 0, eccPrivateKey->GetSize(), reinterpret_cast<const jbyte*>(eccPrivateKey->GetD()));

    jobject retObj = jenv->NewObject(CLS_ECCPrivateKey, MID_ECCPrivateKey_cnstrctr, arrayD);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Couldn't make jobject", __FUNCTION__));
        return NULL;
    }

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    setDSAPrivateKey
 * Signature: (Lorg/alljoyn/bus/common/ECCPrivateKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDSAPrivateKey(JNIEnv* jenv, jobject thiz, jobject jeccPrivateKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    jbyteArray jeccD = (jbyteArray) jenv->GetObjectField(jeccPrivateKey, FID_ECCPrivateKey_d);

    uint8_t* eccD = ToByteArray(jeccD);
    if (jenv->ExceptionCheck()) {
        delete [] eccD;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    ECCPrivateKey eccPrivateKey;

    QStatus status = eccPrivateKey.Import(eccD, jenv->GetArrayLength(jeccD));

    delete [] eccD;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    cryptoPtr->SetDSAPrivateKey(&eccPrivateKey);
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateDSAKeyPair
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateDSAKeyPair(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    QStatus status = cryptoPtr->GenerateDSAKeyPair();
    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSASignDigest
 * Signature: ([BS)Lorg/alljoyn/bus/common/ECCSignature;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSASignDigest(JNIEnv* jenv, jobject thiz, jbyteArray jdigest, jint jdigestlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return NULL;
    }

    ECCSignature eccsignature;

    uint8_t* digest = ToByteArray(jdigest);

    QStatus status = cryptoPtr->DSASignDigest(digest, jdigestlen, &eccsignature);

    delete [] digest;
    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    size_t eccbufferlen = eccsignature.GetSize();
    uint8_t* eccbuffer = new uint8_t[eccbufferlen];

    status = eccsignature.Export(eccbuffer, &eccbufferlen);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        delete [] eccbuffer;
        return NULL;
    }

    jbyteArray jeccsigr = ToJByteArray(eccbuffer, eccbufferlen / 2);
    jbyteArray jeccsigs = ToJByteArray(eccbuffer + eccbufferlen / 2, eccbufferlen / 2);

    jobject retObj = jenv->NewObject(CLS_ECCSignature, MID_ECCSignature_cnstrctr, jeccsigr, jeccsigs);

    delete [] eccbuffer;
    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSASign
 * Signature: ([BS)Lorg/alljoyn/bus/common/ECCSignature;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSASign(JNIEnv* jenv, jobject thiz, jbyteArray jbuffer, jint jbufferlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return NULL;
    }

    ECCSignature eccsignature;

    uint8_t* buffer = ToByteArray(jbuffer);

    QStatus status = cryptoPtr->DSASign(buffer, jbufferlen, &eccsignature);

    delete [] buffer;
    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    size_t eccbufferlen = eccsignature.GetSize();
    uint8_t* eccbuffer = new uint8_t[eccbufferlen];

    status = eccsignature.Export(eccbuffer, &eccbufferlen);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        delete [] eccbuffer;
        return NULL;
    }

    jbyteArray jeccsigr = ToJByteArray(eccbuffer, eccbufferlen / 2);
    jbyteArray jeccsigs = ToJByteArray(eccbuffer + eccbufferlen / 2, eccbufferlen / 2);

    jobject retObj = jenv->NewObject(CLS_ECCSignature, MID_ECCSignature_cnstrctr, jeccsigr, jeccsigs);

    delete [] eccbuffer;
    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSAVerifyDigest
 * Signature: ([BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSAVerifyDigest(JNIEnv* jenv, jobject thiz, jbyteArray jdigest, jint jdigestlen, jobject jeccSignature)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    ECCSignature eccsignature;

    jbyteArray jsigr = (jbyteArray) jenv->GetObjectField(jeccSignature, FID_ECCSignature_r);
    jbyteArray jsigs = (jbyteArray) jenv->GetObjectField(jeccSignature, FID_ECCSignature_s);

    jlong jsigrlen = jenv->GetArrayLength(jsigr);
    jlong jsigslen = jenv->GetArrayLength(jsigs);

    uint8_t* sigr = ToByteArray(jsigr);
    uint8_t* sigs = ToByteArray(jsigs);

    QStatus status = eccsignature.Import(sigr, jsigrlen, sigs, jsigslen);
    delete [] sigr;
    delete [] sigs;

    if (status != ER_OK) {
        QCC_LogError(status, ("import fail %d", jsigrlen));
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    uint8_t* digest = ToByteArray(jdigest);

    status = cryptoPtr->DSAVerifyDigest(digest, jdigestlen, &eccsignature);

    delete [] digest;
    if (status != ER_OK) {
        QCC_LogError(status, ("verifydigest fail"));
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSAVerify
 * Signature: ([BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSAVerify(JNIEnv* jenv, jobject thiz, jbyteArray jbuffer, jint jbufferlen, jobject jeccSignature)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: Exception or NULL pointer", __FUNCTION__));
        return;
    }

    ECCSignature eccsignature;

    jbyteArray jsigr = (jbyteArray) jenv->GetObjectField(jeccSignature, FID_ECCSignature_r);
    jbyteArray jsigs = (jbyteArray) jenv->GetObjectField(jeccSignature, FID_ECCSignature_s);

    jlong jsigrlen = jenv->GetArrayLength(jsigr);
    jlong jsigslen = jenv->GetArrayLength(jsigs);

    uint8_t* sigr = ToByteArray(jsigr);
    uint8_t* sigs = ToByteArray(jsigs);

    QStatus status = eccsignature.Import(sigr, jsigrlen, sigs, jsigslen);
    delete [] sigr;
    delete [] sigs;

    if (status != ER_OK) {
        QCC_LogError(status, ("import fail %d", jsigrlen));
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    uint8_t* buffer = ToByteArray(jbuffer);

    status = cryptoPtr->DSAVerify(buffer, jbufferlen, &eccsignature);

    delete [] buffer;
    if (status != ER_OK) {
        QCC_LogError(status, ("verifydigest fail"));
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}