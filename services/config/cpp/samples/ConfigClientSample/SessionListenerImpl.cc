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

#include "SessionListenerImpl.h"
#include <iostream>

SessionListenerImpl::SessionListenerImpl(qcc::String const& inServiceNAme) :
    serviceName(inServiceNAme)
{
}

SessionListenerImpl::~SessionListenerImpl()
{
}

void SessionListenerImpl::SessionLost(ajn::SessionId sessionId, ajn::SessionListener::SessionLostReason reason)
{
    QCC_UNUSED(sessionId);
    QCC_UNUSED(reason);
    std::cout << "AboutClient session has been lost for " << serviceName.c_str() << std::endl;
}