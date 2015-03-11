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

#ifndef ABOUTCLIENTANNOUNCEHANDLER_H_
#define ABOUTCLIENTANNOUNCEHANDLER_H_

#include <alljoyn/about/AnnounceHandler.h>

typedef void (*AnnounceHandlerCallback)(qcc::String const& busName, unsigned short port);
extern uint32_t g_start_timestamp;
extern uint32_t g_count_ninety_five_percentile;
extern uint32_t g_count_fifty_percentile;
extern uint32_t g_count_remaining;

class AboutClientAnnounceHandler : public ajn::services::AnnounceHandler {

  public:

    virtual void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData);

    AboutClientAnnounceHandler(AnnounceHandlerCallback callback = 0);

    ~AboutClientAnnounceHandler();

  private:

    AnnounceHandlerCallback m_Callback;
};

#endif /* ABOUTCLIENTANNOUNCEHANDLER_H_ */
