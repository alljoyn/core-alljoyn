/**
 * @file
 * Static global creation and destruction
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
#include <alljoyn/BusAttachment.h>
#include "RouterGlobals.h"
#include "ns/IpNameService.h"

namespace ajn {

void RouterGlobals::Init()
{
    IpNameService::Init();
}

void RouterGlobals::Shutdown()
{
    IpNameService::Shutdown();
}

}