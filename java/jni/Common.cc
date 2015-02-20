/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

#include "Common.h"

JavaVM* Common::jvm = NULL;
jclass Common::applicationInfoClass = NULL;
jmethodID Common::appInfoConstructorMID = NULL;
jfieldID Common::infoAppNameFID = NULL;
jfieldID Common::infoDevNameFID = NULL;
jfieldID Common::infoFriendlyNameFID = NULL;
jfieldID Common::infoAppIdFID = NULL;
jfieldID Common::infoPubKeyFID = NULL;
jclass Common::secMgrClass = NULL;
jfieldID Common::mgrPointerFID = NULL;
jmethodID Common::mgrNewEventMID = NULL;
jmethodID Common::mgrMnfCallbackMID = NULL;
jclass Common::ruleClass = NULL;
jmethodID Common::ruleConstructorMID = NULL;
jclass Common::memberClass = NULL;
jmethodID Common::memberConstructorMID = NULL;
Storage* Common::storage = NULL;

#define QCC_MODULE "SEC_MGR"

int Common::SetStringField(JNIEnv* env, jobject object, jfieldID id, qcc::String string)
{
    jstring jString = env->NewStringUTF(string.c_str());
    if (jString == NULL) {
        return -1;
    }
    env->SetObjectField(object, id, jString);
    if (env->ExceptionCheck()) {
        return -1;
    }
    env->DeleteLocalRef(jString);
    return 0;
}

// -1 = error, 0 = success thread was already attached, 1 = success thread had to be attached. Detach when finished
int Common::GetJNIEnv(JNIEnv** envp)
{
    int result = -1;
    if (Common::jvm) {
        jint r = Common::jvm->GetEnv((void**)envp, JNI_VERSION_1_2);
        if (r == JNI_OK) {
            return 0;
        }
        if (r == JNI_EDETACHED) {
#ifndef QCC_OS_ANDROID
            if (JNI_OK == Common::jvm->AttachCurrentThread((void**)envp, NULL)) {
                return 1;
            }
#else
            if (JNI_OK == Common::jvm->AttachCurrentThread(envp, NULL)) {
                return 1;
            }
#endif
        }
    }
    return result;
}

void Common::DetachThread(JNIEnv* env, int attached)
{
    if (attached == 1) {
        Common::jvm->DetachCurrentThread();
    } else {
        //Don't leave any exceptions pending if the thread was already attached to the JVM.
        env->ExceptionClear();
    }
}

Common::Common(JNIEnv* env,
               jobject javaSecurityMgr,
               SecurityManager* mngr) :
    jSecmgr(env->NewGlobalRef(javaSecurityMgr)), secMgr(mngr)
{
    if (jSecmgr) {
        env->SetLongField(jSecmgr, mgrPointerFID, (jlong) this);
        secMgr->SetManifestListener(this);
        secMgr->RegisterApplicationListener(this);
    }
}

Common::~Common()
{
    JNIEnv* env;
    int result = GetJNIEnv(&env);
    if (result != 1) {
        env->DeleteGlobalRef(jSecmgr);
        DetachThread(env, result);
    }
    delete storage;
}

void Common::Throw(JNIEnv* env, const char* name, const char* msg)
{
    jclass clazz = env->FindClass(name);
    if (clazz) {
        env->ThrowNew(clazz, msg);
        env->DeleteLocalRef(clazz);
    }
    clazz = NULL;
}

void Common::initCommon(JNIEnv* env,
                        jclass securityMgrClass,
                        jclass appInfoClass,
                        jclass localRuleClass,
                        jclass localMemberClass)
{
    if (env->GetJavaVM(&jvm)) {
        //not clear if an exception is pending when this code fails
        if (env->ExceptionCheck() == JNI_FALSE) {
            Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Failed to get JVM pointer");
        }
        return;
    }

    applicationInfoClass = (jclass)env->NewGlobalRef(appInfoClass);
    if (applicationInfoClass == NULL) {
        return; //exception will be pending.
    }
    appInfoConstructorMID = env->GetMethodID(appInfoClass, CONSTRUCTOR_METHOD_NAME, "(II)V");
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }
    infoAppIdFID = env->GetFieldID(appInfoClass, "applicationId", STRING_CLASS);
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }
    infoFriendlyNameFID = env->GetFieldID(appInfoClass, "userFriendlyName", STRING_CLASS);
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }
    infoDevNameFID = env->GetFieldID(appInfoClass, "deviceName", STRING_CLASS);
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }
    infoAppNameFID = env->GetFieldID(appInfoClass, "applicationName", STRING_CLASS);
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }
    infoPubKeyFID = env->GetFieldID(appInfoClass, "publicKey", "[B");
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }
    secMgrClass = (jclass)env->NewGlobalRef(securityMgrClass);
    if (secMgrClass == NULL) {
        return; //exception will be pending.
    }
    mgrPointerFID = env->GetFieldID(securityMgrClass, "pointer", "J");
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }
    mgrNewEventMID = env->GetMethodID(
        securityMgrClass,
        "onNewApplicationEvent",
        "(" APPLICATIONINFO_CLASS APPLICATIONINFO_CLASS ")V");
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }
    mgrMnfCallbackMID = env->GetMethodID(
        securityMgrClass,
        "approveManifest",
        "(" APPLICATIONINFO_CLASS "[" RULE_CLASS ")Z");
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }

    ruleClass = (jclass)env->NewGlobalRef(localRuleClass);
    if (ruleClass == NULL) {
        return; //call failed, exception is pending
    }
    ruleConstructorMID = env->GetMethodID(localRuleClass,
                                          CONSTRUCTOR_METHOD_NAME,
                                          "(" STRING_CLASS  "[" MEMBER_CLASS ")V");
    if (env->ExceptionCheck()) {
        return; //call failed, exception is pending
    }

    memberClass = (jclass)env->NewGlobalRef(localMemberClass);
    if (memberClass == NULL) {
        return; //call failed, exception is pending
    }
    memberConstructorMID = env->GetMethodID(localMemberClass,
                                            CONSTRUCTOR_METHOD_NAME,
                                            "(" STRING_CLASS  "II)V");
}

jobject Common::ToApplicationInfoObject(JNIEnv* env, const ApplicationInfo& info)
{
    jobject appInfo = env->NewObject(applicationInfoClass,
                                     appInfoConstructorMID,
                                     (jint)info.runningState,
                                     (jint)info.claimState);
    if (appInfo == NULL) {
        return NULL;
    }
    if (SetStringField(env, appInfo, infoFriendlyNameFID, info.userDefinedName)) {
        return NULL;
    }
    if (SetStringField(env, appInfo, infoAppNameFID, info.appName)) {
        return NULL;
    }
    if (SetStringField(env, appInfo, infoAppIdFID, info.peerID.ToString())) {
        return NULL;
    }
    if (SetStringField(env, appInfo, infoDevNameFID, info.deviceName)) {
        return NULL;
    }
    jbyteArray keyData = env->NewByteArray(KEY_ARRAY_SIZE);
    if (keyData) {
        jbyte* nativeData = env->GetByteArrayElements(keyData, NULL);
        if (nativeData) {
            uint8_t data[KEY_ARRAY_SIZE];
            size_t size = KEY_ARRAY_SIZE;
            if (ER_OK == info.publicKey.Export(data, &size)) {
                memcpy(nativeData, data, size);
                env->ReleaseByteArrayElements(keyData, nativeData, 0);
                env->SetObjectField(appInfo, infoPubKeyFID, keyData);
            } else {
                env->ReleaseByteArrayElements(keyData, nativeData, 0);
                Common::Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Bad key size.");
            }
            if (env->ExceptionCheck() == JNI_FALSE) {
                env->DeleteLocalRef(keyData);
            }
        }
    }
    return appInfo;
}

ApplicationInfo Common::ToNativeInfo(JNIEnv* env, jobject appInfo)
{
    ApplicationInfo info;
    if (appInfo == NULL) {
        Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return info;
    }
    jbyteArray keyData = (jbyteArray)env->GetObjectField(appInfo, infoPubKeyFID);
    if (env->ExceptionCheck()) {
        return info;
    }
    if (keyData == NULL) {
        Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Key not set ApplicationInfo");
        return info;
    }
    uint8_t nativeData[KEY_ARRAY_SIZE];
    env->GetByteArrayRegion(keyData, 0, sizeof(nativeData), (jbyte*)nativeData);
    if (env->ExceptionCheck()) {
        return info;
    }
    env->DeleteLocalRef(keyData);
    if (ER_OK != info.publicKey.Import(nativeData, sizeof(nativeData))) {
        Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Invalid public key data.");
    }
    info.peerID = GetStringField(env, appInfo, infoAppIdFID);
    return info;
}

Common* Common::GetNativePeer(JNIEnv* env,
                              jobject securityMgr)
{
    Common* cmn = (Common*)env->GetLongField(securityMgr, mgrPointerFID);
    if (cmn == NULL) {
        Throw(env, SECURITY_MNGT_EXCEPTION_CLASS, "Not initialized properly");
    }
    return cmn;
}

SecurityManager* Common::GetSecurityManager(JNIEnv* env, jobject securityMgr)
{
    SecurityManager* secMgr = NULL;
    Common* cmn = (Common*)env->GetLongField(securityMgr, mgrPointerFID);
    if (cmn) {
        secMgr = cmn->secMgr;
    }
    if (secMgr == NULL) {
        Throw(env, SECURITY_MNGT_EXCEPTION_CLASS, "Not initialized properly");
    }
    return secMgr;
}

SecurityManager* Common::GetSecurityManager(JNIEnv* env)
{
    if (secMgr == NULL) {
        Throw(env, SECURITY_MNGT_EXCEPTION_CLASS, "Not initialized properly");
    }
    return secMgr;
}

qcc::String Common::ToNativeString(JNIEnv* env, jstring jString)
{
    qcc::String string;
    if (jString) {
        const char* chars = env->GetStringUTFChars(jString, NULL);
        if (chars) {
            string = chars;
            env->ReleaseStringUTFChars(jString, chars);
            env->DeleteLocalRef(jString);
        } else {
            env->ExceptionClear();
        }
    } else {
        Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
    }
    return string;
}

qcc::String Common::GetStringField(JNIEnv* env, jobject object, jfieldID fieldID)
{
    jstring jString = (jstring)env->GetObjectField(object, fieldID);
    if (env->ExceptionCheck()) {
        qcc::String string;
        return string;
    }
    return ToNativeString(env, jString);
}

void Common::OnApplicationStateChange(const ApplicationInfo* oldAppInfo,
                                      const ApplicationInfo* newAppInfo)
{
    JNIEnv* env;
    int attached = GetJNIEnv(&env);
    if (attached == -1) {
        QCC_LogError(ER_FAIL, ("Could not handle ApplicationStateChange event"));
        return;
    }

    do {
        jobject newInfo = newAppInfo ? ToApplicationInfoObject(env, *newAppInfo) : NULL;
        if (env->ExceptionCheck()) {
            break;
        }
        jobject oldInfo = oldAppInfo ? ToApplicationInfoObject(env, *oldAppInfo) : NULL;
        if (env->ExceptionCheck()) {
            break;
        }
        env->CallVoidMethod(jSecmgr, mgrNewEventMID, newInfo, oldInfo);
        env->ExceptionClear(); //clear any pending exception.
        //Don't leave local references hanging around. We don't know how long this thread stays in native java land
        env->DeleteLocalRef(newInfo);
        env->DeleteLocalRef(oldInfo);
    } while (0);

    DetachThread(env, attached);
}

jobjectArray Common::ToMemberRules(JNIEnv* env, const PermissionPolicy::Rule::Member* members,
                                   size_t membersSize)
{
    jobjectArray jMembers = env->NewObjectArray(membersSize, memberClass, NULL);
    if (jMembers == NULL) {
        return NULL;
    }
    jstring jMemberName;
    jobject jMember;
    for (jsize i = 0; i < static_cast<int>(membersSize); i++) {
        const PermissionPolicy::Rule::Member* member = &members[i];
        jMemberName = env->NewStringUTF(member->GetMemberName().c_str());
        if (jMemberName == NULL) {
            goto error;
        }
        jint type = member->GetMemberType(); //ANY member as type info is not available.
        jint actionMask = member->GetActionMask();

        jMember = env->NewObject(memberClass, memberConstructorMID, jMemberName, actionMask, type);
        if (jMember == NULL) {
            goto error;
        }
        env->SetObjectArrayElement(jMembers, i, jMember);
        if (env->ExceptionCheck()) {
            goto error;
        }
        env->DeleteLocalRef(jMemberName); jMemberName = NULL;
        env->DeleteLocalRef(jMember); jMember = NULL;
    }
    return jMembers;
error:
    env->ExceptionClear();
    if (jMemberName) {
        env->DeleteLocalRef(jMemberName);
    }
    env->DeleteLocalRef(jMembers);
    return NULL;
}

jobjectArray Common::ToManifestRules(JNIEnv* env, const PermissionPolicy::Rule* manifestRules,
                                     const size_t manifestRulesCount)
{
    jobjectArray jRules = env->NewObjectArray(manifestRulesCount, ruleClass, NULL);
    if (jRules == NULL) {
        return NULL;
    }

    jobjectArray jMembers = NULL;
    jstring jIntfName = NULL;
    jobject jRule = NULL;
    for (jsize i = 0; i < static_cast<int>(manifestRulesCount); i++) {
        const PermissionPolicy::Rule* rule = &manifestRules[i];

        jMembers = ToMemberRules(env, rule->GetMembers(), rule->GetMembersSize());
        if (jMembers == NULL) {
            goto error;
        }
        jIntfName = env->NewStringUTF(rule->GetInterfaceName().c_str());
        if (jIntfName == NULL) {
            goto error;
        }
        jRule = env->NewObject(ruleClass, ruleConstructorMID, jIntfName, jMembers);
        if (jRule == NULL) {
            goto error;
        }
        env->SetObjectArrayElement(jRules, i, jRule);
        if (env->ExceptionCheck()) {
            goto error;
        }
        env->DeleteLocalRef(jRule); jRule = NULL;
        env->DeleteLocalRef(jMembers); jMembers = NULL;
        env->DeleteLocalRef(jIntfName); jIntfName = NULL;
    }
    return jRules;
error:
    env->ExceptionClear();
    if (jMembers) {
        env->DeleteLocalRef(jMembers);
    }
    if (jIntfName) {
        env->DeleteLocalRef(jIntfName);
    }
    if (jRule) {
        env->DeleteLocalRef(jRule);
    }
    env->DeleteLocalRef(jRules);
    return NULL;
}

bool Common::ApproveManifest(const ApplicationInfo& appInfo,
                             const PermissionPolicy::Rule* manifestRules,
                             const size_t manifestRulesCount)
{
    JNIEnv* env;
    int attached = Common::GetJNIEnv(&env);
    bool accept = false;
    if (attached != -1) {
        jobjectArray jManifestRules = Common::ToManifestRules(env, manifestRules, manifestRulesCount);
        if (jManifestRules != NULL) {
            accept = CallManifestCallback(env, appInfo, jManifestRules);
        }
        Common::DetachThread(env, attached);
    }
    return accept;
}

bool Common::CallManifestCallback(JNIEnv* env, const ApplicationInfo& appInfo, jobject manifest)
{
    bool result = false;
    jobject jAppInfo = ToApplicationInfoObject(env, appInfo);
    if (jAppInfo) {
        jboolean accepted = env->CallBooleanMethod(jSecmgr, mgrMnfCallbackMID, jAppInfo, manifest);
        //There could be a java exception pending
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else {
            result = (accepted == JNI_TRUE);
        }
        env->DeleteLocalRef(manifest);
        env->DeleteLocalRef(jAppInfo);
    }
    return result;
}

jbyteArray Common::ToKeyBytes(JNIEnv* env, const ECCPublicKey& publicKey)
{
    uint8_t data[ECC_COORDINATE_SZ + ECC_COORDINATE_SZ];
    size_t keySize = sizeof(data);
    QStatus status = publicKey.Export(data, &keySize);

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to retrieve publicKey"));
        Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                      "Failed to retrieve publicKey");
        return NULL;
    }

    jbyteArray jKey = env->NewByteArray(keySize);
    if (jKey == NULL) {
        return NULL;
    }
    //This may result in pending exceptions but we are returning anyway.
    env->SetByteArrayRegion(jKey, 0, keySize, (jbyte*)data);
    return jKey;
}

void Common::ToGUID(JNIEnv* env, jbyteArray jGuid, GUID128& guid)
{
    if (jGuid == NULL) {
        Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return;
    }
    jsize length = env->GetArrayLength(jGuid);
    if (env->ExceptionCheck()) {
        return;
    }
    if (length != ((jsize) GUID128::SIZE)) {
        Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Wrong GUID size");
    }
    jbyte* bytes = env->GetByteArrayElements(jGuid, NULL);
    if (bytes == NULL) {
        return;
    }

    guid.SetBytes((uint8_t*)bytes);
    env->ReleaseByteArrayElements(jGuid, bytes, JNI_ABORT);
}

#undef QCC_MODULE
