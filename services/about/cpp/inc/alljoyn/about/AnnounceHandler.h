/**
 * @file
 * MessageReceiver responsible for receiving the Announce signal from the org.alljoyn.About interface
 */
/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#ifndef ANNOUNCELISTENER_H_
#define ANNOUNCELISTENER_H_

#include <map>
#include <vector>
#include <alljoyn/BusAttachment.h>

namespace ajn {
namespace services {

/**
 * AnnounceHandler is a helper class used by an AllJoyn IoE client application to receive AboutService signal notification.
 * The user of the class need to implement   virtual void Announce(...) function
 */

class AnnounceHandler : public ajn::MessageReceiver {

    friend class AnnouncementRegistrar;
  public:
    /**
     *	map of AboutData using qcc::String as key and ajn::MsgArg as value.
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
    AnnounceHandler();

    /**
     * Destruct AnnounceHandler
     */
    ~AnnounceHandler();

    /**
     *
     * @param[in] version of the AboutService.
     * @param[in] port used by the AboutService
     * @param[in] busName unique bus name of the service
     * @param[in] objectDescs map of ObjectDescriptions using qcc::String as key std::vector<qcc::String>   as value, describing interfaces
     * @param[in] aboutData map of AboutData using qcc::String as key and ajn::MsgArg as value
     */
    virtual void Announce(uint16_t version, uint16_t port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData) = 0;

};
inline AnnounceHandler::~AnnounceHandler() {
}

}
}

#endif /* ANNOUNCELISTENER_H_ */
