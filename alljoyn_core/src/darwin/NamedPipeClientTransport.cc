/**
 * @file
 * NamedPipeClientTransport is a specialization of Transport that connects to
 * Daemon on a named pipe.
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

void NamedPipeClientTransport::Init()
{
}

void NamedPipeClientTransport::Shutdown()
{
}

} // namespace ajn