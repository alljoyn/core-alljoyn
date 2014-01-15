/**
 * @file
 * This file defines some worker functions that help in the generation and
 * parsing of JSON format interface messages.
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include <qcc/String.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/StringUtil.h>
#include "RendezvousServerInterface.h"

using namespace std;

#define QCC_MODULE "RENDEZVOUS_SERVER_INTERFACE"

namespace ajn {

/**
 * Worker function used to generate the enum value corresponding
 * to the ICE candidate type.
 */
ICECandidateType GetICECandidateTypeValue(String type)
{
    ICECandidateType retVal = INVALID_CANDIDATE;

    if (type == String("host")) {
        retVal = HOST_CANDIDATE;
    } else if (type == String("srflx")) {
        retVal = SRFLX_CANDIDATE;
    } else if (type == String("prflx")) {
        retVal = PRFLX_CANDIDATE;
    } else if (type == String("relay")) {
        retVal = RELAY_CANDIDATE;
    }

    QCC_DbgPrintf(("GetICECandidateTypeString():%s", type.c_str()));

    return retVal;
}

/**
 * Worker function used to generate the enum value corresponding
 * to the ICE transport type.
 */
ICETransportType GetICETransportTypeValue(String type)
{
    ICETransportType retVal = INVALID_TRANSPORT;

    if (type == String("UDP")) {
        retVal = UDP_TRANSPORT;
    } else if (type == String("TCP")) {
        retVal = TCP_TRANSPORT;
    }

    QCC_DbgPrintf(("GetICETransportTypeValue():%s", type.c_str()));

    return retVal;
}

/**
 * Worker function used to generate the string corresponding
 * to the transport type.
 */
String GetICETransportTypeString(ICETransportType type)
{
    String retStr = String("invalid");

    switch (type) {
    case UDP_TRANSPORT:
        retStr = String("UDP");
        break;

    case TCP_TRANSPORT:
        retStr = String("TCP");
        break;

    case INVALID_TRANSPORT:
    default:
        break;
    }

    QCC_DbgPrintf(("GetICETransportTypeString():%s", retStr.c_str()));

    return retStr;
};

/**
 * Worker function used to generate the string corresponding
 * to the ICE candidate type.
 */
String GetICECandidateTypeString(ICECandidateType type)
{
    String retStr = String("invalid");

    switch (type) {
    case HOST_CANDIDATE:
        retStr = String("host");
        break;

    case SRFLX_CANDIDATE:
        retStr = String("srflx");
        break;

    case PRFLX_CANDIDATE:
        retStr = String("prflx");
        break;

    case RELAY_CANDIDATE:
        retStr = String("relay");
        break;

    case INVALID_CANDIDATE:
    default:
        break;
    }

    QCC_DbgPrintf(("GetICECandidateTypeString():%s", retStr.c_str()));

    return retStr;
};

/**
 * Worker function used to generate the string corresponding
 * to the Message Response Type.
 */
String PrintResponseType(ResponseType type)
{
    String retStr = String("INVALID_RESPONSE");

    switch (type) {
    case SEARCH_MATCH_RESPONSE:
        retStr = String("SEARCH_MATCH_RESPONSE");
        break;

    case MATCH_REVOKED_RESPONSE:
        retStr = String("MATCH_REVOKED_RESPONSE");
        break;

    case ADDRESS_CANDIDATES_RESPONSE:
        retStr = String("ADDRESS_CANDIDATES_RESPONSE");
        break;

    case START_ICE_CHECKS_RESPONSE:
        retStr = String("START_ICE_CHECKS_RESPONSE");
        break;

    case INVALID_RESPONSE:
    default:
        break;
    }

    QCC_DbgPrintf(("PrintResponseType():%s", retStr.c_str()));

    return (retStr);
}

/**
 * Worker function used to generate an Advertisement in
 * the JSON format.
 */
String GenerateJSONAdvertisement(AdvertiseMessage message)
{
    Json::Value advMsg;

    Json::StaticString peerInfo("peerInfo");
    Json::StaticString ads("ads");
    Json::StaticString service("service");
    Json::StaticString attribs("attribs");

    Json::Value peerInfoObj(Json::objectValue);
    Json::Value tempObj(Json::objectValue);
    Json::Value adsObj(Json::arrayValue);
    Json::Value attribsObj(Json::objectValue);

    advMsg[peerInfo] = peerInfoObj;

    Json::UInt i = 0;

    while (!(message.ads.empty())) {

        tempObj[service] = message.ads.front().service.c_str();
        tempObj[attribs] = attribsObj;

        adsObj[i] = tempObj;

        message.ads.pop_front();
        i++;
    }

    advMsg[ads] = adsObj;

    Json::StyledWriter writer;
    String retStr = writer.write(advMsg).c_str();

    QCC_DbgPrintf(("GenerateJSONAdvertisement():%s", retStr.c_str()));

    return retStr;
}

/**
 * Worker function used to generate a Search in
 * the JSON format.
 */
String GenerateJSONSearch(SearchMessage message)
{
    Json::Value searchMsg;

    Json::StaticString peerInfo("peerInfo");
    Json::StaticString search("search");
    Json::StaticString service("service");
    Json::StaticString matchType("matchType");
    Json::StaticString filter("filter");
    Json::StaticString timeExpiry("timeExpiry");

    Json::Value peerInfoObj(Json::objectValue);
    Json::Value tempObj(Json::objectValue);
    Json::Value searchObj(Json::arrayValue);
    Json::Value filterObj(Json::objectValue);

    searchMsg[peerInfo] = peerInfoObj;

    Json::UInt i = 0;

    while (!(message.search.empty())) {

        tempObj[service] = message.search.front().service.c_str();
        tempObj[matchType] = GetSearchMatchTypeString(message.search.front().matchType).c_str();
        tempObj[timeExpiry] = message.search.front().timeExpiry;
        tempObj[filter] = filterObj;

        searchObj[i] = tempObj;

        message.search.pop_front();
        i++;
    }

    searchMsg[search] = searchObj;

    Json::StyledWriter writer;
    String retStr = writer.write(searchMsg).c_str();

    QCC_DbgPrintf(("GenerateJSONSearch():%s", retStr.c_str()));

    return retStr;
}

/**
 * Worker function used to generate a Proximity Message in
 * the JSON format.
 */
String GenerateJSONProximity(ProximityMessage message)
{
    Json::Value proxMsg;

    Json::StaticString proximity("proximity");
    Json::StaticString wifiaps("wifiaps");
    Json::StaticString attached("attached");
    Json::StaticString BSSID("BSSID");
    Json::StaticString SSID("SSID");
    Json::StaticString BTs("BTs");
    Json::StaticString self("self");
    Json::StaticString MAC("MAC");

    Json::Value proxMsgObj(Json::objectValue);
    Json::Value tempWifiapsObj(Json::objectValue);
    Json::Value tempBTsObj(Json::objectValue);
    Json::Value wifiapsObj(Json::arrayValue);
    Json::Value BTsObj(Json::arrayValue);

    Json::UInt i = 0;

    while (!(message.wifiaps.empty())) {

        tempWifiapsObj[attached] = message.wifiaps.front().attached;
        tempWifiapsObj[BSSID] = message.wifiaps.front().BSSID.c_str();
        tempWifiapsObj[SSID] = message.wifiaps.front().SSID.c_str();

        wifiapsObj[i] = tempWifiapsObj;

        message.wifiaps.pop_front();
        i++;
    }

    i = 0;

    while (!(message.BTs.empty())) {

        tempBTsObj[self] = message.BTs.front().self;
        tempBTsObj[MAC] = message.BTs.front().MAC.c_str();

        BTsObj[i] = tempBTsObj;

        message.BTs.pop_front();
        i++;
    }

    proxMsgObj[wifiaps] = wifiapsObj;
    proxMsgObj[BTs] = BTsObj;

    proxMsg[proximity] = proxMsgObj;

    Json::StyledWriter writer;
    String retStr = writer.write(proxMsg).c_str();

    QCC_DbgPrintf(("GenerateJSONProximity():%s", retStr.c_str()));

    return retStr;
}

/**
 * Worker function used to generate an ICE Candidates Message in
 * the JSON format.
 */
String GenerateJSONCandidates(ICECandidatesMessage message)
{
    Json::Value addCandMsg;

    Json::StaticString ice_ufrag("ice-ufrag");
    Json::StaticString ice_pwd("ice-pwd");
    Json::StaticString candidates("candidates");
    Json::StaticString type("type");
    Json::StaticString transport("transport");
    Json::StaticString priority("priority");
    Json::StaticString address("address");
    Json::StaticString port("port");
    Json::StaticString raddress("raddress");
    Json::StaticString rport("rport");
    Json::StaticString foundation("foundation");
    Json::StaticString componentID("componentID");

    addCandMsg[ice_ufrag] = message.ice_ufrag.c_str();
    addCandMsg[ice_pwd] = message.ice_pwd.c_str();

    Json::Value candidatesObj(Json::arrayValue);
    Json::Value tempCandidateObj(Json::objectValue);

    Json::UInt i = 0;

    while (!(message.candidates.empty())) {

        if (message.candidates.front().type != INVALID_CANDIDATE) {

            tempCandidateObj[type] = GetICECandidateTypeString(message.candidates.front().type).c_str();
            tempCandidateObj[foundation] = message.candidates.front().foundation.c_str();
            tempCandidateObj[componentID] = message.candidates.front().componentID;
            tempCandidateObj[transport] = GetICETransportTypeString(message.candidates.front().transport).c_str();
            tempCandidateObj[priority] = (Json::UInt)message.candidates.front().priority;
            tempCandidateObj[address] = message.candidates.front().address.ToString().c_str();
            tempCandidateObj[port] = message.candidates.front().port;

            if (message.candidates.front().type != HOST_CANDIDATE) {
                tempCandidateObj[raddress] = message.candidates.front().raddress.ToString().c_str();
                tempCandidateObj[rport] = message.candidates.front().rport;
            }

            candidatesObj[i] = tempCandidateObj;
            i++;

        }

        message.candidates.pop_front();
    }

    addCandMsg[candidates] = candidatesObj;

    Json::StyledWriter writer;
    String retStr = writer.write(addCandMsg).c_str();

    QCC_DbgPrintf(("GenerateJSONCandidates():%s", retStr.c_str()));

    return retStr;
}

/**
 * Worker function used to parse a generic response
 */
QStatus ParseGenericResponse(Json::Value receivedResponse, GenericResponse& parsedResponse)
{
    QStatus status = ER_OK;

    Json::StaticString peerID("peerID");

    if (receivedResponse.isMember(peerID)) {

        parsedResponse.peerID = String(receivedResponse[peerID].asCString());
        QCC_DbgPrintf(("ParseGenericResponse(): peerID = %s", receivedResponse[peerID].asCString()));

    } else {

        status = ER_FAIL;
        QCC_LogError(status, ("ParseGenericResponse(): Message does not seem to be a generic response"));

    }

    return status;
}

/**
 * Worker function used to parse a refresh token response
 */
QStatus ParseTokenRefreshResponse(Json::Value receivedResponse, TokenRefreshResponse& parsedResponse)
{
    QStatus status = ER_OK;

    Json::StaticString acct("acct");
    Json::StaticString pwd("pwd");
    Json::StaticString expiryTime("expiryTime");

    if (receivedResponse.isMember(acct)) {
        if (receivedResponse.isMember(pwd)) {
            if (receivedResponse.isMember(expiryTime)) {
                parsedResponse.acct = String(receivedResponse[acct].asCString());
                QCC_DbgPrintf(("ParseTokenRefreshResponse(): acct = %s", receivedResponse[acct].asCString()));

                parsedResponse.pwd = String(receivedResponse[pwd].asCString());
                QCC_DbgPrintf(("ParseTokenRefreshResponse(): pwd = %s", receivedResponse[pwd].asCString()));

                parsedResponse.expiryTime = ((receivedResponse[expiryTime].asInt()) - TURN_TOKEN_EXPIRY_TIME_BUFFER_IN_SECONDS) * 1000;
                QCC_DbgPrintf(("ParseTokenRefreshResponse(): expiryTime = %d", parsedResponse.expiryTime));

                parsedResponse.recvTime = GetTimestamp64();
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("ParseTokenRefreshResponse(): Message does not seem to have a expiryTime token"));
            }
        } else {
            status = ER_FAIL;
            QCC_LogError(status, ("ParseTokenRefreshResponse(): Message does not seem to have a pwd token"));
        }
    } else {
        status = ER_FAIL;
        QCC_LogError(status, ("ParseTokenRefreshResponse(): Message does not seem to have a acct token"));
    }

    return status;
}

/**
 * Worker function used to print a parsed response
 */
void PrintMessageResponse(Response msg)
{
    if (msg.type == SEARCH_MATCH_RESPONSE) {

        SearchMatchResponse* SearchMatch = static_cast<SearchMatchResponse*>(msg.response);
        SearchMatchResponse Search = *SearchMatch;
        QCC_DbgPrintf(("PrintMessageResponse(): Search Match Response"));
        QCC_DbgPrintf(("match[service] = %s", Search.service.c_str()));
        QCC_DbgPrintf(("match[searchedService] = %s", Search.searchedService.c_str()));
        QCC_DbgPrintf(("match[peerAddr] = %s", Search.peerAddr.c_str()));
        QCC_DbgPrintf(("match[STUNInfo][address] = %s", Search.STUNInfo.address.ToString().c_str()));
        QCC_DbgPrintf(("match[STUNInfo][port] = %d", Search.STUNInfo.port));
        QCC_DbgPrintf(("match[STUNInfo][acct] = %s", Search.STUNInfo.acct.c_str()));
        QCC_DbgPrintf(("match[STUNInfo][pwd] = %s", Search.STUNInfo.pwd.c_str()));
        QCC_DbgPrintf(("match[STUNInfo][expiryTime] = %d", Search.STUNInfo.expiryTime));
        QCC_DbgPrintf(("match[STUNInfo][recvTime] = %llu", Search.STUNInfo.recvTime));

        if (Search.STUNInfo.relayInfoPresent) {
            QCC_DbgPrintf(("match[STUNInfo][relay][address] = %s", Search.STUNInfo.relay.address.ToString().c_str()));
            QCC_DbgPrintf(("match[STUNInfo][relay][port] = %d", Search.STUNInfo.relay.port));
        }

    } else if (msg.type == MATCH_REVOKED_RESPONSE) {

        MatchRevokedResponse* MatchRevoked = static_cast<MatchRevokedResponse*>(msg.response);
        MatchRevokedResponse Revoked = *MatchRevoked;
        QCC_DbgPrintf(("PrintMessageResponse(): Match Revoked Response"));
        QCC_DbgPrintf(("matchRevoked[peerAddr] = %s", Revoked.peerAddr.c_str()));
        QCC_DbgPrintf(("matchRevoked[deleteAll] = %d", Revoked.deleteAll));
        if (!Revoked.deleteAll) {
            while (!Revoked.services.empty()) {
                QCC_DbgPrintf(("matchRevoked[services] = %s", Revoked.services.front().c_str()));
                Revoked.services.pop_front();
            }
        }

    } else if (msg.type == ADDRESS_CANDIDATES_RESPONSE) {

        AddressCandidatesResponse* AddressCandidates = static_cast<AddressCandidatesResponse*>(msg.response);
        AddressCandidatesResponse Candidates = *AddressCandidates;
        QCC_DbgPrintf(("PrintMessageResponse(): Address Candidate Response"));
        QCC_DbgPrintf(("addressCandidates[peerAddr] = %s", Candidates.peerAddr.c_str()));
        QCC_DbgPrintf(("addressCandidates[ice-ufrag] = %s", Candidates.ice_ufrag.c_str()));
        QCC_DbgPrintf(("addressCandidates[ice-pwd] = %s", Candidates.ice_pwd.c_str()));

        while (!Candidates.candidates.empty()) {
            QCC_DbgPrintf(("addressCandidates[candidates][type] = %s", GetICECandidateTypeString(Candidates.candidates.front().type).c_str()));
            QCC_DbgPrintf(("addressCandidates[candidates][foundation] = %s", Candidates.candidates.front().foundation.c_str()));
            QCC_DbgPrintf(("addressCandidates[candidates][componentID] = %d", Candidates.candidates.front().componentID));
            QCC_DbgPrintf(("addressCandidates[candidates][transport] = %s", GetICETransportTypeString(Candidates.candidates.front().transport).c_str()));
            QCC_DbgPrintf(("addressCandidates[candidates][priority] = %d", Candidates.candidates.front().priority));
            QCC_DbgPrintf(("addressCandidates[candidates][address] = %s", Candidates.candidates.front().address.ToString().c_str()));
            QCC_DbgPrintf(("addressCandidates[candidates][port] = %d", Candidates.candidates.front().port));

            if (Candidates.candidates.front().type != HOST_CANDIDATE) {
                QCC_DbgPrintf(("addressCandidates[candidates][raddress] = %s", Candidates.candidates.front().raddress.ToString().c_str()));
                QCC_DbgPrintf(("addressCandidates[candidates][rport] = %d", Candidates.candidates.front().rport));
            }

            Candidates.candidates.pop_front();
        }

        if (Candidates.STUNInfoPresent) {
            QCC_DbgPrintf(("addressCandidates[STUNInfo][address] = %s", Candidates.STUNInfo.address.ToString().c_str()));
            QCC_DbgPrintf(("addressCandidates[STUNInfo][port] = %d", Candidates.STUNInfo.port));
            QCC_DbgPrintf(("addressCandidates[STUNInfo][acct] = %s", Candidates.STUNInfo.acct.c_str()));
            QCC_DbgPrintf(("addressCandidates[STUNInfo][pwd] = %s", Candidates.STUNInfo.pwd.c_str()));
            QCC_DbgPrintf(("addressCandidates[STUNInfo][expiryTime] = %d", Candidates.STUNInfo.expiryTime));
            QCC_DbgPrintf(("addressCandidates[STUNInfo][recvTime] = %llu", Candidates.STUNInfo.recvTime));

            if (Candidates.STUNInfo.relayInfoPresent) {
                QCC_DbgPrintf(("addressCandidates[STUNInfo][relay][address] = %s", Candidates.STUNInfo.relay.address.ToString().c_str()));
                QCC_DbgPrintf(("addressCandidates[STUNInfo][relay][port] = %d", Candidates.STUNInfo.relay.port));
            }
        }

    } else if (msg.type == START_ICE_CHECKS_RESPONSE) {

        StartICEChecksResponse* StartICEChecks = static_cast<StartICEChecksResponse*>(msg.response);
        StartICEChecksResponse startICEChecks = *StartICEChecks;
        QCC_DbgPrintf(("PrintMessageResponse(): Start ICE Checks Response"));
        QCC_DbgPrintf(("startICEChecks[peerAddr] = %s", startICEChecks.peerAddr.c_str()));

    } else {

        QCC_LogError(ER_FAIL, ("PrintMessageResponse(): Invalid Response"));

    }

}


/**
 * Worker function used to parse a message response
 */
QStatus ParseMessagesResponse(Json::Value receivedResponse, ResponseMessage& parsedResponse)
{
    QStatus status = ER_OK;

    Json::StaticString msgs("msgs");
    Json::StaticString service("service");
    Json::StaticString type("type");
    Json::StaticString match("match");
    Json::StaticString searchedService("searchedService");
    Json::StaticString peerID("peerID");
    Json::StaticString ice_ufrag("ice-ufrag");
    Json::StaticString ice_pwd("ice-pwd");
    Json::StaticString peerAddr("peerAddr");
    Json::StaticString STUNInfo("STUNInfo");
    Json::StaticString address("address");
    Json::StaticString port("port");
    Json::StaticString relay("relay");
    Json::StaticString acct("acct");
    Json::StaticString pwd("pwd");
    Json::StaticString expiryTime("expiryTime");
    Json::StaticString addressCandidates("addressCandidates");
    Json::StaticString source("source");
    Json::StaticString destination("destination");
    Json::StaticString candidates("candidates");
    Json::StaticString transport("transport");
    Json::StaticString priority("priority");
    Json::StaticString raddress("raddress");
    Json::StaticString rport("rport");
    Json::StaticString matchRevoked("matchRevoked");
    Json::StaticString services("services");
    Json::StaticString deleteAll("deleteAll");
    Json::StaticString foundation("foundation");
    Json::StaticString componentID("componentID");
    Json::StaticString startICEChecks("startICEChecks");

    Response tempMsg;

    if (receivedResponse.empty()) {
        status = ER_FAIL;
        QCC_LogError(status, ("ParseMessagesResponse(): Message is empty"));
    } else if (receivedResponse.isMember(msgs)) {
        Json::Value msgsObj = receivedResponse[msgs];
        if (msgsObj.isArray()) {
            if (!msgsObj.empty()) {
                for (Json::UInt j = 0; j < msgsObj.size(); j++) {
                    Json::Value msgsObjArrayMember = msgsObj[j];

                    if (msgsObjArrayMember[type] == "match") {
                        QCC_DbgPrintf(("ParseMessagesResponse(): [%d] Match Message", j));

                        if (msgsObjArrayMember.isMember(match)) {

                            Json::Value matchObj = msgsObjArrayMember[match];

                            if (matchObj.isMember(searchedService)) {
                                if (matchObj.isMember(service)) {
                                    if (matchObj.isMember(peerAddr)) {
                                        if (matchObj.isMember(STUNInfo)) {

                                            Json::Value STUNInfoObj = matchObj[STUNInfo];

                                            if (STUNInfoObj.isMember(address)) {
                                                if (STUNInfoObj.isMember(acct)) {
                                                    if (STUNInfoObj.isMember(pwd)) {
                                                        if (STUNInfoObj.isMember(expiryTime)) {
                                                            tempMsg.type = SEARCH_MATCH_RESPONSE;
                                                            SearchMatchResponse* SearchMatch = new SearchMatchResponse();
                                                            SearchMatch->searchedService = String(matchObj[searchedService].asCString());
                                                            SearchMatch->service = String(matchObj[service].asCString());
                                                            SearchMatch->peerAddr = String(matchObj[peerAddr].asCString());
                                                            status = SearchMatch->STUNInfo.address.SetAddress(String(STUNInfoObj[address].asCString()), true);
                                                            if (status != ER_OK) {
                                                                QCC_LogError(status, ("ParseMessagesResponse(): Invalid STUN Server address specified in Search Match response"));
                                                                return status;
                                                            }
                                                            if (STUNInfoObj.isMember(port)) {
                                                                SearchMatch->STUNInfo.port = STUNInfoObj[port].asInt();
                                                            } else {
                                                                QCC_DbgPrintf(("ParseMessagesResponse(): Setting the port to default value as match[STUNInfo][port] member was not found"));
                                                            }
                                                            SearchMatch->STUNInfo.acct = String(STUNInfoObj[acct].asCString());
                                                            if (SearchMatch->STUNInfo.acct.size() > TURN_ACCT_TOKEN_MAX_SIZE) {
                                                                QCC_LogError(ER_FAIL, ("%s: Size of the TURN acct token (%u) is greater than max allowed %u", __FUNCTION__, SearchMatch->STUNInfo.acct.size(), TURN_ACCT_TOKEN_MAX_SIZE));
                                                            }
                                                            SearchMatch->STUNInfo.pwd = String(STUNInfoObj[pwd].asCString());
                                                            SearchMatch->STUNInfo.expiryTime = ((STUNInfoObj[expiryTime].asInt()) - TURN_TOKEN_EXPIRY_TIME_BUFFER_IN_SECONDS) * 1000;
                                                            SearchMatch->STUNInfo.recvTime = GetTimestamp64();

                                                            if (STUNInfoObj.isMember(relay)) {
                                                                Json::Value relayObj = STUNInfoObj[relay];

                                                                if (relayObj.isMember(address)) {
                                                                    if (relayObj.isMember(port)) {
                                                                        SearchMatch->STUNInfo.relayInfoPresent = true;
                                                                        status = SearchMatch->STUNInfo.relay.address.SetAddress(String(relayObj[address].asCString()));
                                                                        if (status != ER_OK) {
                                                                            QCC_LogError(status, ("ParseMessagesResponse(): Invalid Relay Server address specified in Search Match response"));
                                                                            return status;
                                                                        }
                                                                        SearchMatch->STUNInfo.relay.port = relayObj[port].asInt();

                                                                        tempMsg.response = static_cast<SearchMatchResponse*>(SearchMatch);

                                                                        parsedResponse.msgs.push_back(tempMsg);
                                                                        PrintMessageResponse(tempMsg);
                                                                    } else {
                                                                        status = ER_FAIL;
                                                                        QCC_LogError(status, ("ParseMessagesResponse(): match[STUNInfo][relay][port] member not found"));
                                                                    }
                                                                } else {
                                                                    status = ER_FAIL;
                                                                    QCC_LogError(status, ("ParseMessagesResponse(): match[STUNInfo][relay][address] member not found"));
                                                                }
                                                            } else {
                                                                tempMsg.response = static_cast<SearchMatchResponse*>(SearchMatch);
                                                                parsedResponse.msgs.push_back(tempMsg);
                                                                PrintMessageResponse(tempMsg);
                                                                QCC_DbgPrintf(("ParseMessagesResponse(): match[STUNInfo][relay] member not found"));
                                                            }
                                                        } else {
                                                            status = ER_FAIL;
                                                            QCC_LogError(status, ("ParseMessagesResponse(): match[STUNInfo][expiryTime] member not found"));
                                                        }
                                                    } else {
                                                        status = ER_FAIL;
                                                        QCC_LogError(status, ("ParseMessagesResponse(): match[STUNInfo][pwd] member not found"));
                                                    }
                                                } else {
                                                    status = ER_FAIL;
                                                    QCC_LogError(status, ("ParseMessagesResponse(): match[STUNInfo][acct] member not found"));
                                                }
                                            } else {
                                                status = ER_FAIL;
                                                QCC_LogError(status, ("ParseMessagesResponse(): match[STUNInfo][address] member not found"));
                                            }
                                        } else {
                                            status = ER_FAIL;
                                            QCC_LogError(status, ("ParseMessagesResponse(): match[STUNInfo] member not found"));
                                        }
                                    } else {
                                        status = ER_FAIL;
                                        QCC_LogError(status, ("ParseMessagesResponse(): match[peerAddr] member not found"));
                                    }
                                } else {
                                    status = ER_FAIL;
                                    QCC_LogError(status, ("ParseMessagesResponse(): match[service] member not found"));
                                }
                            } else {
                                status = ER_FAIL;
                                QCC_LogError(status, ("ParseMessagesResponse(): match[searchedService] member not found"));
                            }
                        } else {
                            status = ER_FAIL;
                            QCC_LogError(status, ("ParseMessagesResponse(): match member not found"));
                        }
                    } else if (msgsObjArrayMember[type] == "addressCandidates") {
                        QCC_DbgPrintf(("ParseMessagesResponse(): [%d] Address Candidates Message", j));

                        if (msgsObjArrayMember.isMember(addressCandidates)) {
                            Json::Value addressCandidatesObj = msgsObjArrayMember[addressCandidates];

                            ICECandidates tempCandidateMsg;

                            if (addressCandidatesObj.isMember(peerAddr)) {
                                if (addressCandidatesObj.isMember(ice_ufrag)) {
                                    if (addressCandidatesObj.isMember(ice_pwd)) {
                                        tempMsg.type = ADDRESS_CANDIDATES_RESPONSE;
                                        AddressCandidatesResponse* AddressCandidates = new AddressCandidatesResponse();
                                        AddressCandidates->peerAddr = String(addressCandidatesObj[peerAddr].asCString());
                                        AddressCandidates->ice_ufrag = String(addressCandidatesObj[ice_ufrag].asCString());
                                        AddressCandidates->ice_pwd = String(addressCandidatesObj[ice_pwd].asCString());

                                        if (addressCandidatesObj.isMember(candidates)) {
                                            Json::Value candidatesObj = addressCandidatesObj[candidates];

                                            if (candidatesObj.isArray()) {
                                                if (!candidatesObj.empty()) {
                                                    for (Json::UInt k = 0; k < candidatesObj.size(); k++) {

                                                        Json::Value candidatesObjArrayMember = candidatesObj[k];

                                                        if (candidatesObjArrayMember.isMember(type)) {
                                                            if (candidatesObjArrayMember.isMember(foundation)) {
                                                                if (candidatesObjArrayMember.isMember(componentID)) {
                                                                    if (candidatesObjArrayMember.isMember(transport)) {
                                                                        if (candidatesObjArrayMember.isMember(priority)) {
                                                                            if (candidatesObjArrayMember.isMember(address)) {
                                                                                if (candidatesObjArrayMember.isMember(port)) {
                                                                                    tempCandidateMsg.type = GetICECandidateTypeValue(String(candidatesObjArrayMember[type].asCString()));
                                                                                    tempCandidateMsg.foundation = String(candidatesObjArrayMember[foundation].asCString());
                                                                                    tempCandidateMsg.componentID = candidatesObjArrayMember[componentID].asInt();
                                                                                    tempCandidateMsg.transport = GetICETransportTypeValue(String(candidatesObjArrayMember[transport].asCString()));
                                                                                    tempCandidateMsg.priority = candidatesObjArrayMember[priority].asInt();
                                                                                    tempCandidateMsg.address = IPAddress(String(candidatesObjArrayMember[address].asCString()));
                                                                                    tempCandidateMsg.port = candidatesObjArrayMember[port].asInt();

                                                                                    if (tempCandidateMsg.type != HOST_CANDIDATE) {
                                                                                        if (candidatesObjArrayMember.isMember(raddress)) {
                                                                                            if (candidatesObjArrayMember.isMember(rport)) {
                                                                                                tempCandidateMsg.raddress = IPAddress(String(candidatesObjArrayMember[raddress].asCString()));
                                                                                                tempCandidateMsg.rport = candidatesObjArrayMember[rport].asInt();

                                                                                                AddressCandidates->candidates.push_back(tempCandidateMsg);
                                                                                            } else {
                                                                                                status = ER_FAIL;
                                                                                                QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][rport] member not found for "
                                                                                                                      "candidate type %s", candidatesObjArrayMember[type].asCString()));
                                                                                            }

                                                                                        } else {
                                                                                            status = ER_FAIL;
                                                                                            QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][raddress] member not found for "
                                                                                                                  "candidate type %s", candidatesObjArrayMember[type].asCString()));
                                                                                        }
                                                                                    } else {
                                                                                        AddressCandidates->candidates.push_back(tempCandidateMsg);
                                                                                    }

                                                                                } else {
                                                                                    status = ER_FAIL;
                                                                                    QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][port] member not found"));
                                                                                }

                                                                            } else {
                                                                                status = ER_FAIL;
                                                                                QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][address] member not found"));
                                                                            }

                                                                        } else {
                                                                            status = ER_FAIL;
                                                                            QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][priority] member not found"));
                                                                        }

                                                                    } else {
                                                                        status = ER_FAIL;
                                                                        QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][transport] member not found"));
                                                                    }

                                                                } else {
                                                                    status = ER_FAIL;
                                                                    QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][componentID] member not found"));
                                                                }

                                                            } else {
                                                                status = ER_FAIL;
                                                                QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][foundation] member not found"));
                                                            }
                                                        } else {
                                                            status = ER_FAIL;
                                                            QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates][type] member not found"));
                                                        }
                                                    }

                                                    if (!AddressCandidates->candidates.empty()) {
                                                        if (addressCandidatesObj.isMember(STUNInfo)) {
                                                            Json::Value STUNInfoObj = addressCandidatesObj[STUNInfo];

                                                            if (STUNInfoObj.isMember(address)) {
                                                                if (STUNInfoObj.isMember(acct)) {
                                                                    if (STUNInfoObj.isMember(pwd)) {
                                                                        if (STUNInfoObj.isMember(expiryTime)) {
                                                                            AddressCandidates->STUNInfoPresent = true;
                                                                            status = AddressCandidates->STUNInfo.address.SetAddress(String(STUNInfoObj[address].asCString()));

                                                                            if (status != ER_OK) {
                                                                                QCC_LogError(status, ("ParseMessagesResponse(): Invalid STUN Server address specified in Address Candidates response"));
                                                                                return status;
                                                                            }

                                                                            if (STUNInfoObj.isMember(port)) {
                                                                                AddressCandidates->STUNInfo.port = STUNInfoObj[port].asInt();
                                                                            } else {
                                                                                QCC_DbgPrintf(("ParseMessagesResponse(): Set port to the default value as the member addressCandidates[STUNInfo][port] was not found"));
                                                                            }

                                                                            AddressCandidates->STUNInfo.acct = String(STUNInfoObj[acct].asCString());
                                                                            if (AddressCandidates->STUNInfo.acct.size() > TURN_ACCT_TOKEN_MAX_SIZE) {
                                                                                QCC_LogError(ER_FAIL, ("%s: Size of the TURN acct token (%u) is greater than max allowed %u", __FUNCTION__, AddressCandidates->STUNInfo.acct.size(), TURN_ACCT_TOKEN_MAX_SIZE));
                                                                            }
                                                                            AddressCandidates->STUNInfo.pwd = String(STUNInfoObj[pwd].asCString());
                                                                            AddressCandidates->STUNInfo.expiryTime = ((STUNInfoObj[expiryTime].asInt()) - TURN_TOKEN_EXPIRY_TIME_BUFFER_IN_SECONDS) * 1000;
                                                                            AddressCandidates->STUNInfo.recvTime = GetTimestamp64();

                                                                            if (STUNInfoObj.isMember(relay)) {

                                                                                Json::Value relayObj = STUNInfoObj[relay];

                                                                                if (relayObj.isMember(address)) {
                                                                                    if (relayObj.isMember(port)) {
                                                                                        AddressCandidates->STUNInfo.relayInfoPresent = true;
                                                                                        status = AddressCandidates->STUNInfo.relay.address.SetAddress(String(relayObj[address].asCString()));

                                                                                        if (status != ER_OK) {
                                                                                            QCC_LogError(status, ("ParseMessagesResponse(): Invalid Relay Server address specified in Address Candidates response"));
                                                                                            return status;
                                                                                        }

                                                                                        AddressCandidates->STUNInfo.relay.port = relayObj[port].asInt();

                                                                                        tempMsg.response = static_cast<AddressCandidatesResponse*>(AddressCandidates);
                                                                                        parsedResponse.msgs.push_back(tempMsg);
                                                                                        PrintMessageResponse(tempMsg);
                                                                                    } else {
                                                                                        status = ER_FAIL;
                                                                                        QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[STUNInfo][relay][port] member not found"));
                                                                                    }
                                                                                } else {
                                                                                    status = ER_FAIL;
                                                                                    QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[STUNInfo][relay][address] member not found"));
                                                                                }
                                                                            } else {
                                                                                tempMsg.response = static_cast<AddressCandidatesResponse*>(AddressCandidates);
                                                                                parsedResponse.msgs.push_back(tempMsg);
                                                                                PrintMessageResponse(tempMsg);
                                                                                QCC_DbgPrintf(("ParseMessagesResponse(): addressCandidates[STUNInfo][relay] member not found"));
                                                                            }
                                                                        } else {
                                                                            status = ER_FAIL;
                                                                            QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[STUNInfo][expiryTime] member not found"));
                                                                        }
                                                                    } else {
                                                                        status = ER_FAIL;
                                                                        QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[STUNInfo][pwd] member not found"));
                                                                    }
                                                                } else {
                                                                    status = ER_FAIL;
                                                                    QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[STUNInfo][acct] member not found"));
                                                                }
                                                            } else {
                                                                status = ER_FAIL;
                                                                QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[STUNInfo][address] member not found"));
                                                            }

                                                        } else {
                                                            tempMsg.response = static_cast<AddressCandidatesResponse*>(AddressCandidates);
                                                            parsedResponse.msgs.push_back(tempMsg);
                                                            PrintMessageResponse(tempMsg);
                                                            QCC_DbgPrintf(("ParseMessagesResponse(): addressCandidates[STUNInfo] member not found"));
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            status = ER_FAIL;
                                            QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[candidates] member not found"));
                                        }
                                    } else {
                                        status = ER_FAIL;
                                        QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[ice-pwd] member not found"));
                                    }
                                } else {
                                    status = ER_FAIL;
                                    QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[ice-ufrag] member not found"));
                                }
                            } else {
                                status = ER_FAIL;
                                QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates[peerAddr] member not found"));
                            }
                        } else {
                            status = ER_FAIL;
                            QCC_LogError(status, ("ParseMessagesResponse(): addressCandidates member not found"));
                        }
                    } else if (msgsObjArrayMember[type] == "matchRevoked") {
                        QCC_DbgPrintf(("ParseMessagesResponse(): [%d] Match Revoked Message", j));

                        if (msgsObjArrayMember.isMember(matchRevoked)) {
                            Json::Value revokeObj = msgsObjArrayMember[matchRevoked];

                            if (revokeObj.isMember(peerAddr)) {
                                tempMsg.type = MATCH_REVOKED_RESPONSE;
                                MatchRevokedResponse* MatchRevoked = new MatchRevokedResponse();
                                MatchRevoked->peerAddr = String(revokeObj[peerAddr].asCString());

                                if (revokeObj.isMember(deleteAll)) {
                                    MatchRevoked->deleteAll = revokeObj[deleteAll].asBool();

                                    if (MatchRevoked->deleteAll) {
                                        tempMsg.response = static_cast<MatchRevokedResponse*>(MatchRevoked);
                                        parsedResponse.msgs.push_back(tempMsg);
                                        PrintMessageResponse(tempMsg);
                                    }
                                }

                                if ((!revokeObj.isMember(deleteAll)) || (!MatchRevoked->deleteAll)) {
                                    if (revokeObj.isMember(services)) {
                                        Json::Value servicesObj = revokeObj[services];

                                        if (servicesObj.isArray()) {
                                            if (!servicesObj.empty()) {
                                                for (Json::UInt l = 0; l < servicesObj.size(); l++) {
                                                    MatchRevoked->services.push_back(String(servicesObj[l].asCString()));
                                                }
                                                tempMsg.response = static_cast<MatchRevokedResponse*>(MatchRevoked);
                                                parsedResponse.msgs.push_back(tempMsg);
                                                PrintMessageResponse(tempMsg);
                                            } else {
                                                status = ER_FAIL;
                                                QCC_LogError(status, ("ParseMessagesResponse(): matchRevoked[services] array empty"));
                                            }
                                        }
                                    } else {
                                        status = ER_FAIL;
                                        QCC_LogError(status, ("ParseMessagesResponse(): Either matchRevoked[deleteAll] member not found or not set to true AND matchRevoked[services] member not found"));
                                    }
                                }
                            } else {
                                status = ER_FAIL;
                                QCC_LogError(status, ("ParseMessagesResponse(): matchRevoked[peerAddr] member not found"));
                            }

                        } else {
                            status = ER_FAIL;
                            QCC_LogError(status, ("ParseMessagesResponse(): matchRevoked member not found"));
                        }
                    } else if (msgsObjArrayMember[type] == "startICEChecks") {
                        QCC_DbgPrintf(("ParseMessagesResponse(): [%d] Start ICE Checks Message", j));

                        if (msgsObjArrayMember.isMember(startICEChecks)) {
                            Json::Value startICEChecksObj = msgsObjArrayMember[startICEChecks];

                            if (startICEChecksObj.isMember(peerAddr)) {
                                tempMsg.type = START_ICE_CHECKS_RESPONSE;
                                StartICEChecksResponse* StartICEChecks = new StartICEChecksResponse();
                                StartICEChecks->peerAddr = String(startICEChecksObj[peerAddr].asCString());

                                tempMsg.response = static_cast<StartICEChecksResponse*>(StartICEChecks);
                                parsedResponse.msgs.push_back(tempMsg);
                                PrintMessageResponse(tempMsg);

                            } else {
                                status = ER_FAIL;
                                QCC_LogError(status, ("ParseMessagesResponse(): startICEChecks[peerAddr] member not found"));
                            }

                        } else {
                            status = ER_FAIL;
                            QCC_LogError(status, ("ParseMessagesResponse(): startICEChecks member not found"));
                        }
                    } else {
                        status = ER_FAIL;
                        QCC_LogError(status, ("ParseMessagesResponse(): Unrecognized Message Response received from Rendezvous Server"));
                    }
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("ParseMessagesResponse(): msgs array is empty"));
            }
        } else {
            status = ER_FAIL;
            QCC_LogError(status, ("ParseMessagesResponse(): msgs is not an array"));
        }
    } else {
        status = ER_FAIL;
        QCC_LogError(status, ("ParseMessagesResponse(): No field named msgs in the response"));
    }

    return status;

}

/**
 * Worker function used to generate the string corresponding
 * to the authentication mechanism type.
 */
String GetSASLAuthMechanismString(SASLAuthenticationMechanism authMechanism)
{
    String retStr = String("SCRAM-SHA-1");

    switch (authMechanism) {
    case SCRAM_SHA_1_MECHANISM:
        retStr = String("SCRAM-SHA-1");
        break;

    default:
        break;
    }

    QCC_DbgPrintf(("GetSASLAuthMechanismString():%s", retStr.c_str()));

    return retStr;
}

/**
 * Worker function used to generate an ICE Candidates Message in
 * the JSON format.
 */
String GenerateJSONClientLoginRequest(ClientLoginRequest request)
{
    Json::Value clientLoginRequest;

    Json::StaticString daemonID("daemonID");
    Json::StaticString clearClientState("clearClientState");
    Json::StaticString mechanism("mechanism");
    Json::StaticString message("message");

    clientLoginRequest[daemonID] = request.daemonID.c_str();
    if (request.clearClientState) {
        clientLoginRequest[clearClientState] = request.clearClientState;
    }
    clientLoginRequest[mechanism] = GetSASLAuthMechanismString(request.mechanism).c_str();
    clientLoginRequest[message] = request.message.c_str();

    Json::StyledWriter writer;
    String retStr = writer.write(clientLoginRequest).c_str();

    QCC_DbgPrintf(("GenerateJSONClientLoginRequest():%s", retStr.c_str()));

    return retStr;
}


/**
 * Worker function used to parse the client login first response
 */
QStatus ParseClientLoginFirstResponse(Json::Value receivedResponse, ClientLoginFirstResponse& parsedResponse)
{
    QStatus status = ER_OK;

    Json::StaticString message("message");

    if (receivedResponse.isMember(message)) {

        parsedResponse.message = String(receivedResponse[message].asCString());
        QCC_DbgPrintf(("ParseClientLoginFirstResponse(): message = %s", receivedResponse[message].asCString()));

    } else {

        status = ER_FAIL;
        QCC_LogError(status, ("ParseClientLoginFirstResponse(): Message does not seem to have a message field"));

    }

    return status;
}

/**
 * Worker function used to parse a client login final response
 */
QStatus ParseClientLoginFinalResponse(Json::Value receivedResponse, ClientLoginFinalResponse& parsedResponse)
{
    QStatus status = ER_OK;

    Json::StaticString message("message");
    Json::StaticString peerID("peerID");
    Json::StaticString peerAddr("peerAddr");
    Json::StaticString daemonRegistrationRequired("daemonRegistrationRequired");
    Json::StaticString sessionActive("sessionActive");
    Json::StaticString configData("configData");
    Json::StaticString Tkeepalive("Tkeepalive");

    if (receivedResponse.isMember(message)) {

        parsedResponse.message = String(receivedResponse[message].asCString());
        QCC_DbgPrintf(("ParseClientLoginFinalResponse(): message = %s", receivedResponse[message].asCString()));


        if (receivedResponse.isMember(peerID)) {
            if (receivedResponse.isMember(peerAddr)) {
                if (receivedResponse.isMember(configData)) {

                    parsedResponse.SetpeerID(String(receivedResponse[peerID].asCString()));
                    QCC_DbgPrintf(("ParseClientLoginFinalResponse(): peerID = %s", receivedResponse[peerID].asCString()));

                    parsedResponse.SetpeerAddr(String(receivedResponse[peerAddr].asCString()));
                    QCC_DbgPrintf(("ParseClientLoginFinalResponse(): peerAddr = %s", receivedResponse[peerAddr].asCString()));

                    Json::Value configDataObj = receivedResponse[configData];

                    if (configDataObj.isMember(Tkeepalive)) {

                        ConfigData data;
                        data.SetTkeepalive(configDataObj[Tkeepalive].asInt());
                        parsedResponse.SetconfigData(data);
                        QCC_DbgPrintf(("ParseClientLoginFinalResponse(): configData.Tkeepalive = %d", configDataObj[Tkeepalive].asInt()));

                        if (receivedResponse.isMember(daemonRegistrationRequired)) {

                            parsedResponse.SetdaemonRegistrationRequired(receivedResponse[daemonRegistrationRequired].asBool());
                            QCC_DbgPrintf(("ParseClientLoginFinalResponse(): daemonRegistrationRequired = %d", receivedResponse[daemonRegistrationRequired].asBool()));

                        } else {

                            parsedResponse.SetdaemonRegistrationRequired(false);
                            QCC_DbgPrintf(("ParseClientLoginFinalResponse(): Set daemonRegistrationRequired to false as Server did not send the field"));

                        }

                        if (receivedResponse.isMember(sessionActive)) {

                            parsedResponse.SetsessionActive(receivedResponse[sessionActive].asBool());
                            QCC_DbgPrintf(("ParseClientLoginFinalResponse(): sessionActive = %d", receivedResponse[sessionActive].asBool()));

                        } else {

                            parsedResponse.SetsessionActive(false);
                            QCC_DbgPrintf(("ParseClientLoginFinalResponse(): Set sessionActive to false as Server did not send the field"));

                        }

                    } else {
                        status = ER_FAIL;
                        QCC_LogError(status, ("ParseClientLoginFinalResponse(): configData member in the message does not seem to have the Tkeepalive field"));
                    }
                } else {
                    status = ER_FAIL;
                    QCC_LogError(status, ("ParseClientLoginFinalResponse(): configData member not found"));
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("ParseClientLoginFinalResponse(): peerAddr member not found"));
            }
        }

    } else {

        status = ER_FAIL;
        QCC_LogError(status, ("ParseClientLoginFinalResponse(): Message does not seem to have a message field"));

    }

    return status;
}

/**
 * Worker function used to generate the enum corresponding
 * to the error string.
 */
SASLError GetSASLError(String errorStr)
{
    SASLError retVal = INVALID;

    if (errorStr == String("invalid-encoding")) {
        retVal = INVALID_ENCODING;
    } else if (errorStr == String("extensions-not-supported")) {
        retVal = EXTENSIONS_NOT_SUPPORTED;
    } else if (errorStr == String("invalid-proof")) {
        retVal = INVALID_PROOF;
    } else if (errorStr == String("channel-bindings-dont-match")) {
        retVal = CHANNEL_BINDINGS_DONT_MATCH;
    } else if (errorStr == String("server-does-support-channel-binding")) {
        retVal = SERVER_DOES_NOT_SUPPORT_CHANNEL_BINDING;
    } else if (errorStr == String("channel-binding-not-supported")) {
        retVal = CHANNEL_BINDING_NOT_SUPPORTED;
    } else if (errorStr == String("unsupported-channel-binding-errorStr")) {
        retVal = UNSUPPORTED_CHANNEL_BINDING_TYPE;
    } else if (errorStr == String("unknown-user")) {
        retVal = UNKNOWN_USER;
    } else if (errorStr == String("invalid-username-encoding")) {
        retVal = INVALID_USERNAME_ENCODING;
    } else if (errorStr == String("no-resources")) {
        retVal = NO_RESOURCES;
    } else if (errorStr == String("other-error")) {
        retVal = OTHER_ERROR;
    } else if (errorStr == String("deactivated-user")) {
        retVal = DEACTIVATED_USER;
    }

    QCC_DbgPrintf(("GetSASLError():%s", errorStr.c_str()));

    return retVal;
}

/**
 * Worker function used to print the string equivalent of a SASL error.
 */
String GetSASLErrorString(SASLError error)
{
    String retVal = String("INVALID");

    if (error == INVALID_ENCODING) {
        retVal = String("invalid-encoding");
    } else if (error == EXTENSIONS_NOT_SUPPORTED) {
        retVal = String("extensions-not-supported");
    } else if (error == INVALID_PROOF) {
        retVal = String("invalid-proof");
    } else if (error == CHANNEL_BINDINGS_DONT_MATCH) {
        retVal = String("channel-bindings-dont-match");
    } else if (error == SERVER_DOES_NOT_SUPPORT_CHANNEL_BINDING) {
        retVal = String("server-does-support-channel-binding");
    } else if (error == CHANNEL_BINDING_NOT_SUPPORTED) {
        retVal = String("channel-binding-not-supported");
    } else if (error == UNSUPPORTED_CHANNEL_BINDING_TYPE) {
        retVal = String("unsupported-channel-binding-error");
    } else if (error == UNKNOWN_USER) {
        retVal = String("unknown-user");
    } else if (error == INVALID_USERNAME_ENCODING) {
        retVal = String("invalid-username-encoding");
    } else if (error == NO_RESOURCES) {
        retVal = String("no-resources");
    } else if (error == OTHER_ERROR) {
        retVal = String("other-error");
    } else if (error == DEACTIVATED_USER) {
        retVal = String("deactivated-user");
    }

    QCC_DbgPrintf(("GetSASLErrorString():%s", retVal.c_str()));

    return retVal;
}

/**
 * Worker function used to set an attribute in the SASL Message.
 */
void SetSASLAttribute(char attribute, String attrVal, String& retMsg)
{
    if (!retMsg.empty()) {
        retMsg += ",";
    }

    retMsg += String(attribute) + String("=") + attrVal;
}

/**
 * Worker function used to generate a SASL Message string from the SASL attributes.
 */
String GenerateSASLMessage(SASLMessage message, bool firstMessage)
{
    String retMessage;

    if (firstMessage) {
        retMessage = "n,";
    }

    if (message.is_a_Present()) {
        SetSASLAttribute('a', message.a, retMessage);
    }

    if (message.is_n_Present()) {
        SetSASLAttribute('n', message.n, retMessage);
    }

    if (message.is_m_Present()) {
        SetSASLAttribute('m', message.m, retMessage);
    }

    if (message.is_c_Present()) {
        SetSASLAttribute('c', message.c, retMessage);
    }

    if (message.is_r_Present()) {
        SetSASLAttribute('r', message.r, retMessage);
    }

    if (message.is_s_Present()) {
        SetSASLAttribute('s', message.s, retMessage);
    }

    if (message.is_i_Present()) {
        SetSASLAttribute('i', U32ToString(message.i), retMessage);
    }

    if (message.is_p_Present()) {
        SetSASLAttribute('p', message.p, retMessage);
    }

    if (message.is_v_Present()) {
        SetSASLAttribute('v', message.v, retMessage);
    }

    if (message.is_e_Present()) {
        SetSASLAttribute('e', message.e, retMessage);
    }

    QCC_DbgPrintf(("GenerateSASLMessage(): retMessage = %s", retMessage.c_str()));

    return retMessage;
}

/**
 * Worker function used to parse a SASL Message.
 */
SASLMessage ParseSASLMessage(String message)
{
    SASLMessage retMsg;
    retMsg.clear();

    map<String, String> argMap;

    String argStr = message;

    size_t pos = 0;
    size_t endPos = 0;

    /* Parse the key value pairs into the argMap */
    while (String::npos != endPos) {
        size_t eqPos = argStr.find_first_of('=', pos);
        endPos = (eqPos == String::npos) ? String::npos : argStr.find_first_of(",", eqPos);
        if (String::npos != eqPos) {
            argMap[argStr.substr(pos, eqPos - pos)] = (String::npos == endPos) ? argStr.substr(eqPos + 1) : argStr.substr(eqPos + 1, endPos - eqPos - 1);
        }
        pos = endPos + 1;
    }

    /* Generate a SASLMessage from the values in the argMap */
    map<String, String>::iterator i;

    i = argMap.find("a");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_a(i->second);
        }
    }

    i = argMap.find("n");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_n(i->second);
        }
    }

    i = argMap.find("m");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_m(i->second);
        }
    }

    i = argMap.find("r");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_r(i->second);
        }
    }

    i = argMap.find("c");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_c(i->second);
        }
    }

    i = argMap.find("s");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_s(i->second);
        }
    }

    i = argMap.find("i");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_i(i->second);
        }
    }

    i = argMap.find("p");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_p(i->second);
        }
    }

    i = argMap.find("v");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_v(i->second);
        }
    }

    i = argMap.find("e");
    if (i != argMap.end()) {
        if (!(i->second).empty()) {
            retMsg.Set_e(GetSASLError(i->second));
        }
    }

    return retMsg;
}

/**
 * Worker function used to generate the string corresponding
 * to the OS type.
 */
String GetOSTypeString(OSType type)
{
    String retStr = String("NONE");

    switch (type) {

    case ANDROID_OS:
        retStr = String("ANDROID");
        break;

    case WINDOWS_OS:
        retStr = String("WINDOWS");
        break;

    case DARWIN_OS:
        retStr = String("DARWIN");
        break;

    case LINUX_OS:
        retStr = String("LINUX");
        break;

    case WINRT_OS:
        retStr = String("WINRT");
        break;

    default:
        break;
    }

    return retStr;
};

/**
 * Worker function used to generate the string corresponding
 * to the Search Match Type type.
 */
String GetSearchMatchTypeString(SearchMatchType type)
{
    String retStr = String("ProximityBased");

    switch (type) {
    case PROXIMITY_BASED:
        retStr = String("ProximityBased");
        break;

    default:
        break;
    }

    QCC_DbgPrintf(("GetSearchMatchTypeString():%s", retStr.c_str()));

    return retStr;
}

/**
 * Worker function used to generate an Daemon Registration Message in
 * the JSON format.
 */
String GenerateJSONDaemonRegistrationMessage(DaemonRegistrationMessage message)
{
    Json::Value daemonRegMsg;

    Json::StaticString daemonID("daemonID");
    Json::StaticString daemonVersion("daemonVersion");
    Json::StaticString devMake("devMake");
    Json::StaticString devModel("devModel");
    Json::StaticString osType("osType");
    Json::StaticString osVersion("osVersion");

    daemonRegMsg[daemonID] = message.daemonID.c_str();
    daemonRegMsg[daemonVersion] = message.daemonVersion.c_str();
    daemonRegMsg[devMake] = message.devMake.c_str();
    daemonRegMsg[devModel] = message.devModel.c_str();
    daemonRegMsg[osType] = GetOSTypeString(message.osType).c_str();
    daemonRegMsg[osVersion] = message.osVersion.c_str();

    Json::StyledWriter writer;
    String retStr = writer.write(daemonRegMsg).c_str();

    QCC_DbgPrintf(("GenerateJSONDaemonRegistrationMessage():%s", retStr.c_str()));

    return retStr;
}

/**
 * Returns the Advertisement message URI.
 */
String GetAdvertisementUri(String peerID)
{
    char buffer[500];
    sprintf(buffer, AdvertisementUri.c_str(), peerID.c_str());
    return(String(buffer));
}

/**
 * Returns the Search message URI.
 */
String GetSearchUri(String peerID)
{
    char buffer[500];
    sprintf(buffer, SearchUri.c_str(), peerID.c_str());
    return(String(buffer));
}

/**
 * Returns the Proximity message URI.
 */
String GetProximityUri(String peerID)
{
    char buffer[500];
    sprintf(buffer, ProximityUri.c_str(), peerID.c_str());
    return(String(buffer));
}

/**
 * Returns the Address Candidates message URI.
 */
String GetAddressCandidatesUri(String selfPeerID, String destPeerAddress, bool addStun)
{
    char buffer[600];
    if (addStun) {
        sprintf(buffer, AddressCandidatesWithSTUNUri.c_str(), selfPeerID.c_str(), destPeerAddress.c_str());
    } else {
        sprintf(buffer, AddressCandidatesUri.c_str(), selfPeerID.c_str(), destPeerAddress.c_str());
    }
    return(String(buffer));
}

/**
 * Returns the Rendezvous Session Delete message URI.
 */
String GetRendezvousSessionDeleteUri(String peerID)
{
    char buffer[500];
    sprintf(buffer, RendezvousSessionDeleteUri.c_str(), peerID.c_str());
    return(String(buffer));
}

/**
 * Returns the GET message URI.
 */
String GetGETUri(String peerID)
{
    char buffer[500];
    sprintf(buffer, GETUri.c_str(), peerID.c_str());
    return(String(buffer));
}

/**
 * Returns the Client Login URI.
 */
String GetClientLoginUri(void)
{
    return(ClientLoginUri);
}

/**
 * Returns the Daemon Registration message URI.
 */
String GetDaemonRegistrationUri(String peerID)
{
    char buffer[500];
    sprintf(buffer, DaemonRegistrationUri.c_str(), peerID.c_str());
    return(String(buffer));
}

/**
 * Returns the refresh token URI.
 */
String GetTokenRefreshUri(String peerID)
{
    char buffer[800];
    sprintf(buffer, TokenRefreshUri.c_str(), peerID.c_str());
    return(String(buffer));
}

}
