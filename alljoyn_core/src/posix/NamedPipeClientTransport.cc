/**
 * @file
 * NamedPipeClientTransport is a specialization of Transport that connects to
 * Daemon on a named pipe.
 */

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

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <alljoyn/BusAttachment.h>

#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "ClientTransport.h"
#include "NamedPipeClientTransport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {
/*
 * This platform does not support NamedPipe Transport
 */
const char* NamedPipeClientTransport::NamedPipeTransportName = NULL;

QStatus NamedPipeClientTransport::IsConnectSpecValid(const char* connectSpec)
{
    return ER_FAIL;
}

QStatus NamedPipeClientTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    return ER_FAIL;
}

QStatus NamedPipeClientTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep)
{
    return ER_FAIL;
}

NamedPipeClientTransport::NamedPipeClientTransport(BusAttachment& bus)
    : ClientTransport(bus), m_bus(bus)
{
}

} // namespace ajn