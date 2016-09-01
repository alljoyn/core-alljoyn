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
#include <alljoyn/SecurityApplicationProxy.h>
#include <qcc/Crypto.h>

#include "JBusAttachment.h"
#include "alljoyn_java.h"
#include "alljoyn_jni_helper.h"

using namespace ajn;
using namespace qcc;

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    create
 * Signature: (Lorg/alljoyn/bus/BusAttachment;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_create(JNIEnv* jenv, jobject thiz, jobject jbusAttachment, jstring jbusName, jint sessionId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbusAttachment);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("%s: NULL bus pointer", __FUNCTION__));
        Throw("java/lang/NullPointerException", "NULL bus pointer");
        return;
    }

    QCC_DbgPrintf(("%s: Refcount on busPtr is %d", __FUNCTION__, busPtr->GetRef()));

    JString busName(jbusName);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception converting busName", __FUNCTION__));
        return;
    }

    SecurityApplicationProxy* securityApplicationProxyPtr = new SecurityApplicationProxy(*busPtr, busName.c_str(), sessionId);
    if (!securityApplicationProxyPtr) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    SetHandle(thiz, securityApplicationProxyPtr);

    if (jenv->ExceptionCheck()) {
        busPtr->DecRef();
        delete securityApplicationProxyPtr;
        securityApplicationProxyPtr = NULL;
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_destroy(JNIEnv* jenv, jobject thiz, jobject jbus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* sapPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (sapPtr == NULL) {
        QCC_DbgPrintf(("%s: Already destroyed. Returning.", __FUNCTION__));
        return;
    }

    delete sapPtr;
    sapPtr = NULL;

    SetHandle(thiz, NULL);

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (busPtr == NULL) {
        QCC_DbgPrintf(("%s: Already destroyed. Returning.", __FUNCTION__));
        return;
    }

    // Decrement the ref pointer so the BusAttachment can be released.
    busPtr->DecRef();
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getSecurityApplicationVersion
 * Signature: ()S
 */
JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getSecurityApplicationVersion(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return 0;
    }

    uint16_t version;
    QStatus status = secPtr->GetSecurityApplicationVersion(version);
    QCC_DbgPrintf(("%s: versionNumber = %d", __FUNCTION__, version));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return version;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getApplicationState
 * Signature: ()Lorg/alljoyn/bus/PermissionConfigurator/ApplicationState;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getApplicationState(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception or secPtr is null", __FUNCTION__));
        return NULL;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return NULL;
    }

    PermissionConfigurator::ApplicationState state;
    QStatus status = secPtr->GetApplicationState(state);
    QCC_DbgPrintf(("%s: state = %d", __FUNCTION__, state));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jobject retState = NULL;
    jfieldID jfid = NULL;

    if (state == PermissionConfigurator::ApplicationState::NOT_CLAIMABLE) {
        jfid = jenv->GetStaticFieldID(CLS_PermissionConfiguratorApplicationState, "NOT_CLAIMABLE", "Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;");
    } else if (state == PermissionConfigurator::ApplicationState::CLAIMABLE) {
        jfid = jenv->GetStaticFieldID(CLS_PermissionConfiguratorApplicationState, "CLAIMABLE", "Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;");
    } else if (state == PermissionConfigurator::ApplicationState::CLAIMED) {
        jfid = jenv->GetStaticFieldID(CLS_PermissionConfiguratorApplicationState, "CLAIMED", "Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;");
    } else if (state == PermissionConfigurator::ApplicationState::NEED_UPDATE) {
        jfid = jenv->GetStaticFieldID(CLS_PermissionConfiguratorApplicationState, "NEED_UPDATE", "Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;");
    }

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: error getting fieldID", __FUNCTION__));
        return NULL;
    }

    if (!jfid) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(ER_INVALID_APPLICATION_STATE));
        return NULL;
    }

    retState = jenv->GetStaticObjectField(CLS_PermissionConfiguratorApplicationState, jfid);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: error getting field", __FUNCTION__));
        return NULL;
    }

    return retState;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getManifestTemplateDigest
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getManifestTemplateDigest(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return NULL;
    }

    size_t expectedSize = Crypto_SHA256::DIGEST_SIZE;
    uint8_t* manifestTemplateDigest = new uint8_t[expectedSize];

    QStatus status = secPtr->GetManifestTemplateDigest(manifestTemplateDigest, expectedSize);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jbyteArray retArr = ToJByteArray(manifestTemplateDigest, expectedSize);

    delete manifestTemplateDigest;

    return retArr;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getEccPublicKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getEccPublicKey(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception or secPtr is null", __FUNCTION__));
        return NULL;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return NULL;
    }

    ECCPublicKey eccPublicKey;
    QStatus status = secPtr->GetEccPublicKey(eccPublicKey);
    QCC_DbgPrintf(("%s: eccPublicKey = %s", __FUNCTION__, eccPublicKey.ToString().c_str()));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jmethodID mid = jenv->GetMethodID(CLS_ECCPublicKey, "<init>", "([B[B)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find ECCPublicKey constructor", __FUNCTION__));
        Throw("java/lang/NoSuchMethodException", NULL);
        return NULL;
    }

    jbyteArray arrayX = ToJByteArray(eccPublicKey.GetX(), eccPublicKey.GetCoordinateSize());
    jbyteArray arrayY = ToJByteArray(eccPublicKey.GetY(), eccPublicKey.GetCoordinateSize());

    return jenv->NewObject(CLS_ECCPublicKey, mid, arrayX, arrayY);
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getManufacturerCertificate
 * Signature: ()[Lorg/alljoyn/bus/common/CertificateX509;
 */
JNIEXPORT jobjectArray JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getManufacturerCertificate(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception or secPtr is null", __FUNCTION__));
        return NULL;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return NULL;
    }

    MsgArg manufacturerCertificate;

    QStatus status = secPtr->GetManufacturerCertificate(manufacturerCertificate);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
    //TODO ASACORE-3233 need to convert msgarg to certificate x509 array
    return NULL;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getManifestTemplate
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getManifestTemplate(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception or secPtr is null", __FUNCTION__));
        return NULL;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return NULL;
    }

    AJ_PSTR manifestTemplate = nullptr;

    QStatus status = secPtr->GetManifestTemplate(&manifestTemplate);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        secPtr->DestroyManifestTemplate(manifestTemplate);
        return NULL;
    }

    jstring retStr = (jstring) jenv->NewStringUTF(manifestTemplate);

    secPtr->DestroyManifestTemplate(manifestTemplate);

    return retStr;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getClaimCapabilities
 * Signature: ()S
 */
JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getClaimCapabilities(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return 0;
    }

    uint16_t claimCapabilities;
    QStatus status = secPtr->GetClaimCapabilities(claimCapabilities);
    QCC_DbgPrintf(("%s: claimCapabilities = %d", __FUNCTION__, claimCapabilities));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return claimCapabilities;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getClaimCapabilityAdditionalInfo
 * Signature: ()S
 */
JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getClaimCapabilityAdditionalInfo(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return 0;
    }

    uint16_t claimCapabilitiesAdditionalInfo;
    QStatus status = secPtr->GetClaimCapabilityAdditionalInfo(claimCapabilitiesAdditionalInfo);
    QCC_DbgPrintf(("%s: claimCapabilitiesAdditionalInfo = %d", __FUNCTION__, claimCapabilitiesAdditionalInfo));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return claimCapabilitiesAdditionalInfo;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    claim
 * Signature: (Lorg/alljoyn/bus/common/KeyInfoNISTP256;Ljava/util/UUID;Lorg/alljoyn/bus/common/KeyInfoNISTP256;[Lorg/alljoyn/bus/common/CertificateX509;J[Ljava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_claim(JNIEnv* jenv, jobject thiz, jobject jcertAuth, jobject jadminGroupId, jobject jadminGroup, jobjectArray jcertArray, jlong jcertChainCount, jobjectArray jmanifestobjs, jlong jmaniCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
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

    jfieldID fidX = jenv->GetFieldID(CLS_ECCPublicKey, "x", "[B");

    if (!fidX) {
        QCC_LogError(ER_FAIL, ("%s: Can't find x field in ECCPrivateKey", __FUNCTION__));
        return;
    }

    jfieldID fidY = jenv->GetFieldID(CLS_ECCPublicKey, "y", "[B");

    if (!fidY) {
        QCC_LogError(ER_FAIL, ("%s: Can't find y field in ECCPrivateKey", __FUNCTION__));
        return;
    }

    jobject jcertAuthPublicKey = (jobject) jenv->GetObjectField(jcertAuth, fid);
    jbyteArray jkeyID = (jbyteArray) jenv->GetObjectField(jcertAuth, fidKeyId);

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jcertAuthPublicKey, fidX);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jcertAuthPublicKey, fidY);

    unsigned char* eccX = ToByteArray(jeccX);
    unsigned char* eccY = ToByteArray(jeccY);
    unsigned char* keyID = ToByteArray(jkeyID);

    if (jenv->ExceptionCheck()) {
        delete eccX;
        delete eccY;
        delete keyID;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    ECCPublicKey eccPublicKey;

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));

    if (status != ER_OK) {
        delete eccX;
        delete eccY;
        delete keyID;
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    KeyInfoNISTP256 certificateAuthority;

    certificateAuthority.SetPublicKey(&eccPublicKey);
    certificateAuthority.SetKeyId(keyID, jenv->GetArrayLength(jkeyID));

    jmethodID mid = jenv->GetMethodID(CLS_JAVA_UTIL_UUID, "toString", "()Ljava/lang/String;");
    if (!mid) {
        delete eccX;
        delete eccY;
        delete keyID;
        QCC_LogError(ER_FAIL, ("%s: Can't find UUID.toString", __FUNCTION__));
        return;
    }

    JString jstrguid((jstring) CallObjectMethod(jenv, jadminGroupId, mid));
    if (jenv->ExceptionCheck()) {
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

    jbyteArray jadminGroupEccX = (jbyteArray) jenv->GetObjectField(jadminGroupPublicKey, fidX);
    jbyteArray jadminGroupEccY = (jbyteArray) jenv->GetObjectField(jadminGroupPublicKey, fidY);

    unsigned char* adminGroupEccX = ToByteArray(jadminGroupEccX);
    unsigned char* adminGroupEccY = ToByteArray(jadminGroupEccY);
    unsigned char* adminGroupKeyID = ToByteArray(jadminGroupKeyID);

    if (jenv->ExceptionCheck()) {
        delete eccX;
        delete eccY;
        delete keyID;
        delete adminGroupEccX;
        delete adminGroupEccY;
        delete adminGroupKeyID;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    ECCPublicKey eccPublicKeyAdminGroup;

    status = eccPublicKeyAdminGroup.Import(adminGroupEccX, jenv->GetArrayLength(jadminGroupEccX), adminGroupEccY, jenv->GetArrayLength(jadminGroupEccY));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    KeyInfoNISTP256 adminGroup;

    certificateAuthority.SetPublicKey(&eccPublicKeyAdminGroup);
    certificateAuthority.SetKeyId(adminGroupKeyID, jenv->GetArrayLength(jadminGroupKeyID));

    CertificateX509* certChain = new CertificateX509[jcertChainCount];

    for (long i = 0; i < jcertChainCount; ++i) {
        jobject jcertX509 = GetObjectArrayElement(jenv, jcertArray, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            return;
        }

        QCC_ASSERT(jcertX509);

        CertificateX509* certX509 = GetHandle<CertificateX509*>(jcertX509);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            return;
        }
        QCC_ASSERT(certX509);

        certChain[i] = *certX509;
    }

    size_t maniCount = (size_t) jmaniCount;
    const char** manifests = new const char*[maniCount];
    jstring* jmanifests = new jstring[maniCount];
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

    status = secPtr->Claim(certificateAuthority, adminGroupId, adminGroup, certChain, jcertChainCount, manifests, jmaniCount);

exit:
    /*
     * Java garbage collector will trigger CertificateX509
     * destroy function to take care of memory
     * occupied by individual certificates
     */
    certChain = NULL;

    for (size_t i = 0; i < maniCount; ++i) {
        if (manifests[i]) {
            jenv->ReleaseStringUTFChars(jmanifests[i], manifests[i]);
        }
    }

    delete [] manifests;
    delete [] jmanifests;
    delete eccX;
    delete eccY;
    delete keyID;
    delete adminGroupEccX;
    delete adminGroupEccY;
    delete adminGroupKeyID;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getClaimableApplicationVersion
 * Signature: ()S
 */
JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getClaimableApplicationVersion(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return 0;
    }

    uint16_t version;
    QStatus status = secPtr->GetClaimableApplicationVersion(version);
    QCC_DbgPrintf(("%s: version = %d", __FUNCTION__, version));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return version;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_reset(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return;
    }

    QStatus status = secPtr->Reset();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    updateIdentity
 * Signature: ([Lorg/alljoyn/bus/common/CertificateX509;J[Ljava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_updateIdentity(JNIEnv* jenv, jobject thiz, jobjectArray jcertArray, jlong jcertCount, jobjectArray jmanifestobjs, jlong jmaniCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return;
    }

    QStatus status = ER_OK;

    CertificateX509* certArray = new CertificateX509[jcertCount];

    for (long i = 0; i < jcertCount; ++i) {
        jobject jcertX509 = GetObjectArrayElement(jenv, jcertArray, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            return;
        }

        QCC_ASSERT(jcertX509);

        CertificateX509* certX509 = GetHandle<CertificateX509*>(jcertX509);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            return;
        }
        QCC_ASSERT(certX509);

        certArray[i] = *certX509;
    }

    const char** manifests = new const char*[jmaniCount];
    jstring* jmanifests = new jstring[jmaniCount];
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

    status = secPtr->UpdateIdentity(certArray, jcertCount, manifests, jmaniCount);

exit:

    /*
     * Java garbage collector will trigger CertificateX509
     * destroy function to take care of memory
     * occupied by individual certificates
     */
    certArray = NULL;

    for (jlong i = 0; i < jmaniCount; ++i) {
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

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    updatePolicy
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_updatePolicy(JNIEnv* jenv, jobject thiz, jstring jpolicy)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return;
    }

    JString policy(jpolicy);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    QStatus status = secPtr->UpdatePolicy(policy.c_str());

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    resetPolicy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_resetPolicy(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return;
    }

    QStatus status = secPtr->ResetPolicy();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    installMembership
 * Signature: ([Lorg/alljoyn/bus/common/CertificateX509;J)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_installMembership(JNIEnv* jenv, jobject thiz, jobjectArray jcertArray, jlong jcertCount)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return;
    }

    CertificateX509* certArray = new CertificateX509[jcertCount];

    for (long i = 0; i < jcertCount; ++i) {
        jobject jcertX509 = GetObjectArrayElement(jenv, jcertArray, i);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            return;
        }

        QCC_ASSERT(jcertX509);

        CertificateX509* certX509 = GetHandle<CertificateX509*>(jcertX509);
        if (jenv->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
            return;
        }
        QCC_ASSERT(certX509);

        certArray[i] = *certX509;
    }

    QStatus status = secPtr->InstallMembership(certArray, jcertCount);

    /*
     * Java garbage collector will trigger CertificateX509
     * destroy function to take care of memory
     * occupied by individual certificates
     */
    certArray = NULL;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    removeMembership
 * Signature: (Ljava/lang/String;Lorg/alljoyn/bus/common/KeyInfoNISTP256;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_removeMembership(JNIEnv* jenv, jobject thiz, jstring jserial, jobject jissuerKeyInfo)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return;
    }

    JString serial(jserial);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
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

    jfieldID fidX = jenv->GetFieldID(CLS_ECCPublicKey, "x", "[B");

    if (!fidX) {
        QCC_LogError(ER_FAIL, ("%s: Can't find x field in ECCPrivateKey", __FUNCTION__));
        return;
    }

    jfieldID fidY = jenv->GetFieldID(CLS_ECCPublicKey, "y", "[B");

    if (!fidY) {
        QCC_LogError(ER_FAIL, ("%s: Can't find y field in ECCPrivateKey", __FUNCTION__));
        return;
    }

    jobject jIssuerPublicKey = (jobject) jenv->GetObjectField(jissuerKeyInfo, fid);
    jbyteArray jkeyID = (jbyteArray) jenv->GetObjectField(jissuerKeyInfo, fidKeyId);

    jbyteArray jeccX = (jbyteArray) jenv->GetObjectField(jIssuerPublicKey, fidX);
    jbyteArray jeccY = (jbyteArray) jenv->GetObjectField(jIssuerPublicKey, fidY);

    unsigned char* eccX = ToByteArray(jeccX);
    unsigned char* eccY = ToByteArray(jeccY);
    unsigned char* keyID = ToByteArray(jkeyID);

    if (jenv->ExceptionCheck()) {
        delete eccX;
        delete eccY;
        delete keyID;
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    ECCPublicKey eccPublicKey;

    QStatus status = eccPublicKey.Import(eccX, jenv->GetArrayLength(jeccX), eccY, jenv->GetArrayLength(jeccY));

    if (status != ER_OK) {
        delete eccX;
        delete eccY;
        delete keyID;
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }

    KeyInfoNISTP256 certificateAuthority;

    certificateAuthority.SetPublicKey(&eccPublicKey);
    certificateAuthority.SetKeyId(keyID, jenv->GetArrayLength(jkeyID));

    status = secPtr->RemoveMembership(serial.c_str(), certificateAuthority);

    delete eccX;
    delete eccY;
    delete keyID;

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getManagedApplicationVersion
 * Signature: ()S
 */
JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getManagedApplicationVersion(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return 0;
    }

    uint16_t version;
    QStatus status = secPtr->GetManagedApplicationVersion(version);
    QCC_DbgPrintf(("%s: version = %d", __FUNCTION__, version));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return version;
}

/*
 * Class:     org_alljoyn_bus_PermissionConfigurator
 * Method:    getIdentity
 * Signature: ()Lorg/alljoyn/bus/common/CertificateX509;
 */
JNIEXPORT jobjectArray JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getIdentity(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return NULL;
    }

    MsgArg identity;
    QStatus status = secPtr->GetIdentity(identity);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    Throw("java/lang/NoSuchMethodException", "Method not implemented yet");
    //TODO ASACORE-3233 need to convert msgarg to certificate x509 array
    return NULL;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getPolicyVersion
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getPolicyVersion(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return 0;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return 0;
    }

    uint32_t version;
    QStatus status = secPtr->GetPolicyVersion(version);
    QCC_DbgPrintf(("%s: version = %d", __FUNCTION__, version));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }

    return version;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    startManagement
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_startManagement(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return;
    }

    QStatus status = secPtr->StartManagement();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    endManagement
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_endManagement(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return;
    }

    if (!secPtr) {
        QCC_LogError(ER_FAIL, ("%s: secPtr is null", __FUNCTION__));
        Throw("java/lang/NullPointerException", "SecurityApplicationProxy object is null");
        return;
    }

    QStatus status = secPtr->EndManagement();

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return;
    }
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    signManifest
 * Signature: (Lorg/alljoyn/bus/common/CertificateX509;Lorg/alljoyn/bus/common/ECCPrivateKey;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_signManifest(JNIEnv* jenv, jclass, jobject identityCertificate, jobject jeccPrivateKey, jstring junsignedManifestXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    JString unsignedManifestXml(junsignedManifestXml);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception converting busName", __FUNCTION__));
        return NULL;
    }

    CertificateX509* cx509Ptr = GetHandle<CertificateX509*>(identityCertificate);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    ECCPrivateKey eccPrivateKey;

    jfieldID fid = jenv->GetFieldID(CLS_ECCPrivateKey, "d", "[B");

    if (!fid) {
        QCC_LogError(ER_FAIL, ("%s: Can't find d field in ECCPrivateKey", __FUNCTION__));
        return NULL;
    }

    jbyteArray jeccD = (jbyteArray) jenv->GetObjectField(jeccPrivateKey, fid);

    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    unsigned char* eccD = ToByteArray(jeccD);
    QStatus status = eccPrivateKey.Import(eccD, jenv->GetArrayLength(jeccD));

    if (status != ER_OK) {
        delete eccD;
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    AJ_PSTR retStr = nullptr;

    status = SecurityApplicationProxy::SignManifest(*cx509Ptr, eccPrivateKey, unsignedManifestXml.c_str(), &retStr);

    if (status != ER_OK) {
        delete eccD;
        SecurityApplicationProxy::DestroySignedManifest(retStr);
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jstring jretStr = (jstring) jenv->NewStringUTF(retStr);

    delete eccD;
    SecurityApplicationProxy::DestroySignedManifest(retStr);

    return jretStr;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    computeManifestDigest
 * Signature: (Ljava/lang/String;Lorg/alljoyn/bus/common/CertificateX509;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_computeManifestDigest(JNIEnv* jenv, jclass, jstring junsignedManifestXml, jobject identityCertificate)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    JString unsignedManifestXml(junsignedManifestXml);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception converting busName", __FUNCTION__));
        return NULL;
    }

    CertificateX509* cx509Ptr = GetHandle<CertificateX509*>(identityCertificate);
    if (jenv->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("%s: Exception", __FUNCTION__));
        return NULL;
    }

    size_t digest_size;

    SecurityApplicationProxy::ComputeManifestDigest(unsignedManifestXml.c_str(), *cx509Ptr, NULL, &digest_size);

    uint8_t* digest = nullptr;

    QStatus status = SecurityApplicationProxy::ComputeManifestDigest(unsignedManifestXml.c_str(), *cx509Ptr, &digest, &digest_size);

    if (status != ER_OK) {
        SecurityApplicationProxy::DestroyManifestDigest(digest);
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jbyteArray retArray = ToJByteArray(digest, digest_size);

    SecurityApplicationProxy::DestroyManifestDigest(digest);

    return retArray;
}
