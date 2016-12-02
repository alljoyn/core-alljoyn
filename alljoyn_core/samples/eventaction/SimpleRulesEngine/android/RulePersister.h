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

#include <jni.h>
#include "../Rule.h"
#include "Constants.h"

#ifndef _RULE_PERSISTER_ANDROID_
#define _RULE_PERSISTER_ANDROID_

class RulePersister {
  public:
    RulePersister(JavaVM* vm, jobject jobj) : vm(vm), jobj(jobj) { };

    ~RulePersister() {

    }

    void saveRule(Rule* rule);

    void loadRules();

    void clearRules();

  private:
    JavaVM* vm;
    jobject jobj;
};

#endif //_RULE_PERSISTER_ANDROID_
