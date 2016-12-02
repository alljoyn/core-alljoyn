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

#ifndef ALLJOYN_SECMGR_SECURITYAGENTFACTORY_H_
#define ALLJOYN_SECMGR_SECURITYAGENTFACTORY_H_

#include <memory>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Status.h>

#include "SecurityAgent.h"
#include "AgentCAStorage.h"

namespace ajn {
namespace securitymgr {
/**
 * @brief SecurityAgentFactory is a singleton factory to return security agents.
 * */
class SecurityAgentFactory {
  public:
    /**
     * @brief Get a singleton instance of the security agent factory.
     *
     * @return SecurityAgentFactory reference to the singleton security agent factory.
     */
    static SecurityAgentFactory& GetInstance()
    {
        static SecurityAgentFactory smf;
        return smf;
    }

    /* @brief Get a security agent instance.
     *
     * @param[in]  caStorage   The CaStorage this agent should use.
     * @param[out] agentRef    A reference to shared_ptr. This reference will be updated on success and will
     *                         contain the agent.
     * @param[in]  ba          The bus attachment to be used.
     *                         The bus attachment will be started
     *                         and connected if it was not so originally.
     *                         If nullptr, then one will be created and owned
     *                         by the returned security agent.
     *
     * @return ER_OK           If successful, otherwise an error code.
     */
    QStatus GetSecurityAgent(const shared_ptr<AgentCAStorage>& caStorage,
                             shared_ptr<SecurityAgent>& agentRef,
                             BusAttachment* ba = nullptr);

  private:

    SecurityAgentFactory();

    ~SecurityAgentFactory();

    /**
     * @brief private copy constructor and assignment operator.
     */
    SecurityAgentFactory(const SecurityAgentFactory& sf);
    SecurityAgentFactory& operator=(const SecurityAgentFactory& sf);
};
}
}
#endif /* ALLJOYN_SECMGR_SECURITYAGENTFACTORY_H_ */