/**
 * @file
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
#include <qcc/String.h>

#include "../DaemonTransport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

const char* DaemonTransport::TransportName = "";

void* DaemonTransport::Run(void* arg)
{
    QCC_UNUSED(arg);
    return NULL;
}

QStatus DaemonTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    QCC_UNUSED(inSpec);
    QCC_UNUSED(outSpec);
    QCC_UNUSED(argMap);
    return ER_OK;
}

QStatus DaemonTransport::StartListen(const char* listenSpec)
{
    QCC_UNUSED(listenSpec);
    return ER_OK;
}

QStatus DaemonTransport::StopListen(const char* listenSpec)
{
    QCC_UNUSED(listenSpec);
    return ER_OK;
}

} // namespace ajn