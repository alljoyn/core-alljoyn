/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <jni.h>
#include <string.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include "MyAllJoynCode.h"

/* Static data */
static MyAllJoynCode* myAllJoynCode = NULL;

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_allseen_sample_action_tester_BusHandler
 * Method:    initialize
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_allseen_sample_action_tester_BusHandler_initialize(JNIEnv* env, jobject jobj, jstring packageNameStrObj) {
    if (myAllJoynCode == NULL) {
        JavaVM* vm;
        env->GetJavaVM(&vm);
        jobject gjobj = env->NewGlobalRef(jobj);
        myAllJoynCode = new MyAllJoynCode(vm, gjobj);
    }
    jboolean iscopy;
    const char* packageNameStr = env->GetStringUTFChars(packageNameStrObj, &iscopy);
    myAllJoynCode->initialize(packageNameStr);
    env->ReleaseStringUTFChars(packageNameStrObj, packageNameStr);
}

/*
 * Class:     org_allseen_sample_action_tester_BusHandler
 * Method:    dointrospection
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)Ljava/langString;
 */
JNIEXPORT jstring JNICALL Java_org_allseen_sample_action_tester_BusHandler_doIntrospection(JNIEnv* env, jobject jobj, jstring jname, jstring jpath, jint sessionId) {
    jboolean iscopy;
    jboolean iscopy2;
    const char* sessionName = env->GetStringUTFChars(jname, &iscopy);
    const char* path = env->GetStringUTFChars(jpath, &iscopy2);
    char* xml = myAllJoynCode->introspectWithDescriptions(sessionName, path, sessionId);
    env->ReleaseStringUTFChars(jname, sessionName);
    env->ReleaseStringUTFChars(jpath, path);
    jstring ret = env->NewStringUTF(xml);
    delete xml;
    return ret;
}

JNIEXPORT void JNICALL Java_org_allseen_sample_action_tester_BusHandler_introspectionDone(JNIEnv* env, jobject jobj, jint sessionId) {
    myAllJoynCode->leaveSession(sessionId);
}


JNIEXPORT void JNICALL Java_org_allseen_sample_action_tester_BusHandler_callAction(JNIEnv*env, jobject jobj,
                                                                                   //action
                                                                                   jstring jAUniqueName, jstring jAPath,
                                                                                   jstring jAIface, jstring jAMember, jstring jASig)
{
    jboolean iscopy;

    const char* aUniqueName = env->GetStringUTFChars(jAUniqueName, &iscopy);
    const char* aPath = env->GetStringUTFChars(jAPath, &iscopy);
    const char* aIface = env->GetStringUTFChars(jAIface, &iscopy);
    const char* aMember = env->GetStringUTFChars(jAMember, &iscopy);
    const char* aSig = env->GetStringUTFChars(jASig, &iscopy);
    ActionInfo* action = new ActionInfo(aUniqueName, aPath, aIface, aMember, aSig);

    LOGTHIS("Calling an action on %s", aUniqueName);
    myAllJoynCode->callAction(action);

    delete action;

    env->ReleaseStringUTFChars(jAUniqueName, aUniqueName);
    env->ReleaseStringUTFChars(jAPath, aPath);
    env->ReleaseStringUTFChars(jAIface, aIface);
    env->ReleaseStringUTFChars(jAMember, aMember);
    env->ReleaseStringUTFChars(jASig, aSig);
}

/*
 * Class:     org_allseen_sample_action_tester_BusHandler
 * Method:    shutdown
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_allseen_sample_action_tester_BusHandler_shutdown(JNIEnv* env, jobject jobj) {
//	if(myAllJoynCode)
//		myAllJoynCode->shutdown();
//	delete myAllJoynCode;
//	myAllJoynCode = NULL;
}

#ifdef __cplusplus
}
#endif

