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

#include "RuleBusObject.h"

using namespace std;
using namespace ajn;
using namespace qcc;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

RuleBusObject::RuleBusObject(BusAttachment* busAttachment, const char* path, SimpleRuleEngine* ruleEngine) : BusObject(path), mBusAttachment(busAttachment), mRuleEngine(ruleEngine)
{
    QStatus status;
    InterfaceDescription* ruleEngineIntf = NULL;
    if (!mBusAttachment->GetInterface("org.allseen.sample.rule.engine")) {
        status = mBusAttachment->CreateInterface("org.allseen.sample.rule.engine", ruleEngineIntf);

        //Add BusMethods
        ruleEngineIntf->AddMethod("addRule", "(sssssssq)(sssssssq)b", "", "event,action,persist", 0);
        ruleEngineIntf->AddMethod("deleteAllRules", "", "", "", 0);

        if (ER_OK == status) {
            ruleEngineIntf->Activate();
        } else {
        }
    }

    /* Add the service interface to this object */
    const InterfaceDescription* ruleEngineTestIntf = mBusAttachment->GetInterface("org.allseen.sample.rule.engine");
    QCC_ASSERT(ruleEngineTestIntf);
    AddInterface(*ruleEngineTestIntf, ANNOUNCED);
    const MethodEntry methodEntries[] = {
        { ruleEngineTestIntf->GetMember("addRule"), static_cast<MessageReceiver::MethodHandler>(&RuleBusObject::addRule) },
        { ruleEngineTestIntf->GetMember("deleteAllRules"), static_cast<MessageReceiver::MethodHandler>(&RuleBusObject::deleteAllRules) },
    };
    status = AddMethodHandlers(methodEntries, ARRAY_SIZE(methodEntries));
}

void RuleBusObject::addRule(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    const ajn::MsgArg* args = 0;
    size_t numArgs = 0;
    msg->GetArgs(numArgs, args);
    if (numArgs == 3) {
        char* eUniqueName;
        char* ePath;
        char* eIface;
        char* eMember;
        char* eSig;
        char* eDeviceId;
        char* eAppId;
        uint16_t ePort;
        args[0].Get("(sssssssq)", &eUniqueName, &ePath, &eIface, &eMember, &eSig, &eDeviceId, &eAppId, &ePort);
        RuleInfo* event = new RuleInfo(eUniqueName, ePath, eIface, eMember, eSig, eDeviceId, eAppId, ePort);

        char* aUniqueName;
        char* aPath;
        char* aIface;
        char* aMember;
        char* aSig;
        char* aDeviceId;
        char* aAppId;
        uint16_t aPort;
        args[1].Get("(sssssssq)", &aUniqueName, &aPath, &aIface, &aMember, &aSig, &aDeviceId, &aAppId, &aPort);
        RuleInfo* action = new RuleInfo(aUniqueName, aPath, aIface, aMember, aSig, aDeviceId, aAppId, aPort);

        Rule* rule = new Rule(mBusAttachment, event, action);
        mBusAttachment->EnableConcurrentCallbacks();
        mRuleEngine->addRule(rule, args[2].v_bool);
        LOGTHIS("Added rule");
    } else {
        LOGTHIS("Incorrect number of args!");
    }
    QStatus status = MethodReply(msg);
    if (ER_OK != status) {
        printf("addRule: Error sending reply.\n");
    }
}

void RuleBusObject::deleteAllRules(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    LOGTHIS("Removing all rules");
    mBusAttachment->EnableConcurrentCallbacks();
    mRuleEngine->removeAllRules();
    QStatus status = MethodReply(msg);
    if (ER_OK != status) {
        printf("addRule: Error sending reply.\n");
    }
}