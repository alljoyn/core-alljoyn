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
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>


#include "AboutListenerInternal.h"

using namespace ajn;

#define QCC_MODULE "ALLJOYN_ABOUT_ANNOUNCE_HANDLER"

AboutListenerInternal::AboutListenerInternal(BusAttachment& bus) :
    bus(bus),
    announceSignalMemberSet(false),
    emptyMatchRule("type='signal',interface='org.alljoyn.About',member='Announce',sessionless='t'") {
    QCC_DbgTrace(("AboutListenerInternal::%s", __FUNCTION__));
}

AboutListenerInternal::~AboutListenerInternal() {
    QCC_DbgTrace(("AboutListenerInternal::%s", __FUNCTION__));
    // don't delete the announceMap if another thread holds the lock
    announceMapLock.Lock(MUTEX_CONTEXT);
    announceMap.clear();
    announceMapLock.Unlock(MUTEX_CONTEXT);

    announceHandlerLock.Lock(MUTEX_CONTEXT);
    /* clear the contents of the announceHandlerList and wait for any outstanding callbacks. */
    std::vector<ProtectedAboutListener>::iterator pahit = announceHandlerList.begin();
    while (pahit != announceHandlerList.end()) {
        ProtectedAboutListener l = *pahit;

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

QStatus AboutListenerInternal::AddHandler(AboutListener& handler, const char** implementsInterfaces, size_t numberInterfaces) {
    QCC_DbgTrace(("AboutListenerInternal::%s", __FUNCTION__));
    if (!announceSignalMemberSet) {
        announceSignalMemberSet = true;
        const InterfaceDescription* p_InterfaceDescription = bus.GetInterface(org::alljoyn::About::InterfaceName);
        assert(p_InterfaceDescription);
        const ajn::InterfaceDescription::Member* announceSignalMember = p_InterfaceDescription->GetMember("Announce");
        assert(announceSignalMember);
        QStatus status = bus.RegisterSignalHandler(this,
                                                   static_cast<MessageReceiver::SignalHandler>(&ajn::AboutListenerInternal::AnnounceSignalHandler),
                                                   announceSignalMember,
                                                   0);
        QCC_DbgTrace(("AboutListenerInternal::%s RegisteredSignalHandler", __FUNCTION__));
        if (status != ER_OK) {
            return status;
        }
    }

    std::set<qcc::String> interfaces;
    for (size_t i = 0; i < numberInterfaces; ++i) {
        interfaces.insert(implementsInterfaces[i]);
    }

    announceMapLock.Lock(MUTEX_CONTEXT);
    std::pair<AboutListener*, std::set<qcc::String> > hi_pair = std::make_pair(&handler, interfaces);
    announceMap.insert(hi_pair);
    announceMapLock.Unlock(MUTEX_CONTEXT);

    qcc::String matchRule = emptyMatchRule;
    for (std::set<qcc::String>::iterator it = interfaces.begin(); it != interfaces.end(); ++it) {
        matchRule += qcc::String(",implements='") + *it + qcc::String("'");
    }

    QCC_DbgTrace(("Calling AddMatch(\"%s\")", matchRule.c_str()));
    return bus.AddMatch(matchRule.c_str());
}

QStatus AboutListenerInternal::RemoveHandler(AboutListener& handler, const char** implementsInterfaces, size_t numberInterfaces) {
    QCC_DbgTrace(("AboutListenerInternal::%s", __FUNCTION__));
    QStatus status = ER_INVALID_DATA;
    std::pair <AnnounceMap::iterator, AnnounceMap::iterator> handlers;
    announceMapLock.Lock(MUTEX_CONTEXT);
    std::vector<AnnounceMap::iterator> toRemove;
    std::vector<qcc::String> ruleToRemove;

    handlers = announceMap.equal_range(&handler);
    for (AnnounceMap::iterator it = handlers.first; it != handlers.second; ++it) {
        if (implementsInterfaces == NULL) {
            if (it->second.empty()) {
                QCC_DbgTrace(("AboutListenerInternal::%s successfully removed the wild card AboutListener", __FUNCTION__));
                toRemove.push_back(it);
                status = ER_OK;
                break;
            }
        } else {
            std::set<qcc::String> interfaces;
            for (size_t i = 0; i < numberInterfaces; ++i) {
                interfaces.insert(implementsInterfaces[i]);
            }

            if (interfaces == it->second) {
                QCC_DbgTrace(("AboutListenerInternal::%s successfully removed the interface AboutListener", __FUNCTION__));
                toRemove.push_back(it);
                status = ER_OK;
                break;
            }
        }
    }

    for (std::vector<AnnounceMap::iterator>::iterator trit = toRemove.begin(); trit != toRemove.end(); ++trit) {
        //call remove match for the interface
        qcc::String matchRule = emptyMatchRule;
        for (std::set<qcc::String>::iterator it = (*trit)->second.begin(); it != (*trit)->second.end(); ++it) {
            matchRule += qcc::String(",implements='") + *it + qcc::String("'");
        }
        // The MatchRule can not be removed while the announceMapLock is
        // held since it could result in a deadlock.  So we build a list
        // of all the match rules that should be removed and then remove
        // them after the announceMapLock is released.
        ruleToRemove.push_back(matchRule);

        bool erased = false;
        while (!erased) {
            //remove the announce handler from the announceMap
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

    for (uint32_t i = 0; i < ruleToRemove.size(); ++i) {
        QCC_DbgTrace(("Calling RemoveMatch(\"%s\")", ruleToRemove[i].c_str()));
        status = bus.RemoveMatch(ruleToRemove[i].c_str());
    }
    return status;
}

QStatus AboutListenerInternal::RemoveAllHandlers() {
    QCC_DbgTrace(("InternalAnnounceHandler::%s", __FUNCTION__));
    QStatus status = ER_OK;

    announceMapLock.Lock(MUTEX_CONTEXT);
    AnnounceMap tmpAnnounceMap = announceMap;
    announceMap.clear();
    announceMapLock.Unlock(MUTEX_CONTEXT);

    for (AnnounceMap::iterator amit = tmpAnnounceMap.begin(); amit != tmpAnnounceMap.end(); ++amit) {
        //call remove match for each interface
        qcc::String matchRule = emptyMatchRule;
        for (std::set<qcc::String>::iterator it = amit->second.begin(); it != amit->second.end(); ++it) {
            matchRule += qcc::String(",implements='") + *it + qcc::String("'");
        }

        QCC_DbgTrace(("Calling RemoveMatch(\"%s\")", matchRule.c_str()));
        status = bus.RemoveMatch(matchRule.c_str());
    }
    return status;
}
void AboutListenerInternal::AnnounceSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath,
                                                  ajn::Message& message) {

    QCC_DbgTrace(("AboutListenerInternal::%s", __FUNCTION__));

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

        status = args[0].Get("q", &version);
        if (status != ER_OK) {
            return;
        }
        status = args[1].Get("q", &receivedPort);
        if (status != ER_OK) {
            return;
        }
        AboutObjectDescription objectDescription;
        objectDescription.CreateFromMsgArg(args[2]);
        announceMapLock.Lock(MUTEX_CONTEXT);
        //look through map and send out the Announce if able to match the interfaces
        for (AnnounceMap::iterator it = announceMap.begin();
             it != announceMap.end(); ++it) {
            bool matchFound = true;
            //if second.size is zero then the user is trying to match an any interface
            for (std::set<qcc::String>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                // TODO look through the MsgArg directly without using the AboutObjectDescription
                matchFound = objectDescription.HasInterface(*it2);
                // if the interface is not in the objectDescription we can exit
                // the loop instantly with out checking the other interfaces
                if (!matchFound) {
                    break;
                }
            }
            if (matchFound) {
                ProtectedAboutListener pah = it->first;
                announceHandlerList.push_back(pah);
            }
        }
        announceMapLock.Unlock(MUTEX_CONTEXT);

        announceHandlerLock.Lock(MUTEX_CONTEXT);
        for (size_t i = 0; i < announceHandlerList.size(); ++i) {
            ProtectedAboutListener l = announceHandlerList[i];
            announceHandlerLock.Unlock(MUTEX_CONTEXT);
            (*l)->Announced(message->GetSender(), version, static_cast<SessionPort>(receivedPort), args[2], args[3]);
            announceHandlerLock.Lock(MUTEX_CONTEXT);
        }

        announceHandlerList.clear();
        announceHandlerLock.Unlock(MUTEX_CONTEXT);
    }
}
