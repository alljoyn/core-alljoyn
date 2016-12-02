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
#ifndef _MESSAGEREPLYHOST_H
#define _MESSAGEREPLYHOST_H

#include "BusAttachment.h"
#include "BusObject.h"
#include "MessageHost.h"
#include "ScriptableObject.h"
#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <qcc/String.h>

class _MessageReplyHost : public _MessageHost {
  public:
    _MessageReplyHost(Plugin& plugin, BusAttachment& busAttachment, BusObject& busObject, ajn::Message& message, qcc::String replySignature);
    virtual ~_MessageReplyHost();

  private:
    BusObject busObject;
    qcc::String replySignature;

    bool reply(const NPVariant* npargs, uint32_t npargCount, NPVariant* npresult);
    bool replyError(const NPVariant* npargs, uint32_t npargCount, NPVariant* npresult);
};

typedef qcc::ManagedObj<_MessageReplyHost> MessageReplyHost;

#endif // _MESSAGEREPLYHOST_H