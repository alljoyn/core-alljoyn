/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#include "SocketStreamEvent.h"

/** @internal */
#define QCC_MODULE "SOCKETSTREAM_EVENT"

namespace AllJoyn {

SocketStreamEvent::SocketStreamEvent(SocketStream ^ sock)
{
    // Add default handler for DataReceived
    DataReceived += ref new SocketStreamDataReceivedHandler([&]() {
                                                                DefaultSocketStreamDataReceivedHandler();
                                                            });
    // Add event handler for sockfd SocketEventsChanged
    sock->_sockfd->SocketEventsChanged += ref new qcc::winrt::SocketWrapperEventsChangedHandler([&](Platform::Object ^ source, int events) {
                                                                                                    SocketEventsChangedHandler(source, events);
                                                                                                });
}

SocketStreamEvent::~SocketStreamEvent()
{
}

void SocketStreamEvent::DefaultSocketStreamDataReceivedHandler()
{
}

void SocketStreamEvent::SocketEventsChangedHandler(Platform::Object ^ source, int events)
{
    // Pulse data reeived Event if sockfd is read ready
    if (events & (int)qcc::winrt::Events::Read) {
        DataReceived();
    }
}

}
