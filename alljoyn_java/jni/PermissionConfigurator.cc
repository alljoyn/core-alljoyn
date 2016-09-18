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
#include <qcc/Debug.h>
#include <alljoyn/PermissionConfigurator.h>
#include <algorithm>

#include "alljoyn_jni_helper.h"
#include "alljoyn_java.h"

#define QCC_MODULE "ALLJOYN_JAVA"

using namespace ajn;
using namespace qcc;

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getManifestTemplateAsXml
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getManifestTemplateAsXml(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return NULL;
    }

    std::string manifestTemplate;
    QStatus status = pconfPtr->GetManifestTemplateAsXml(manifestTemplate);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    return jenv->NewStringUTF(manifestTemplate.c_str());
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    setManifestTemplateFromXml
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_setManifestTemplateFromXml(JNIEnv* jenv, jobject thiz, jstring jxml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    JString manifestTemplate(jxml);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QStatus status = pconfPtr->SetManifestTemplateFromXml(manifestTemplate.c_str());

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getApplicationState
 * Signature: ()Lorg/alljoyn/bus/PermissionConfigurator/ApplicationState;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getApplicationState(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return NULL;
    }

    PermissionConfigurator::ApplicationState state;
    QStatus status = pconfPtr->GetApplicationState(state);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    if (state == PermissionConfigurator::ApplicationState::NOT_CLAIMABLE) {
        return PermissionConfiguratorApplicationState_NOT_CLAIMABLE;
    } else if (state == PermissionConfigurator::ApplicationState::CLAIMABLE) {
        return PermissionConfiguratorApplicationState_CLAIMABLE;
    } else if (state == PermissionConfigurator::ApplicationState::CLAIMED) {
        return PermissionConfiguratorApplicationState_CLAIMED;
    } else if (state == PermissionConfigurator::ApplicationState::NEED_UPDATE) {
        return PermissionConfiguratorApplicationState_NEED_UPDATE;
    }

    jenv->ThrowNew(CLS_BusException, QCC_StatusText(ER_INVALID_APPLICATION_STATE));
    return NULL;
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    setApplicationState
 * Signature: (Lorg/alljoyn/bus/PermissionConfigurator/ApplicationState;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_setApplicationState(JNIEnv* jenv, jobject thiz, jobject jstate)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    PermissionConfigurator::ApplicationState state;

    if (jenv->IsSameObject(jstate, PermissionConfiguratorApplicationState_NOT_CLAIMABLE)) {
        state = PermissionConfigurator::ApplicationState::NOT_CLAIMABLE;
    } else if (jenv->IsSameObject(jstate, PermissionConfiguratorApplicationState_CLAIMABLE)) {
        state = PermissionConfigurator::ApplicationState::CLAIMABLE;
    } else if (jenv->IsSameObject(jstate, PermissionConfiguratorApplicationState_CLAIMED)) {
        state = PermissionConfigurator::ApplicationState::CLAIMED;
    } else if (jenv->IsSameObject(jstate, PermissionConfiguratorApplicationState_NEED_UPDATE)) {
        state = PermissionConfigurator::ApplicationState::NEED_UPDATE;
    } else {
        Throw("org/alljoyn/bus/BusException", "INVALID_APPLICATION_STATE");
        return;
    }

    QStatus status = pconfPtr->SetApplicationState(state);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getSigningPublicKey
 * Signature: ()Lorg/alljoyn/bus/common/KeyInfoECC;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getSigningPublicKey(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return NULL;
    }

    KeyInfoNISTP256 keyInfo;
    QStatus status = pconfPtr->GetSigningPublicKey(keyInfo);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    const ECCPublicKey* retKey = keyInfo.GetPublicKey();
    if (retKey == NULL) {
        QCC_LogError(ER_FAIL, ("%s: retKey is null", __FUNCTION__));
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_KeyInfoNISTP256, "<init>", "()V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find KeyInfoNISTP256 constructor", __FUNCTION__));
        return NULL;
    }

    JLocalRef<jobject> retObj = jenv->NewObject(CLS_KeyInfoNISTP256, mid);

    jmethodID midC = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!midC) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        return NULL;
    }

    JLocalRef<jbyteArray> arrayX = ToJByteArray(retKey->GetX(), retKey->GetCoordinateSize());
    JLocalRef<jbyteArray> arrayY = ToJByteArray(retKey->GetY(), retKey->GetCoordinateSize());

    jobject jretKey = jenv->NewObject(CLS_ECCPublicKey, midC, arrayX.move(), arrayY.move());

    jmethodID midSet = jenv->GetMethodID(CLS_KeyInfoNISTP256, "setPublicKey", "(Lorg/alljoyn/bus/common/ECCPublicKey;)V");
    if (!midSet) {
        QCC_LogError(ER_FAIL, ("%s: Can't find setPublicKey", __FUNCTION__));
        return NULL;
    }
    CallObjectMethod(jenv, retObj, midSet, jretKey);

    return retObj;
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    signCertificate
 * Signature: (Lorg/alljoyn/bus/common/CertificateX509;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_signCertificate(JNIEnv* jenv, jobject thiz, jobject jcertificate)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    CertificateX509* cx509Ptr = GetHandle<CertificateX509*>(jcertificate);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QStatus status = pconfPtr->SignCertificate(*cx509Ptr);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_reset(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    QStatus status = pconfPtr->Reset();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getConnectedPeerPublicKey
 * Signature: (Ljava/util/UUID;)Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getConnectedPeerPublicKey(JNIEnv* jenv, jobject thiz, jobject jguid)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_JAVA_UTIL_UUID, "toString", "()Ljava/lang/String;");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find UUID.toString", __FUNCTION__));
        return NULL;
    }

    JString jstrguid((jstring) CallObjectMethod(jenv, jguid, mid));
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: clientGUID is null or has not been generated", __FUNCTION__));
        return NULL;
    }

    String strguid(jstrguid.c_str());
    std::string stringguid = static_cast<std::string>(strguid);
    stringguid.erase(std::remove(stringguid.begin(), stringguid.end(), '-'), stringguid.end());
    strguid = stringguid;

    GUID128 guid(strguid);

    ECCPublicKey retKey;
    QStatus status = pconfPtr->GetConnectedPeerPublicKey(guid, &retKey);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jmethodID midC = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!midC) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        Throw("java/lang/NoSuchMethodException", NULL);
        return NULL;
    }

    jbyteArray arrayX = ToJByteArray(retKey.GetX(), retKey.GetCoordinateSize());
    jbyteArray arrayY = ToJByteArray(retKey.GetY(), retKey.GetCoordinateSize());

    jobject retObj = jenv->NewObject(CLS_ECCPublicKey, midC, arrayX, arrayY);

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
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    setClaimCapabilities
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_setClaimCapabilities(JNIEnv* jenv, jobject thiz, jshort jclaimCapabilities)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    QStatus status = pconfPtr->SetClaimCapabilities(jclaimCapabilities);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getClaimCapabilities
 * Signature: ()I
 */
JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getClaimCapabilities(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return 0;
    }

    uint16_t claimCapabilities;

    QStatus status = pconfPtr->GetClaimCapabilities(claimCapabilities);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return claimCapabilities;
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    setClaimCapabilityAdditionalInfo
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_setClaimCapabilityAdditionalInfo(JNIEnv* jenv, jobject thiz, jshort jclaimCapabilitiesAdditional)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    QStatus status = pconfPtr->SetClaimCapabilityAdditionalInfo(jclaimCapabilitiesAdditional);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getClaimCapabilityAdditionalInfo
 * Signature: ()I
 */
JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getClaimCapabilityAdditionalInfo(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return 0;
    }

    uint16_t claimCapabilitiesAdditional;

    QStatus status = pconfPtr->GetClaimCapabilityAdditionalInfo(claimCapabilitiesAdditional);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return claimCapabilitiesAdditional;
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    claim
 * Signature: (Lorg/alljoyn/bus/common/KeyInfoNISTP256;Ljava/util/UUID;Lorg/alljoyn/bus/common/KeyInfoNISTP256;[Lorg/alljoyn/bus/common/CertificateX509;J[Ljava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_claim(JNIEnv* jenv, jobject thiz, jobject jcertAuth, jobject jadminGroupId, jobject jadminGroup, jobjectArray jcertArray, jlong jcertChainCount, jobjectArray jmanifestobjs, jlong jmaniCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    jfieldID fid = jenv->GetFieldID(CLS_KeyInfoNISTP256, "key", "Lorg/alljoyn/bus/common/ECCPublicKey;");

    if (!fid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find key field in KeyInfoNISTP256", __FUNCTION__));
        return;
    }

    jfieldID fidKeyId = jenv->GetFieldID(CLS_KeyInfoNISTP256, "m_keyId", "[B");

    if (!fid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find keyId field in KeyInfoNISTP256", __FUNCTION__));
        return;
    }

    jobject jcertAuthPublicKey = (jobject) jenv->GetObjectField(jcertAuth, fid);
    jbyteArray jkeyID = (jbyteArray) jenv->GetObjectField(jcertAuth, fidKeyId);

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jcertAuthPublicKey, FID_ECCPublicKey_x);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jcertAuthPublicKey, FID_ECCPublicKey_y);

    uint8_t* eccX = ToByteArray(jeccX);
    uint8_t* eccY = ToByteArray(jeccY);
    uint8_t* keyID = ToByteArray(jkeyID);

    if (jenv->ExceptionCheck()) {
        delete [] eccX;
        delete [] eccY;
        delete [] keyID;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    ECCPublicKey eccPublicKey;

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));

    if (status != ER_OK) {
        delete [] eccX;
        delete [] eccY;
        delete [] keyID;
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    KeyInfoNISTP256 certificateAuthority;

    certificateAuthority.SetPublicKey(&eccPublicKey);
    certificateAuthority.SetKeyId(keyID, jenv->GetArrayLength(jkeyID));

    jmethodID mid = jenv->GetMethodID(CLS_JAVA_UTIL_UUID, "toString", "()Ljava/lang/String;");
    if (!mid) {
        delete [] eccX;
        delete [] eccY;
        delete [] keyID;
        QCC_LogError(ER_FAIL, ("%s: Can't find UUID.toString", __FUNCTION__));
        return;
    }

    JString jstrguid((jstring) CallObjectMethod(jenv, jadminGroupId, mid));
    if (jenv->ExceptionCheck()) {
        delete [] eccX;
        delete [] eccY;
        delete [] keyID;
        QCC_LogError(ER_FAIL, ("%s: clientGUID is null or has not been generated", __FUNCTION__));
        return;
    }

    String strguid(jstrguid.c_str());
    std::string stringguid = static_cast<std::string>(strguid);
    stringguid.erase(std::remove(stringguid.begin(), stringguid.end(), '-'), stringguid.end());
    strguid = stringguid;

    GUID128 adminGroupId(strguid);

    jobject jadminGroupPublicKey = (jobject) jenv->GetObjectField(jadminGroup, fid);
    jbyteArray jadminGroupKeyID = (jbyteArray) jenv->GetObjectField(jadminGroup, fidKeyId);

    jbyteArray jadminGroupEccX = (jbyteArray) jenv->GetObjectField(jadminGroupPublicKey, FID_ECCPublicKey_x);
    jbyteArray jadminGroupEccY = (jbyteArray) jenv->GetObjectField(jadminGroupPublicKey, FID_ECCPublicKey_y);

    uint8_t* adminGroupEccX = ToByteArray(jadminGroupEccX);
    uint8_t* adminGroupEccY = ToByteArray(jadminGroupEccY);
    uint8_t* adminGroupKeyID = ToByteArray(jadminGroupKeyID);

    if (jenv->ExceptionCheck()) {
        delete [] eccX;
        delete [] eccY;
        delete [] keyID;
        delete [] adminGroupEccX;
        delete [] adminGroupEccY;
        delete [] adminGroupKeyID;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    ECCPublicKey eccPublicKeyAdminGroup;

    status = eccPublicKeyAdminGroup.Import(adminGroupEccX, jenv->GetArrayLength(jadminGroupEccX), adminGroupEccY, jenv->GetArrayLength(jadminGroupEccY));

    if (status != ER_OK) {
        delete [] eccX;
        delete [] eccY;
        delete [] keyID;
        delete [] adminGroupEccX;
        delete [] adminGroupEccY;
        delete [] adminGroupKeyID;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    KeyInfoNISTP256 adminGroup;

    certificateAuthority.SetPublicKey(&eccPublicKeyAdminGroup);
    certificateAuthority.SetKeyId(adminGroupKeyID, jenv->GetArrayLength(jadminGroupKeyID));

    CertificateX509* certChain = new CertificateX509[jcertChainCount];
    size_t maniCount = (size_t) jmaniCount;
    const char** manifests = new const char*[maniCount];
    jstring* jmanifests = new jstring[maniCount];

    for (long i = 0; i < jcertChainCount; ++i) {
        jobject jcertX509 = GetObjectArrayElement(jenv, jcertArray, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }

        QCC_ASSERT(jcertX509);

        CertificateX509* certX509 = GetHandle<CertificateX509*>(jcertX509);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }
        QCC_ASSERT(certX509);

        certChain[i] = *certX509;
    }

    memset(manifests, 0, maniCount * sizeof(manifests[0]));
    memset(jmanifests, 0, maniCount * sizeof(jmanifests[0]));

    for (size_t i = 0; i < maniCount; ++i) {
        jmanifests[i] = (jstring) GetObjectArrayElement(jenv, jmanifestobjs, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }
        manifests[i] = jenv->GetStringUTFChars(jmanifests[i], NULL);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }
    }

    status = pconfPtr->Claim(certificateAuthority, adminGroupId, adminGroup, certChain, jcertChainCount, manifests, jmaniCount);

exit:

    for (size_t i = 0; i < maniCount; ++i) {
        if (manifests[i]) {
            jenv->ReleaseStringUTFChars(jmanifests[i], manifests[i]);
        }
    }

    delete [] certChain;
    delete [] manifests;
    delete [] jmanifests;
    delete [] eccX;
    delete [] eccY;
    delete [] keyID;
    delete [] adminGroupEccX;
    delete [] adminGroupEccY;
    delete [] adminGroupKeyID;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    updateIdentity
 * Signature: ([Lorg/alljoyn/bus/common/CertificateX509;J[Ljava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_updateIdentity(JNIEnv* jenv, jobject thiz, jobjectArray jcertArray, jlong jcertCount, jobjectArray jmanifestobjs, jlong jmaniCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }
    QStatus status = ER_OK;

    CertificateX509* certArray = new CertificateX509[jcertCount];
    const char** manifests = new const char*[jmaniCount];
    jstring* jmanifests = new jstring[jmaniCount];

    for (long i = 0; i < jcertCount; ++i) {
        jobject jcertX509 = GetObjectArrayElement(jenv, jcertArray, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }

        QCC_ASSERT(jcertX509);

        CertificateX509* certX509 = GetHandle<CertificateX509*>(jcertX509);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }
        QCC_ASSERT(certX509);

        certArray[i] = *certX509;
    }

    memset(manifests, 0, jmaniCount * sizeof(manifests[0]));
    memset(jmanifests, 0, jmaniCount * sizeof(jmanifests[0]));

    for (long i = 0; i < jmaniCount; ++i) {
        jmanifests[i] = (jstring) GetObjectArrayElement(jenv, jmanifestobjs, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }
        manifests[i] = jenv->GetStringUTFChars(jmanifests[i], NULL);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }
    }

    status = pconfPtr->UpdateIdentity(certArray, jcertCount, manifests, jmaniCount);

exit:

    for (jlong i = 0; i < jmaniCount; ++i) {
        if (manifests[i]) {
            jenv->ReleaseStringUTFChars(jmanifests[i], manifests[i]);
        }
    }

    delete [] certArray;
    delete [] manifests;
    delete [] jmanifests;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getIdentity
 * Signature: ()[Lorg/alljoyn/bus/common/CertificateX509;
 */
JNIEXPORT jobjectArray JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getIdentity(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return NULL;
    }
    std::vector<CertificateX509> identity;
    QStatus status = pconfPtr->GetIdentity(identity);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_CertificateX509, "<init>", "()V");
    JLocalRef<jobjectArray> retIdentity = jenv->NewObjectArray(identity.size(), CLS_CertificateX509, NULL);

    for (unsigned int i = 0; i < identity.size(); i++) {
        JLocalRef<jobject> jidcert = jenv->NewObject(CLS_CertificateX509, mid);
        if (!jidcert) {
            return NULL;
        }

        SetHandle(jidcert, &identity[i]);
        if (jenv->ExceptionCheck()) {
            return NULL;
        }

        jenv->SetObjectArrayElement(retIdentity, i, jidcert);
        if (jenv->ExceptionCheck()) {
            return NULL;
        }
    }

    return retIdentity.move();
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getIdentityCertificateId
 * Signature: ()Lorg/alljoyn/bus/CertificateId;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getIdentityCertificateId(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return NULL;
    }

    String serial;
    KeyInfoNISTP256 retKeyInfo;
    QStatus status = pconfPtr->GetIdentityCertificateId(serial, retKeyInfo);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    const ECCPublicKey* retKey = retKeyInfo.GetPublicKey();

    jmethodID mid = jenv->GetMethodID(CLS_KeyInfoNISTP256, "<init>", "()V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find KeyInfoNISTP256 constructor", __FUNCTION__));
        return NULL;
    }

    JLocalRef<jobject> jretKeyInfo = jenv->NewObject(CLS_KeyInfoNISTP256, mid);

    jmethodID midC = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!midC) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        return NULL;
    }

    JLocalRef<jbyteArray> arrayX = ToJByteArray(retKey->GetX(), retKey->GetCoordinateSize());
    JLocalRef<jbyteArray> arrayY = ToJByteArray(retKey->GetY(), retKey->GetCoordinateSize());

    jobject jretKey = jenv->NewObject(CLS_ECCPublicKey, midC, arrayX.move(), arrayY.move());
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    jmethodID midSet = jenv->GetMethodID(CLS_KeyInfoNISTP256, "setPublicKey", "(Lorg/alljoyn/bus/common/ECCPublicKey;)");
    if (!midSet) {
        QCC_LogError(ER_FAIL, ("%s: Can't find setPublicKey", __FUNCTION__));
        return NULL;
    }

    CallObjectMethod(jenv, jretKeyInfo, midSet, jretKey);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    jmethodID midCCert = jenv->GetMethodID(CLS_CertificateId, "<init>", "(Ljava/util/String;Lorg/alljoyn/bus/common/KeyInfoNISTP256)V");
    if (!midC) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        return NULL;
    }

    jstring jserial = jenv->NewStringUTF(serial.c_str());
    return jenv->NewObject(CLS_CertificateId, midCCert, jserial, jretKeyInfo.move());
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    resetPolicy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_resetPolicy(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    QStatus status = pconfPtr->ResetPolicy();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getMembershipSummaries
 * Signature: ()[Lorg/alljoyn/bus/CertificateId;
 */
JNIEXPORT jobjectArray JNICALL Java_org_alljoyn_bus_PermissionConfigurator_getMembershipSummaries(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_KeyInfoNISTP256, "<init>", "()V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find KeyInfoNISTP256 constructor", __FUNCTION__));
        return NULL;
    }

    jmethodID midC = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!midC) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        return NULL;
    }

    jmethodID midSet = jenv->GetMethodID(CLS_KeyInfoNISTP256, "setPublicKey", "(Lorg/alljoyn/bus/common/ECCPublicKey;)");
    if (!midSet) {
        QCC_LogError(ER_FAIL, ("%s: Can't find setPublicKey", __FUNCTION__));
        return NULL;
    }

    jmethodID midCCert = jenv->GetMethodID(CLS_CertificateId, "<init>", "(Ljava/util/String;Lorg/alljoyn/bus/common/KeyInfoNISTP256)V");
    if (!midC) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        return NULL;
    }

    std::vector<String> serial;
    std::vector<KeyInfoNISTP256> retKeyInfo;
    QStatus status = pconfPtr->GetMembershipSummaries(serial, retKeyInfo);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    JLocalRef<jobjectArray> retMemberSummaries = jenv->NewObjectArray(serial.size(), CLS_CertificateId, NULL);

    for (unsigned int i = 0; i < serial.size(); i++) {
        const ECCPublicKey* retKey = retKeyInfo[i].GetPublicKey();

        JLocalRef<jobject> jretKeyInfo = jenv->NewObject(CLS_KeyInfoNISTP256, mid);

        JLocalRef<jbyteArray> arrayX = ToJByteArray(retKey->GetX(), retKey->GetCoordinateSize());
        JLocalRef<jbyteArray> arrayY = ToJByteArray(retKey->GetY(), retKey->GetCoordinateSize());

        jobject jretKey = jenv->NewObject(CLS_ECCPublicKey, midC, arrayX.move(), arrayY.move());
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            return NULL;
        }

        CallObjectMethod(jenv, jretKeyInfo, midSet, jretKey);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            return NULL;
        }

        jstring jserial = jenv->NewStringUTF(serial[i].c_str());
        jobject jcertid = jenv->NewObject(CLS_CertificateId, midCCert, jserial, jretKeyInfo.move());

        jenv->SetObjectArrayElement(retMemberSummaries, i, jcertid);
        if (jenv->ExceptionCheck()) {
            return NULL;
        }

    }

    return retMemberSummaries;

}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    installMembership
 * Signature: ([Lorg/alljoyn/bus/common/CertificateX509;J)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_installMembership(JNIEnv* jenv, jobject thiz, jobjectArray jcertArray, jlong jcertCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    CertificateX509* certArray = new CertificateX509[jcertCount];

    for (long i = 0; i < jcertCount; ++i) {
        jobject jcertX509 = GetObjectArrayElement(jenv, jcertArray, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            delete [] certArray;
            return;
        }

        QCC_ASSERT(jcertX509);

        CertificateX509* certX509 = GetHandle<CertificateX509*>(jcertX509);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            delete [] certArray;
            return;
        }
        QCC_ASSERT(certX509);

        certArray[i] = *certX509;
    }

    QStatus status = pconfPtr->InstallMembership(certArray, jcertCount);

    delete [] certArray;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    startManagement
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_startManagement(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    QStatus status = pconfPtr->StartManagement();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    endManagement
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_endManagement(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    QStatus status = pconfPtr->EndManagement();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    installManifests
 * Signature: ([Ljava/lang/String;JZ)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_PermissionConfigurator_installManifests(JNIEnv* jenv, jobject thiz, jobjectArray jmanifestobjs, jlong maniCount, jboolean jappend)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator* pconfPtr = GetHandle<PermissionConfigurator*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!pconfPtr) {
        QCC_LogError(ER_FAIL, ("%s: pconfPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "PermissionConfigurator object is null");
        return;
    }

    QStatus status = ER_OK;

    const char** manifests = new const char*[maniCount];
    jstring* jmanifests = new jstring[maniCount];
    memset(manifests, 0, maniCount * sizeof(manifests[0]));
    memset(jmanifests, 0, maniCount * sizeof(jmanifests[0]));

    for (jlong i = 0; i < maniCount; ++i) {
        jmanifests[i] = (jstring) GetObjectArrayElement(jenv, jmanifestobjs, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }
        manifests[i] = jenv->GetStringUTFChars(jmanifests[i], NULL);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            goto exit;
        }
    }

    status = pconfPtr->InstallManifests(manifests, maniCount, jappend);

exit:

    for (jlong i = 0; i < maniCount; ++i) {
        if (manifests[i]) {
            jenv->ReleaseStringUTFChars(jmanifests[i], manifests[i]);
        }
    }

    delete [] manifests;
    delete [] jmanifests;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}
