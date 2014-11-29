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

#include "PresenceDetection.h"

#define PING_WAIT_TIME    5000

using namespace ajn;
using namespace qcc;

PresenceDetection::PresenceDetection(ajn::BusAttachment* busAttachment, JavaVM* vm, jobject jobj)
    : mBusAttachment(busAttachment), vm(vm), jobj(jobj)
{
    pthread_mutex_init(&mLock, NULL);
    pthread_create(&mThread, NULL, PingThreadLoop, this);
}

PresenceDetection::~PresenceDetection()
{
    Stop();
}

void PresenceDetection::StartPing(qcc::String bus, SessionPort p) {
    PingContext* newCtx = new PingContext(bus, p);
    PingContext* ctx;
    newCtx->isFound = true;
    pthread_mutex_lock(&mLock);
    bool isDuplicate = false;
    std::vector<PingContext*>::const_iterator it = mPingList.begin();
    while (it != mPingList.end()) {
        ctx = *it++;
        if (ctx->busName.compare(bus) == 0) {
            isDuplicate = true;
            break;
        }
    }
    if (!isDuplicate) {
        mPingList.push_back(newCtx);
    }
    pthread_mutex_unlock(&mLock);
}

void PresenceDetection::Stop() {
    pthread_mutex_lock(&mLock);
    pthread_join(mThread, NULL);
    pthread_mutex_unlock(&mLock);
    pthread_mutex_destroy(&mLock);
}

void* PresenceDetection::PingThreadLoop(void* context)
{
    PresenceDetection* pd = (PresenceDetection*)context;
    PingContext* ctx;
    while (1) {
        pthread_mutex_lock(&pd->mLock);
        std::vector<PingContext*>::const_iterator it = pd->mPingList.begin();
        while (it != pd->mPingList.end()) {
            ctx = *it++;
            LOGTHIS("Calling PingAsync BusName = %s\n", ctx->busName.c_str());
            QStatus status = pd->mBusAttachment->PingAsync(ctx->busName.c_str(), PING_WAIT_TIME, pd, ctx);
            /*
             * The PingAsync call failed make sure pointers are cleaned up so no memory
             * leaks occur.
             */
            if (ER_OK != status) {
                LOGTHIS("Unable to ping %s reason reported: %s", ctx->busName.c_str(), QCC_StatusText(status));
            }
        }
        pthread_mutex_unlock(&pd->mLock);
        sleep(5);
    }
    return 0;
}

void PresenceDetection::PingCB(QStatus status, void* context) {
    PingContext* ctx = (PingContext*)context;
    //If we didn't get a successful Ping then the device is gone
    //We keep it in the list to continue to check for presence
    if (ER_OK == status && !ctx->isFound) {
        ctx->isFound = true;
        //Device is detected so it is back
        //If we get About Signal again then the logic will not end up re-adding
        //a duplicate entry but will update the previous data updated
        JNIEnv* env;
        jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        if (JNI_EDETACHED == jret) {
            vm->AttachCurrentThread(&env, NULL);
        }

        jclass jcls = env->GetObjectClass(jobj);
        jmethodID mid = env->GetMethodID(jcls, "returnedEventActionApplication", "(Ljava/lang/String;)V");
        if (mid == 0) {
            LOGTHIS("Failed to get Java returnedEventActionApplication");
        } else {
            jstring jBusName = env->NewStringUTF(ctx->busName.c_str());
            env->CallVoidMethod(jobj, mid, jBusName);
            env->DeleteLocalRef(jBusName);
        }
        if (JNI_EDETACHED == jret) {
            vm->DetachCurrentThread();
        }
    }
    if (ER_ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE == status) {
        //This device does not support PING so stop
        LOGTHIS("Sorry BusName = %s does not support BusAttachment::Ping\n", ctx->busName.c_str());
        pthread_mutex_lock(&mLock);
        std::vector<PingContext*>::const_iterator it = mPingList.begin();
        int index = 0;
        while (it != mPingList.end()) {
            if ((*it++)->busName.compare(ctx->busName) == 0) {
                mPingList.erase(mPingList.begin() + index);
                break;
            }
            ++index;
        }
        pthread_mutex_unlock(&mLock);
    } else if (ER_OK != status && ctx->isFound) {
        ctx->isFound = false;
        //Device is no longer detected
        LOGTHIS("Lost BusName = %s\n", ctx->busName.c_str());
        JNIEnv* env;
        jint jret = vm->GetEnv((void**)&env, JNI_VERSION_1_2);
        if (JNI_EDETACHED == jret) {
            vm->AttachCurrentThread(&env, NULL);
        }

        jclass jcls = env->GetObjectClass(jobj);
        jmethodID mid = env->GetMethodID(jcls, "lostEventActionApplication", "(Ljava/lang/String;)V");
        if (mid == 0) {
            LOGTHIS("Failed to get Java lostEventActionApplication");
        } else {
            jstring jBusName = env->NewStringUTF(ctx->busName.c_str());
            env->CallVoidMethod(jobj, mid, jBusName);
            env->DeleteLocalRef(jBusName);
        }
        if (JNI_EDETACHED == jret) {
            vm->DetachCurrentThread();
        }
    }
}
