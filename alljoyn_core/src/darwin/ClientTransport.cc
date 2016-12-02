/**
 * @file
 * ClientTransport is an implementation of Transport that listens
 * on an AF_UNIX socket.
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

#include <qcc/platform.h>

#include <map>

#include <alljoyn/Session.h>
#include "ClientTransport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

/*
 * This platform only supports a bundled daemon so has no client transport
 */
const char* ClientTransport::TransportName = NULL;

QStatus ClientTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    return ER_FAIL;
}

QStatus ClientTransport::Connect(const char* connectArgs, const SessionOpts& opts, BusEndpoint& newep)
{
    return ER_FAIL;
}

} // namespace ajn