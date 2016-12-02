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

#ifndef SESSIONLISTENERIMPL_H_
#define SESSIONLISTENERIMPL_H_

#include <alljoyn/SessionListener.h>
#include <qcc/String.h>

#if defined(QCC_OS_GROUP_WINDOWS)
/* Disabling warning C 4100. Function doesnt use all passed in parameters */
#pragma warning(push)
#pragma warning(disable: 4100)
#endif

/**
 * SessionListenerImpl
 */
class SessionListenerImpl : public ajn::SessionListener {
  public:
    /**
     * SessionListenerImpl
     * @param inServiceNAme
     */
    SessionListenerImpl(qcc::String const& inServiceNAme);

    /**
     * destructor
     */
    virtual ~SessionListenerImpl();

    /**
     * SessionLost
     * @param sessionId
     * @param reason
     */
    void SessionLost(ajn::SessionId sessionId, ajn::SessionListener::SessionLostReason reason);

  private:

    qcc::String serviceName;

};

#endif /* SESSIONLISTENERIMPL_H_ */

