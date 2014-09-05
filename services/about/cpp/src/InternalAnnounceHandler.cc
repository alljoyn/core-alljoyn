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
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <alljoyn/about/AnnounceHandler.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>


#include "InternalAnnounceHandler.h"

using namespace ajn;
using namespace services;

#define QCC_MODULE "ALLJOYN_ABOUT_ANNOUNCE_HANDLER"

InternalAnnounceHandler::InternalAnnounceHandler(BusAttachment& bus) :
    bus(bus), announceSignalMember(NULL), emptyMatchRule("type='signal',interface='org.alljoyn.About',member='Announce',sessionless='t'") {
    QCC_DbgTrace(("InternalAnnounceHandler::%s", __FUNCTION__));
}

InternalAnnounceHandler::~InternalAnnounceHandler() {
    QCC_DbgTrace(("InternalAnnounceHandler::%s", __FUNCTION__));
    // don't delete the announceMap if another thread holds the lock
    announceMapLock.Lock(MUTEX_CONTEXT);
    announceMap.clear();
    announceMapLock.Unlock(MUTEX_CONTEXT);

    announceHandlerLock.Lock(MUTEX_CONTEXT);
    /* clear the contents of the announceHandlerList and wait for any outstanding callbacks. */
    std::vector<ProtectedAnnounceHandler>::iterator pahit = announceHandlerList.begin();
    while (pahit != announceHandlerList.end()) {
        ProtectedAnnounceHandler l = *pahit;

        /* Remove listener and wait for any outstanding listener callback(s) to complete */
        announceHandlerList.erase(pahit);
        announceHandlerLock.Unlock(MUTEX_CONTEXT);
        while (l.GetRefCount() > 1) {
            qcc::Sleep(4);
        }

        announceHandlerLock.Lock(MUTEX_CONTEXT);
        pahit = announceHandlerList.begin();
    }
    assert(announceHandlerList.empty());
    announceHandlerLock.Unlock(MUTEX_CONTEXT);
}

QStatus InternalAnnounceHandler::AddHandler(AnnounceHandler& handler, const char** implementsInterfaces, size_t numberInterfaces) {
    QCC_DbgTrace(("InternalAnnounceHandler::%s", __FUNCTION__));

    RegisteredHandlerState rhs;
    for (size_t i = 0; i < numberInterfaces; ++i) {
        rhs.interfaces.insert(implementsInterfaces[i]);
    }

    announceMapLock.Lock(MUTEX_CONTEXT);
    AnnounceMap::value_type hi_pair = std::make_pair(&handler, rhs);
    announceMap.insert(hi_pair);
    announceMapLock.Unlock(MUTEX_CONTEXT);

    qcc::String matchRule = GetMatchRule(rhs.interfaces);
    QCC_DbgTrace(("Calling AddMatch(\"%s\")", matchRule.c_str()));
    return bus.AddMatch(matchRule.c_str());
}

QStatus InternalAnnounceHandler::RemoveHandler(AnnounceHandler& handler, const char** implementsInterfaces, size_t numberInterfaces) {
    QCC_DbgTrace(("InternalAnnounceHandler::%s", __FUNCTION__));
    QStatus status = ER_INVALID_DATA;
    std::pair <AnnounceMap::iterator, AnnounceMap::iterator> handlers;
    announceMapLock.Lock(MUTEX_CONTEXT);
    std::vector<AnnounceMap::iterator> toRemove;
    std::vector<qcc::String> ruleToRemove;

    handlers = announceMap.equal_range(&handler);
    for (AnnounceMap::iterator it = handlers.first; it != handlers.second; ++it) {
        if (implementsInterfaces == NULL) {
            if (it->second.interfaces.empty()) {
                QCC_DbgTrace(("InternalAnnounceHandler::%s successfully removed the wild card AnnounceHandler", __FUNCTION__));
                toRemove.push_back(it);
                status = ER_OK;
                break;
            }
        } else {
            std::set<qcc::String> interfaces;
            for (size_t i = 0; i < numberInterfaces; ++i) {
                interfaces.insert(implementsInterfaces[i]);
            }

            if (interfaces == it->second.interfaces) {
                QCC_DbgTrace(("InternalAnnounceHandler::%s successfully removed the interface AnnounceHandler", __FUNCTION__));
                toRemove.push_back(it);
                status = ER_OK;
                break;
            }
        }
    }

    for (std::vector<AnnounceMap::iterator>::iterator trit = toRemove.begin(); trit != toRemove.end(); ++trit) {
        //call remove match for the interface
        qcc::String matchRule = GetMatchRule((*trit)->second.interfaces);
        // The MatchRule can not be removed while the announceMapLock is
        // held since it could result in a deadlock.  So we build a list
        // of all the match rules that should be removed and then remove
        // them after the announceMapLock is released.
        ruleToRemove.push_back(matchRule);

        bool erased = false;
        while (!erased) {
            //remove the announce handler from the announceMap;
            announceHandlerLock.Lock(MUTEX_CONTEXT);
            if (announceHandlerList.empty()) {
                announceMap.erase(*trit);
                erased = true;
            }
            announceHandlerLock.Unlock(MUTEX_CONTEXT);
            if (!erased) {
                qcc::Sleep(4);
            }
        }
    }
    announceMapLock.Unlock(MUTEX_CONTEXT);
    if (ER_OK == status) {
        for (uint32_t i = 0; i < ruleToRemove.size(); ++i) {
            QCC_DbgTrace(("Calling RemoveMatch(\"%s\")", ruleToRemove[i].c_str()));
            QStatus match_status = bus.RemoveMatch(ruleToRemove[i].c_str());
            // Its possible that the call to RemoveMatch could fail. This would
            // happen for one of two reasons. The router node is unreachable or
            // the match rule does not exist to remove. In both cases there is
            // nothing the user can to to fix the error so we log the error and
            // swallow the error.
            if (ER_OK != match_status) {
                QCC_LogError(match_status, ("Failed to removed match rule %s.", ruleToRemove[i].c_str()));
            }
        }
    }
    return status;
}


void InternalAnnounceHandler::RemoveAllHandlers() {
    QCC_DbgTrace(("InternalAnnounceHandler::%s", __FUNCTION__));
    QStatus status = ER_OK;
    announceMapLock.Lock(MUTEX_CONTEXT);
    AnnounceMap tmpAnnounceMap = announceMap;
    announceMap.clear();
    announceMapLock.Unlock(MUTEX_CONTEXT);

    // Its possible that the call to RemoveAllHandlers could fail. This would
    // happen for one of two reasons. The router node is unreachable or the
    // match rule does not exist to remove. In both cases there is nothing the
    // user can to do to fix the error so we log an error and swallow the error.
    for (AnnounceMap::iterator amit = tmpAnnounceMap.begin(); amit != tmpAnnounceMap.end(); ++amit) {
        //call remove match for each interface
        qcc::String matchRule = GetMatchRule(amit->second.interfaces);

        QCC_DbgTrace(("Calling RemoveMatch(\"%s\")", matchRule.c_str()));
        status = bus.RemoveMatch(matchRule.c_str());
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to removed match rule %s.", matchRule.c_str()));
        }
    }
}

bool InternalAnnounceHandler::ContainsInterface(const ObjectDescriptions& objectDescriptions, const qcc::String interface) const {
    size_t n = interface.find_first_of('*');
    for (ObjectDescriptions::const_iterator oit = objectDescriptions.begin(); oit != objectDescriptions.end(); ++oit) {
        for (size_t j = 0; j < oit->second.size(); ++j) {
            if (n == qcc::String::npos && interface == oit->second[j]) {
                return true;
            } else if (n != qcc::String::npos && interface.compare(0, n, oit->second[j].substr(0, n)) == 0) {
                return true;
            }
        }
    }
    return false;
}

void InternalAnnounceHandler::AnnounceSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath,
                                                    ajn::Message& message) {

    QCC_DbgTrace(("InternalAnnounceHandler::%s", __FUNCTION__));

    QCC_DbgPrintf(("received signal interface=%s method=%s", message->GetInterface(), message->GetMemberName()));

    if (strcmp(message->GetInterface(), "org.alljoyn.About") != 0 || strcmp(message->GetMemberName(), "Announce") != 0) {
        QCC_DbgPrintf(("This is not the signal we are looking for"));
        return;
    }
    const ajn::MsgArg* args = 0;
    size_t numArgs = 0;
    QStatus status;
    message->GetArgs(numArgs, args);
    if (numArgs == 4) {
#if !defined(NDEBUG)
        for (int i = 0; i < 4; i++) {
            QCC_DbgPrintf(("args[%d]=%s", i, args[i].ToString().c_str()));
        }
#endif
        uint16_t version = 0;
        uint16_t receivedPort = 0;
        AboutData aboutData;
        ObjectDescriptions objectDescriptions;

        status = args[0].Get("q", &version);
        if (status != ER_OK) {
            return;
        }
        status = args[1].Get("q", &receivedPort);
        if (status != ER_OK) {
            return;
        }

        MsgArg* objectDescriptionsArgs;
        size_t objectNum;
        status = args[2].Get("a(oas)", &objectNum, &objectDescriptionsArgs);
        if (status != ER_OK) {
            return;
        }

        for (size_t i = 0; i < objectNum; i++) {
            char* objectDescriptionPath;
            MsgArg* interfaceEntries;
            size_t interfaceNum;
            status = objectDescriptionsArgs[i].Get("(oas)", &objectDescriptionPath, &interfaceNum, &interfaceEntries);
            if (status != ER_OK) {
                return;
            }

            std::vector<qcc::String> localVector;
            for (size_t i = 0; i < interfaceNum; i++) {
                char* interfaceName;
                status = interfaceEntries[i].Get("s", &interfaceName);
                if (status != ER_OK) {
                    return;
                }
                localVector.push_back(interfaceName);
            }
            objectDescriptions.insert(std::pair<qcc::String, std::vector<qcc::String> >(objectDescriptionPath, localVector));
        }
        MsgArg* tempControlArg2;
        size_t languageTagNumElements;
        status = args[3].Get("a{sv}", &languageTagNumElements, &tempControlArg2);
        if (status != ER_OK) {
            return;
        }
        for (size_t i = 0; i < languageTagNumElements; i++) {
            char* tempKey;
            MsgArg* tempValue;
            status = tempControlArg2[i].Get("{sv}", &tempKey, &tempValue);
            if (status != ER_OK) {
                return;
            }
            aboutData.insert(std::pair<qcc::String, ajn::MsgArg>(tempKey, *tempValue));
        }

        announceMapLock.Lock(MUTEX_CONTEXT);
        qcc::String sender = message->GetSender();
        //look through map and send out the Announce if able to match the interfaces
        for (AnnounceMap::iterator it = announceMap.begin();
             it != announceMap.end(); ++it) {
            bool matchFound = true;
            //if second.size is zero then the user is trying to match an any interface
            for (std::set<qcc::String>::iterator it2 = it->second.interfaces.begin(); it2 != it->second.interfaces.end(); ++it2) {
                matchFound = ContainsInterface(objectDescriptions, (*it2));
                // if the interface is not in the objectDescription we can exit
                // the loop instantly with out checking the other interfaces
                if (!matchFound) {
                    break;
                }
            }
            bool invokeHandler = false;
            if (matchFound) {
                it->second.matchingPeers.insert(sender);
                invokeHandler = true;
            } else {
                std::set<qcc::String>::size_type deleted = it->second.matchingPeers.erase(sender);
                if (deleted != 0) {
                    /* The previous announcement for this peer matched the criteria for
                     * this handler. Invoke the handler one final time to alert the
                     * application that this peer no longer matches the criteria. */
                    invokeHandler = true;
                }
            }
            if (invokeHandler) {
                ProtectedAnnounceHandler pah = it->first;
                announceHandlerList.push_back(pah);
            }
        }
        announceMapLock.Unlock(MUTEX_CONTEXT);

        announceHandlerLock.Lock(MUTEX_CONTEXT);
        for (size_t i = 0; i < announceHandlerList.size(); ++i) {
            ProtectedAnnounceHandler l = announceHandlerList[i];
            announceHandlerLock.Unlock(MUTEX_CONTEXT);
            (*l)->Announce(version, receivedPort, message->GetSender(), objectDescriptions, aboutData);
            announceHandlerLock.Lock(MUTEX_CONTEXT);
        }

        announceHandlerList.clear();
        announceHandlerLock.Unlock(MUTEX_CONTEXT);
    }
}

qcc::String InternalAnnounceHandler::GetMatchRule(const std::set<qcc::String>& interfaces) const
{
    qcc::String matchRule = emptyMatchRule;
    for (std::set<qcc::String>::const_iterator it = interfaces.begin(); it != interfaces.end(); ++it) {
        matchRule += qcc::String(",implements='") + *it + qcc::String("'");
    }
    return matchRule;
}
