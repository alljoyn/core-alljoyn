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

#include "JBusAttachment.h"
#include "alljoyn_java.h"
#include "alljoyn_jni_helper.h"

using namespace ajn;

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
        Throw("java/lang/NullPointerException", "BusAttachment was null");
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
    if (jenv->ExceptionCheck() || !secPtr) {
        QCC_LogError(ER_FAIL, ("%s: Exception or secPtr is null", __FUNCTION__));
        return NULL;
    }

    PermissionConfigurator::ApplicationState state;
    QStatus status = secPtr->GetApplicationState(state);
    QCC_DbgPrintf(("%s: state = %d", __FUNCTION__, state));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    jobject retState;

    if (state == PermissionConfigurator::ApplicationState::NOT_CLAIMABLE) {
        jfieldID jfid = jenv->GetStaticFieldID(CLS_PermissionConfiguratorApplicationState, "NOT_CLAIMABLE", "Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;");
        retState = jenv->GetStaticObjectField(CLS_PermissionConfiguratorApplicationState, jfid);
    } else if (state == PermissionConfigurator::ApplicationState::CLAIMABLE) {
        jfieldID jfid = jenv->GetStaticFieldID(CLS_PermissionConfiguratorApplicationState, "CLAIMABLE", "Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;");
        retState = jenv->GetStaticObjectField(CLS_PermissionConfiguratorApplicationState, jfid);
    } else if (state == PermissionConfigurator::ApplicationState::CLAIMED) {
        jfieldID jfid = jenv->GetStaticFieldID(CLS_PermissionConfiguratorApplicationState, "CLAIMED", "Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;");
        retState = jenv->GetStaticObjectField(CLS_PermissionConfiguratorApplicationState, jfid);
    } else if (state == PermissionConfigurator::ApplicationState::NEED_UPDATE) {
        jfieldID jfid = jenv->GetStaticFieldID(CLS_PermissionConfiguratorApplicationState, "NEED_UPDATE", "Lorg/alljoyn/bus/PermissionConfigurator$ApplicationState;");
        retState = jenv->GetStaticObjectField(CLS_PermissionConfiguratorApplicationState, jfid);
    }

    return retState;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getManifestTemplateDigest
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getManifestTemplateDigest
    (JNIEnv*, jobject);

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getEccPublicKey
 * Signature: ()Lorg/alljoyn/bus/common/ECCPublicKey;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getEccPublicKey(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck() || !secPtr) {
        QCC_LogError(ER_FAIL, ("%s: Exception or secPtr is null", __FUNCTION__));
        return NULL;
    }

    qcc::ECCPublicKey eccPublicKey;
    QStatus status = secPtr->GetEccPublicKey(eccPublicKey);
    QCC_DbgPrintf(("%s: eccPublicKey = %d", __FUNCTION__, eccPublicKey.ToString()));

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }

    //TODO
    return NULL;
}

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getManufacturerCertificate
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getManufacturerCertificate
    (JNIEnv*, jobject);

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getManifestTemplate
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getManifestTemplate(JNIEnv* jenv, jobject thiz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    SecurityApplicationProxy* secPtr = GetHandle<SecurityApplicationProxy*>(thiz);
    if (jenv->ExceptionCheck() || !secPtr) {
        QCC_LogError(ER_FAIL, ("%s: Exception or secPtr is null", __FUNCTION__));
        return NULL;
    }

    char* manifestTemplate;
    QStatus status = secPtr->GetManifestTemplate(&manifestTemplate);

    if (status != ER_OK) {
        jenv->ThrowNew(CLS_BusException, QCC_StatusText(status));
        delete manifestTemplate;
        manifestTemplate = NULL;
        return NULL;
    }

    return jenv->NewStringUTF(manifestTemplate);
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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_claim(JNIEnv* jenv, jobject thiz, jobject, jobject, jobject, jobjectArray, jlong jcertChainCount, jobjectArray jmanifestobjs, jlong jmaniCount)
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

    QStatus status = ER_FAIL;

    qcc::KeyInfoNISTP256 certificateAuthority;
    qcc::GUID128 adminGroupId;
    qcc::KeyInfoNISTP256 adminGroup;
    qcc::CertificateX509* certChain = new qcc::CertificateX509[jcertChainCount];

    size_t maniCount = (size_t) jmaniCount;
    const char** manifests = new const char*[maniCount];
    jstring* jmanifests = new jstring[maniCount];
    memset(manifests, 0, maniCount * sizeof(manifests[0]));
    memset(jmanifests, 0, maniCount * sizeof(jmanifests[0]));

    for (size_t i = 0; i < maniCount; ++i) {
        jmanifests[i] = (jstring) GetObjectArrayElement(jenv, jmanifestobjs, i);
        if (jenv->ExceptionCheck()) {
            goto exit;
        }
        manifests[i] = jenv->GetStringUTFChars(jmanifests[i], NULL);
        if (jenv->ExceptionCheck()) {
            goto exit;
        }
    }

    status = secPtr->Claim(certificateAuthority, adminGroupId, adminGroup, certChain, jcertChainCount, manifests, jmaniCount);

exit:
    for (size_t i = 0; i < maniCount; ++i) {
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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_updateIdentity
    (JNIEnv*, jobject, jobjectArray, jlong, jobjectArray, jlong);

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    updatePolicy
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_updatePolicy
    (JNIEnv*, jobject, jstring);

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
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_installMembership
    (JNIEnv*, jobject, jobjectArray, jlong);

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    removeMembership
 * Signature: (Ljava/lang/String;Lorg/alljoyn/bus/common/KeyInfoNISTP256;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_removeMembership
    (JNIEnv*, jobject, jstring, jobject);

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
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    getIdentity
 * Signature: ()Lorg/alljoyn/bus/common/IdentityCertificate;
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_getIdentity
    (JNIEnv*, jobject);

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
JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_signManifest
    (JNIEnv*, jclass, jobject, jobject, jstring);

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    destroySignedManifest
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_destroySignedManifest
    (JNIEnv*, jclass, jstring);

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    computeManifestDigest
 * Signature: (Ljava/lang/String;Lorg/alljoyn/bus/common/CertificateX509;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_computeManifestDigest
    (JNIEnv*, jclass, jstring, jobject);

/*
 * Class:     org_alljoyn_bus_SecurityApplicationProxy
 * Method:    destroyManifestDigest
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_SecurityApplicationProxy_destroyManifestDigest
    (JNIEnv*, jclass, jbyteArray);

