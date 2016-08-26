/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
 *
 ******************************************************************************/

#include <jni.h>
#include <qcc/CryptoECC.h>
#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/Debug.h>

#include "alljoyn_java.h"
#include "alljoyn_jni_helper.h"

#define QCC_MODULE "ALLJOYN_JAVA_CRYPTOECC"

using namespace qcc;
using namespace std;

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_create
  (JNIEnv * jenv, jobject thiz)
{
    QCC_DbgPrintf(("CryptoECC_create"));

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
        QCC_DbgPrintf(("CryptoECC_create(): exception"));
        delete cryptoPtr;
        cryptoPtr = NULL;
    }
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateDHKeyPair
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateDHKeyPair
  (JNIEnv * jenv, jobject thiz)
{
    QCC_DbgPrintf(("CryptoECC_generateDHKeyPair()"));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("CryptoECC_generateDHKeyPair(): Exception or NULL pointer"));
        return;
    }

    cryptoPtr->GenerateDHKeyPair();
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateSPEKEKeyPair
 * Signature: (BILorg/alljoyn/bus/common/GUID128;Lorg/alljoyn/bus/common/GUID128;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateSPEKEKeyPair
  (JNIEnv * jenv, jobject thiz, jbyteArray jpw, jlong jpwLen, jobject jclientGUID, jobject jserviceGUID)
{
    QCC_DbgPrintf(("CryptoECC_generateSPEKEKeyPair()"));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("CryptoECC_generateSPEKEKeyPair(): Exception or NULL pointer"));
        return;
    }

    uint8_t* pw = ToByteArray(jpw);

    jmethodID mid = jenv->GetMethodID(CLS_JAVA_UTIL_UUID, "toString", "()Ljava/lang/String;");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("CryptoECC_generateSPEKEKeyPair: Can't find UUID.toString"));
        Throw("java/lang/NoSuchMethodException", NULL);
        return;
    }

    JString cguid((jstring) CallObjectMethod(jenv, jclientGUID, mid));
    QCC_DbgPrintf(("CryptoECC_generateSPEKEKeyPair: clientGUID %s", cguid.c_str()));
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("CryptoECC_generateSPEKEKeyPair: clientGUID is null or has not been generated"));
        return;
    }

    JString sguid((jstring) CallObjectMethod(jenv, jserviceGUID, mid));
    QCC_DbgPrintf(("CryptoECC_generateSPEKEKeyPair: serviceGUID %s", sguid.c_str()));
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("CryptoECC_generateSPEKEKeyPair: serviceGUID is null or has not been generated"));
        return;
    }

    const String clguid(cguid.c_str());
    const String seguid(sguid.c_str());

    GUID128 clientGUID(clguid);
    GUID128 serviceGUID(seguid);

    cryptoPtr->GenerateSPEKEKeyPair(pw, jpwLen, clientGUID, serviceGUID);

}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateSharedSecret
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;Lorg/alljoyn/bus/common/ECCSecret;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateSharedSecret
  (JNIEnv *, jobject, jobject, jobject)
{
    //TODO ASACORE-3213
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    getDHPublicKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_getDHPublicKey
  (JNIEnv * jenv, jobject thiz)
{
    QCC_DbgPrintf(("CryptoECC_getDHPublicKey()"));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDHPublicKey(): Exception or NULL pointer"));
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDHPublicKey: Can't find ECCPublicKey constructor"));
        Throw("java/lang/NoSuchMethodException", NULL);
        return NULL;
    }

    const ECCPublicKey* eccPublicKey = cryptoPtr->GetDHPublicKey();

    jbyteArray arrayX = ToJByteArray(eccPublicKey->GetX(), ECC_COORDINATE_SZ);
    jbyteArray arrayY = ToJByteArray(eccPublicKey->GetY(), ECC_COORDINATE_SZ);

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, mid, arrayX, arrayY);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDHPublicKey: Couldn't make jobject"));
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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDHPublicKey
  (JNIEnv *, jobject, jobject)
{
    //TODO ASACORE-3213
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    getDHPrivateKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPrivateKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_getDHPrivateKey
  (JNIEnv * jenv, jobject thiz)
{
    QCC_DbgPrintf(("CryptoECC_getDHPrivateKey()"));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDHPrivateKey(): Exception or NULL pointer"));
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_ECCPrivateKey, "<init>", "([B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDHPrivateKey: Can't find ECCPublicKey constructor"));
        Throw("java/lang/NoSuchMethodException", NULL);
        return NULL;
    }

    const ECCPrivateKey* eccPrivateKey = cryptoPtr->GetDHPrivateKey();

    jbyteArray arrayD = jenv->NewByteArray(ECC_COORDINATE_SZ);
    jenv->SetByteArrayRegion(arrayD, 0, ECC_COORDINATE_SZ, reinterpret_cast<const jbyte*>(eccPrivateKey->GetD()));

    jobject retObj = jenv->NewObject(CLS_ECCPrivateKey, mid, arrayD);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDHPrivateKey: Couldn't make jobject"));
        return NULL;
    }

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    setDHPrivateKey
 * Signature: (Lorg/alljoyn/bus/common/ECCPrivateKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDHPrivateKey
  (JNIEnv *, jobject, jobject)
{
    //TODO ASACORE-3213
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    getDSAPublicKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_getDSAPublicKey
  (JNIEnv * jenv, jobject thiz)
{
    QCC_DbgPrintf(("CryptoECC_getDSAPublicKey()"));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDSAPublicKey(): Exception or NULL pointer"));
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDSAPublicKey: Can't find ECCPublicKey constructor"));
        Throw("java/lang/NoSuchMethodException", NULL);
        return NULL;
    }

    const ECCPublicKey* eccPublicKey = cryptoPtr->GetDSAPublicKey();

    jbyteArray arrayX = ToJByteArray(eccPublicKey->GetX(), ECC_COORDINATE_SZ);
    jbyteArray arrayY = ToJByteArray(eccPublicKey->GetY(), ECC_COORDINATE_SZ);

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, mid, arrayX, arrayY);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDSAPublicKey: Couldn't make jobject"));
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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDSAPublicKey
  (JNIEnv *, jobject, jobject)
{
    //TODO ASACORE-3213
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    getDSAPrivateKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPrivateKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CryptoECC_getDSAPrivateKey
  (JNIEnv * jenv, jobject thiz)
{
    QCC_DbgPrintf(("CryptoECC_getDSAPrivateKey()"));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDSAPrivateKey(): Exception or NULL pointer"));
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_ECCPrivateKey, "<init>", "([B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDSAPrivateKey: Can't find ECCPublicKey constructor"));
        Throw("java/lang/NoSuchMethodException", NULL);
        return NULL;
    }

    const ECCPrivateKey* eccPrivateKey = cryptoPtr->GetDSAPrivateKey();

    jbyteArray arrayD = jenv->NewByteArray(ECC_COORDINATE_SZ);
    jenv->SetByteArrayRegion(arrayD, 0, ECC_COORDINATE_SZ, reinterpret_cast<const jbyte*>(eccPrivateKey->GetD()));

    jobject retObj = jenv->NewObject(CLS_ECCPrivateKey, mid, arrayD);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("CryptoECC_getDSAPrivateKey: Couldn't make jobject"));
        return NULL;
    }

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    setDSAPrivateKey
 * Signature: (Lorg/alljoyn/bus/common/ECCPrivateKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDSAPrivateKey
  (JNIEnv *, jobject, jobject)
{
    //TODO ASACORE-3213
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateDSAKeyPair
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateDSAKeyPair
  (JNIEnv * jenv, jobject thiz)
{
    QCC_DbgPrintf(("CryptoECC_generateDSAKeyPair()"));

    Crypto_ECC* cryptoPtr = GetHandle<Crypto_ECC*>(thiz);
    if (jenv->ExceptionCheck() || cryptoPtr == NULL) {
        QCC_LogError(ER_FAIL, ("CryptoECC_generateDSAKeyPair(): Exception or NULL pointer"));
        return;
    }

    cryptoPtr->GenerateDSAKeyPair();
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSASignDigest
 * Signature: (BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSASignDigest
  (JNIEnv *, jobject, jbyte, jshort, jobject)
{
    //TODO ASACORE-3213
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSASign
 * Signature: (BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSASign
  (JNIEnv *, jobject, jbyte, jshort, jobject)
{
    //TODO ASACORE-3213
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSAVerifyDigest
 * Signature: (BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSAVerifyDigest
  (JNIEnv *, jobject, jbyte, jshort, jobject)
{
    //TODO ASACORE-3213
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSAVerify
 * Signature: (BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSAVerify
  (JNIEnv *, jobject, jbyte, jshort, jobject)
{
    //TODO ASACORE-3213
}
