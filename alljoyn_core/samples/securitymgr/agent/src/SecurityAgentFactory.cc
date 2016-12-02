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

#include <alljoyn/securitymgr/SecurityAgentFactory.h>

#include "SecurityAgentImpl.h"

#define QCC_MODULE "SECMGR_AGENT"

using namespace qcc;

namespace ajn {
namespace securitymgr {
SecurityAgentFactory::SecurityAgentFactory()
{
}

SecurityAgentFactory::~SecurityAgentFactory()
{
}

QStatus SecurityAgentFactory::GetSecurityAgent(const shared_ptr<AgentCAStorage>& caStorage,
                                               shared_ptr<SecurityAgent>& agentRef,
                                               BusAttachment* ba)
{
    shared_ptr<SecurityAgentImpl> sa = nullptr;
    QStatus status = ER_OK;

    sa = make_shared<SecurityAgentImpl>(caStorage, ba);
    if (ER_OK == (status = sa->Init())) {
        agentRef = sa;
    } else {
        agentRef = nullptr;
    }
    return status;
}
}
}
#undef QCC_MODULE