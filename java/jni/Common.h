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

#ifndef JAVA_JNI_COMMON_H_
#define JAVA_JNI_COMMON_H_

#include <jni.h>
#include <alljoyn/securitymgr/SecurityManager.h>

#define SECURITY_MNGT_EXCEPTION_CLASS "org/alljoyn/securitymgr/SecurityMngtException"
#define NULLPOINTEREXCEPTION_CLASS "java/lang/NullPointerException"
#define ILLEGALARGUMENTEXCEPTION_CLASS "java/lang/IllegalArgumentException"
#define STRING_CLASS "Ljava/lang/String;"
#define APPLICATIONINFO_CLASS "Lorg/alljoyn/securitymgr/ApplicationInfo;"
#define RULE_CLASS "Lorg/alljoyn/securitymgr/access/Rule;"
#define MEMBER_CLASS "Lorg/alljoyn/securitymgr/access/Member;"
#define OUTOFMEMORYERROR_CLASS "java/lang/OutOfMemoryError"
#define CONSTRUCTOR_METHOD_NAME "<init>"

#define GUID_SIZE 16
#define KEY_ARRAY_SIZE (qcc::ECC_COORDINATE_SZ << 1)

using namespace qcc;
using namespace ajn;
using namespace ajn::securitymgr;

class Common :
    ApplicationListener,
    public ManifestListener {
  private:
    jobject jSecmgr;
    SecurityManager* secMgr;

    static int SetStringField(JNIEnv* env,
                              jobject object,
                              jfieldID id,
                              qcc::String string);

    static jobjectArray ToMemberRules(JNIEnv* env,
                                      const PermissionPolicy::Rule::Member* members,
                                      size_t membersSize);

    void OnApplicationStateChange(const ApplicationInfo* oldAppInfo,
                                  const ApplicationInfo* newAppInfo);

  public:
    Common(JNIEnv* env,
           jobject javaSecurityMgr,
           SecurityManager* mngr);
    ~Common();

    bool ApproveManifest(const ApplicationInfo& appInfo,
                         const PermissionPolicy::Rule* manifestRules,
                         const size_t manifestRulesCount);

    static Storage* storage;
    static JavaVM* jvm;

    static jclass applicationInfoClass;
    static jmethodID appInfoConstructorMID;
    static jfieldID infoAppNameFID;
    static jfieldID infoDevNameFID;
    static jfieldID infoFriendlyNameFID;
    static jfieldID infoAppIdFID;
    static jfieldID infoPubKeyFID;

    static jclass secMgrClass;
    static jfieldID mgrPointerFID;
    static jmethodID mgrNewEventMID;
    static jmethodID mgrMnfCallbackMID;

    static jclass ruleClass;
    static jmethodID ruleConstructorMID;

    static jclass memberClass;
    static jmethodID memberConstructorMID;

    static void initCommon(JNIEnv* env,
                           jclass secMgrClass,
                           jclass appInfoClass,
                           jclass ruleClass,
                           jclass memberClass);

    static int GetJNIEnv(JNIEnv** envp);

    static void DetachThread(JNIEnv* env,
                             int attached);

    static void Throw(JNIEnv* env,
                      const char* name,
                      const char* msg);

    static jobject ToApplicationInfoObject(JNIEnv* env,
                                           const ApplicationInfo& info);

    static ApplicationInfo ToNativeInfo(JNIEnv* env,
                                        jobject appInfo);

    static SecurityManager* GetSecurityManager(JNIEnv* env,
                                               jobject securityMgr);

    static Common* GetNativePeer(JNIEnv* env,
                                 jobject securityMgr);

    static qcc::String GetStringField(JNIEnv* env,
                                      jobject object,
                                      jfieldID fieldID);

    static qcc::String ToNativeString(JNIEnv* env,
                                      jstring object);

    static jobjectArray ToManifestRules(JNIEnv* env,
                                        const PermissionPolicy::Rule* manifestRules,
                                        const size_t manifestRulesCount);

    static jbyteArray ToKeyBytes(JNIEnv* env,
                                 const qcc::ECCPublicKey& key);

    static void ToGUID(JNIEnv* env,
                       jbyteArray data,
                       GUID128& guid);

    SecurityManager* GetSecurityManager(JNIEnv* env);

    bool CallManifestCallback(JNIEnv* env,
                              const ApplicationInfo& appInfo,
                              jobject manifest);
};

#endif /* JAVA_JNI_COMMON_H_ */
