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
#include <string>
#include <algorithm>

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

    uint8_t* pw = ToByteArray(jpw);

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

    QStatus status = cryptoPtr->GenerateSPEKEKeyPair(pw, jpwLen, clientGUID, serviceGUID);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }

    delete pw;

}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    generateSharedSecret
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;Lorg/alljoyn/bus/common/ECCSecret;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_generateSharedSecret(JNIEnv*, jobject, jobject, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
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

    jmethodID mid = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        Throw("java/lang/NoSuchMethodException", NULL);
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

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, mid, arrayX, arrayY);

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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDHPublicKey(JNIEnv*, jobject, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
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

    jmethodID mid = jenv->GetMethodID(CLS_ECCPrivateKey, "<init>", "([B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPrivateKey constructor", __FUNCTION__));
        Throw("java/lang/NoSuchMethodException", NULL);
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

    jobject retObj = jenv->NewObject(CLS_ECCPrivateKey, mid, arrayD);

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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDHPrivateKey(JNIEnv*, jobject, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
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

    jmethodID mid = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        Throw("java/lang/NoSuchMethodException", NULL);
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

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, mid, arrayX, arrayY);

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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDSAPublicKey(JNIEnv*, jobject, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
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

    jmethodID mid = jenv->GetMethodID(CLS_ECCPrivateKey, "<init>", "([B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        Throw("java/lang/NoSuchMethodException", NULL);
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

    jobject retObj = jenv->NewObject(CLS_ECCPrivateKey, mid, arrayD);

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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_setDSAPrivateKey(JNIEnv*, jobject, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
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
 * Signature: (BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSASignDigest(JNIEnv*, jobject, jbyte, jshort, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSASign
 * Signature: (BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSASign(JNIEnv*, jobject, jbyte, jshort, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSAVerifyDigest
 * Signature: (BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSAVerifyDigest(JNIEnv*, jobject, jbyte, jshort, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
}

/*
 * Class:     org_alljoyn_bus_common_CryptoECC
 * Method:    DSAVerify
 * Signature: (BSLorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CryptoECC_DSAVerify(JNIEnv*, jobject, jbyte, jshort, jobject)
{
    //TODO ASACORE-3213
    QCC_DbgTrace(("%s", __FUNCTION__));
    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
}
