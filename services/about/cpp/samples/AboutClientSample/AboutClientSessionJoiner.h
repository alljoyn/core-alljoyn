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

#ifndef ABOUTCLIENTSESSIONJOINER_H_
#define ABOUTCLIENTSESSIONJOINER_H_

#include <alljoyn/about/AboutClient.h>
#include <alljoyn/BusAttachment.h>
#include "AboutClientSessionListener.h"

typedef void (*SessionJoinedCallback)(qcc::String const& busName, ajn::SessionId id);

struct SessionJoinerContext {
    qcc::String busName;
    AboutClientSessionListener* aboutClientSessionListener;

    SessionJoinerContext(qcc::String name, AboutClientSessionListener* absl) :
        busName(name), aboutClientSessionListener(absl) { }
};

class AboutClientSessionJoiner : public ajn::BusAttachment::JoinSessionAsyncCB {

  public:

    AboutClientSessionJoiner(ajn::BusAttachment& bus, const qcc::String& busName, SessionJoinedCallback callback = 0);

    virtual ~AboutClientSessionJoiner();

    void JoinSessionCB(QStatus status, ajn::SessionId id, const ajn::SessionOpts& opts, void* context);

  private:
    /* Private assigment operator - does nothing */
    AboutClientSessionJoiner operator=(const AboutClientSessionJoiner&);

    ajn::BusAttachment& bus;

    qcc::String m_BusName;

    SessionJoinedCallback m_Callback;
};

#endif /* ABOUTCLIENTSESSIONJOINER_H_ */