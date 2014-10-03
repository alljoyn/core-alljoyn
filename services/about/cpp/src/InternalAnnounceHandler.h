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

#ifndef INTERNALANNOUNCEHANDLER_H_
#define INTERNALANNOUNCEHANDLER_H_

#include <map>
#include <set>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/about/AnnounceHandler.h>

#include <qcc/Mutex.h>

namespace ajn {
namespace services {

/**
 * AnnounceHandler is a helper class used by an AllJoyn IoE client application to receive AboutService signal notification.
 * The user of the class need to implement   virtual void Announce(...) function
 */

class InternalAnnounceHandler : public ajn::MessageReceiver {

    friend class AnnouncementRegistrar;
  public:
    /**
     *  map of AboutData using qcc::String as key and ajn::MsgArg as value.
     */
    typedef std::map<qcc::String, ajn::MsgArg> AboutData;

    /**
     * map of ObjectDescriptions using qcc::String as key std::vector<qcc::String>   as value, describing interfaces
     *
     */
    typedef std::map<qcc::String, std::vector<qcc::String> > ObjectDescriptions;

    /**
     * Construct an AnnounceHandler.
     */
    InternalAnnounceHandler(ajn::BusAttachment& bus);

    /**
     * Destruct AnnounceHandler
     */
    ~InternalAnnounceHandler();

    /**
     * Add the announce handler to the map of handlers
     */
    QStatus AddHandler(AnnounceHandler& handler, const char** implementsInterfaces, size_t numberInterfaces);

    /**
     * Remove the announce handler from the map of handlers
     */
    QStatus RemoveHandler(AnnounceHandler& handler, const char** implementsInterfaces, size_t numberInterfaces);

    /**
     * Remove all announce handlers from the map of handlers
     */
    void RemoveAllHandlers();
  private:
    /**
     * AnnounceHandler is a callback registered to receive AllJoyn Signal.
     * @param[in] member
     * @param[in] srcPath
     * @param[in] message
     */
    void AnnounceSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& message);

    bool ContainsInterface(const ObjectDescriptions& objectDescriptions, const qcc::String interface) const;

    qcc::String GetMatchRule(const std::set<qcc::String>& interfaces) const;

    /**
     * reference to the BusAttachment used by this InternalAnnounceHandler
     */
    ajn::BusAttachment& bus;
    /**
     *  pointer to InterfaceDescription::Member
     */
    const ajn::InterfaceDescription::Member* announceSignalMember;

    /**
     * the state of a single AnnounceHandler registration
     */
    struct RegisteredHandlerState {
        std::set<qcc::String> interfaces;
        std::set<qcc::String> matchingPeers;
    };

    /**
     * Map of the AnnounceHandler with the interfaces it is listening for.
     */
    typedef std::multimap<AnnounceHandler*, RegisteredHandlerState> AnnounceMap;

    /**
     * protected AnnounceHandlers
     */
    typedef qcc::ManagedObj<AnnounceHandler*> ProtectedAnnounceHandler;

    /**
     * the announceHandlerList
     */
    std::vector<ProtectedAnnounceHandler> announceHandlerList;

    /**
     * lock used to prevent deletion of announceHandlers while in a callback
     */
    qcc::Mutex announceHandlerLock;

    /*
     * Mutex that protects the announceMap
     */
    qcc::Mutex announceMapLock;

    /**
     * the announce Map contans all the handlers and all the interfaces that the
     * should be implemented if you are going to get a callback on that handler.
     */
    AnnounceMap announceMap;

    /**
     * default matchrule without any `implements` rules added to it.
     */
    qcc::String emptyMatchRule;

};

}
}

#endif /* INTERNALANNOUNCEHANDLER_H_ */
