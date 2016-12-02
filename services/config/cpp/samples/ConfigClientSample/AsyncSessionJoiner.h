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

#ifndef ASYNCSESSIONJOINER_H_
#define ASYNCSESSIONJOINER_H_

#include <alljoyn/BusAttachment.h>

#if defined(QCC_OS_GROUP_WINDOWS)
/* Disabling warning C 4100. Function doesnt use all passed in parameters */
#pragma warning(push)
#pragma warning(disable: 4100)
#endif

typedef void (*SessionJoinedCallback)(qcc::String const& busName, ajn::SessionId id);

/**
 * class AsyncSessionJoiner
 */
class AsyncSessionJoiner : public ajn::BusAttachment::JoinSessionAsyncCB {

  public:
    /**
     * Constructor
     * @param name
     * @param callback
     */
    AsyncSessionJoiner(const char* name, SessionJoinedCallback callback = 0);

    /**
     * destructor
     */
    virtual ~AsyncSessionJoiner();

    /**
     * JoinSessionCB
     * @param status
     * @param id
     * @param opts
     * @param context
     */
    void JoinSessionCB(QStatus status, ajn::SessionId id, const ajn::SessionOpts& opts, void* context);

  private:

    qcc::String m_Busname;

    SessionJoinedCallback m_Callback;
};

#endif /* ASYNCSESSIONJOINER_H_ */