/**
 * @file
 * BTAccessor declaration for BlueZ
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_BLUEZBTENDPOINT_H
#define _ALLJOYN_BLUEZBTENDPOINT_H

#include <qcc/platform.h>

#include <alljoyn/BusAttachment.h>

#include "BlueZUtils.h"
#include "BTEndpoint.h"
#include "BTNodeInfo.h"

namespace ajn {
class _BlueZBTEndpoint;
typedef qcc::ManagedObj<_BlueZBTEndpoint> BlueZBTEndpoint;
class _BlueZBTEndpoint : public _BTEndpoint {
  public:

    /**
     * Bluetooth endpoint constructor
     */
    _BlueZBTEndpoint(BusAttachment& bus,
                     bool incoming,
                     qcc::SocketFd sockFd,
                     const BTNodeInfo& node,
                     const BTBusAddress& redirect) :
        _BTEndpoint(bus, incoming, sockStream, node, redirect),
        sockStream(sockFd)
    { }

  private:
    bluez::BTSocketStream sockStream;
};

} // namespace ajn

#endif
