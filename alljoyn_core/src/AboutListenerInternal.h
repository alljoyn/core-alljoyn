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
#ifndef ANNOUNCELISTENERINTERNAL_H_
#define ANNOUNCELISTENERINTERNAL_H_

#include <map>
#include <set>
#include <signal.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/AboutListener.h>

#include <qcc/Mutex.h>

namespace ajn {
/**
 * AboutListener is a helper class used by an AllJoyn IoE client application to receive AboutService signal notification.
 * The user of the class need to implement   virtual void Announce(...) function
 */

class AboutListenerInternal : public ajn::MessageReceiver {
    friend class BusAttachment;
  public:
    /**
     * Construct an AboutListener.
     */
    AboutListenerInternal(BusAttachment& bus);

    /**
     * Destruct AboutListener
     */
    ~AboutListenerInternal();

    /**
     * Add the announce handler to the map of handlers
     */
    QStatus AddHandler(AboutListener& handler, const char** implementsInterfaces, size_t numberInterfaces);

    /**
     * Remove the announce handler from the map of handelers
     */
    QStatus RemoveHandler(AboutListener& handler, const char** implementsInterfaces, size_t numberInterfaces);

    /**
     * clear all the handlers from the map of handlers
     */
    QStatus RemoveAllHandlers();
  private:
    /**
     * AboutListener is a callback registered to receive AllJoyn Signal.
     * @param[in] member
     * @param[in] srcPath
     * @param[in] message
     */
    void AnnounceSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& message);

//    bool ContainsInterface(const ObjectDescriptions& objectDescriptions, const qcc::String interface) const;

    /* Reference back to the bus attachment */
    ajn::BusAttachment& bus;

    volatile sig_atomic_t announceSignalMemberSet;
//    /**
//     *  pointer to InterfaceDescription::Member
//     */
//    const ajn::InterfaceDescription::Member* announceSignalMember;

    /**
     * Map of the AboutListener with the interfaces it is listening for.
     */
    typedef std::multimap<AboutListener*, std::set<qcc::String> > AnnounceMap;

    /**
     * protected AboutListeners
     */
    typedef qcc::ManagedObj<AboutListener*> ProtectedAboutListener;

    /**
     * the announceHandlerList
     */
    std::vector<ProtectedAboutListener> announceHandlerList;

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
#endif
