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

#include "BusListenerImpl.h"
#include <iostream>

using namespace ajn;

BusListenerImpl::BusListenerImpl() :
    BusListener(), SessionPortListener(), m_SessionPort(0)
{
}

BusListenerImpl::BusListenerImpl(ajn::SessionPort sessionPort) :
    BusListener(), SessionPortListener(), m_SessionPort(sessionPort)
{

}

BusListenerImpl::~BusListenerImpl()
{
}

void BusListenerImpl::setSessionPort(ajn::SessionPort sessionPort)
{
    m_SessionPort = sessionPort;
}

SessionPort BusListenerImpl::getSessionPort()
{
    return m_SessionPort;
}

bool BusListenerImpl::AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
{
    if (sessionPort != m_SessionPort) {
        std::cout << "Rejecting join attempt on unexpected session port " << sessionPort << std::endl;
        return false;
    }

    std::cout << "Accepting JoinSessionRequest from " << joiner << " (opts.proximity= " << opts.proximity
              << ", opts.traffic=" << opts.traffic << ", opts.transports=" << opts.transports << ")." << std::endl;
    return true;
}