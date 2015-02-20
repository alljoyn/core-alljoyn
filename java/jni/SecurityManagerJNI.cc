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

#include <alljoyn/securitymgr/SecurityManagerFactory.h>
#include <alljoyn/securitymgr/SecurityManager.h>
#include <alljoyn/securitymgr/Storage.h>
#include <alljoyn/securitymgr/sqlstorage/SQLStorageFactory.h>
#include <alljoyn/Status.h>
#include <qcc/Environ.h>
#include "org_alljoyn_securitymgr_SecurityManagerJNI.h"
#include "Common.h"
#include "vector"

#define QCC_MODULE "SEC_MGR"

/* Impl for class SecurityManagerJNI */

using namespace ajn::securitymgr;

static jmethodID GetAddMethodID(JNIEnv* env, jobject jList)
{
    if (jList == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return NULL;
    }
    jclass clazz = env->GetObjectClass(jList);
    if (clazz == NULL) {
        return NULL;
    }
    return env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z");
}

/**
 * Always check env for pending exceptions after calling this function !!!
 */
static GuildInfo createGuildInfo(JNIEnv* env, jbyteArray jGuid, jbyteArray jKey, SecurityManager* smc)
{
    GuildInfo info;
    if (jGuid == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
    } else {
        jbyte guidData[GUID_SIZE];
        env->GetByteArrayRegion(jGuid, 0, GUID_SIZE, guidData);
        if (!env->ExceptionCheck()) {
            info.guid.SetBytes((uint8_t*)guidData);
        }
        if (jKey == NULL) {
            info.authority = smc->GetPublicKey();
        } else {
            size_t size = (size_t)env->GetArrayLength(jKey);
            jbyte* data = env->GetByteArrayElements(jKey, NULL);
            if (data) {
                QStatus status = info.authority.Import((const uint8_t*)data, size);
                env->ReleaseByteArrayElements(jKey, data, JNI_ABORT);
                if (status != ER_OK) {
                    Common::Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Bad key data");
                }
            }
        }
    }
    return info;
}

static ajn::PermissionPolicy::Rule* GetRules(JNIEnv* env, jobject jTerm, jmethodID nativeRuleMID, size_t* count)
{
    jobjectArray jRuleArray = (jobjectArray)env->CallObjectMethod(jTerm, nativeRuleMID);
    if (env->ExceptionCheck()) {
        return NULL;
    }
    if (jRuleArray == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return NULL;
    }
    jsize length = env->GetArrayLength(jRuleArray);
    if (length == 0) {
        Common::Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Empty rules set");
        return NULL;
    }
    jobject jRule = env->GetObjectArrayElement(jRuleArray, 0);
    if (jRule == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return NULL;
    }
    jclass ruleClass = env->GetObjectClass(jRule);
    if (ruleClass == NULL) {
        return NULL;
    }
    jmethodID nativeDataMID = env->GetMethodID(ruleClass, "getNativeMemberInfo", "()[Ljava/lang/Object;");
    if (nativeDataMID == NULL) {
        return NULL;
    }
    jfieldID nameFID = env->GetFieldID(ruleClass, "name", STRING_CLASS);
    if (nameFID == NULL) {
        return NULL;
    }

    QCC_LogError(ER_OK, ("new ajn::PermissionPolicy::Rule[%d]", length));

    *count = length;
    ajn::PermissionPolicy::Rule* rules = new ajn::PermissionPolicy::Rule[length];
    if (rules == NULL) {
        Common::Throw(env, OUTOFMEMORYERROR_CLASS, "");
        return NULL;
    }

    ajn::PermissionPolicy::Rule::Member* members = NULL;

    for (jsize i = 0;;) {
        qcc::String ifn = Common::GetStringField(env, jRule, nameFID);
        if (env->ExceptionCheck()) {
            goto out;
        }
        jobjectArray jNativeData = (jobjectArray)env->CallObjectMethod(jRule, nativeDataMID);
        if (env->ExceptionCheck()) {
            goto out;
        }
        if (jNativeData == NULL) {
            //This should never happen, but just to be robust
            Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
            goto out;
        }
        jobjectArray memberNames = (jobjectArray)env->GetObjectArrayElement(jNativeData, 0);
        if (env->ExceptionCheck()) {
            goto out;
        }
        jintArray types = (jintArray)env->GetObjectArrayElement(jNativeData, 1);
        if (env->ExceptionCheck()) {
            goto out;
        }
        jintArray actions = (jintArray)env->GetObjectArrayElement(jNativeData, 2);
        if (env->ExceptionCheck()) {
            goto out;
        }
        if (memberNames == NULL || types == NULL || actions == NULL) {
            //This should never happen, but just to be robust
            Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
            goto out;
        }
        jsize nrOfMembers = env->GetArrayLength(memberNames);
        if (env->ExceptionCheck()) {
            goto out;
        }

        QCC_LogError(ER_OK, ("ajn::PermissionPolicy::Rule::Member[%d]", nrOfMembers));
        members = new ajn::PermissionPolicy::Rule::Member[nrOfMembers];
        if (members == NULL) {
            Common::Throw(env, OUTOFMEMORYERROR_CLASS, "");
            goto out;
        }

        for (jsize j = 0; j < nrOfMembers; j++) {
            jstring jString = (jstring)env->GetObjectArrayElement(memberNames, j);
            if (env->ExceptionCheck()) {
                goto out;
            }
            qcc::String mbr = Common::ToNativeString(env, jString);
            if (env->ExceptionCheck()) {
                goto out;
            }
            members[j].SetMutualAuth(true);
            members[j].SetMemberName(mbr);
            jint value;
            env->GetIntArrayRegion(types, j, 1, &value);
            if (env->ExceptionCheck()) {
                goto out;
            }
            members[j].SetMemberType((ajn::PermissionPolicy::Rule::Member::MemberType)value);
            env->GetIntArrayRegion(actions, j, 1, &value);
            if (env->ExceptionCheck()) {
                goto out;
            }
            members[j].SetActionMask((uint8_t)value);
        }
        env->DeleteLocalRef(memberNames);
        env->DeleteLocalRef(types);
        env->DeleteLocalRef(actions);

        rules[i].SetInterfaceName(ifn);
        rules[i].SetMembers(nrOfMembers, members);
        members = NULL; //ownership is transferred.

        if (++i >= length) {
            return rules;
        }
        env->DeleteLocalRef(jRule);
        jRule = env->GetObjectArrayElement(jRuleArray, i);
        if (jRule == NULL) {
            Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
            goto out;
        }
    }
out:
    delete[] rules;
    delete[] members;
    return NULL;
}

static ajn::PermissionPolicy::Peer* GetPeers(JNIEnv* env, jobject jTerm, jmethodID nativePeerMID, size_t* count)
{
    jobjectArray jPeerArray = (jobjectArray)env->CallObjectMethod(jTerm, nativePeerMID);
    if (env->ExceptionCheck()) {
        return NULL;
    }
    if (jPeerArray == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return NULL;
    }
    jsize length = env->GetArrayLength(jPeerArray);
    if (length == 0) {
        Common::Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Empty peer set");
        return NULL;
    }
    jobject jPeer = env->GetObjectArrayElement(jPeerArray, 0);
    if (jPeer == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return NULL;
    }
    jclass peerClass = env->GetObjectClass(jPeer);
    if (peerClass == NULL) {
        return NULL;
    }
    jmethodID getTypeMID = env->GetMethodID(peerClass, "getPeerType", "()I");
    if (getTypeMID == NULL) {
        return NULL;
    }

    jfieldID guildIdFID = env->GetFieldID(peerClass, "id", "[B");
    if (guildIdFID == NULL) {
        return NULL;
    }

    jfieldID keyFID = env->GetFieldID(peerClass, "keyInfo", "[B");
    if (keyFID == NULL) {
        return NULL;
    }

    QCC_LogError(ER_OK, ("ajn::PermissionPolicy::Peer[%d]", length));
    *count = length;
    ajn::PermissionPolicy::Peer* peers = new ajn::PermissionPolicy::Peer[length];
    QCC_LogError(ER_OK, ("ajn::PermissionPolicy::Peer[%d] Done", length));
    if (peers == NULL) {
        Common::Throw(env, OUTOFMEMORYERROR_CLASS, "");
        return NULL;
    }

    for (jsize i = 0;;) {
        ajn::PermissionPolicy::Peer::PeerType type = (ajn::PermissionPolicy::Peer::PeerType)env->CallIntMethod(
            jPeer,
            getTypeMID);
        if (env->ExceptionCheck()) {
            goto out;
        }
        peers[i].SetType(type);
        if (type != ajn::PermissionPolicy::Peer::PEER_ANY) {
            jbyteArray guildIdBytes = (jbyteArray)env->GetObjectField(jPeer, guildIdFID);
            if (env->ExceptionCheck()) {
                goto out;
            }

            jbyteArray keyBytes = (jbyteArray)env->GetObjectField(jPeer, keyFID);
            if (env->ExceptionCheck()) {
                goto out;
            }

            if (guildIdBytes == NULL || keyBytes == NULL) {
                Common::Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Bad GUID and/or Key");
                goto out;
            }

            KeyInfoNISTP256 info;
            jsize guildIdLength;

            guildIdLength = env->GetArrayLength(guildIdBytes);
            if (env->ExceptionCheck()) {
                goto out;
            }

            jbyte* guildIdData = env->GetByteArrayElements(guildIdBytes, NULL);

            if (guildIdData == NULL) {
                goto out;
            }

            info.SetKeyId((uint8_t*)guildIdData, guildIdLength);
            env->ReleaseByteArrayElements(guildIdBytes, guildIdData, JNI_ABORT);
            env->DeleteLocalRef(guildIdBytes);

            jsize keyLength = env->GetArrayLength(keyBytes);
            if (env->ExceptionCheck()) {
                goto out;
            }
            jbyte* keyData  = env->GetByteArrayElements(keyBytes, NULL);

            if (keyData == NULL) {
                goto out;
            }

            ECCPublicKey eccKey;
            eccKey.Import((uint8_t*)keyData, keyLength);
            info.SetPublicKey(&eccKey);
            peers[i].SetKeyInfo(new KeyInfoNISTP256(info));

            env->ReleaseByteArrayElements(keyBytes, keyData, JNI_ABORT);
            env->DeleteLocalRef(keyBytes);
        }

        if (++i >= length) {
            return peers;
        }
        env->DeleteLocalRef(jPeer);
        jobject jPeer = env->GetObjectArrayElement(jPeerArray, 0);
        if (jPeer == NULL) {
            Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
            goto out;
        }
    }
out:
    delete[] peers;
    return NULL;
}

static ajn::PermissionPolicy::Term* GetTerms(JNIEnv* env, jobjectArray jTermArray,
                                             size_t* nrOfTerms)
{
    if (jTermArray == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return NULL;
    }
    jsize length = env->GetArrayLength(jTermArray);
    if (length == 0) {
        //Should we return an error in this case? or do we allow an empty policy?
        //triggering an error as an empty policy sounds wrong.
        return NULL;
    }
    jobject jTerm = env->GetObjectArrayElement(jTermArray, 0);
    if (jTerm == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return NULL;
    }
    jclass termClass = env->GetObjectClass(jTerm);
    if (termClass == NULL) {
        return NULL;
    }
    jmethodID nativeRuleMID =
        env->GetMethodID(termClass, "getRuleArray",
                         "()[Lorg/alljoyn/securitymgr/access/Rule;");
    if (nativeRuleMID == NULL) {
        return NULL;
    }

    jmethodID nativePeerMID =
        env->GetMethodID(termClass, "getPeerArray",
                         "()[Lorg/alljoyn/securitymgr/access/Peer;");
    if (nativePeerMID == NULL) {
        return NULL;
    }

    QCC_LogError(ER_OK, ("ajn::PermissionPolicy::Term[%d]", length));
    *nrOfTerms = length;
    ajn::PermissionPolicy::Term* terms = new ajn::PermissionPolicy::Term[length];
    if (terms == NULL) {
        Common::Throw(env, OUTOFMEMORYERROR_CLASS, "");
        return NULL;
    }

    for (jsize i = 0;;) {
        size_t count;
        ajn::PermissionPolicy::Rule* rules =
            GetRules(env, jTerm, nativeRuleMID, &count);
        if (rules == NULL) {
            goto out;
        }
        terms[i].SetRules(count, rules);
        ajn::PermissionPolicy::Peer* peers =
            GetPeers(env, jTerm, nativePeerMID, &count);
        if (peers == NULL) {
            goto out;
        }
        terms[i].SetPeers(count, peers);
        if (++i >= length) {
            return terms;
        }
        env->DeleteLocalRef(jTerm);
        jTerm = env->GetObjectArrayElement(jTermArray, i);
        if (jTerm == NULL) {
            Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
            return NULL;
        }
    }
out:
    delete[] terms;
    return NULL;
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_initJNI(JNIEnv* env,
                                                                               jclass thisClass,
                                                                               jclass appInfoClass,
                                                                               jclass ruleClass,
                                                                               jclass memberClass)
{
    Common::initCommon(env, thisClass, appInfoClass, ruleClass, memberClass);
}

JNIEXPORT jboolean JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_init(JNIEnv* env,
                                                                                jobject thisObj,
                                                                                jstring path)
{
    if (path == NULL) {
        return JNI_FALSE;
    }

    // QCC_SetLogLevels("SEC_MGR=7;ALLJOYN=7");

    SecurityManagerFactory& secFac = SecurityManagerFactory::GetInstance();

    const char* nativePath;

    SecurityManager* secMgr = NULL;
    do {
        nativePath = env->GetStringUTFChars(path, 0);

        if (!nativePath) {
            QCC_LogError(ER_FAIL, ("Bad arguments !"));
            break;
        }

        qcc::Environ::GetAppEnviron()->Add("HOME", qcc::String(nativePath).append("/"));
        QCC_DbgPrintf(("HOME was set to %s", qcc::Environ::GetAppEnviron()->Find("HOME").c_str()));

        delete Common::storage;
        Common::storage = SQLStorageFactory::GetInstance().GetStorage();

        if (Common::storage == NULL) {
            QCC_LogError(ER_FAIL, ("Could not initialize storage needed for the security manager"));
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                          "Could not initialize storage needed for the security manager");
            break;
        }

        secMgr =
            secFac.GetSecurityManager(Common::storage, NULL);
        env->ReleaseStringUTFChars(path, nativePath);

        if (secMgr == NULL) {
            QCC_LogError(ER_FAIL, ("Could not initialize a security manager"));
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                          "Could not initialize a security manager");
            break;
        }
        Common* cmn = new Common(env, thisObj, secMgr);
        if (cmn == NULL) {
            QCC_LogError(ER_FAIL, ("Could not allocate native object"));
        }
    } while (0);

    return secMgr ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_alljoyn_securitymgr_SecurityManagerJNI_getApplications(JNIEnv* env,
                                                                jobject
                                                                thisObj,
                                                                jobject
                                                                jlist)
{
    jmethodID addID = GetAddMethodID(env, jlist);
    if (addID == NULL) {
        return;
    }

    SecurityManager* smc = Common::GetSecurityManager(env, thisObj);
    if (smc) {
        std::vector<ApplicationInfo> list = smc->GetApplications();
        std::vector<ApplicationInfo>::const_iterator it = list.begin();
        for (; it != list.end(); ++it) {
            ApplicationInfo info = *it;
            jobject appInfo = Common::ToApplicationInfoObject(env, info);
            if (env->ExceptionCheck()) {
                return;
            }
            env->CallBooleanMethod(jlist, addID, appInfo);
            if (env->ExceptionCheck()) {
                return;
            }
            env->DeleteLocalRef(appInfo);
        }
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_deleteGuild(JNIEnv* env,
                                                                                   jobject
                                                                                   thisObj,
                                                                                   jbyteArray
                                                                                   jGuid)
{
    if (jGuid == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return;
    }
    SecurityManager* smc = Common::GetSecurityManager(env, thisObj);
    if (smc) {
        GuildInfo info;
        GUID128 guid;
        jbyte guidData[GUID_SIZE];
        env->GetByteArrayRegion(jGuid, 0, GUID_SIZE, guidData);
        if (env->ExceptionCheck()) {
            return;
        }
        guid.SetBytes((uint8_t*)guidData);
        info.guid = guid;
        QStatus status = smc->RemoveGuild(info);
        if (status != ER_OK) {
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS, "");
        }
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_createGuild(JNIEnv* env,
                                                                                   jobject thisObj,
                                                                                   jstring jName,
                                                                                   jstring jDescription,
                                                                                   jbyteArray jGuid,
                                                                                   jbyteArray jKey)
{
    if (jName == NULL || jDescription == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return;
    }
    SecurityManager* smc = Common::GetSecurityManager(env, thisObj);
    if (smc) {
        GuildInfo info = createGuildInfo(env, jGuid, jKey, smc);
        if (env->ExceptionCheck()) {
            return;
        }
        const char* chars = env->GetStringUTFChars(jName, NULL);
        if (chars) {
            info.name = chars;
            env->ReleaseStringUTFChars(jName, chars);
        } else {
            return;
        }
        chars = env->GetStringUTFChars(jDescription, NULL);
        if (chars) {
            info.desc = chars;
            env->ReleaseStringUTFChars(jDescription, chars);
        } else {
            return;
        }
        QStatus status = smc->StoreGuild(info);
        if (status != ER_OK) {
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS, "");
        }
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_getGuilds(JNIEnv* env,
                                                                                 jobject thisObj,
                                                                                 jobject jlist,
                                                                                 jclass guildClass)
{
    if (guildClass == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return;
    }

    jmethodID addID = GetAddMethodID(env, jlist);
    if (addID == NULL) {
        return;
    }

    jmethodID constructor =
        env->GetMethodID(guildClass, "<init>", "(" STRING_CLASS STRING_CLASS "[B[B)V");
    if (constructor == NULL) {
        return;
    }

    SecurityManager* smc = Common::GetSecurityManager(env, thisObj);
    if (smc) {
        std::vector<GuildInfo> list;
        smc->GetGuilds(list);
        std::vector<GuildInfo>::const_iterator it = list.begin();
        for (; it != list.end(); ++it) {
            GuildInfo info = *it;
            jstring jName = env->NewStringUTF(info.name.c_str());
            if (jName == NULL) {
                return;
            }
            jstring jDescription = env->NewStringUTF(info.desc.c_str());
            if (jDescription == NULL) {
                return;
            }
            jbyteArray jGuid = env->NewByteArray(GUID_SIZE);
            if (jGuid == NULL) {
                return;
            }
            env->SetByteArrayRegion(jGuid, 0, GUID_SIZE, (jbyte*)info.guid.GetBytes());
            if (env->ExceptionCheck()) {
                return;
            }
            jbyteArray jKey = Common::ToKeyBytes(env, info.authority);
            if (env->ExceptionCheck()) {
                return;
            }

            jobject guild =
                env->NewObject(guildClass, constructor, jName, jDescription, jGuid, jKey);
            if (env->ExceptionCheck()) {
                return;
            }
            env->DeleteLocalRef(jName);
            env->DeleteLocalRef(jDescription);
            env->DeleteLocalRef(jGuid);
            env->DeleteLocalRef(jKey);
            env->CallBooleanMethod(jlist, addID, guild);
            if (env->ExceptionCheck()) {
                return;
            }
            env->DeleteLocalRef(guild);
        }
    }
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_getGuild(JNIEnv* env,
                                                                                   jobject thisObj,
                                                                                   jbyteArray jGuid,
                                                                                   jclass guildClass)
{
    GuildInfo guild;
    if (guildClass == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return NULL;
    }

    Common::ToGUID(env, jGuid, guild.guid);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    jmethodID constructor =
        env->GetMethodID(guildClass, "<init>", "(" STRING_CLASS STRING_CLASS "[B[B)V");
    if (constructor == NULL) {
        return NULL;
    }

    SecurityManager* smc = Common::GetSecurityManager(env, thisObj);
    if (smc == NULL) {
        return NULL;
    }
    if (smc->GetGuild(guild) != ER_OK) {
        Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS, "Guild Not found");
        return NULL;
    }
    jstring jName = env->NewStringUTF(guild.name.c_str());
    if (jName == NULL) {
        return NULL;
    }
    jstring jDescription = env->NewStringUTF(guild.desc.c_str());
    if (jDescription == NULL) {
        return NULL;
    }
    jbyteArray jKey = Common::ToKeyBytes(env, guild.authority);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    return env->NewObject(guildClass, constructor, jName, jDescription, jGuid, jKey);
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_claimApplication(JNIEnv* env,
                                                                                        jobject thisObject,
                                                                                        jobject appInfo,
                                                                                        jbyteArray idGuid,
                                                                                        jbyteArray jKey)
{
    Common* cmn = Common::GetNativePeer(env, thisObject);
    IdentityInfo idInfo;
    if (cmn) {
        Common::ToGUID(env, idGuid, idInfo.guid);
        if (env->ExceptionCheck()) {
            return;
        }
        ApplicationInfo info = Common::ToNativeInfo(env, appInfo);
        if (env->ExceptionCheck()) {
            return;
        }

        SecurityManager* smc = cmn->GetSecurityManager(env);
        if (smc) {
            if (jKey == NULL) {
                idInfo.authority = smc->GetPublicKey();
            } else {
                size_t size = (size_t)env->GetArrayLength(jKey);
                jbyte* data = env->GetByteArrayElements(jKey, NULL);
                if (data) {
                    QStatus status = idInfo.authority.Import((const uint8_t*)data, size);
                    env->ReleaseByteArrayElements(jKey, data, JNI_ABORT);
                    if (status != ER_OK) {
                        Common::Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Bad key data");
                        return;
                    }
                } else {
                    return;
                }
            }

            QStatus status = smc->Claim(info, idInfo);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to claim application"));
                Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                              "Failed to claim application");
            }
        }
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_installPolicy(JNIEnv* env,
                                                                                     jobject
                                                                                     thisObject,
                                                                                     jobject
                                                                                     jAppInfo,
                                                                                     jlong
                                                                                     serialNr,
                                                                                     jobjectArray
                                                                                     jTermArray)
{
    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        ApplicationInfo appInfo = Common::ToNativeInfo(env, jAppInfo);
        if (env->ExceptionCheck()) {
            return;
        }

        ajn::PermissionPolicy policy;
        policy.SetSerialNum((uint32_t)serialNr);
        size_t term_count;
        ajn::PermissionPolicy::Term* terms = GetTerms(env, jTermArray, &term_count);
        if (terms == NULL) {
            return;
        }
        policy.SetTerms(term_count, terms);
        QStatus status = smc->UpdatePolicy(appInfo, policy);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to set policy"));
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS, "Failed to set Policy");
        }
    }
}

JNIEXPORT void JNICALL
Java_org_alljoyn_securitymgr_SecurityManagerJNI_installMembership(JNIEnv* env,
                                                                  jobject
                                                                  thisObject,
                                                                  jobject
                                                                  jAppInfo,
                                                                  jbyteArray
                                                                  jGuildGuid,
                                                                  jbyteArray
                                                                  jKey,
                                                                  jobjectArray
                                                                  jTermArray)
{
    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        GuildInfo info = createGuildInfo(env, jGuildGuid, jKey, smc);
        if (env->ExceptionCheck()) {
            return;
        }
        ApplicationInfo appInfo = Common::ToNativeInfo(env, jAppInfo);
        if (env->ExceptionCheck()) {
            return;
        }

        ajn::PermissionPolicy* authData;
        ajn::PermissionPolicy data;

        if (jTermArray) {
            size_t term_count;
            ajn::PermissionPolicy::Term* terms = GetTerms(env, jTermArray, &term_count);
            data.SetTerms(term_count, terms);
            authData = &data;
        } else {
            authData = NULL;
        }

        QStatus status = smc->InstallMembership(appInfo, info, authData);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to add membership"));
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                          "Failed to add membership");
        }
    }
}

JNIEXPORT void JNICALL
Java_org_alljoyn_securitymgr_SecurityManagerJNI_deleteMembership(JNIEnv* env,
                                                                 jobject
                                                                 thisObject,
                                                                 jobject
                                                                 jAppInfo,
                                                                 jbyteArray
                                                                 jGuildGuid,
                                                                 jbyteArray jKey)
{
    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        GuildInfo info = createGuildInfo(env, jGuildGuid, jKey, smc);
        if (env->ExceptionCheck()) {
            return;
        }
        ApplicationInfo appInfo = Common::ToNativeInfo(env, jAppInfo);
        if (env->ExceptionCheck()) {
            return;
        }
        QStatus status = smc->RemoveMembership(appInfo, info);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to delete membership"));
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                          "Failed to delete membership");
        }
    }
}

JNIEXPORT void JNICALL
Java_org_alljoyn_securitymgr_SecurityManagerJNI_unclaimApplication(JNIEnv* env,
                                                                   jobject
                                                                   thisObject,
                                                                   jobject
                                                                   applicationInfo)
{
    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        ApplicationInfo appInfo = Common::ToNativeInfo(env, applicationInfo);
        if (env->ExceptionCheck()) {
            return;
        }
        QStatus status = smc->Reset(appInfo);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to unclaim application"));
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                          "Failed to unclaim application");
        }
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_createIdentity(JNIEnv* env,
                                                                                      jobject thisObject,
                                                                                      jstring jName,
                                                                                      jbyteArray jGuid,
                                                                                      jbyteArray jKey)
{
    IdentityInfo info;
    if (jName == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return;
    }

    Common::ToGUID(env, jGuid, info.guid);
    if (env->ExceptionCheck()) {
        return;
    }

    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        const char* name = env->GetStringUTFChars(jName, NULL);
        if (name) {
            info.name = name;
            env->ReleaseStringUTFChars(jName, name);
            if (jKey == NULL) {
                info.authority = smc->GetPublicKey();
            } else {
                size_t size = (size_t)env->GetArrayLength(jKey);
                jbyte* data = env->GetByteArrayElements(jKey, NULL);
                if (data) {
                    QStatus status = info.authority.Import((const uint8_t*)data, size);
                    env->ReleaseByteArrayElements(jKey, data, JNI_ABORT);
                    if (status != ER_OK) {
                        Common::Throw(env, ILLEGALARGUMENTEXCEPTION_CLASS, "Bad key data");
                        return;
                    }
                } else {
                    return;
                }
            }
            QStatus status = smc->StoreIdentity(info);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to update/create identity"));
                Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                              "Failed to update/create Identity");
            }
        }
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_deleteIdentity(JNIEnv* env,
                                                                                      jobject thisObject,
                                                                                      jbyteArray jGuid)
{
    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        IdentityInfo identity;
        GUID128 guid;
        Common::ToGUID(env, jGuid, guid);
        identity.guid = guid;
        if (!env->ExceptionCheck()) {
            QStatus status = smc->RemoveIdentity(identity);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to delete identity"));
                Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                              "Failed to delete Identity");
            }
        }
    }
}

JNIEXPORT JNIEXPORT jstring JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_getIdentity(JNIEnv* env,
                                                                                                jobject thisObject,
                                                                                                jbyteArray jGuid)
{
    IdentityInfo info;

    Common::ToGUID(env, jGuid, info.guid);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        QStatus status = smc->GetIdentity(info);
        if (ER_OK == status) {
            return env->NewStringUTF(info.name.c_str());
        } else {
            QCC_LogError(status, ("Failed to retrieve identity"));
            Common::Throw(env, SECURITY_MNGT_EXCEPTION_CLASS,
                          "Failed to retrieve Identity");
        }
    }
    return NULL;
}

JNIEXPORT void JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_getIdentities(JNIEnv* env,
                                                                                     jobject thisObject,
                                                                                     jobject jList,
                                                                                     jclass identityClass)
{
    if (identityClass == NULL) {
        Common::Throw(env, NULLPOINTEREXCEPTION_CLASS, "");
        return;
    }

    jmethodID addID = GetAddMethodID(env, jList);
    if (addID == NULL) {
        return;
    }

    jmethodID constructor =
        env->GetMethodID(identityClass, "<init>", "(" STRING_CLASS "[B[B)V");
    if (constructor == NULL) {
        return;
    }

    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        std::vector<IdentityInfo> list;
        smc->GetIdentities(list);
        std::vector<IdentityInfo>::const_iterator it = list.begin();
        for (; it != list.end(); ++it) {
            IdentityInfo info = *it;
            jstring jName = env->NewStringUTF(info.name.c_str());
            if (jName == NULL) {
                return;
            }
            jbyteArray jGuid = env->NewByteArray(GUID_SIZE);
            if (jGuid == NULL) {
                return;
            }
            env->SetByteArrayRegion(jGuid, 0, GUID_SIZE, (jbyte*)info.guid.GetBytes());
            if (env->ExceptionCheck()) {
                return;
            }
            jbyteArray jKey = Common::ToKeyBytes(env, info.authority);
            if (env->ExceptionCheck()) {
                return;
            }

            jobject jIdentity =
                env->NewObject(identityClass, constructor, jName, jGuid, jKey);
            if (env->ExceptionCheck()) {
                return;
            }
            env->DeleteLocalRef(jName);
            env->DeleteLocalRef(jGuid);
            env->CallBooleanMethod(jList, addID, jIdentity);
            if (env->ExceptionCheck()) {
                return;
            }
            env->DeleteLocalRef(jIdentity);
        }
    }
}

JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_securitymgr_SecurityManagerJNI_getPublicKey(JNIEnv* env,
                                                                                          jobject thisObject)
{
    jbyteArray jKey = NULL;
    SecurityManager* smc = Common::GetSecurityManager(env, thisObject);
    if (smc) {
        jKey = Common::ToKeyBytes(env, smc->GetPublicKey());
    }
    return jKey;
}

#undef QCC_MODULE
