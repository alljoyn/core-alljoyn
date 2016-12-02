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

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>

#include "../SimpleRuleEngine.h"

#ifndef _RULE_BUS_OBJECT_
#define _RULE_BUS_OBJECT_

class RuleBusObject : public ajn::BusObject {
  public:
    /**
     * Construct a RuleBusObject object
     *
     */
    RuleBusObject(ajn::BusAttachment* busAttachment, const char* path, SimpleRuleEngine* ruleEngine);

  private:
    void addRule(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);

    void deleteAllRules(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);


  private:
    ajn::BusAttachment* mBusAttachment;
    SimpleRuleEngine* mRuleEngine;
};

#endif //_RULE_BUS_OBJECT_