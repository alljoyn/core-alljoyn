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

#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>
#include <vector>

#include <alljoyn/about/AboutClient.h>

#include "Rule.h"
#if TARGET_ANDROID
#include "android/RulePersister.h"
#else
#include "posix/RulePersister.h"
#endif

#ifndef LOGTHIS
#if TARGET_ANDROID
#include <jni.h>
#include <android/log.h>
#define LOG_TAG  "JNI_EventActionBrowser"
#define LOGTHIS(...) (__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#else
#define LOGTHIS(...) { printf(__VA_ARGS__); printf("\n"); }
#endif
#endif

#ifndef _SIMPLE_RULE_ENGINE_
#define _SIMPLE_RULE_ENGINE_

class SimpleRuleEngine {
  public:
    /**
     * Construct a MyAllJoynCode object
     *
     */
    SimpleRuleEngine(
        #if TARGET_ANDROID
        JavaVM* vm,
        jobject jobj
        #endif
        ) :
        #if TARGET_ANDROID
        vm(vm), jobj(jobj), mRulePersister(vm, jobj)
        #else
        mRulePersister()
        #endif
    { };

    /**
     * Destructor
     */
    ~SimpleRuleEngine() {
        shutdown();
    };

    /**
     * .
     *
     * @param engineName
     * @param bus
     *
     */
    QStatus initialize(const char* engineName, ajn::BusAttachment* bus);

    QStatus addRule(Rule* rule, bool persist);

    QStatus removeRule(Rule* rule);

    QStatus removeAllRules();

    Rule* getRules(size_t& len);

    //Application using this engine is responsible for registering for About handler and passing
    //through the call to this engine
    void Announce(unsigned short version, unsigned short port, const char* busName,
                  const ajn::services::AboutClient::ObjectDescriptions& objectDescs,
                  const ajn::services::AboutClient::AboutData& aboutData);

    /**
     * Free up and release the objects used
     */
    QStatus shutdown();

  private:
#if TARGET_ANDROID
    JavaVM * vm;
    jobject jobj;
#endif

    RulePersister mRulePersister;

    struct NearbyAppInfo {
        qcc::String friendlyName;
        qcc::String deviceId;
        qcc::String appId;
        short port;
    };
    std::map<qcc::String, NearbyAppInfo*> mNearbyAppMap;

    std::vector<Rule*> mRules;
};

#endif //_SIMPLE_RULE_ENGINE_
