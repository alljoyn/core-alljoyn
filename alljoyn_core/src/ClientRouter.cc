/**
 * @file
 * ClientRouter is a simplified ("client-side only") router that is capable
 * of routing messages between a single remote and a single local endpoint.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>

#include <qcc/Debug.h>
#include <qcc/Util.h>
#include <qcc/String.h>

#include <alljoyn/Status.h>

#include "Transport.h"
#include "BusEndpoint.h"
#include "LocalTransport.h"
#include "ClientRouter.h"
#include "BusInternal.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

QStatus ClientRouter::PushMessage(Message& msg, BusEndpoint& sender)
{
    QStatus status = ER_OK;

    if (!localEndpoint->IsValid() || !nonLocalEndpoint->IsValid() || !sender->IsValid()) {
        status = ER_BUS_NO_ENDPOINT;
    } else {
        if (sender == BusEndpoint::cast(localEndpoint)) {
            localEndpoint->UpdateSerialNumber(msg);
            status = nonLocalEndpoint->PushMessage(msg);
        } else {
            status = localEndpoint->PushMessage(msg);
        }
    }

    if (ER_OK != status) {
        QCC_DbgHLPrintf(("ClientRouter::PushMessage failed: %s", QCC_StatusText(status)));
    }
    return status;
}

QStatus ClientRouter::RegisterEndpoint(BusEndpoint& endpoint)
{
    bool isLocal = endpoint->GetEndpointType() == ENDPOINT_TYPE_LOCAL;
    bool hadNonLocal = nonLocalEndpoint->IsValid();

    QCC_DbgHLPrintf(("ClientRouter::RegisterEndpoint"));

    /* Keep track of local and (at least one) non-local endpoint */
    if (isLocal) {
        localEndpoint = LocalEndpoint::cast(endpoint);
    } else {
        nonLocalEndpoint = endpoint;
    }

    /* Local and non-local endpoints must have the same unique name */
    if ((isLocal && nonLocalEndpoint->IsValid()) || (!isLocal && localEndpoint->IsValid() && !hadNonLocal)) {
        localEndpoint->SetUniqueName(nonLocalEndpoint->GetUniqueName());
    }

    /* Notify local endpoint we have both a local and at least one non-local endpoint */
    if (localEndpoint->IsValid() && nonLocalEndpoint->IsValid() && (isLocal || !hadNonLocal)) {
        localEndpoint->OnBusConnected();
    }
    return ER_OK;
}

void ClientRouter::UnregisterEndpoint(const String& epName, EndpointType epType)
{
    QCC_DbgHLPrintf(("ClientRouter::UnregisterEndpoint"));

    if ((localEndpoint->GetUniqueName() == epName) && (nonLocalEndpoint->GetEndpointType() == epType)) {
        localEndpoint->OnBusDisconnected();
    }

    /* Unregister static endpoints */
    if ((nonLocalEndpoint->GetUniqueName() == epName) && (nonLocalEndpoint->GetEndpointType() == epType)) {
        /*
         * Let the bus know that the nonlocalEndpoint endpoint disconnected
         */
        localEndpoint->GetBus().GetInternal().NonLocalEndpointDisconnected();
        nonLocalEndpoint->Invalidate();
        nonLocalEndpoint = BusEndpoint();
    }

}

BusEndpoint ClientRouter::FindEndpoint(const qcc::String& busname)
{
    return nonLocalEndpoint;
}

ClientRouter::~ClientRouter()
{
    QCC_DbgHLPrintf(("ClientRouter::~ClientRouter()"));
}


}
