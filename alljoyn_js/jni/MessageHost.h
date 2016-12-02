/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
#ifndef _MESSAGEHOST_H
#define _MESSAGEHOST_H

#include "BusAttachment.h"
#include "ScriptableObject.h"
#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <qcc/String.h>

class _MessageHost : public ScriptableObject {
  public:
    _MessageHost(Plugin& plugin, BusAttachment& busAttachment, ajn::Message& message);
    virtual ~_MessageHost();

  protected:
    BusAttachment busAttachment;
    ajn::Message message;

    bool getSender(NPVariant* npresult);
    bool getDestination(NPVariant* npresult);
    bool getFlags(NPVariant* npresult);
    bool getInterfaceName(NPVariant* npresult);
    bool getObjectPath(NPVariant* npresult);
    bool getAuthMechanism(NPVariant* npresult);
    bool getIsUnreliable(NPVariant* npresult);
    bool getMemberName(NPVariant* npresult);
    bool getSignature(NPVariant* npresult);
    bool getSessionId(NPVariant* npresult);
    bool getTimestamp(NPVariant* npresult);
};

typedef qcc::ManagedObj<_MessageHost> MessageHost;

#endif // _MESSAGEHOST_H