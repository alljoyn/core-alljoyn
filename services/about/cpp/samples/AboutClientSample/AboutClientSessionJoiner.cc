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

#include "AboutClientSessionJoiner.h"
#include "AboutClientSessionListener.h"
#include <iostream>

using namespace ajn;

AboutClientSessionJoiner::AboutClientSessionJoiner(BusAttachment& bus, const qcc::String& busName, SessionJoinedCallback callback) :
    bus(bus), m_BusName(busName), m_Callback(callback) {
}

AboutClientSessionJoiner::~AboutClientSessionJoiner()
{

}

void AboutClientSessionJoiner::JoinSessionCB(QStatus status, SessionId id, const SessionOpts& opts, void* context)
{
    QCC_UNUSED(opts);

    if (status == ER_OK) {
        std::cout << "JoinSessionCB(" << m_BusName << ") succeeded with id: " << id << std::endl;
        if (m_Callback) {
            std::cout << "Calling SessionJoiner Callback" << std::endl;
            m_Callback(m_BusName, id);
        }
    } else {
        std::cout << "JoinSessionCB(" << m_BusName << ") failed with status: " << QCC_StatusText(status) << std::endl;
    }


    bus.SetSessionListener(id, NULL);

    AboutClientSessionListener* t = (AboutClientSessionListener*) context;
    delete t;
    delete this;
}