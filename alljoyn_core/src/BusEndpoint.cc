/**
 * @file This file defines the class for handling the client and server
 * endpoints for the message bus wire protocol
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
#include <qcc/GUID.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>

#include <BusEndpoint.h>

#define QCC_MODULE "ALLJOYN"

using namespace qcc;
using namespace ajn;

String _BusEndpoint::GetControllerUniqueName() const {

    /* An endpoint with unique name :X.Y has a controller with a unique name :X.1 */
    String ret = GetUniqueName();
    ret[qcc::GUID128::SIZE_SHORT + 2] = '1';
    ret.resize(qcc::GUID128::SIZE_SHORT + 3);
    return ret;
}

void _BusEndpoint::Invalidate()
{
    QCC_DbgPrintf(("Invalidating endpoint type=%d %s", endpointType, GetUniqueName().c_str()));
    isValid = false;
}
