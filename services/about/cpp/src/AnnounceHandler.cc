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
#include <stdio.h>
#include <alljoyn/about/AnnounceHandler.h>
#include <qcc/Debug.h>

using namespace ajn;
using namespace services;

#define QCC_MODULE "ALLJOYN_ABOUT_ANNOUNCE_HANDLER"

AnnounceHandler::AnnounceHandler() {
    QCC_DbgTrace(("AnnounceHandler::%s", __FUNCTION__));
}