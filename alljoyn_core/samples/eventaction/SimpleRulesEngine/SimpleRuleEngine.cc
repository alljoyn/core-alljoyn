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

#include "SimpleRuleEngine.h"

using namespace std;
using namespace ajn;
using namespace qcc;
using namespace services;

QStatus SimpleRuleEngine::initialize(const char* engineName, BusAttachment* bus)
{
    QStatus status = ER_OK;
    mRulePersister.loadRules();
    return status;
}

QStatus SimpleRuleEngine::addRule(Rule* rule, bool persist)
{
    QStatus status = ER_OK;
    if (rule->actionReady() == 0) {
        rule->setActionPort(mNearbyAppMap[rule->getActionSessionName()]->port);
    }
    //TODO: re-investigate if needed
//	if(!rule->eventReady() == 0)
//		rule->addToEvent(mNearbyAppMap[rule->getEventSessionName()]->deviceId, mNearbyAppMap[rule->getEventSessionName()]->appId);
    rule->enable();
    mRules.push_back(rule);
    if (persist) {
        mRulePersister.saveRule(rule);
    }
    return status;
}

QStatus SimpleRuleEngine::removeRule(Rule* rule)
{
    QStatus status = ER_OK;

    return status;
}

QStatus SimpleRuleEngine::removeAllRules()
{
    QStatus status = ER_OK;
    for (std::vector<Rule*>::iterator it = mRules.begin(); it != mRules.end(); ++it) {
        (*it)->disable();
    }
    mRules.clear();
    mRulePersister.clearRules();
    return status;
}

Rule* SimpleRuleEngine::getRules(size_t& len)
{
    return NULL;
}

QStatus SimpleRuleEngine::shutdown()
{
    QStatus status = ER_OK;

    return status;
}

void SimpleRuleEngine::Announce(unsigned short version, unsigned short port, const char* busName,
                                const ajn::services::AboutClient::ObjectDescriptions& objectDescs,
                                const ajn::services::AboutClient::AboutData& aboutData)
{
    NearbyAppInfo* nearbyAppInfo = new NearbyAppInfo();
    for (AboutClient::AboutData::const_iterator it = aboutData.begin(); it != aboutData.end(); ++it) {
        qcc::String key = it->first;
        ajn::MsgArg value = it->second;
        if (value.typeId == ALLJOYN_STRING) {
            if (key.compare("DeviceName")) {
                nearbyAppInfo->friendlyName = value.v_string.str;
            } else if (key.compare("DeviceId")) {
                nearbyAppInfo->deviceId = value.v_string.str;
            } else if (key.compare("AppId")) {
                nearbyAppInfo->appId = value.v_string.str;
            }
        }
    }
    nearbyAppInfo->port = port;
    mNearbyAppMap.insert(std::pair<qcc::String, NearbyAppInfo*>(busName, nearbyAppInfo));

    //Search rules for a deviceId and appId that matches, if so update sessionName with busName
    for (std::vector<Rule*>::iterator it = mRules.begin(); it != mRules.end(); ++it) {
        if ((*it)->isEventMatch(nearbyAppInfo->deviceId, nearbyAppInfo->appId)) {
            (*it)->modifyEventSessionName(busName);
        }
    }
}


