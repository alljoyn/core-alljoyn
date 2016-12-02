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

#include "AsyncSessionJoiner.h"
#include "SessionListenerImpl.h"
#include <iostream>

using namespace ajn;

AsyncSessionJoiner::AsyncSessionJoiner(const char* name, SessionJoinedCallback callback) :
    m_Busname(""), m_Callback(callback)
{
    if (name) {
        m_Busname.assign(name);
    }
}

AsyncSessionJoiner::~AsyncSessionJoiner()
{

}

void AsyncSessionJoiner::JoinSessionCB(QStatus status, SessionId id, const SessionOpts& opts, void* context)
{
    QCC_UNUSED(opts);
    if (status == ER_OK) {
        std::cout << "JoinSessionCB(" << m_Busname.c_str() << ") succeeded with id" << id << std::endl;
        if (m_Callback) {
            std::cout << "Calling SessionJoiner Callback" << std::endl;
            m_Callback(m_Busname, id);
        }
    } else {
        std::cout << "JoinSessionCB(" << m_Busname.c_str() << ") failed with status: " << QCC_StatusText(status) << std::endl;
    }

    SessionListenerImpl* listener = (SessionListenerImpl*) context;
    delete listener;
    delete this;
}