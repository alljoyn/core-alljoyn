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
#ifndef _ALLJOYN_BTENDPOINT_H
#define _ALLJOYN_BTENDPOINT_H

#include <qcc/platform.h>

#include <qcc/Stream.h>

#include <alljoyn/BusAttachment.h>

#include "BTNodeInfo.h"
#include "RemoteEndpoint.h"


namespace ajn {
class _BTEndpoint;
typedef qcc::ManagedObj<_BTEndpoint> BTEndpoint;

class _BTEndpoint : public _RemoteEndpoint {
  public:

    /**
     * Bluetooth endpoint constructor
     */
    _BTEndpoint(BusAttachment& bus,
                bool incoming,
                qcc::Stream& stream,
                const BTNodeInfo& node,
                const BTBusAddress& redirect) :
        _RemoteEndpoint(bus, incoming, node->GetBusAddress().ToSpec(), &stream, "bluetooth"),
        node(node),
        redirect(redirect)
    { }

    virtual ~_BTEndpoint() { }


    BTNodeInfo& GetNode() { return node; }

  private:
    qcc::String RedirectionAddress() { return redirect.IsValid() ? redirect.ToSpec() : ""; }

    BTNodeInfo node;
    BTBusAddress redirect;
};

} // namespace ajn

#endif
