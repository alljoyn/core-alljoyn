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
#ifndef _ALLJOYN_JABOUTOBJECT_H
#define _ALLJOYN_JABOUTOBJECT_H

#include <jni.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/AboutDataListener.h>
#include <qcc/Debug.h>

#include "alljoyn_jni_helper.h"

#define QCC_MODULE "ALLJOYN_JAVA"

using namespace ajn;

class JBusAttachment;

/**
 * This is classes primary responsibility is to convert the value returned from
 * the Java AboutDataListener to a C++ values expected for a C++ AboutDataListener
 *
 * This class also implements the C++ AboutObj so that for every Java AboutObj
 * an instance of this AboutDataListener also exists.
 */
class JAboutObject : public AboutObj, public AboutDataListener {
  public:
    JAboutObject(JBusAttachment* bus, AnnounceFlag isAboutIntfAnnounced) :
        AboutObj(*reinterpret_cast<BusAttachment*>(bus), isAboutIntfAnnounced), busPtr(bus) {
        QCC_DbgPrintf(("JAboutObject::JAboutObject"));
        MID_getAboutData = NULL;
        MID_getAnnouncedAboutData = NULL;
        jaboutDataListenerRef = NULL;
        jaboutObjGlobalRef = NULL;
    }

    QStatus announce(JNIEnv* env, jobject thiz, jshort sessionPort, jobject jaboutDataListener) {
        QCC_UNUSED(thiz);
        // Make sure the jaboutDataListener is the latest version of the Java AboutDataListener
        if (env->IsInstanceOf(jaboutDataListener, CLS_AboutDataListener)) {
            JLocalRef<jclass> clazz = env->GetObjectClass(jaboutDataListener);

            MID_getAboutData = env->GetMethodID(clazz, "getAboutData", "(Ljava/lang/String;)Ljava/util/Map;");
            if (!MID_getAboutData) {
                return ER_FAIL;
            }
            MID_getAnnouncedAboutData = env->GetMethodID(clazz, "getAnnouncedAboutData", "()Ljava/util/Map;");
            if (!MID_getAnnouncedAboutData) {
                return ER_FAIL;
            }
        } else {
            return ER_FAIL;
        }
        QCC_DbgPrintf(("AboutObj_announce jaboutDataListener is an instance of CLS_AboutDataListener"));


        /*
         * The weak global reference jaboutDataListener cannot be directly used.  We
         * have to get a "hard" reference to it and then use that.  If you try to
         * use a weak reference directly you will crash and burn.
         */
        //The user can change the AboutDataListener between calls check to see
        // we already have a a jaboutDataListenerRef if we do delete that ref
        // and create a new one.
        if (jaboutDataListenerRef != NULL) {
            GetEnv()->DeleteGlobalRef(jaboutDataListenerRef);
            jaboutDataListenerRef = NULL;
        }
        jaboutDataListenerRef = env->NewGlobalRef(jaboutDataListener);
        if (!jaboutDataListenerRef) {
            QCC_LogError(ER_FAIL, ("Can't get new local reference to AboutDataListener"));
            return ER_FAIL;
        }

        return Announce(static_cast<SessionPort>(sessionPort), *this);
    }

    ~JAboutObject() {
        QCC_DbgPrintf(("JAboutObject::~JAboutObject"));
        if (jaboutDataListenerRef != NULL) {
            GetEnv()->DeleteGlobalRef(jaboutDataListenerRef);
            jaboutDataListenerRef = NULL;
        }
    }

    QStatus GetAboutData(MsgArg* msgArg, const char* language)
    {
        QCC_DbgPrintf(("JAboutObject::GetMsgArg"));

        /*
         * JScopedEnv will automagically attach the JVM to the current native
         * thread.
         */
        JScopedEnv env;

        // Note we don't check that if the jlanguage is null because null is an
        // acceptable value for the getAboutData Method call.
        JLocalRef<jstring> jlanguage = env->NewStringUTF(language);

        QStatus status = ER_FAIL;
        if (jaboutDataListenerRef != NULL && MID_getAboutData != NULL) {
            QCC_DbgPrintf(("Calling getAboutData for %s language.", language));
            JLocalRef<jobject> jannounceArg = CallObjectMethod(env, jaboutDataListenerRef, MID_getAboutData, (jstring)jlanguage);
            QCC_DbgPrintf(("JAboutObj::GetMsgArg Made Java Method call getAboutData"));
            // check for ErrorReplyBusException exception
            status = CheckForThrownException(env);
            if (ER_OK == status) {
                // Marshal the returned value
                if (!Marshal("a{sv}", jannounceArg, msgArg)) {
                    QCC_LogError(ER_FAIL, ("JAboutData(): GetMsgArgAnnounce() marshaling error"));
                    return ER_FAIL;
                }
            } else {
                QCC_DbgPrintf(("JAboutObj::GetMsgArg exception with status %s", QCC_StatusText(status)));
                return status;
            }
        }
        return ER_OK;
    }

    QStatus GetAnnouncedAboutData(MsgArg* msgArg)
    {
        QCC_DbgPrintf(("JAboutObject::~GetMsgArgAnnounce"));
        QStatus status = ER_FAIL;
        if (jaboutDataListenerRef != NULL && MID_getAnnouncedAboutData != NULL) {
            QCC_DbgPrintf(("AboutObj_announce obtained jo local ref of jaboutDataListener"));
            /*
             * JScopedEnv will automagically attach the JVM to the current native
             * thread.
             */
            JScopedEnv env;

            JLocalRef<jobject> jannounceArg = CallObjectMethod(env, jaboutDataListenerRef, MID_getAnnouncedAboutData);
            QCC_DbgPrintf(("AboutObj_announce Made Java Method call getAnnouncedAboutData"));
            // check for ErrorReplyBusException exception
            status = CheckForThrownException(env);
            if (ER_OK == status) {
                if (!Marshal("a{sv}", jannounceArg, msgArg)) {
                    QCC_LogError(ER_FAIL, ("JAboutData(): GetMsgArgAnnounce() marshaling error"));
                    return ER_FAIL;
                }
            } else {
                QCC_DbgPrintf(("JAboutObj::GetAnnouncedAboutData exception with status %s", QCC_StatusText(status)));
                return status;
            }
        }
        return status;
    }

    /**
     * This will check if the last method call threw an exception Since we are
     * looking for ErrorReplyBusExceptions we know that the exception thrown
     * correlates to a QStatus that we are trying to get.  If ER_FAIL is returned
     * then we had an issue resolving the java method calls.
     *
     * @return QStatus indicating the status that was thrown from the ErrReplyBusException
     */
    QStatus CheckForThrownException(JScopedEnv& env) {
        JLocalRef<jthrowable> ex = env->ExceptionOccurred();
        if (ex) {
            env->ExceptionClear();
            JLocalRef<jclass> clazz = env->GetObjectClass(ex);
            if (env->IsInstanceOf(ex, CLS_ErrorReplyBusException) && clazz != NULL) {
                jmethodID mid = env->GetMethodID(clazz, "getErrorStatus", "()Lorg/alljoyn/bus/Status;");
                if (!mid) {
                    return ER_FAIL;
                }
                JLocalRef<jobject> jstatus = CallObjectMethod(env, ex, mid);
                if (env->ExceptionCheck()) {
                    return ER_FAIL;
                }
                JLocalRef<jclass> statusClazz = env->GetObjectClass(jstatus);
                mid = env->GetMethodID(statusClazz, "getErrorCode", "()I");
                if (!mid) {
                    return ER_FAIL;
                }
                QStatus errorCode = (QStatus)env->CallIntMethod(jstatus, mid);
                if (env->ExceptionCheck()) {
                    return ER_FAIL;
                }
                return errorCode;
            }
            return ER_FAIL;
        }
        return ER_OK;
    }
    JBusAttachment* busPtr;
    jmethodID MID_getAboutData;
    jmethodID MID_getAnnouncedAboutData;
    jobject jaboutDataListenerRef;
    jobject jaboutObjGlobalRef;
};

#endif
