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

#include "AJInitializer.h"
#include <alljoyn/Init.h>

QStatus AJInitializer::Initialize()
{
    QStatus status = AllJoynInit();
    if (status != ER_OK) {
        return status;
    }
#ifdef ROUTER
    status = AllJoynRouterInit();
    if (status != ER_OK) {
        AllJoynShutdown();
        return status;
    }
#endif
    return status;
}

AJInitializer::~AJInitializer()
{
#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
}