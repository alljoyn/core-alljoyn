/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>
#include <vector>
#include <map>

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
    void Announce(const char* busName, uint16_t version,
                  ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg,
                  const ajn::MsgArg& aboutDataArg);

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