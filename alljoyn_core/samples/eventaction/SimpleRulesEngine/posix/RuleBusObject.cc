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
    assert(ruleEngineTestIntf);
    AddInterface(*ruleEngineTestIntf);
    const MethodEntry methodEntries[] = {
        { ruleEngineTestIntf->GetMember("addRule"), static_cast<MessageReceiver::MethodHandler>(&RuleBusObject::addRule) },
        { ruleEngineTestIntf->GetMember("deleteAllRules"), static_cast<MessageReceiver::MethodHandler>(&RuleBusObject::deleteAllRules) },
    };
    status = AddMethodHandlers(methodEntries, ARRAY_SIZE(methodEntries));
}

void RuleBusObject::addRule(const InterfaceDescription::Member* member, Message& msg)
{
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
    LOGTHIS("Removing all rules");
    mBusAttachment->EnableConcurrentCallbacks();
    mRuleEngine->removeAllRules();
    QStatus status = MethodReply(msg);
    if (ER_OK != status) {
        printf("addRule: Error sending reply.\n");
    }
}
