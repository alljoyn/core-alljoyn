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
 ******************************************************************************/

#include "SimpleRuleEngine.h"

using namespace std;
using namespace ajn;
using namespace qcc;

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
        rule->addToAction(mNearbyAppMap[rule->getActionSessionName()]->deviceId, mNearbyAppMap[rule->getActionSessionName()]->appId);
    }
    if (rule->eventReady() == 0) {
        if (mNearbyAppMap[rule->getEventSessionName()] != NULL) {
            rule->setEventPort(mNearbyAppMap[rule->getEventSessionName()]->port);
            rule->addToEvent(mNearbyAppMap[rule->getEventSessionName()]->deviceId, mNearbyAppMap[rule->getEventSessionName()]->appId);
        }
    }
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
        delete (*it);
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

void SimpleRuleEngine::Announce(const char* busName, uint16_t version,
                                SessionPort port, const MsgArg& objectDescriptionArg,
                                const MsgArg& aboutDataArg)
{
    NearbyAppInfo* nearbyAppInfo = new NearbyAppInfo();
    AboutData aboutData(aboutDataArg);
    char* deviceName;
    aboutData.GetDeviceName(&deviceName);
    nearbyAppInfo->friendlyName = deviceName;
    char* deviceId;
    aboutData.GetDeviceId(&deviceId);
    nearbyAppInfo->deviceId = deviceId;

    uint8_t* appIdBuffer;
    size_t numElements;
    aboutData.GetAppId(&appIdBuffer, &numElements);
    char temp[(numElements + 1) * 2];               //*2 due to hex format
    for (size_t i = 0; i < numElements; i++) {
        sprintf(temp + (i * 2), "%02x", appIdBuffer[i]);
    }
    temp[numElements * 2] = '\0';
    nearbyAppInfo->appId = temp;

    nearbyAppInfo->port = port;
    mNearbyAppMap.insert(std::pair<qcc::String, NearbyAppInfo*>(busName, nearbyAppInfo));

    //Search rules for a deviceId and appId that matches, if so update sessionName with busName
    for (std::vector<Rule*>::iterator it = mRules.begin(); it != mRules.end(); ++it) {
        if ((*it)->isEventMatch(nearbyAppInfo->deviceId, nearbyAppInfo->appId)) {
            (*it)->modifyEventSessionName(busName);
        }
        if ((*it)->isActionMatch(nearbyAppInfo->deviceId, nearbyAppInfo->appId)) {
            (*it)->modifyActionSessionName(busName);
        }
    }
}


