/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

#ifndef ANNOUNCEHANDLERIMPL_H_
#define ANNOUNCEHANDLERIMPL_H_

#include <signal.h>

#include <qcc/Log.h>
#include <alljoyn/version.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/about/AnnounceHandler.h>
#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AboutIconClient.h>

namespace ajn {
namespace services {
/**
 *  AnnouncementRegistrar is a helper class enable the Application to register AnnounceHandlers to receive org.alljoyn.about Announce signals.
 */
class AnnouncementRegistrar {

  public:
    /**
     * RegisterAnnounceHandler Registers the AnnounceHandler to receive  org.alljoyn.about Announce signals
     * @param[in] bus reference to BusAttachment
     * @param[in] handler reference to AnnounceHandler
     * @return status
     */
    static QStatus RegisterAnnounceHandler(ajn::BusAttachment& bus, AnnounceHandler& handler);

    /**
     * UnRegisterAnnounceHandler  UnRegisters the AnnounceHandler from  receiving   org.alljoyn.about Announce signal
     * @param[in] bus reference to BusAttachment
     * @param[in] handler reference to AnnounceHandler
     * @return status
     */
    static QStatus UnRegisterAnnounceHandler(ajn::BusAttachment& bus, AnnounceHandler& handler);
};

}
}
#endif /* ANNOUNCEHANDLERIMPL_H_ */
