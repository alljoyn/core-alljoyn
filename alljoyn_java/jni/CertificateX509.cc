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
#include <qcc/String.h>
#include <qcc/CertificateECC.h>
#include <qcc/Crypto.h>
#include <qcc/Debug.h>

#include "alljoyn_java.h"
#include "alljoyn_jni_helper.h"

#define QCC_MODULE "ALLJOYN_JAVA"

using namespace qcc;

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    create
 * Signature: (Lorg/alljoyn/bus/common/CertificateX509/CertificateType;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_create(JNIEnv* jenv, jobject thiz, jobject jcertType)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = NULL;

    if (jcertType == NULL) {
        certPtr = new CertificateX509();
    } else {
        if (jenv->IsSameObject(CertificateX509Type_UNRESTRICTED, jcertType)) {
            certPtr = new CertificateX509(CertificateX509::CertificateType::UNRESTRICTED_CERTIFICATE);
        } else if (jenv->IsSameObject(CertificateX509Type_IDENTITY, jcertType)) {
            certPtr = new CertificateX509(CertificateX509::CertificateType::IDENTITY_CERTIFICATE);
        } else if (jenv->IsSameObject(CertificateX509Type_MEMBERSHIP, jcertType)) {
            certPtr = new CertificateX509(CertificateX509::CertificateType::MEMBERSHIP_CERTIFICATE);
        } else if (jenv->IsSameObject(CertificateX509Type_INVALID, jcertType)) {
            certPtr = new CertificateX509(CertificateX509::CertificateType::INVALID_CERTIFICATE);
        }
    }

    if (!certPtr) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    SetHandle(thiz, certPtr);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        delete certPtr;
        certPtr = NULL;
    }
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_destroy(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* cx509Ptr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (cx509Ptr == NULL) {
        QCC_DbgPrintf(("%s: Already destroyed. Returning.", __FUNCTION__));
        return;
    }

    delete cx509Ptr;
    cx509Ptr = NULL;

    SetHandle(thiz, NULL);
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    decodeCertificatePEM
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_decodeCertificatePEM(JNIEnv* jenv, jobject thiz, jstring jpem, jlong jpemlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    JString pem(jpem);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception converting pem", __FUNCTION__));
        return;
    }

    String pemstr(pem.c_str(), jpemlen);

    QStatus status = certPtr->DecodeCertificatePEM(pemstr);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    encodeCertificatePEM
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_common_CertificateX509_encodeCertificatePEM(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    qcc::String pem;
    QStatus status = certPtr->EncodeCertificatePEM(pem);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    return jenv->NewStringUTF(pem.c_str());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    decodeCertificateDER
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_decodeCertificateDER(JNIEnv* jenv, jobject thiz, jbyteArray jder, jlong jderlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    const char* der = reinterpret_cast<const char*>(ToByteArray(jder));
    String strDER(der, jderlen);

    QStatus status = certPtr->DecodeCertificateDER(strDER);

    delete [] der;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    encodeCertificateDER
 * Signature: ()[B;
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_encodeCertificateDER(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    String der;
    QStatus status = certPtr->EncodeCertificateDER(der);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    return ToJByteArray(reinterpret_cast<const uint8_t*>(der.data()), der.size());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    encodeCertificateTBS
 * Signature: ()[B;
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_encodeCertificateTBS(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    qcc::String tbs;
    QStatus status = certPtr->EncodeCertificateTBS(tbs);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    return ToJByteArray(reinterpret_cast<const uint8_t*>(tbs.data()), tbs.size());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    encodePrivateKeyPEM
 * Signature: (Lorg/alljoyn/bus/common/ECCPrivateKey;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_common_CertificateX509_encodePrivateKeyPEM(JNIEnv* jenv, jclass, jobject jeccPrivateKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ECCPrivateKey eccPrivateKey;
    String retStr;

    jbyteArray jeccD = (jbyteArray) jenv->GetObjectField(jeccPrivateKey, FID_ECCPrivateKey_d);

    uint8_t* eccD = ToByteArray(jeccD);
    if (jenv->ExceptionCheck()) {
        delete [] eccD;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    QStatus status = eccPrivateKey.Import(eccD, jenv->GetArrayLength(jeccD));
    delete [] eccD;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    status = CertificateX509::EncodePrivateKeyPEM(&eccPrivateKey, retStr);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    return jenv->NewStringUTF(retStr.c_str());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    decodePrivateKeyPEM
 * Signature: (Ljava/lang/String;)Lorg/alljoyn/bus/common/ECCPrivateKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CertificateX509_decodePrivateKeyPEM(JNIEnv* jenv, jclass, jstring jencPrivateKey, jlong jencPrivateKeyLen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    JString encPrivateKey(jencPrivateKey);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: exception converting jencPrivateKey to JString", __FUNCTION__));
        return NULL;
    }
    String encPrivateKeyStr(encPrivateKey.c_str(), jencPrivateKeyLen);

    ECCPrivateKey eccPrivateKey;

    QStatus status = CertificateX509::DecodePrivateKeyPEM(encPrivateKeyStr, &eccPrivateKey);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jbyteArray jeccD = ToJByteArray(eccPrivateKey.GetD(), eccPrivateKey.GetDSize());

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: exception converting byte array", __FUNCTION__));
        return NULL;
    }

    jobject retObj = jenv->NewObject(CLS_ECCPrivateKey, MID_ECCPrivateKey_cnstrctr, jeccD);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: exception converting byte array", __FUNCTION__));
        return NULL;
    }

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    encodePublicKeyPEM
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_common_CertificateX509_encodePublicKeyPEM(JNIEnv* jenv, jclass, jobject jeccPublicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ECCPublicKey eccPublicKey;
    String retStr;

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_x);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_y);

    uint8_t* eccX = ToByteArray(jeccX);
    uint8_t* eccY = ToByteArray(jeccY);

    if (jenv->ExceptionCheck()) {
        delete [] eccX;
        delete [] eccY;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));
    delete [] eccX;
    delete [] eccY;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    status = CertificateX509::EncodePublicKeyPEM(&eccPublicKey, retStr);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    return jenv->NewStringUTF(retStr.c_str());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    decodePublicKeyPEM
 * Signature: (Ljava/lang/String;)Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CertificateX509_decodePublicKeyPEM(JNIEnv* jenv, jclass, jstring jencPublicKey, jlong jencPublicKeyLen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    JString encPublicKey(jencPublicKey);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: exception converting jencPublicKey to JString", __FUNCTION__));
        return NULL;
    }
    String encPublicKeyStr(encPublicKey.c_str(), jencPublicKeyLen);

    ECCPublicKey eccPublicKey;

    QStatus status = CertificateX509::DecodePublicKeyPEM(encPublicKeyStr, &eccPublicKey);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jbyteArray jeccX = ToJByteArray(eccPublicKey.GetX(), eccPublicKey.GetCoordinateSize());

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: exception converting byte array", __FUNCTION__));
        return NULL;
    }

    jbyteArray jeccY = ToJByteArray(eccPublicKey.GetY(), eccPublicKey.GetCoordinateSize());

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: exception converting byte array", __FUNCTION__));
        return NULL;
    }

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, MID_ECCPublicKey_cnstrctr, jeccX, jeccY);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: exception making new jobject", __FUNCTION__));
        return NULL;
    }

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    sign
 * Signature: (Lorg/alljoyn/bus/common/ECCPrivateKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_sign(JNIEnv* jenv, jobject thiz, jobject jeccPrivateKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
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

    status = certPtr->Sign(&eccPrivateKey);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    jenv->SetObjectField(jeccPrivateKey, FID_ECCPrivateKey_d, ToJByteArray(eccPrivateKey.GetD(), eccPrivateKey.GetDSize()));

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setSignature
 * Signature: (Lorg/alljoyn/bus/common/ECCSignature;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setSignature(JNIEnv* jenv, jobject thiz, jobject jsign)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    ECCSignature eccSignature;

    jbyteArray jeccR = (jbyteArray) jenv->GetObjectField(jsign, FID_ECCSignature_r);
    jbyteArray jeccS = (jbyteArray) jenv->GetObjectField(jsign, FID_ECCSignature_s);

    uint8_t* eccR = ToByteArray(jeccR);
    uint8_t* eccS = ToByteArray(jeccS);

    if (jenv->ExceptionCheck()) {
        delete [] eccR;
        delete [] eccS;

        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QStatus status = eccSignature.Import(eccR, jenv->GetArrayLength(jeccR), eccS, jenv->GetArrayLength(jeccS));

    delete [] eccR;
    delete [] eccS;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    certPtr->SetSignature(eccSignature);

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    signAndGenerateAuthorityKeyId
 * Signature: (Lorg/alljoyn/bus/common/ECCPrivateKey;Lorg/alljoyn/bus/common/ECCPublicKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_signAndGenerateAuthorityKeyId(JNIEnv* jenv, jobject thiz, jobject jeccPrivateKey, jobject jeccPublicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
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

    status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));

    delete [] eccX;
    delete [] eccY;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    status = certPtr->SignAndGenerateAuthorityKeyId(&eccPrivateKey, &eccPublicKey);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    jenv->SetObjectField(jeccPrivateKey, FID_ECCPrivateKey_d, ToJByteArray(eccPrivateKey.GetD(), eccPrivateKey.GetDSize()));
    jenv->SetObjectField(jeccPublicKey, FID_ECCPublicKey_x, ToJByteArray(eccPublicKey.GetX(), eccPublicKey.GetCoordinateSize()));
    jenv->SetObjectField(jeccPublicKey, FID_ECCPublicKey_y, ToJByteArray(eccPublicKey.GetY(), eccPublicKey.GetCoordinateSize()));

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    verify
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_verify__(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    QStatus status = certPtr->Verify();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    verify
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_verify__Lorg_alljoyn_bus_common_ECCPublicKey_2(JNIEnv* jenv, jobject thiz, jobject jeccPublicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
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

    status = certPtr->Verify(&eccPublicKey);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    verifyValidity
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_verifyValidity(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    QStatus status = certPtr->VerifyValidity();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setSerial
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setSerial(JNIEnv* jenv, jobject thiz, jbyteArray jserial, jlong jserialLen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    uint8_t* serial = ToByteArray(jserial);

    certPtr->SetSerial(serial, jserialLen);

    delete [] serial;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    generateRandomSerial
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_generateRandomSerial(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    QStatus status = certPtr->GenerateRandomSerial();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getSerial
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getSerial(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    return ToJByteArray(certPtr->GetSerial(), certPtr->GetSerialLen());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setIssuerOU
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setIssuerOU(JNIEnv* jenv, jobject thiz, jbyteArray jissuerOU, jlong jissuerOUlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    uint8_t* issuerOU = ToByteArray(jissuerOU);

    certPtr->SetIssuerOU(issuerOU, jissuerOUlen);

    delete [] issuerOU;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getIssuerOU
 * Signature: ()B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getIssuerOU(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return 0;
    }

    return ToJByteArray(certPtr->GetIssuerOU(), certPtr->GetIssuerOULength());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setIssuerCN
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setIssuerCN(JNIEnv* jenv, jobject thiz, jbyteArray jissuerCN, jlong jissuerCNlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    uint8_t* issuerCN = ToByteArray(jissuerCN);

    certPtr->SetIssuerCN(issuerCN, jissuerCNlen);

    delete [] issuerCN;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getIssuerCN
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getIssuerCN(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return 0;
    }

    return ToJByteArray(certPtr->GetIssuerCN(), certPtr->GetIssuerCNLength());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setSubjectOU
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setSubjectOU(JNIEnv* jenv, jobject thiz, jbyteArray jsubjectOU, jlong jsubjectOUlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    uint8_t* subjectOU = ToByteArray(jsubjectOU);

    certPtr->SetSubjectOU(subjectOU, jsubjectOUlen);

    delete [] subjectOU;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getSubjectOU
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getSubjectOU(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return 0;
    }

    return ToJByteArray(certPtr->GetSubjectOU(), certPtr->GetSubjectOULength());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setSubjectCN
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setSubjectCN(JNIEnv* jenv, jobject thiz, jbyteArray jsubjectCN, jlong jsubjectCNlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    uint8_t* subjectCN = ToByteArray(jsubjectCN);

    certPtr->SetSubjectCN(subjectCN, jsubjectCNlen);

    delete [] subjectCN;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getSubjectCN
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getSubjectCN(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return 0;
    }

    return ToJByteArray(certPtr->GetSubjectCN(), certPtr->GetSubjectCNLength());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setSubjectAltName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setSubjectAltName(JNIEnv* jenv, jobject thiz, jbyteArray jsubjectAltName, jlong jsubjectlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    const char* altName = reinterpret_cast<const char*>(ToByteArray(jsubjectAltName));

    String subjectAltName(altName, jsubjectlen);

    certPtr->SetSubjectAltName(subjectAltName);

    delete [] altName;
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getSubjectAltName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getSubjectAltName(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    qcc::String altName = certPtr->GetSubjectAltName();

    return ToJByteArray(reinterpret_cast<const uint8_t*>(altName.data()), altName.size());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    generateAuthorityKeyId
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;)Ljava/lang/String;
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_generateAuthorityKeyId(JNIEnv* jenv, jclass, jobject jeccPublicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String authorityKeyId;

    ECCPublicKey eccPublicKey;

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_x);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_y);

    uint8_t* eccX = ToByteArray(jeccX);
    uint8_t* eccY = ToByteArray(jeccY);

    if (jenv->ExceptionCheck()) {
        delete [] eccX;
        delete [] eccY;

        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));

    delete [] eccX;
    delete [] eccY;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    status = CertificateX509::GenerateAuthorityKeyId(&eccPublicKey, authorityKeyId);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    return ToJByteArray(reinterpret_cast<const uint8_t*>(authorityKeyId.data()), authorityKeyId.size());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getAuthorityKeyId
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getAuthorityKeyId(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    qcc::String authorityKeyId = certPtr->GetAuthorityKeyId();

    return ToJByteArray(reinterpret_cast<const uint8_t*>(authorityKeyId.data()), authorityKeyId.size());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setAuthorityKeyId
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setAuthorityKeyId(JNIEnv* jenv, jobject thiz, jbyteArray jauthorityKeyId, jlong akiLen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    const char* aki = reinterpret_cast<const char*>(ToByteArray(jauthorityKeyId));
    String strAKI(aki, akiLen);

    certPtr->SetAuthorityKeyId(strAKI);

    delete [] aki;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setValidity
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setValidity(JNIEnv* jenv, jobject thiz, jlong jvalidFrom, jlong jvalidTo)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    CertificateX509::ValidPeriod validPeriod;
    validPeriod.validFrom = jvalidFrom;
    validPeriod.validTo = jvalidTo;

    certPtr->SetValidity(&validPeriod);

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getValidFrom
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_common_CertificateX509_getValidFrom(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return 0;
    }

    const CertificateX509::ValidPeriod* validPeriod = certPtr->GetValidity();

    return validPeriod->validFrom;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getValidTo
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_common_CertificateX509_getValidTo(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return 0;
    }

    const CertificateX509::ValidPeriod* validPeriod = certPtr->GetValidity();

    return validPeriod->validTo;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setSubjectPublicKey
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setSubjectPublicKey(JNIEnv* jenv, jobject thiz, jobject jeccPublicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    ECCPublicKey* eccPublicKey = new ECCPublicKey();

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_x);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jeccPublicKey, FID_ECCPublicKey_y);

    uint8_t* eccX = ToByteArray(jeccX);
    uint8_t* eccY = ToByteArray(jeccY);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        delete [] eccX;
        delete [] eccY;
        return;
    }

    QStatus status = eccPublicKey->Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));

    delete [] eccX;
    delete [] eccY;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    certPtr->SetSubjectPublicKey(eccPublicKey);

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getSubjectPublicKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_common_CertificateX509_getSubjectPublicKey(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    const ECCPublicKey* eccPublicKey = certPtr->GetSubjectPublicKey();

    jbyteArray jeccX = ToJByteArray(eccPublicKey->GetX(), eccPublicKey->GetCoordinateSize());
    jbyteArray jeccY = ToJByteArray(eccPublicKey->GetY(), eccPublicKey->GetCoordinateSize());

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, MID_ECCPublicKey_cnstrctr, jeccX, jeccY);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: exception making new jobject", __FUNCTION__));
        return NULL;
    }

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setCA
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setCA(JNIEnv* jenv, jobject thiz, jboolean jca)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    certPtr->SetCA(jca);

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    isCA
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_common_CertificateX509_isCA(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return JNI_FALSE;
    }

    return certPtr->IsCA();

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    setDigest
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_setDigest(JNIEnv* jenv, jobject thiz, jbyteArray jdigest, jlong jdigestlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    uint8_t* digest = ToByteArray(jdigest);

    certPtr->SetDigest(digest, jdigestlen);

    delete [] digest;

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getDigest
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getDigest(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return 0;
    }

    return ToJByteArray(certPtr->GetDigest(), certPtr->GetDigestSize());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    isDigestPresent
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_common_CertificateX509_isDigestPresent(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return JNI_FALSE;
    }

    return certPtr->IsDigestPresent();

}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getPEM
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_common_CertificateX509_getPEM(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    qcc::String pem = certPtr->GetPEM();

    return jenv->NewStringUTF(pem.c_str());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    loadPEM
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_common_CertificateX509_loadPEM(JNIEnv* jenv, jobject thiz, jstring jpem, jlong jpemlen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return;
    }

    JString pem(jpem);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception converting pem", __FUNCTION__));
        return;
    }
    String pemStr(pem.c_str(), jpemlen);

    QStatus status = certPtr->LoadPEM(pemStr);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    toJavaString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_common_CertificateX509_toJavaString(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return NULL;
    }

    qcc::String certString = certPtr->ToString();

    return jenv->NewStringUTF(certString.c_str());
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    isIssuerOf
 * Signature: (Lorg/alljoyn/bus/common/CertificateX509;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_common_CertificateX509_isIssuerOf(JNIEnv* jenv, jobject thiz, jobject jcertificate)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return JNI_FALSE;
    }

    CertificateX509* otherCertPtr = GetHandle<CertificateX509*>(jcertificate);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    if (!otherCertPtr) {
        QCC_LogError(ER_FAIL, ("%s: otherCertPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return JNI_FALSE;
    }

    return certPtr->IsIssuerOf(*otherCertPtr);
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    isDNEqual
 * Signature: ([BJ[BJ)Z
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_common_CertificateX509_isDNEqual___3BJ_3BJ(JNIEnv* jenv, jobject thiz, jbyteArray jcn, jlong jcnlen, jbyteArray jou, jlong joulen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return JNI_FALSE;
    }

    uint8_t* cn = ToByteArray(jcn);
    uint8_t* ou = ToByteArray(jou);

    if (jenv->ExceptionCheck()) {
        delete [] cn;
        delete [] ou;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    jboolean ret = certPtr->IsDNEqual(cn, jcnlen, ou, joulen);

    delete [] cn;
    delete [] ou;

    return ret;
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    isDNEqual
 * Signature: (Lorg/alljoyn/bus/common/CertificateX509;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_common_CertificateX509_isDNEqual__Lorg_alljoyn_bus_common_CertificateX509_2(JNIEnv* jenv, jobject thiz, jobject jcertificate)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return JNI_FALSE;
    }

    CertificateX509* otherCertPtr = GetHandle<CertificateX509*>(jcertificate);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    if (!otherCertPtr) {
        QCC_LogError(ER_FAIL, ("%s: otherCertPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return JNI_FALSE;
    }

    return certPtr->IsDNEqual(*otherCertPtr);
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    isSubjectPublicKeyEqual
 * Signature: (Lorg/alljoyn/bus/common/ECCPublicKey;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_common_CertificateX509_isSubjectPublicKeyEqual(JNIEnv* jenv, jobject thiz, jobject jeccPublicKey)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return JNI_FALSE;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return JNI_FALSE;
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
        return JNI_FALSE;
    }

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));

    delete [] eccX;
    delete [] eccY;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return JNI_FALSE;
    }

    return certPtr->IsSubjectPublicKeyEqual(&eccPublicKey);
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    getSHA256Thumbprint
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_getSHA256Thumbprint(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certPtr = GetHandle<CertificateX509*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!certPtr) {
        QCC_LogError(ER_FAIL, ("%s: certPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "CertificateX509 object is null");
        return 0;
    }

    uint8_t thumbprint[Crypto_SHA256::DIGEST_SIZE];

    QStatus status = certPtr->GetSHA256Thumbprint(thumbprint);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return ToJByteArray(thumbprint, sizeof(thumbprint));
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    decodeCertChainPEM
 * Signature: (Ljava/lang/String;JJ)[Lorg/alljoyn/bus/common/CertificateX509;
 */
JNIEXPORT jobjectArray JNICALL Java_org_alljoyn_bus_common_CertificateX509_decodeCertChainPEM(JNIEnv* jenv, jclass, jstring jencoded, jlong jencodedLen, jlong jcertExpectedCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    JString encoded(jencoded);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception converting encoded string", __FUNCTION__));
        return NULL;
    }
    String encodedStr(encoded.c_str(), jencodedLen);

    CertificateX509* certArray = new CertificateX509[jcertExpectedCount];

    QStatus status = CertificateX509::DecodeCertChainPEM(encodedStr, certArray, jcertExpectedCount);

    if (status != ER_OK) {
        delete [] certArray;
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_CertificateX509, "<init>", "()V");
    jobjectArray retArray = jenv->NewObjectArray(jcertExpectedCount, CLS_CertificateX509, NULL);

    for (jlong i = 0; i < jcertExpectedCount; i++) {
        jobject jcert = jenv->NewObject(CLS_CertificateX509, mid);
        if (!jcert) {
            delete [] certArray;
            return NULL;
        }

        CertificateX509* certTemp = new CertificateX509();
        *certTemp = certArray[i];
        SetHandle(jcert, certTemp);

        if (jenv->ExceptionCheck()) {
            delete [] certArray;
            delete certTemp;
            SetHandle(jcert, NULL);
            return NULL;
        }

        jenv->SetObjectArrayElement(retArray, i, jcert);
        if (jenv->ExceptionCheck()) {
            delete [] certArray;
            delete certTemp;
            SetHandle(jcert, NULL);
            return NULL;
        }
    }

    delete [] certArray;
    return retArray;
}

/*
 * Class:     org_alljoyn_bus_common_CertificateX509
 * Method:    validateCertificateTypeInCertChain
 * Signature: ([Lorg/alljoyn/bus/common/CertificateX509;J)Z
 */
JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_common_CertificateX509_validateCertificateTypeInCertChain(JNIEnv* jenv, jclass, jobjectArray jcertChain, jlong jcertChainLen)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CertificateX509* certArray = new CertificateX509[jcertChainLen];

    for (jlong i = 0; i < jcertChainLen; ++i) {
        jobject jcertX509 = GetObjectArrayElement(jenv, jcertChain, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            delete [] certArray;
            return JNI_FALSE;
        }

        QCC_ASSERT(jcertX509);

        CertificateX509* certX509 = GetHandle<CertificateX509*>(jcertX509);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            delete [] certArray;
            return JNI_FALSE;
        }
        QCC_ASSERT(certX509);

        certArray[i] = *certX509;
    }

    jboolean retBool = CertificateX509::ValidateCertificateTypeInCertChain(certArray, jcertChainLen);

    delete [] certArray;

    return retBool;

}
